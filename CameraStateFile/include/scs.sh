#!/usr/bin/env python3
"""
SCS (Simple Camera State) - Python implementation with CLI

This script provides a Python implementation of the SCS (Simple Camera State) file format
with a command line interface for basic operations.
"""

import os
import sys
import json
import struct
import argparse
import io
from typing import Dict, Any, Optional, List, Tuple, TypeVar, Generic, Union
import time
import threading
from dataclasses import dataclass
import requests

PROJECT_NAME = "SCS 0.1.0"

T = TypeVar('T')

@dataclass
class NDArray(Generic[T]):
    """N-dimensional array implementation"""
    shape: List[int]
    data: List[T]
    default_value: T

    def __init__(self, shape: List[int], default_value: T):
        self.shape = shape
        self.default_value = default_value
        size = 1
        for dim in shape:
            size *= dim
        self.data = [default_value] * size

    def index(self, coords: List[int]) -> int:
        """Convert n-dimensional coordinates to flat array index"""
        if len(coords) != len(self.shape):
            raise ValueError(f"Expected {len(self.shape)} dimensions, got {len(coords)}")
        
        idx = 0
        stride = 1
        for i in range(len(coords) - 1, -1, -1):
            if coords[i] >= self.shape[i] or coords[i] < 0:
                raise IndexError(f"Index {coords[i]} out of bounds for dimension {i} with size {self.shape[i]}")
            idx += coords[i] * stride
            stride *= self.shape[i]
        return idx

    def at(self, coords: List[int]) -> T:
        """Get value at specified coordinates"""
        return self.data[self.index(coords)]

    def set(self, coords: List[int], value: T) -> None:
        """Set value at specified coordinates"""
        self.data[self.index(coords)] = value

    def total_bytes(self) -> int:
        """Calculate total bytes for serialization"""
        # This is a simplified implementation
        return len(self.data) * 8  # Assuming 8 bytes per element as an approximation

    def __str__(self) -> str:
        return f"NDArray(shape={self.shape}, elements={len(self.data)})"


class HttpStream:
    """HTTP stream implementation with range support"""
    
    def __init__(self, url: str):
        self.url = url
        self.position = 0
        self.content_length = self._get_content_length()
        
    def _get_content_length(self) -> int:
        """Get content length of the remote resource"""
        response = requests.head(self.url)
        if 'Content-Length' in response.headers:
            return int(response.headers['Content-Length'])
        return 0
        
    def read(self, size: int = -1) -> bytes:
        """Read bytes from the stream"""
        if size == -1:
            end = self.content_length - 1
        else:
            end = min(self.position + size - 1, self.content_length - 1)
            
        if self.position > end:
            return b''
            
        headers = {'Range': f'bytes={self.position}-{end}'}
        response = requests.get(self.url, headers=headers)
        
        if response.status_code not in (200, 206):
            raise IOError(f"Failed to read from URL: {response.status_code}")
            
        data = response.content
        self.position += len(data)
        return data
        
    def seek(self, offset: int, whence: int = 0) -> int:
        """Seek to a position in the stream"""
        if whence == 0:  # SEEK_SET
            self.position = offset
        elif whence == 1:  # SEEK_CUR
            self.position += offset
        elif whence == 2:  # SEEK_END
            self.position = self.content_length + offset
            
        self.position = max(0, min(self.position, self.content_length))
        return self.position
        
    def tell(self) -> int:
        """Return current position"""
        return self.position


