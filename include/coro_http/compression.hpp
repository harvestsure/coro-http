#pragma once

#include <string>
#include <zlib.h>
#include <stdexcept>

namespace coro_http {

inline std::string decompress_gzip(const std::string& compressed_data) {
    z_stream stream{};
    stream.avail_in = compressed_data.size();
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));
    
    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Failed to initialize gzip decompression");
    }
    
    std::string decompressed;
    char buffer[32768];
    
    int ret;
    do {
        stream.avail_out = sizeof(buffer);
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("Failed to decompress gzip data");
        }
        
        size_t have = sizeof(buffer) - stream.avail_out;
        decompressed.append(buffer, have);
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&stream);
    return decompressed;
}

inline std::string decompress_deflate(const std::string& compressed_data) {
    z_stream stream{};
    stream.avail_in = compressed_data.size();
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));
    
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("Failed to initialize deflate decompression");
    }
    
    std::string decompressed;
    char buffer[32768];
    
    int ret;
    do {
        stream.avail_out = sizeof(buffer);
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("Failed to decompress deflate data");
        }
        
        size_t have = sizeof(buffer) - stream.avail_out;
        decompressed.append(buffer, have);
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&stream);
    return decompressed;
}

}
