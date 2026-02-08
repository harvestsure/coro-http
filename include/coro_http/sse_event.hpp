#pragma once

#include <string>
#include <map>
#include <sstream>
#include <vector>

namespace coro_http {

// SSE Event structure according to https://html.spec.whatwg.org/multipage/server-sent-events.html
struct SseEvent {
    std::string type;           // event field
    std::string data;           // data field (combines all data lines)
    std::string id;             // id field
    std::string retry;          // retry field
    std::map<std::string, std::string> fields;  // all raw fields

    bool empty() const {
        return data.empty() && type.empty() && id.empty();
    }

    std::string to_string() const {
        std::ostringstream oss;
        if (!type.empty()) oss << "event: " << type << "\n";
        if (!id.empty()) oss << "id: " << id << "\n";
        if (!retry.empty()) oss << "retry: " << retry << "\n";
        if (!data.empty()) oss << "data: " << data << "\n";
        return oss.str();
    }
};

// Parse SSE format stream line by line
// Returns parsed events as they come in
inline std::vector<SseEvent> parse_sse_stream(const std::string& stream_data) {
    std::vector<SseEvent> events;
    std::istringstream stream(stream_data);
    std::string line;
    
    SseEvent current_event;
    std::vector<std::string> data_lines;

    while (std::getline(stream, line)) {
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Empty line indicates end of event
        if (line.empty()) {
            if (!data_lines.empty() || !current_event.type.empty() || 
                !current_event.id.empty() || !current_event.retry.empty()) {
                // Concatenate all data lines with newlines
                for (size_t i = 0; i < data_lines.size(); ++i) {
                    if (i > 0) current_event.data += "\n";
                    current_event.data += data_lines[i];
                }
                
                if (!current_event.empty()) {
                    events.push_back(current_event);
                }
                
                current_event = SseEvent{};
                data_lines.clear();
            }
            continue;
        }

        // Line starting with : is a comment
        if (line[0] == ':') {
            continue;
        }

        // Parse field: value
        size_t colon_pos = line.find(':');
        std::string field;
        std::string value;

        if (colon_pos == std::string::npos) {
            // Line with no colon
            field = line;
            value = "";
        } else {
            field = line.substr(0, colon_pos);
            value = line.substr(colon_pos + 1);
            
            // Remove leading space if present
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
        }

        // Process field
        if (field == "event") {
            current_event.type = value;
        } else if (field == "data") {
            data_lines.push_back(value);
        } else if (field == "id") {
            current_event.id = value;
        } else if (field == "retry") {
            current_event.retry = value;
        } else {
            // Unknown field - store it anyway
            current_event.fields[field] = value;
        }
    }

    // Handle last event if stream doesn't end with blank line
    if (!data_lines.empty() || !current_event.type.empty() || 
        !current_event.id.empty() || !current_event.retry.empty()) {
        for (size_t i = 0; i < data_lines.size(); ++i) {
            if (i > 0) current_event.data += "\n";
            current_event.data += data_lines[i];
        }
        
        if (!current_event.empty()) {
            events.push_back(current_event);
        }
    }

    return events;
}

// Parse a single line and append to current event data
// Used for streaming line-by-line SSE processing
inline void parse_sse_line(const std::string& line, SseEvent& current_event, 
                          std::vector<std::string>& data_lines, 
                          std::vector<SseEvent>& events) {
    std::string processed_line = line;
    
    // Remove trailing \r if present
    if (!processed_line.empty() && processed_line.back() == '\r') {
        processed_line.pop_back();
    }

    // Empty line indicates end of event
    if (processed_line.empty()) {
        if (!data_lines.empty() || !current_event.type.empty() || 
            !current_event.id.empty() || !current_event.retry.empty()) {
            
            // Concatenate all data lines with newlines
            for (size_t i = 0; i < data_lines.size(); ++i) {
                if (i > 0) current_event.data += "\n";
                current_event.data += data_lines[i];
            }
            
            if (!current_event.empty()) {
                events.push_back(current_event);
            }
            
            current_event = SseEvent{};
            data_lines.clear();
        }
        return;
    }

    // Line starting with : is a comment
    if (processed_line[0] == ':') {
        return;
    }

    // Parse field: value
    size_t colon_pos = processed_line.find(':');
    std::string field;
    std::string value;

    if (colon_pos == std::string::npos) {
        field = processed_line;
        value = "";
    } else {
        field = processed_line.substr(0, colon_pos);
        value = processed_line.substr(colon_pos + 1);
        
        // Remove leading space if present
        if (!value.empty() && value[0] == ' ') {
            value = value.substr(1);
        }
    }

    // Process field
    if (field == "event") {
        current_event.type = value;
    } else if (field == "data") {
        data_lines.push_back(value);
    } else if (field == "id") {
        current_event.id = value;
    } else if (field == "retry") {
        current_event.retry = value;
    } else {
        current_event.fields[field] = value;
    }
}

}
