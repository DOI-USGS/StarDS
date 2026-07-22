/**
 * @file MockHttpServer.h
 * @brief Minimal in-process HTTP server for testing the /vsicurl and /vsis3
 *        remote-I/O code paths without any network or cloud credentials.
 *
 * Inspired by GDAL's autotest webserver: a tiny localhost server that serves a
 * registered set of objects (path -> bytes) and records the requests it received
 * so tests can assert the client's outgoing behavior (HEAD probe, Range reads,
 * Authorization header, etc.).
 *
 * Supports: HEAD (Content-Length), GET with `Range: bytes=a-b` (206 + Content-Range)
 * or full GET (200), PUT (captures body), and 404 for unknown paths. Optionally a
 * one-shot 301 redirect with an x-amz-bucket-region header (for S3 region-redirect
 * tests).
 *
 * POSIX sockets + Winsock2 (macOS, Linux, Windows). Header-only; requires <thread>
 * (tests already link Threads::Threads). On Windows link ws2_32 (done in the test
 * CMake, and via #pragma comment below for MSVC).
 */
#pragma once

#ifdef _WIN32
// winsock2.h MUST precede windows.h; these tests include this header before
// stards.h/<curl/curl.h>, so the ordering is satisfied. winsock2.h transitively
// includes windows.h, whose <windef.h> #defines min/max macros that break the
// std::min/std::max calls in stards.h — NOMINMAX suppresses them. (The ERROR
// collision is handled by stards.h's STARDS_-prefixed logger enum, so no NOGDI.)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace star_test {

// --- Cross-platform socket compat shim ------------------------------------
// Keeps the server body identical across platforms: the POSIX path is exactly as
// before; Windows maps the same calls onto Winsock (SOCKET handle, closesocket,
// SD_BOTH, int-length recv/send).
#ifdef _WIN32
using socket_t = SOCKET;
static const socket_t kInvalidSocket = INVALID_SOCKET;
inline int  closesock(socket_t s) { return ::closesocket(s); }
inline int  recvbuf(socket_t s, char* b, size_t n) {
    return ::recv(s, b, static_cast<int>(n), 0);
}
inline int  sendbuf(socket_t s, const char* b, size_t n) {
    return ::send(s, b, static_cast<int>(n), 0);
}
#define STARDS_SHUT_BOTH SD_BOTH
// One-time Winsock init/teardown, ref-counted by the OS so multiple servers are
// safe. Tied to server lifetime via a member declared before the socket.
struct WsaInit {
    WsaInit()  { WSADATA w; WSAStartup(MAKEWORD(2, 2), &w); }
    ~WsaInit() { WSACleanup(); }
};
#else
using socket_t = int;
static const socket_t kInvalidSocket = -1;
inline int     closesock(socket_t s) { return ::close(s); }
inline ssize_t recvbuf(socket_t s, char* b, size_t n) { return ::recv(s, b, n, 0); }
inline ssize_t sendbuf(socket_t s, const char* b, size_t n) { return ::send(s, b, n, 0); }
#define STARDS_SHUT_BOTH SHUT_RDWR
#endif

class MockHttpServer {
public:
    struct RequestLog {
        std::string method;
        std::string path;
        std::string range;          // value of the Range header, if any
        bool had_authorization = false;
        std::string host_header;
        std::vector<char> body;     // for PUT
    };

    MockHttpServer() {
        m_listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_listen_fd == kInvalidSocket) return;

        int one = 1;
        ::setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char*>(&one), sizeof(one));
