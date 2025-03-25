#pragma once

#include <variant>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "ezgz.hpp" 



class DeflateCompressor {
public:

    static std::vector<uint8_t> compress(const VariantType& data) {
        std::ostringstream oss;
        std::visit([&oss](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                oss << arg;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                for (const auto& str : arg) {
                    oss << str << '\0';
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::vector<std::string>>>) {
                for (const auto& vec : arg) {
                    for (const auto& str : vec) {
                        oss << str << '\0';
                    }
                    oss << '\0'; // Separate inner vectors
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::vector<int>>> || std::is_same_v<T, std::vector<std::vector<float>>> || std::is_same_v<T, std::vector<std::vector<double>>>) {
                for (const auto& vec : arg) {
                    for (const auto& elem : vec) {
                        oss << elem << ' ';
                    }
                    oss << '\0'; // Separate inner vectors
                }
            } else {
                for (const auto& elem : arg) {
                    oss << elem << ' ';
                }
            }
        }, data);

        std::string strData = oss.str();
        std::vector<uint8_t> compressedData;
        ezgz::compress(strData.begin(), strData.end(), std::back_inserter(compressedData));

        return compressedData;
    }

    static VariantType decompress(const std::vector<uint8_t>& compressedData, const VariantType& typeHint) {
        std::string decompressedStr;
        ezgz::decompress(compressedData.begin(), compressedData.end(), std::back_inserter(decompressedStr));

        std::istringstream iss(decompressedStr);
        VariantType resultData;
        std::visit([&iss, &resultData](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                resultData = decompressedStr;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                std::vector<std::string> vec;
                std::string str;
                while (std::getline(iss, str, '\0')) {
                    vec.push_back(str);
                }
                resultData = vec;
            } else if constexpr (std::is_same_v<T, std::vector<std::vector<std::string>>>) {
                std::vector<std::vector<std::string>> vec;
                std::string str;
                while (std::getline(iss, str, '\0')) {
                    std::vector<std::string> innerVec;
                    std::istringstream innerIss(str);
                    std::string innerStr;
                    while (std::getline(innerIss, innerStr, '\0')) {
                        innerVec.push_back(innerStr);
                    }
                    vec.push_back(innerVec);
                }
                resultData = vec;
            } else if constexpr (std::is_same_v<T, std::vector<std::vector<int>>> || std::is_same_v<T, std::vector<std::vector<float>>> || std::is_same_v<T, std::vector<std::vector<double>>>) {
                T vec;
                std::string line;
                while (std::getline(iss, line, '\0')) {
                    typename T::value_type innerVec;
                    std::istringstream lineStream(line);
                    typename T::value_type::value_type value;
                    while (lineStream >> value) {
                        innerVec.push_back(value);
                    }
                    vec.push_back(innerVec);
                }
                resultData = vec;
            } else {
                T vec;
                typename T::value_type value;
                while (iss >> value) {
                    vec.push_back(value);
                }
                resultData = vec;
            }
        }, typeHint);

        return resultData;
    }
};