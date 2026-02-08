# Examples

## Basic GET Request

```cpp
#include <coro_http/coro_http.hpp>
#include <iostream>

int main() {
    asio::io_context io_ctx;
    coro_http::HttpClient client(io_ctx);
    
    auto response = client.get("https://httpbin.org/get");
    
    std::cout << "Status: " << response.status_code() << "\n";
    std::cout << "Body: " << response.body() << "\n";
    
    return 0;
}
```

## POST with JSON

```cpp
auto response = client.post(
    "https://httpbin.org/post",
    R"({"name": "test", "value": 123})"
);

if (response.status_code() == 200) {
    std::cout << "Success: " << response.body() << "\n";
}
```

## Custom Headers

```cpp
coro_http::HttpRequest request(coro_http::HttpMethod::GET, 
                               "https://api.github.com/user");
request.add_header("Authorization", "Bearer YOUR_TOKEN");
request.add_header("Accept", "application/vnd.github.v3+json");

auto response = client.get(request);
```

## Coroutine - Concurrent Requests

```cpp
int main() {
    asio::io_context io_ctx;
    coro_http::CoroHttpClient client(io_ctx);
    
    client.run([&client]() -> asio::awaitable<void> {
        auto req1 = client.co_get("https://httpbin.org/get");
        auto req2 = client.co_get("https://httpbin.org/delay/2");
        
        auto [resp1, resp2] = co_await asio::when_all(req1, req2);
        
        std::cout << "Response 1: " << resp1.status_code() << "\n";
        std::cout << "Response 2: " << resp2.status_code() << "\n";
    });
    
    return 0;
}
```

## SSE - Sync Version

```cpp
#include <coro_http/coro_http.hpp>

int main() {
    asio::io_context io_ctx;
    coro_http::HttpClient client(io_ctx);
    
    coro_http::HttpRequest request(coro_http::HttpMethod::GET,
                                   "http://localhost:8888/events");
    
    client.stream_events(request, [](const coro_http::SseEvent& event) {
        std::cout << "Event: " << event.type << "\n";
        std::cout << "Data: " << event.data << "\n";
    });
    
    return 0;
}
```

## SSE - Async/Coroutine Version

```cpp
#include <coro_http/coro_http.hpp>

int main() {
    asio::io_context io_ctx;
    coro_http::CoroHttpClient client(io_ctx);
    
    client.run([&client]() -> asio::awaitable<void> {
        coro_http::HttpRequest request(coro_http::HttpMethod::GET,
                                       "http://localhost:8888/events");
        
        int count = 0;
        co_await client.co_stream_events(request, 
            [&count](const coro_http::SseEvent& event) {
                count++;
                std::cout << "Event " << count << ": " 
                          << event.type << "\n";
            });
        
        std::cout << "Completed " << count << " events\n";
    });
    
    return 0;
}
```

## With Authentication

```cpp
coro_http::HttpRequest request(coro_http::HttpMethod::GET,
                               "https://api.example.com/data");
request.add_header("Authorization", "Bearer token123");

// For Basic Auth
std::string credentials = "user:password";
std::string encoded = base64_encode(credentials);
request.add_header("Authorization", "Basic " + encoded);

auto response = client.get(request);
```

## Retry Policy

```cpp
coro_http::RetryPolicy retry_policy;
retry_policy.max_retries = 3;
retry_policy.initial_delay = std::chrono::milliseconds(100);
retry_policy.max_delay = std::chrono::seconds(10);
retry_policy.backoff_factor = 2.0;

// Apply to request...
```

## Running Examples

Build examples:
```bash
cd coro-http
mkdir build && cd build
cmake ..
make example_sse_sync example_sse_coro
```

Run sync example:
```bash
./example_sse_sync http://localhost:8888/events
```

Run async example:
```bash
./example_sse_coro http://localhost:8888/events
```

Test server (in another terminal):
```bash
python3 examples/test_sse_server.py
```
