#include <iostream>
#include <chrono>
#include "coro_http/http_client.hpp"
#include "coro_http/coro_http_client.hpp"

using namespace coro_http;

void sync_retry_demo() {
    std::cout << "=== Synchronous Retry Demo ===\n\n";
    
    ClientConfig config;
    config.enable_retry = true;
    config.max_retries = 3;
    config.initial_retry_delay = std::chrono::milliseconds(500);
    config.retry_backoff_factor = 2.0;
    config.retry_on_timeout = true;
    config.retry_on_connection_error = true;
    config.connect_timeout = std::chrono::milliseconds(2000);
    
    HttpClient client(config);
    
    std::cout << "Retry configuration:\n";
    std::cout << "  Max retries: " << config.max_retries << "\n";
    std::cout << "  Initial delay: " << config.initial_retry_delay.count() << "ms\n";
    std::cout << "  Backoff factor: " << config.retry_backoff_factor << "x\n\n";
    
    // Test 1: Connection to non-existent host (will retry)
    std::cout << "Test 1: Connecting to non-existent host (will retry 3 times)...\n";
    auto start = std::chrono::steady_clock::now();
    
    try {
        auto response = client.get("http://this-host-definitely-does-not-exist-12345.com");
        std::cout << "Success: " << response.status_code() << "\n";
    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Failed after retries: " << e.what() << "\n";
        std::cout << "Total time: " << duration.count() << "ms\n";
        std::cout << "(Expected: ~2s connect timeout × 4 attempts = ~8s)\n\n";
    }
    
    // Test 2: Delayed endpoint (simulates timeout)
    std::cout << "Test 2: Requesting delayed endpoint...\n";
    try {
        // httpbin.org/delay/N delays response by N seconds
        auto response = client.get("http://httpbin.org/delay/1");
        std::cout << "Success: " << response.status_code() << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
}

void sync_5xx_retry_demo() {
    std::cout << "=== 5xx Error Retry Demo ===\n\n";
    
    ClientConfig config;
    config.enable_retry = true;
    config.max_retries = 2;
    config.initial_retry_delay = std::chrono::milliseconds(1000);
    config.retry_on_5xx = true;  // Enable retry on server errors
    
    HttpClient client(config);
    
    std::cout << "Testing 5xx error retry (retry_on_5xx = true)...\n";
    
    try {
        // httpbin.org/status/500 returns 500 Internal Server Error
        auto response = client.get("http://httpbin.org/status/503");
        std::cout << "Response status: " << response.status_code() << "\n";
        std::cout << "Note: Successfully got response even though it's 5xx\n";
        std::cout << "(Retries exhaused, returning last response)\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
}

void async_retry_demo() {
    std::cout << "=== Asynchronous Retry Demo ===\n\n";
    
    ClientConfig config;
    config.enable_retry = true;
    config.max_retries = 3;
    config.initial_retry_delay = std::chrono::milliseconds(500);
    config.retry_backoff_factor = 2.0;
    config.retry_on_timeout = true;
    config.connect_timeout = std::chrono::milliseconds(2000);
    
    CoroHttpClient client(config);
    
    client.run([&]() -> asio::awaitable<void> {
        std::cout << "Async retry with exponential backoff...\n";
        std::cout << "Connecting to unreachable host...\n\n";
        
        auto start = std::chrono::steady_clock::now();
        
        try {
            auto response = co_await client.co_get("http://192.0.2.1:9999");  // Reserved IP, will timeout
            std::cout << "Success: " << response.status_code() << "\n";
        } catch (const std::exception& e) {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "Failed after retries: " << e.what() << "\n";
            std::cout << "Total time: " << duration.count() << "ms\n";
            std::cout << "Retry delays: 500ms + 1000ms + 2000ms = 3500ms\n";
            std::cout << "Plus " << config.max_retries + 1 << " connect timeouts\n\n";
        }
    });
}

void production_example() {
    std::cout << "=== Production Configuration Example ===\n\n";
    
    ClientConfig config;
    // Connection pool for performance
    config.enable_connection_pool = true;
    config.max_connections_per_host = 5;
    
    // Rate limiting to respect API limits
    config.enable_rate_limit = true;
    config.rate_limit_requests = 10;
    config.rate_limit_window = std::chrono::seconds(1);
    
    // Automatic retry for reliability
    config.enable_retry = true;
    config.max_retries = 3;
    config.initial_retry_delay = std::chrono::milliseconds(1000);
    config.retry_on_timeout = true;
    config.retry_on_connection_error = true;
    config.retry_on_5xx = false;  // Only retry transient errors
    
    // Reasonable timeouts
    config.connect_timeout = std::chrono::milliseconds(5000);
    config.read_timeout = std::chrono::milliseconds(10000);
    
    CoroHttpClient client(config);
    
    std::cout << "Configuration:\n";
    std::cout << "  - Connection pooling: enabled\n";
    std::cout << "  - Rate limiting: 10 req/s\n";
    std::cout << "  - Auto retry: up to 3 attempts\n";
    std::cout << "  - Retry on: timeouts, connection errors\n\n";
    
    client.run([&]() -> asio::awaitable<void> {
        std::cout << "Making reliable API calls...\n\n";
        
        for (int i = 1; i <= 3; ++i) {
            try {
                std::cout << "Request " << i << "... ";
                auto response = co_await client.co_get("http://httpbin.org/uuid");
                std::cout << "OK (" << response.status_code() << ")\n";
            } catch (const std::exception& e) {
                std::cout << "Failed: " << e.what() << "\n";
            }
        }
        
        std::cout << "\nAll requests completed with automatic retry protection!\n\n";
    });
}

int main() {
    std::cout << "HTTP Client Retry Examples\n";
    std::cout << "===========================\n\n";
    
    std::cout << "This demo shows automatic retry with exponential backoff:\n";
    std::cout << "1. Retrying connection failures\n";
    std::cout << "2. Retrying timeout errors\n";
    std::cout << "3. Optionally retrying 5xx server errors\n";
    std::cout << "4. Production-ready configuration\n\n";
    
    // Comment out to run specific tests
    sync_retry_demo();
    sync_5xx_retry_demo();
    async_retry_demo();
    production_example();
    
    return 0;
}
