#include <coro_http/coro_http.hpp>
#include <iostream>
#include <asio.hpp>
#include <string>

// Real SSE test endpoints
const std::string SSE_ENDPOINT = "http://localhost:8888/events";
// Other available options:
// - Local: "http://localhost:8888/events" (run python3 test_sse_server.py)
// - Hugging Face: "https://api-inference.huggingface.co/models/gpt2" (requires API token)

int main(int argc, char* argv[]) {
    try {
        asio::io_context io_ctx;
        coro_http::HttpClient client(io_ctx);

        std::cout << "=== SSE Streaming Example (Sync) ===" << "\n\n";

        // Use command-line argument or default URL
        std::string url = (argc > 1) ? argv[1] : SSE_ENDPOINT;
        std::cout << "Connecting to: " << url << "\n\n";

        // Create an HTTP request for SSE endpoint
        coro_http::HttpRequest request(coro_http::HttpMethod::GET, url);

        // Add any custom headers if needed
        request.add_header("Accept", "text/event-stream");

        // Counter for events
        int event_count = 0;
        const int max_events = 10;  // Limit to 10 events for demo

        std::cout << "Listening for SSE events...\n" << std::endl;

        // Stream events with callback
        client.stream_events(request, [&event_count, max_events](const coro_http::SseEvent& event) {
            event_count++;
            
            std::cout << "--- Event " << event_count << " ---\n";
            
            if (!event.type.empty()) {
                std::cout << "Type: " << event.type << "\n";
            }
            
            if (!event.id.empty()) {
                std::cout << "ID: " << event.id << "\n";
            }
            
            if (!event.data.empty()) {
                std::cout << "Data: " << event.data << "\n";
            }
            
            if (!event.retry.empty()) {
                std::cout << "Retry: " << event.retry << " ms\n";
            }
            
            std::cout << std::endl;
            
            // For demo purposes, stop after some events
            if (event_count >= max_events) {
                std::cout << "Reached max events limit. Exiting...\n";
                // Note: In a real application, you'd need a way to break from the stream
                // This is just for demonstration purposes
            }
        });

        std::cout << "Stream completed. Total events received: " << event_count << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
