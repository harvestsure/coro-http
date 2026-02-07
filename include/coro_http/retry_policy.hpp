#pragma once

#include <chrono>
#include <thread>
#include <random>
#include <string>

namespace coro_http {

class RetryPolicy {
public:
    RetryPolicy(int max_retries, 
                std::chrono::milliseconds initial_delay,
                double backoff_factor,
                std::chrono::milliseconds max_delay,
                bool retry_on_timeout,
                bool retry_on_connection_error,
                bool retry_on_5xx)
        : max_retries_(max_retries),
          initial_delay_(initial_delay),
          backoff_factor_(backoff_factor),
          max_delay_(max_delay),
          retry_on_timeout_(retry_on_timeout),
          retry_on_connection_error_(retry_on_connection_error),
          retry_on_5xx_(retry_on_5xx),
          current_attempt_(0) {}
    
    bool should_retry(const std::exception& e, int status_code = 0) const {
        if (current_attempt_ >= max_retries_) {
            return false;
        }
        
        std::string error_msg = e.what();
        
        // Check for timeout errors
        if (retry_on_timeout_) {
            if (error_msg.find("timeout") != std::string::npos ||
                error_msg.find("Timeout") != std::string::npos ||
                error_msg.find("timed out") != std::string::npos) {
                return true;
            }
        }
        
        // Check for connection errors
        if (retry_on_connection_error_) {
            if (error_msg.find("Connection") != std::string::npos ||
                error_msg.find("connection") != std::string::npos ||
                error_msg.find("refused") != std::string::npos ||
                error_msg.find("reset") != std::string::npos ||
                error_msg.find("broken pipe") != std::string::npos ||
                error_msg.find("network") != std::string::npos) {
                return true;
            }
        }
        
        // Check for 5xx server errors
        if (retry_on_5xx_ && status_code >= 500 && status_code < 600) {
            return true;
        }
        
        return false;
    }
    
    std::chrono::milliseconds get_delay() const {
        if (current_attempt_ == 0) {
            return initial_delay_;
        }
        
        // Calculate exponential backoff with jitter
        double base_delay = initial_delay_.count() * 
                           std::pow(backoff_factor_, current_attempt_);
        
        // Add jitter (±25% random variation)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.75, 1.25);
        double jitter = dis(gen);
        
        auto delay = std::chrono::milliseconds(
            static_cast<long long>(base_delay * jitter)
        );
        
        // Cap at max delay
        if (delay > max_delay_) {
            delay = max_delay_;
        }
        
        return delay;
    }
    
    void increment_attempt() {
        current_attempt_++;
    }
    
    void reset() {
        current_attempt_ = 0;
    }
    
    int current_attempt() const {
        return current_attempt_;
    }
    
    int max_retries() const {
        return max_retries_;
    }
    
    void sleep_for_retry() const {
        auto delay = get_delay();
        std::this_thread::sleep_for(delay);
    }

private:
    int max_retries_;
    std::chrono::milliseconds initial_delay_;
    double backoff_factor_;
    std::chrono::milliseconds max_delay_;
    bool retry_on_timeout_;
    bool retry_on_connection_error_;
    bool retry_on_5xx_;
    mutable int current_attempt_;
};

}