#ifdef SO_NOSIGPIPE
        ::setsockopt(m_listen_fd, SOL_SOCKET, SO_NOSIGPIPE,
                     reinterpret_cast<const char*>(&one), sizeof(one));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;  // ephemeral port
        if (::bind(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            closesock(m_listen_fd);
            m_listen_fd = kInvalidSocket;
            return;
        }
        if (::listen(m_listen_fd, 16) != 0) {
            closesock(m_listen_fd);
            m_listen_fd = kInvalidSocket;
            return;
        }
        socklen_t len = sizeof(addr);
        if (::getsockname(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
            m_port = ntohs(addr.sin_port);
        }
        m_running = true;
        m_thread = std::thread([this] { runLoop(); });
    }

    ~MockHttpServer() { stop(); }

    MockHttpServer(const MockHttpServer&) = delete;
    MockHttpServer& operator=(const MockHttpServer&) = delete;

    bool ok() const { return m_listen_fd != kInvalidSocket && m_port != 0; }
    int port() const { return m_port; }

    // Register the bytes served for an exact request path (e.g. "/data.stards"
    // for /vsicurl, or "/bucket/key.stards" for path-style /vsis3).
    void addObject(const std::string& path, const std::string& bytes) {
        std::lock_guard<std::mutex> lk(m_mu);
        m_objects[path] = bytes;
    }

    void addObject(const std::string& path, const std::vector<char>& bytes) {
        addObject(path, std::string(bytes.begin(), bytes.end()));
    }

    bool hasObject(const std::string& path) {
        std::lock_guard<std::mutex> lk(m_mu);
        return m_objects.count(path) > 0;
    }

    std::string getObject(const std::string& path) {
        std::lock_guard<std::mutex> lk(m_mu);
        auto it = m_objects.find(path);
        return it == m_objects.end() ? std::string() : it->second;
    }

    // Arrange a single 301 response (with x-amz-bucket-region) for the next
    // request to `path`, after which the object is served normally. Used to test
    // S3 region-redirect handling.
    void addRegionRedirectOnce(const std::string& path, const std::string& region) {
        std::lock_guard<std::mutex> lk(m_mu);
        m_redirect_once[path] = region;
    }

    std::vector<RequestLog> requests() {
        std::lock_guard<std::mutex> lk(m_mu);
        return m_requests;
    }

    void stop() {
        if (!m_running.exchange(false)) {
            if (m_listen_fd != kInvalidSocket) { closesock(m_listen_fd); m_listen_fd = kInvalidSocket; }
            return;
        }
        if (m_listen_fd != kInvalidSocket) {
            ::shutdown(m_listen_fd, STARDS_SHUT_BOTH);
            closesock(m_listen_fd);
            m_listen_fd = kInvalidSocket;
        }
        if (m_thread.joinable()) m_thread.join();
    }

private:
    void runLoop() {
        while (m_running) {
            socket_t client = ::accept(m_listen_fd, nullptr, nullptr);
            if (client == kInvalidSocket) {
                if (!m_running) break;
                continue;
            }
            handleClient(client);
            closesock(client);
        }
    }

    static std::string recvHeaders(socket_t fd, std::string& leftover) {
        std::string data = leftover;
        char buf[4096];
        while (data.find("\r\n\r\n") == std::string::npos) {
            auto n = recvbuf(fd, buf, sizeof(buf));
            if (n <= 0) break;
            data.append(buf, static_cast<size_t>(n));
        }
        size_t hdr_end = data.find("\r\n\r\n");
        if (hdr_end == std::string::npos) { leftover.clear(); return data; }
        leftover = data.substr(hdr_end + 4);
        return data.substr(0, hdr_end);
    }

    static std::string headerValue(const std::string& headers, const std::string& name) {
        // Case-insensitive line search for "name:".
        std::istringstream ss(headers);
        std::string line;
        std::string lname = name;
        for (auto& c : lname) c = (char)std::tolower(c);
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string key = line.substr(0, colon);
            for (auto& c : key) c = (char)std::tolower(c);
            if (key == lname) {
                std::string v = line.substr(colon + 1);
                size_t s = v.find_first_not_of(" \t");
                size_t e = v.find_last_not_of(" \t");
                if (s == std::string::npos) return "";
                return v.substr(s, e - s + 1);
            }
        }
        return "";
    }

    static void sendAll(socket_t fd, const std::string& s) {
        size_t off = 0;
        while (off < s.size()) {
            auto n = sendbuf(fd, s.data() + off, s.size() - off);
            if (n <= 0) break;
            off += (size_t)n;
        }
    }

    void handleClient(socket_t fd) {
        std::string leftover;
        std::string headers = recvHeaders(fd, leftover);
        if (headers.empty()) return;

        // Parse request line: METHOD PATH HTTP/1.x
        std::istringstream ss(headers);
        std::string request_line;
        std::getline(ss, request_line);
        if (!request_line.empty() && request_line.back() == '\r') request_line.pop_back();
        std::istringstream rl(request_line);
        RequestLog log;
        std::string version;
        rl >> log.method >> log.path >> version;

        log.range = headerValue(headers, "Range");
        log.had_authorization = !headerValue(headers, "Authorization").empty();
        log.host_header = headerValue(headers, "Host");

        // For PUT, read the body per Content-Length.
        if (log.method == "PUT") {
            std::string cl = headerValue(headers, "Content-Length");
            size_t want = cl.empty() ? 0 : (size_t)std::stoul(cl);
            std::string body = leftover;
            char buf[4096];
            while (body.size() < want) {
                auto n = recvbuf(fd, buf, sizeof(buf));
                if (n <= 0) break;
                body.append(buf, static_cast<size_t>(n));
            }
            log.body.assign(body.begin(), body.end());
        }

        {
            std::lock_guard<std::mutex> lk(m_mu);
            m_requests.push_back(log);
        }

        respond(fd, log);
    }

    void respond(socket_t fd, const RequestLog& log) {
        // One-shot region redirect?
        {
            std::lock_guard<std::mutex> lk(m_mu);
            auto rit = m_redirect_once.find(log.path);
            if (rit != m_redirect_once.end()) {
                std::string region = rit->second;
                m_redirect_once.erase(rit);
                std::ostringstream resp;
                resp << "HTTP/1.1 301 Moved Permanently\r\n"
                     << "x-amz-bucket-region: " << region << "\r\n"
                     << "Content-Length: 0\r\n"
                     << "Connection: close\r\n\r\n";
                sendAll(fd, resp.str());
                return;
            }
        }

        if (log.method == "PUT") {
            // Store the uploaded object and 200.
            {
                std::lock_guard<std::mutex> lk(m_mu);
                m_objects[log.path] = std::string(log.body.begin(), log.body.end());
            }
            std::string resp = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(fd, resp);
            return;
        }

        std::string body;
        bool found;
        {
            std::lock_guard<std::mutex> lk(m_mu);
            auto it = m_objects.find(log.path);
            found = it != m_objects.end();
            if (found) body = it->second;
        }

        if (!found) {
            std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            sendAll(fd, resp);
            return;
        }

        if (log.method == "HEAD") {
            std::ostringstream resp;
            resp << "HTTP/1.1 200 OK\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Accept-Ranges: bytes\r\n"
                 << "Connection: close\r\n\r\n";
            sendAll(fd, resp.str());
            return;
        }

        // GET (optionally ranged)
        if (!log.range.empty()) {
            size_t start = 0, end = body.size() ? body.size() - 1 : 0;
            // Parse "bytes=start-end"
            const std::string prefix = "bytes=";
            std::string r = log.range;
            size_t p = r.find(prefix);
            if (p != std::string::npos) {
                r = r.substr(p + prefix.size());
                size_t dash = r.find('-');
                if (dash != std::string::npos) {
                    if (dash > 0) start = (size_t)std::stoull(r.substr(0, dash));
                    std::string es = r.substr(dash + 1);
                    if (!es.empty()) end = (size_t)std::stoull(es);
                }
            }
            if (start >= body.size()) start = body.size();
            if (end >= body.size()) end = body.size() ? body.size() - 1 : 0;
            size_t len = (end >= start && !body.empty()) ? (end - start + 1) : 0;
            std::string chunk = body.substr(start, len);

            std::ostringstream resp;
            resp << "HTTP/1.1 206 Partial Content\r\n"
                 << "Content-Length: " << chunk.size() << "\r\n"
                 << "Content-Range: bytes " << start << "-"
                 << (start + (chunk.empty() ? 0 : chunk.size() - 1)) << "/" << body.size() << "\r\n"
                 << "Accept-Ranges: bytes\r\n"
                 << "Connection: close\r\n\r\n";
            resp << chunk;
            sendAll(fd, resp.str());
            return;
        }

        // Full GET
        std::ostringstream resp;
        resp << "HTTP/1.1 200 OK\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Accept-Ranges: bytes\r\n"
             << "Connection: close\r\n\r\n";
        resp << body;
        sendAll(fd, resp.str());
    }

#ifdef _WIN32
    // Declared FIRST so Winsock is initialized (WSAStartup) before the ctor body
    // creates m_listen_fd, and torn down (WSACleanup) after the socket is closed.
    WsaInit m_wsa;
#endif
    socket_t m_listen_fd = kInvalidSocket;
    int m_port = 0;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mu;
    std::map<std::string, std::string> m_objects;
    std::map<std::string, std::string> m_redirect_once;
    std::vector<RequestLog> m_requests;
};

// Read an entire local file into a string (helper for serving real .stards bytes).
inline std::string read_file_bytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace star_test
