// Modules/Utils/AutoRelease.h
#ifndef MODULES_UTILS_AUTORELEASE_H
#define MODULES_UTILS_AUTORELEASE_H

#include <functional> // For std::function
#include <utility>    // For std::move

/**
 * @brief A lightweight RAII helper to ensure cleanup logic is always executed.
 *
 * Usage:
 *
 * ```cpp
 * // Acquire a resource
 * auto conn = acquireConnection();
 * // Create an AutoRelease guard for cleanup
 * AutoRelease releaseGuard([&]() {
 * releaseConnection(conn); // This lambda captures 'conn' by reference
 * });
 * // Perform operations with 'conn'
 * // ...
 * // When 'releaseGuard' goes out of scope (e.g., function returns, exception thrown),
 * // 'releaseConnection(conn)' will be automatically called.
 * ```
 *
 * The cleanup function will be called automatically when `releaseGuard` goes out of scope,
 * even if an exception is thrown or early return occurs.
 */
class AutoRelease {
public:
    /**
     * @brief Constructor accepting any callable cleanup function.
     *
     * @param cleanupFunc The function to execute upon destruction. It should take no arguments and return void.
     */
    explicit AutoRelease(std::function<void()> cleanupFunc)
        : cleanupFunc_(std::move(cleanupFunc)), active_(true) {}

    /**
     * @brief Move constructor to allow RAII guards to be passed by value or returned from functions.
     * The ownership of the cleanup function is transferred to the new object, and the original is disarmed.
     */
    AutoRelease(AutoRelease&& other) noexcept
        : cleanupFunc_(std::move(other.cleanupFunc_)), active_(other.active_) {
        other.active_ = false; // Disarm the other object
    }

    /**
     * @brief Deleted copy constructor to prevent accidental copying, which would lead to double-cleanup issues.
     */
    AutoRelease(const AutoRelease&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent accidental copying.
     */
    AutoRelease& operator=(const AutoRelease&) = delete;

    /**
     * @brief Destructor: will call the cleanup function if still active.
     * This is the core of RAII.
     */
    ~AutoRelease() {
        if (active_ && cleanupFunc_) {
            cleanupFunc_();
        }
    }

    /**
     * @brief Manually cancel the cleanup call.
     * This is useful if the resource is transferred or cleaned up manually before the guard goes out of scope.
     */
    void dismiss() {
        active_ = false;
    }

private:
    std::function<void()> cleanupFunc_; /**< The cleanup function to call. */
    bool active_;                       /**< Flag indicating if the cleanup function should be called. */
};

#endif // MODULES_UTILS_AUTORELEASE_H