#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "url_parser.hpp"
#include "http_parser.hpp"
#include "client_config.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <asio/steady_timer.hpp>
#include <system_error>
#include <memory>

namespace coro_http {

class HttpClient {
public:
    HttpClient() : HttpClient(ClientConfig{}) {}
    
    explicit HttpClient(const ClientConfig& config) 
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

    HttpResponse execute(const HttpRequest& request) {
        return execute_with_redirects(request, 0);
    }

private:
    HttpResponse execute_with_redirects(const HttpRequest& request, int redirect_count) {
        auto url_info = parse_url(request.url());
        
        HttpResponse response;
        if (url_info.is_https) {
            response = execute_https(request, url_info);
        } else {
            response = execute_http(request, url_info);
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
                
                auto redirect_resp = execute_with_redirects(redirect_req, redirect_count + 1);
                for (const auto& url : response.redirect_chain()) {
                    redirect_resp.add_redirect(url);
                }
                return redirect_resp;
            }
        }
        
        return response;
    }

    HttpResponse execute_http(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(url_info.host, url_info.port);
        
        asio::ip::tcp::socket socket(io_context_);
        
        asio::steady_timer timer(io_context_);
        timer.expires_after(config_.connect_timeout);
        bool timeout_occurred = false;
        
        timer.async_wait([&](const std::error_code& ec) {
            if (!ec) {
                timeout_occurred = true;
                socket.close();
            }
        });
        
        std::error_code connect_ec;
        asio::connect(socket, endpoints, connect_ec);
        timer.cancel();
        
        if (timeout_occurred || connect_ec == asio::error::operation_aborted) {
            throw std::runtime_error("Connection timeout");
        }
        if (connect_ec) {
            throw std::system_error(connect_ec);
        }
        
        std::string request_str = build_request(request, url_info, config_.enable_compression);
        asio::write(socket, asio::buffer(request_str));
        
        std::string response_data = read_with_timeout(socket);
        
        return parse_response(response_data);
    }

    HttpResponse execute_https(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(url_info.host, url_info.port);
        
        asio::ssl::stream<asio::ip::tcp::socket> ssl_socket(io_context_, ssl_context_);
        
        if (config_.verify_ssl) {
            SSL_set_tlsext_host_name(ssl_socket.native_handle(), url_info.host.c_str());
        }
        
        asio::steady_timer timer(io_context_);
        timer.expires_after(config_.connect_timeout);
        bool timeout_occurred = false;
        
        timer.async_wait([&](const std::error_code& ec) {
            if (!ec) {
                timeout_occurred = true;
                ssl_socket.next_layer().close();
            }
        });
        
        std::error_code connect_ec;
        asio::connect(ssl_socket.next_layer(), endpoints, connect_ec);
        
        if (!timeout_occurred && !connect_ec) {
            ssl_socket.handshake(asio::ssl::stream_base::client, connect_ec);
        }
        
        timer.cancel();
        
        if (timeout_occurred || connect_ec == asio::error::operation_aborted) {
            throw std::runtime_error("Connection timeout");
        }
        if (connect_ec) {
            throw std::system_error(connect_ec);
        }
        
        std::string request_str = build_request(request, url_info, config_.enable_compression);
        asio::write(ssl_socket, asio::buffer(request_str));
        
        std::string response_data = read_with_timeout(ssl_socket);
        
        return parse_response(response_data);
    }

    template<typename SyncReadStream>
    std::string read_with_timeout(SyncReadStream& stream) {
        std::string response_data;
        std::array<char, 8192> buffer;
        
        asio::steady_timer timer(io_context_);
        timer.expires_after(config_.read_timeout);
        bool timeout_occurred = false;
        
        timer.async_wait([&](const std::error_code& ec) {
            if (!ec) {
                timeout_occurred = true;
                std::error_code close_ec;
                stream.lowest_layer().close(close_ec);
            }
        });
        
        std::error_code ec;
        while (true) {
            size_t len = stream.read_some(asio::buffer(buffer), ec);
            if (len > 0) {
                response_data.append(buffer.data(), len);
            }
            
            if (ec == asio::error::eof || ec == asio::ssl::error::stream_truncated) {
                break;
            } else if (ec == asio::error::operation_aborted && timeout_occurred) {
                timer.cancel();
                throw std::runtime_error("Read timeout");
            } else if (ec) {
                timer.cancel();
                throw std::system_error(ec);
            }
        }
        
        timer.cancel();
        return response_data;
    }

public:

    HttpResponse get(const std::string& url) {
        return execute(HttpRequest(HttpMethod::GET, url));
    }

    HttpResponse post(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::POST, url).set_body(body));
    }

    HttpResponse put(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::PUT, url).set_body(body));
    }

    HttpResponse del(const std::string& url) {
        return execute(HttpRequest(HttpMethod::DELETE, url));
    }

    HttpResponse head(const std::string& url) {
        return execute(HttpRequest(HttpMethod::HEAD, url));
    }

    HttpResponse patch(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::PATCH, url).set_body(body));
    }

    HttpResponse options(const std::string& url) {
        return execute(HttpRequest(HttpMethod::OPTIONS, url));
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
