#pragma once

#include <string>
#include <map>
#include <vector>
#include <algorithm>

namespace coro_http {

inline bool strcasecmp_impl(const std::string& a, const std::string& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
        [](char ca, char cb) { return std::tolower(ca) == std::tolower(cb); });
}

class HttpResponse {
public:
    HttpResponse() : status_code_(0) {}

    void set_status_code(int code) { status_code_ = code; }
    void set_reason(const std::string& reason) { reason_ = reason; }
    void add_header(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }
    void set_body(const std::string& body) { body_ = body; }
    void add_redirect(const std::string& url) { redirect_chain_.push_back(url); }

    int status_code() const { return status_code_; }
    const std::string& reason() const { return reason_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }
    const std::string& body() const { return body_; }
    const std::vector<std::string>& redirect_chain() const { return redirect_chain_; }

    std::string get_header(const std::string& key) const {
        auto it = headers_.find(key);
        if (it != headers_.end()) return it->second;
        for (const auto& [k, v] : headers_) {
            if (strcasecmp_impl(k, key)) return v;
        }
        return "";
    }

private:
    int status_code_;
    std::string reason_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::vector<std::string> redirect_chain_;
};

}
