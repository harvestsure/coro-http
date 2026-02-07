#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "url_parser.hpp"
#include "http_parser.hpp"
#include "client_config.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/steady_timer.hpp>

namespace coro_http {

class CoroHttpClient {
public:
    CoroHttpClient() : CoroHttpClient(ClientConfig{}) {}
    
    explicit CoroHttpClient(const ClientConfig& config)
        : io_context_(), 
          ssl_context_(asio::ssl::context::tlsv12_client),
          config_(config) {
        ssl_context_.set_default_verify_paths();
        
        if (config_.verify_ssl) {
            ssl_context_.set_verify_mode(asio::ssl::verify_peer);
            if (!config_.ca_cert_file.empty()) {
                ssl_context_.load_verify_file(config_.ca_cert_file);
            }
            if (!config_.ca_cert_path.empty()) {
                ssl_context_.add_verify_path(config_.ca_cert_path);
            }
        } else {
            ssl_context_.set_verify_mode(asio::ssl::verify_none);
        }
    }

    asio::awaitable<HttpResponse> co_execute(const HttpRequest& request) {
        co_return co_await co_execute_with_redirects(request, 0);
    }

private:
    asio::awaitable<HttpResponse> co_execute_with_redirects(const HttpRequest& request, int redirect_count) {
        auto url_info = parse_url(request.url());
        
        HttpResponse response;
        if (url_info.is_https) {
            response = co_await co_execute_https(request, url_info);
        } else {
            response = co_await co_execute_http(request, url_info);
        }
        
        if (config_.follow_redirects && 
            redirect_count < config_.max_redirects &&
            (response.status_code() >= 300 && response.status_code() < 400)) {
            
            std::string location = response.get_header("Location");
            if (!location.empty()) {
                response.add_redirect(location);
                
                if (location[0] == '/') {
                    location = url_info.scheme + "://" + url_info.host + 
                              (url_info.port != (url_info.is_https ? "443" : "80") ? ":" + url_info.port : "") + 
                              location;
                }
                
                HttpRequest redirect_req(HttpMethod::GET, location);
                for (const auto& [key, value] : request.headers()) {
                    redirect_req.add_header(key, value);
                }
                
                auto redirect_resp = co_await co_execute_with_redirects(redirect_req, redirect_count + 1);
                for (const auto& url : response.redirect_chain()) {
                    redirect_resp.add_redirect(url);
                }
                co_return redirect_resp;
            }
        }
        
        co_return response;
    }

    asio::awaitable<HttpResponse> co_execute_http(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = co_await resolver.async_resolve(
            url_info.host, 
            url_info.port, 
            asio::use_awaitable
        );
        
        asio::ip::tcp::socket socket(io_context_);
        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
        
        std::string request_str = build_request(request, url_info, config_.enable_compression);
        co_await asio::async_write(socket, asio::buffer(request_str), asio::use_awaitable);
        
        std::string response_data = co_await co_read_response(socket);
        
        co_return parse_response(response_data);
    }

    asio::awaitable<HttpResponse> co_execute_https(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = co_await resolver.async_resolve(
            url_info.host, 
            url_info.port, 
            asio::use_awaitable
        );
        
        asio::ssl::stream<asio::ip::tcp::socket> ssl_socket(io_context_, ssl_context_);
        
        if (config_.verify_ssl) {
            SSL_set_tlsext_host_name(ssl_socket.native_handle(), url_info.host.c_str());
        }
        
        co_await asio::async_connect(ssl_socket.next_layer(), endpoints, asio::use_awaitable);
        co_await ssl_socket.async_handshake(asio::ssl::stream_base::client, asio::use_awaitable);
        
        std::string request_str = build_request(request, url_info, config_.enable_compression);
        co_await asio::async_write(ssl_socket, asio::buffer(request_str), asio::use_awaitable);
        
        std::string response_data = co_await co_read_response(ssl_socket);
        
        co_return parse_response(response_data);
    }

    template<typename AsyncReadStream>
    asio::awaitable<std::string> co_read_response(AsyncReadStream& stream) {
        std::string response_data;
        std::array<char, 8192> buffer;
        
        while (true) {
            auto [ec, len] = co_await stream.async_read_some(
                asio::buffer(buffer),
                asio::as_tuple(asio::use_awaitable)
            );
            
            if (len > 0) {
                response_data.append(buffer.data(), len);
            }
            
            if (ec == asio::error::eof || ec == asio::ssl::error::stream_truncated) {
                break;
            } else if (ec) {
                throw std::system_error(ec);
            }
        }
        
        co_return response_data;
    }

public:

    asio::awaitable<HttpResponse> co_get(const std::string& url) {
        co_return co_await co_execute(HttpRequest(HttpMethod::GET, url));
    }

    asio::awaitable<HttpResponse> co_post(const std::string& url, const std::string& body) {
        co_return co_await co_execute(HttpRequest(HttpMethod::POST, url).set_body(body));
    }

    asio::awaitable<HttpResponse> co_put(const std::string& url, const std::string& body) {
        co_return co_await co_execute(HttpRequest(HttpMethod::PUT, url).set_body(body));
    }

    asio::awaitable<HttpResponse> co_delete(const std::string& url) {
        co_return co_await co_execute(HttpRequest(HttpMethod::DELETE, url));
    }

    asio::awaitable<HttpResponse> co_head(const std::string& url) {
        co_return co_await co_execute(HttpRequest(HttpMethod::HEAD, url));
    }

    asio::awaitable<HttpResponse> co_patch(const std::string& url, const std::string& body) {
        co_return co_await co_execute(HttpRequest(HttpMethod::PATCH, url).set_body(body));
    }

    asio::awaitable<HttpResponse> co_options(const std::string& url) {
        co_return co_await co_execute(HttpRequest(HttpMethod::OPTIONS, url));
    }

    template<typename CoroFunc>
    void run(CoroFunc&& coro) {
        asio::co_spawn(io_context_, std::forward<CoroFunc>(coro), asio::detached);
        io_context_.run();
    }
    
    void set_config(const ClientConfig& config) {
        config_ = config;
    }
    
    const ClientConfig& get_config() const {
        return config_;
    }

private:
    asio::io_context io_context_;
    asio::ssl::context ssl_context_;
    ClientConfig config_;
};

}
