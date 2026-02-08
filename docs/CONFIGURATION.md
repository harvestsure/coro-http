# Configuration

## ClientConfig Structure

```cpp
struct ClientConfig {
    // Timeout settings
    std::chrono::milliseconds connect_timeout = 10s;
    std::chrono::milliseconds read_timeout = 30s;
    std::chrono::milliseconds request_timeout = 60s;
    
    // Feature flags
    bool enable_compression = true;
    bool verify_ssl = true;
    bool enable_cookies = true;
    bool allow_redirects = true;
    
    // Pool settings
    size_t max_pool_connections = 10;
    std::chrono::seconds keepalive_timeout = 30s;
};
```

## Basic Configuration

```cpp
asio::io_context io_ctx;
coro_http::CoroHttpClient client(io_ctx);

coro_http::ClientConfig config;
config.connect_timeout = std::chrono::seconds(5);
config.read_timeout = std::chrono::seconds(15);
config.verify_ssl = true;
config.enable_compression = true;

client.set_config(config);
```

## Timeout Settings

```cpp
// Connection timeout
config.connect_timeout = std::chrono::seconds(10);

// Read timeout per operation
config.read_timeout = std::chrono::seconds(30);

// Total request timeout
config.request_timeout = std::chrono::seconds(60);
```

## SSL/TLS Configuration

```cpp
// Disable SSL verification (NOT recommended for production!)
config.verify_ssl = false;

// Custom CA certificate
// (Requires implementation in ssl_context setup)
```

## Compression

```cpp
// Enable automatic gzip/deflate decompression
config.enable_compression = true;

// This automatically handles:
// - Content-Encoding: gzip
// - Content-Encoding: deflate
// - Content-Encoding: identity (no compression)
```

## Cookies

```cpp
// Enable automatic cookie jar
config.enable_cookies = true;

// Cookies are automatically saved and sent with matching requests
```

## Redirects

```cpp
// Enable automatic redirect following
config.allow_redirects = true;

// Automatically follows 301, 302, 303, 307, 308 redirects
```

## Connection Pooling

```cpp
// Maximum connections to keep alive
config.max_pool_connections = 10;

// Timeout for idle connections
config.keepalive_timeout = std::chrono::seconds(30);
```

## Rate Limiting

```cpp
// Built-in rate limiter prevents API throttling
coro_http::RateLimiter limiter(
    10,  // Max requests per second
    std::chrono::seconds(1)
);

client.set_rate_limiter(limiter);
```

## Proxy Configuration

```cpp
// HTTP Proxy
config.proxy_url = "http://proxy.example.com:8080";

// SOCKS5 Proxy  
config.proxy_url = "socks5://proxy.example.com:1080";

// With authentication
config.proxy_url = "http://user:pass@proxy.example.com:8080";
```

## Retry Policy

```cpp
coro_http::RetryPolicy retry_policy;
retry_policy.max_retries = 3;
retry_policy.initial_delay = std::chrono::milliseconds(100);
retry_policy.max_delay = std::chrono::seconds(10);
retry_policy.backoff_factor = 2.0;

// Exponential backoff: 100ms, 200ms, 400ms...
```

## Per-Request Configuration

Individual requests can override global settings:

```cpp
coro_http::HttpRequest request(coro_http::HttpMethod::GET, "https://api.example.com");

// Add custom headers
request.add_header("X-Custom-Header", "value");
request.add_header("Accept", "application/json");

// Override timeout for this request
request.set_timeout(std::chrono::seconds(5));
```