class SCStore:
    """Simple Camera State store implementation"""
    
    def __init__(self, filename: str):
        self.filename = filename
        self.index = {}  # Dict[str, Tuple[int, int, bool]]
        self.cache = {}  # Dict[str, Any]
        self.header_dirty = True
        self.lock = threading.RLock()
        
        # Check if file exists and load header
        if self._is_url(filename):
            self._init_from_url()
        elif os.path.exists(filename):
            self._load_header()
        else:
            # Create new file
            with open(filename, 'wb') as f:
                f.write(PROJECT_NAME.encode())
                
    def _is_url(self, path: str) -> bool:
        """Check if path is a URL"""
        return path.startswith(('http://', 'https://', '/vsicurl/'))
        
    def _init_from_url(self) -> None:
        """Initialize from URL"""
        url = self.filename
        if url.startswith('/vsicurl/'):
            url = url[9:]  # Remove /vsicurl/ prefix
            
        stream = HttpStream(url)
        # Read header
        stream.seek(0)
        header = stream.read(len(PROJECT_NAME))
        if header.decode() != PROJECT_NAME:
            raise ValueError(f"Invalid file format: {header.decode()}")
            
        # Read index
        stream.seek(len(PROJECT_NAME))
        index_size_bytes = stream.read(8)
        index_size = struct.unpack('<Q', index_size_bytes)[0]
        index_data = stream.read(index_size)
        self.index = json.loads(index_data.decode())
        
    def _load_header(self) -> None:
        """Load header from file"""
        with open(self.filename, 'rb') as f:
                
            # Read index
            index_size_bytes = f.read(8)
            if not index_size_bytes:
                # Empty file, just the header
                return
                
            index_size = struct.unpack('<Q', index_size_bytes)[0]
            index_data = f.read(index_size)
            self.index = json.loads(index_data.decode())
            
    def flush(self) -> None:
        """Flush changes to disk"""
        with self.lock:
            if not self.header_dirty and not any(dirty for _, _, dirty in self.index.values()):
                return
                
            # Create a temporary file
            temp_filename = f"{self.filename}.tmp"
            with open(temp_filename, 'wb') as temp_file:
                # Write header
                temp_file.write(PROJECT_NAME.encode())
                
                # Placeholder for index size
                temp_file.write(struct.pack('<Q', 0))
                
                # Write values
                for key, (position, size, dirty) in self.index.items():
                    if position == 0 or dirty:
                        # Value needs to be written
                        if key in self.cache:
                            value = self.cache[key]
                            # Record position
                            new_position = temp_file.tell()
                            # Serialize value
                            value_data = json.dumps(value).encode()
                            temp_file.write(struct.pack('<Q', len(value_data)))
                            temp_file.write(value_data)
                            # Update index
                            self.index[key] = (new_position, len(value_data), False)
                        else:
                            # Key exists but value not in cache, copy from original file
                            with open(self.filename, 'rb') as orig_file:
                                orig_file.seek(position)
                                value_data = orig_file.read(size)
                                new_position = temp_file.tell()
                                temp_file.write(value_data)
                                self.index[key] = (new_position, size, False)
                    else:
                        # Value unchanged, copy from original file
                        with open(self.filename, 'rb') as orig_file:
                            orig_file.seek(position)
                            value_data = orig_file.read(size)
                            new_position = temp_file.tell()
                            temp_file.write(value_data)
                            self.index[key] = (new_position, size, False)
                
                # Write index
                index_pos = temp_file.tell()
                index_data = json.dumps(self.index).encode()
                temp_file.write(struct.pack('<Q', len(index_data)))
                temp_file.write(index_data)
                
                # Update index size
                temp_file.seek(len(PROJECT_NAME))
                temp_file.write(struct.pack('<Q', len(index_data)))
            
            # Replace original file with temp file
            os.replace(temp_filename, self.filename)
            self.header_dirty = False
            
    def put(self, key: str, value: Any) -> None:
        """Store a value with the given key"""
        with self.lock:
            self.cache[key] = value
            # Mark as not yet written to file
            self.index[key] = (0, 0, True)
            self.header_dirty = True
            
    def get(self, key: str) -> Optional[Any]:
        """Retrieve a value by key"""
        with self.lock:
            if key in self.cache:
                return self.cache[key]
                
            if key not in self.index:
                return None
                
            position, size, _ = self.index[key]
            if position == 0:  # Not yet written to file
                return None
                
            if self._is_url(self.filename):
                stream = HttpStream(self.filename)
                stream.seek(position)
                size_bytes = stream.read(8)
                value_size = struct.unpack('<Q', size_bytes)[0]
                value_data = stream.read(value_size)
                value = json.loads(value_data.decode())
            else:
                with open(self.filename, 'rb') as f:
                    f.seek(position)
                    size_bytes = f.read(8)
                    value_size = struct.unpack('<Q', size_bytes)[0]
                    value_data = f.read(value_size)
                    value = json.loads(value_data.decode())
                    
            self.cache[key] = value
            return value
            
    def contains(self, key: str) -> bool:
        """Check if key exists in store"""
        with self.lock:
            return key in self.index
            
    def remove(self, key: str) -> None:
        """Remove a key-value pair"""
        with self.lock:
            if key in self.index:
                self.cache.pop(key, None)
                self.index.pop(key)
                self.header_dirty = True
                self.flush()
                
    def size(self) -> int:
        """Return number of key-value pairs"""
        with self.lock:
            return len(self.index)
            
    def keys(self) -> List[str]:
        """Return list of keys"""
        with self.lock:
            return list(self.index.keys())


def main():
    """Command line interface for SCS"""
    parser = argparse.ArgumentParser(description='Simple Camera State (SCS) CLI')
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    
    # Create command
    create_parser = subparsers.add_parser('create', help='Create a new SCS file')
    create_parser.add_argument('filename', help='Path to SCS file')
    
    # Put command
    put_parser = subparsers.add_parser('put', help='Store a value')
    put_parser.add_argument('filename', help='Path to SCS file')
    put_parser.add_argument('key', help='Key to store')
    put_parser.add_argument('value', help='Value to store (JSON format)')
    
    # Get command
    get_parser = subparsers.add_parser('get', help='Retrieve a value')
    get_parser.add_argument('filename', help='Path to SCS file')
    get_parser.add_argument('key', help='Key to retrieve')
    
    # List command
    list_parser = subparsers.add_parser('list', help='List all keys')
    list_parser.add_argument('filename', help='Path to SCS file')
    
    # Remove command
    remove_parser = subparsers.add_parser('remove', help='Remove a key-value pair')
    remove_parser.add_argument('filename', help='Path to SCS file')
    remove_parser.add_argument('key', help='Key to remove')
    
    args = parser.parse_args()
    
    if args.command == 'create':
        store = SCStore(args.filename)
        print(f"Created SCS file: {args.filename}")
        
    elif args.command == 'put':
        store = SCStore(args.filename)
        try:
            value = json.loads(args.value)
            store.put(args.key, value)
            store.flush()
            print(f"Stored value for key: {args.key}")
        except json.JSONDecodeError:
            print(f"Error: Value must be valid JSON")
            return 1
            
    elif args.command == 'get':
        store = SCStore(args.filename)
        value = store.get(args.key)
        if value is not None:
            print(json.dumps(value, indent=2))
        else:
            print(f"Key not found: {args.key}")
            return 1
            
    elif args.command == 'list':
        store = SCStore(args.filename)
        keys = store.keys()
        for key in keys:
            print(key)
        print(f"Total keys: {len(keys)}")
        
    elif args.command == 'remove':
        store = SCStore(args.filename)
        if store.contains(args.key):
            store.remove(args.key)
            print(f"Removed key: {args.key}")
        else:
            print(f"Key not found: {args.key}")
            return 1
    else:
        parser.print_help()
        
    return 0


if __name__ == "__main__":
    sys.exit(main())
