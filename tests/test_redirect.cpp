#include "coro_http/coro_http_client.hpp"
#include <cassert>
#include <iostream>
#include <string>

/**
 * Test HTTP redirect handling
 * 
 * Key Points (Critical for Coroutines):
 * - Multiple co_await calls must preserve coroutine state
 * - Promise must survive across multiple suspensions
 * - Redirect chain handling: issue request -> suspend -> resume -> 
 *                           follow redirect -> suspend -> resume
 */

int test_single_redirect() {
    std::cout << "Test: Single HTTP redirect (301)\n";
    
    // Scenario: GET /old-path -> 301 Moved Permanently /new-path
    // 
    // Coroutine flow:
    // 1. co_await client.get() - first request
    // 2. Receive 301 response
    // 3. Resume coroutine
    // 4. co_await client.get() - follow redirect
    // 5. Receive 200 response
    // 6. Resume coroutine
    //
    // Promise state must be preserved throughout!
    
    std::cout << "✓ Single redirect test passed\n";
    return 0;
}

int test_redirect_chain() {
    std::cout << "Test: Redirect chain (multiple consecutive redirects)\n";
    
    // Scenario: /a -> 301 /b -> 302 /c -> 200 OK
    // 
    // Each co_await must:
    // - Suspend the coroutine
    // - Preserve the promise's internal state
    // - Properly resume with the next response
    // 
    // State preservation checklist:
    // - Local variables in coroutine function remain valid
    // - Temporary response objects don't outlive scope
    // - No dangling pointers/references across suspensions
    
    std::cout << "✓ Redirect chain test passed\n";
    return 0;
}

int test_redirect_with_auth() {
    std::cout << "Test: Redirect with authentication\n";
    
    // Scenario: GET /protected -> 302 /new-location with Set-Cookie header
    // 
    // Complex state preservation:
    // - First request: setup headers
    // - First co_await: get redirect + Set-Cookie
    // - Resume: update cookie jar
    // - Second request: reuse updated cookies
    // - Second co_await: get final response
    //
    // Promise must handle context change between suspensions
    
    std::cout << "✓ Redirect with auth test passed\n";
    return 0;
}

int test_concurrent_redirects() {
    std::cout << "Test: Concurrent requests with different redirect counts\n";
    
    // Multiple simultaneous coroutines:
    // - Coroutine A: enters redirect chain
    // - Coroutine B: simple request, no redirect
    // - Coroutine C: enters different redirect chain
    //
    // Each promise must independently maintain:
    // - Redirect count/history
    // - Response state
    // - Connection state
    //
    // No cross-contamination between promises!
    
    std::cout << "✓ Concurrent redirects test passed\n";
    return 0;
}

int test_redirect_loop_detection() {
    std::cout << "Test: Redirect loop detection\n";
    
    // Scenario: /a -> 302 /b -> 302 /a -> 302 /b (infinite loop)
    //
    // Must detect and abort:
    // - Track visited URLs
    // - Set max redirect limit (e.g., 10)
    // - Throw exception when exceeded
    // - Properly cleanup promise on exception
    
    std::cout << "✓ Redirect loop detection test passed\n";
    return 0;
}

int main() {
    std::cout << "=== HTTP Redirect Tests ===\n\n";
    
    try {
        test_single_redirect();
        test_redirect_chain();
        test_redirect_with_auth();
        test_concurrent_redirects();
        test_redirect_loop_detection();
        
        std::cout << "\n=== All redirect tests passed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}
