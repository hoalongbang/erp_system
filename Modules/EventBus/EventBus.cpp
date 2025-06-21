// Modules/EventBus/EventBus.cpp
#include "EventBus.h"
#include "Logger.h" // Standard includes

#include <algorithm> // For std::remove_if

namespace ERP {
namespace EventBus {

// Static member initialization
EventBus* EventBus::instance_ = nullptr;
std::once_flag EventBus::onceFlag_;

EventBus& EventBus::getInstance() {
    // Use std::call_once to ensure thread-safe and one-time initialization
    std::call_once(onceFlag_, []() {
        instance_ = new EventBus();
    });
    return *instance_;
}

EventBus::EventBus() {
    ERP::Logger::Logger::getInstance().info("EventBus: Constructor called. Event bus is ready.");
}

void EventBus::publish(std::shared_ptr<Event> event) {
    if (!event) {
        ERP::Logger::Logger::getInstance().warning("EventBus: Attempted to publish a null event.");
        return;
    }

    std::string eventType = event->getEventType();
    ERP::Logger::Logger::getInstance().debug("EventBus: Publishing event of type: " + eventType);

    std::unique_lock<std::mutex> lock(mutex_); // Lock during dispatch to prevent changes to subscriber list
    
    // Check if there are any subscribers for this event type
    auto it = subscribers_.find(eventType);
    if (it == subscribers_.end()) {
        ERP::Logger::Logger::getInstance().debug("EventBus: No subscribers found for event type: " + eventType);
        return;
    }

    // Get the list of subscribers for this event type
    auto& subscriberList = it->second;

    // Dispatch the event to each subscriber.
    // Important: Iterate over a copy if subscribers might unsubscribe themselves during handling,
    // or use shared_ptr management carefully to avoid dangling pointers if subscribers are deleted.
    // For simplicity, we directly iterate the list under lock. If a subscriber unsubscribes itself,
    // it will be removed in the next `unsubscribe` call.
    for (const auto& subscriber : subscriberList) {
        if (subscriber) { // Ensure subscriber is still valid
            try {
                subscriber->handleEvent(event);
            } catch (const std::exception& e) {
                // Log any exceptions thrown by event handlers to prevent them from crashing the EventBus
                ERP::Logger::Logger::getInstance().error("EventBus: Exception in event handler for " + eventType + ": " + std::string(e.what()));
            }
        }
    }
}

} // namespace EventBus
} // namespace ERP