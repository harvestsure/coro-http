# Features

## HTTP Methods

- ✅ GET - Retrieve resource
- ✅ POST - Create resource  
- ✅ PUT - Update resource
- ✅ DELETE - Remove resource
- ✅ HEAD - Get headers only
- ✅ PATCH - Partial update
- ✅ OPTIONS - Get allowed methods

## Protocol Support

- ✅ HTTP/1.1
- ✅ HTTPS with SSL/TLS
- ✅ Transfer-Encoding: chunked
- ✅ Content-Encoding: gzip, deflate
- ✅ Keep-Alive connections

## API Styles

- ✅ Synchronous blocking API (HttpClient)
- ✅ C++20 Coroutine async/await (CoroHttpClient)
- ✅ Both operate on same underlying pool

## Performance Features

- ✅ Connection pooling with Keep-Alive
- ✅ Automatic connection reuse
- ✅ Configurable timeout control
- ✅ Rate limiting per client
- ✅ Concurrent request support

## Advanced Features

- ✅ Automatic HTTP redirects (3xx)
- ✅ Gzip/Deflate decompression
- ✅ Automatic retry with exponential backoff
- ✅ SSL/TLS certificate verification
- ✅ Custom CA certificate support
- ✅ Proxy support (HTTP/HTTPS/SOCKS5)

## Data Handling

- ✅ String/binary request bodies
- ✅ Form data encoding
- ✅ JSON support
- ✅ File streaming
- ✅ Custom headers
- ✅ Cookie jar with persistence

## Server-Sent Events

- ✅ Full WHATWG EventSource spec
- ✅ Multi-line data field support
- ✅ Custom event types
- ✅ Event IDs and retry timing
- ✅ Both sync and async APIs
- ✅ Automatic reconnection support

## Development

- ✅ Header-only library
- ✅ Modern C++20 features
- ✅ Zero-copy designs where possible
- ✅ Comprehensive error handling
- ✅ CMake integration
- ✅ Example code included

## Cross-Platform

- ✅ Linux
- ✅ macOS  
- ✅ Windows (via vcpkg)
- ✅ POSIX systems
