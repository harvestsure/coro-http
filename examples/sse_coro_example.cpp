#include <coro_http/coro_http.hpp>
#include <iostream>
#include <asio.hpp>
#include <string>

// Real SSE test endpoints
const std::string SSE_ENDPOINT = "http://localhost:8888/events";
// Other available options:
// - Local: "http://localhost:8888/events" (run python3 test_sse_server.py)
// - Hugging Face: "https://api-inference.huggingface.co/models/gpt2" (requires API token)

// Coroutine example: SSE stream handler
asio::awaitable<void> handle_sse_stream(coro_http::CoroHttpClient& client, const std::string& url) {
    try {
        std::cout << "=== SSE Streaming Example (Coroutine) ===" << "\n\n";
        std::cout << "Connecting to: " << url << "\n\n";

        // Create an HTTP request for SSE endpoint
        coro_http::HttpRequest request(coro_http::HttpMethod::GET, url);

        // Add any custom headers if needed
        request.add_header("Accept", "text/event-stream");
        request.add_header("Cache-Control", "no-cache");

        // Counter for events
        int event_count = 0;
        const int max_events = 10;

        std::cout << "Listening for SSE events (async)...\n" << std::endl;

        // Stream events with callback using coroutine
        co_await client.co_stream_events(request, [&event_count, max_events](const coro_http::SseEvent& event) {
            event_count++;
            
            std::cout << "--- Event " << event_count << " ---\n";
            
            if (!event.type.empty()) {
                std::cout << "Type: " << event.type << "\n";
            }
            
            if (!event.id.empty()) {
                std::cout << "ID: " << event.id << "\n";
            }
            
            if (!event.data.empty()) {
                // Handle multi-line data
                std::cout << "Data: " << event.data << "\n";
            }
            
            if (!event.retry.empty()) {
                std::cout << "Retry: " << event.retry << " ms\n";
            }
            
            // Print any custom fields
            for (const auto& [key, value] : event.fields) {
                std::cout << key << ": " << value << "\n";
            }
            
            std::cout << std::endl;
            std::cout.flush();
        });

        std::cout << "Stream completed. Total events received: " << event_count << "\n";
        std::cout.flush();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    co_return;
}

// Example: Multiple concurrent SSE streams
asio::awaitable<void> multi_stream_example(coro_http::CoroHttpClient& client) {
    std::cout << "=== Multiple SSE Streams (Concurrent) ===" << "\n\n";

    // Stream 1
    auto stream1 = [&client]() -> asio::awaitable<void> {
        std::cout << "Stream 1: Starting...\n";
        coro_http::HttpRequest req1(coro_http::HttpMethod::GET, SSE_ENDPOINT);
        int count1 = 0;
        co_await client.co_stream_events(req1, [&count1](const coro_http::SseEvent& event) {
            count1++;
            std::cout << "Stream 1, Event " << count1 << ": " << event.data << "\n";
        });
        std::cout << "Stream 1: Completed with " << count1 << " events\n";
    };

    // Stream 2
    auto stream2 = [&client]() -> asio::awaitable<void> {
        std::cout << "Stream 2: Starting...\n";
        coro_http::HttpRequest req2(coro_http::HttpMethod::GET, SSE_ENDPOINT);
        int count2 = 0;
        co_await client.co_stream_events(req2, [&count2](const coro_http::SseEvent& event) {
            count2++;
            std::cout << "Stream 2, Event " << count2 << ": " << event.data << "\n";
        });
        std::cout << "Stream 2: Completed with " << count2 << " events\n";
    };

    // Run both streams concurrently with asio::co_spawn if needed
    // For now, just run them sequentially for demo purposes
    co_await stream1();
    co_await stream2();
}

int main(int argc, char* argv[]) {
    try {
        asio::io_context io_ctx;
        coro_http::CoroHttpClient client(io_ctx);

        // Use command-line argument or default URL
        std::string url = (argc > 1) ? argv[1] : SSE_ENDPOINT;

        // Run the coroutine handler
        client.run([&client, &url]() -> asio::awaitable<void> {
            co_await handle_sse_stream(client, url);
        });

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
