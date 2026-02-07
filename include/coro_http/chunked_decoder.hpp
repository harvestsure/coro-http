#pragma once

#include <string>
#include <sstream>

namespace coro_http {

inline std::string decode_chunked(const std::string& data) {
    std::istringstream stream(data);
    std::string result;
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.back() == '\r') line.pop_back();
        
        if (line.empty()) continue;
        
        size_t chunk_size = 0;
        try {
            chunk_size = std::stoul(line, nullptr, 16);
        } catch (...) {
            break;
        }
        
        if (chunk_size == 0) break;
        
        std::string chunk_data(chunk_size, '\0');
        stream.read(&chunk_data[0], chunk_size);
        result.append(chunk_data);
        
        std::getline(stream, line);
    }
    
    return result;
}

}
