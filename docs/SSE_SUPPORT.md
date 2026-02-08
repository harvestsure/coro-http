# Server-Sent Events (SSE) Support

## Overview

The coro_http library provides full support for consuming Server-Sent Events (SSE) streams over HTTP and HTTPS, following the WHATWG EventSource specification.

## Features

- ✅ Full WHATWG EventSource spec compliance
- ✅ Multi-line data field support
- ✅ Custom event fields and types
- ✅ Automatic retry timing handling
- ✅ Async API with C++20 coroutines
- ✅ HTTP and HTTPS support

## SseEvent Structure

```cpp
struct SseEvent {
    std::string type;                          // Event type
    std::string data;                          // Combined data from multiple lines
    std::string id;                            // Event identifier
    std::string retry;                         // Retry interval in milliseconds
    std::map<std::string, std::string> fields; // Custom fields
};
```

## Asynchronous API Example

```cpp
#include <coro_http/coro_http.hpp>
#include <iostream>

int main() {
    asio::io_context io_ctx;
    coro_http::CoroHttpClient client(io_ctx);
    
    client.run([&client]() -> asio::awaitable<void> {
        coro_http::HttpRequest request(coro_http::HttpMethod::GET,
                                       "https://example.com/events");
        request.add_header("Accept", "text/event-stream");
        request.add_header("Cache-Control", "no-cache");
        
        int event_count = 0;
        
        co_await client.co_stream_events(request, 
            [&event_count](const coro_http::SseEvent& event) {
                event_count++;
                
                if (!event.type.empty()) {
                    std::cout << "Type: " << event.type << "\n";
                }
                if (!event.id.empty()) {
                    std::cout << "ID: " << event.id << "\n";
                }
                std::cout << "Data: " << event.data << "\n";
                
                // Handle custom fields
                for (const auto& [key, value] : event.fields) {
                    std::cout << key << ": " << value << "\n";
                }
                std::cout << "\n";
            });
        
        std::cout << "Stream completed. Total events: " << event_count << "\n";
    });
    
    return 0;
}
```

## Example: Multiple Event Streams

```cpp
#include <coro_http/coro_http.hpp>

asio::awaitable<void> handle_stream(coro_http::CoroHttpClient& client, 
                                    const std::string& url) {
    coro_http::HttpRequest request(coro_http::HttpMethod::GET, url);
    request.add_header("Accept", "text/event-stream");
    
    co_await client.co_stream_events(request,
        [url](const coro_http::SseEvent& event) {
            std::cout << "Stream(" << url << "): " << event.data << "\n";
        });
}

int main() {
    asio::io_context io_ctx;
    coro_http::CoroHttpClient client(io_ctx);
    
    client.run([&]() -> asio::awaitable<void> {
        // Handle multiple streams concurrently
        co_await handle_stream(client, "https://stream1.example.com");
        co_await handle_stream(client, "https://stream2.example.com");
    });
    
    return 0;
}

## SSE Message Format

The EventSource protocol uses simple text-based messages:

```
event: message
id: 1
data: {"value": "test"}

event: custom_type
id: 2
retry: 5000
data: Line 1
data: Line 2
data: Line 3
custom_field: custom_value

```

### Field Descriptions

- `event:` - Event type (optional, defaults to "message")
- `id:` - Event ID for reconnection support
- `data:` - Event payload (can span multiple lines)
- `retry:` - Recommended reconnection timeout in milliseconds
- Custom fields - Any line starting with field name followed by colon

## Implementation Details

### Multi-line Data Handling

Data fields spanning multiple lines are automatically concatenated with newlines:

```
data: Line 1
data: Line 2
data: Line 3
```

Results in:
```
"Line 1\nLine 2\nLine 3"
```

### Custom Event Types

Events can have custom types using the `event:` field:

```
event: custom_notification
id: 42
data: Event data here
```

Access via:
```cpp
if (event.type == "custom_notification") {
    // Handle custom event
}
```

### Heartbeat Support

Comments (lines starting with `:`) are ignored and useful for heartbeats:

```
: heartbeat
```

These keep the connection alive and are automatically discarded.

## Testing

A test server is included for local testing:

```bash
python3 examples/test_sse_server.py
```

Then run the example:

```bash
./build/example_sse_coro http://localhost:8888/events
```

The test server provides 6 sample events demonstrating all SSE features.
