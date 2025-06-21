// Modules/EventBus/EventBus.h
#ifndef MODULES_EVENTBUS_EVENTBUS_H
#define MODULES_EVENTBUS_EVENTBUS_H
#include <string>       // For std::string
#include <vector>       // For std::vector
#include <map>          // For std::map
#include <memory>       // For std::shared_ptr
#include <functional>   // For std::function
#include <mutex>        // For std::mutex, std::unique_lock
#include <typeindex>    // For std::type_index

// Rút gọn includes
#include "Event.h"          // Base Event class
#include "Logger.h"         // For logging

namespace ERP {
namespace EventBus {

/**
 * @brief IEventSubscriber interface defines the contract for classes that want to subscribe to events.
 * Any class that wants to handle specific events must inherit from this class and implement the handleEvent method.
 */
class IEventSubscriber {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~IEventSubscriber() = default;
    /**
     * @brief This method is called when an event that the subscriber is interested in is published.
     * @param event A shared pointer to the published event. The subscriber should dynamic_cast
     * this to the specific event type it expects.
     */
    virtual void handleEvent(std::shared_ptr<Event> event) = 0;
    /**
     * @brief Gets the type name of the event that this subscriber is interested in.
     * This allows the EventBus to efficiently dispatch events only to relevant subscribers.
     * @return The string event type name (e.g., "UserLoggedIn", "ProductCreated").
     */
    virtual std::string getEventType() const = 0; // Đã sửa: xóa khoảng trắng giữa 'get' và 'EventType'
};


/**
 * @brief The EventBus class provides a publish/subscribe mechanism for inter-module communication.
 * It allows different parts of the application to communicate without direct dependencies.
 * Implemented as a Singleton.
 */
class EventBus {
public:
    /**
     * @brief Gets the singleton instance of the EventBus.
     * @return A reference to the EventBus instance.
     */
    static EventBus& getInstance();

    /**
     * @brief Subscribes an event handler to a specific event type.
     * @tparam EventType The type of event to subscribe to.
     * @param subscriber A shared pointer to the IEventSubscriber instance.
     */
    template<typename EventType>
    void subscribe(std::shared_ptr<IEventSubscriber> subscriber) {
        std::unique_lock<std::mutex> lock(mutex_);
        // Use typeid(EventType).name() as key
        subscribers_[typeid(EventType).name()].push_back(subscriber);
        ERP::Logger::Logger::getInstance().debug("EventBus: Subscriber for event type '" + std::string(typeid(EventType).name()) + "' added.");
    }
    
    /**
     * @brief Unsubscribes an event handler from a specific event type.
     * @tparam EventType The type of event to unsubscribe from.
     * @param subscriber The shared pointer to the IEventSubscriber instance to remove.
     */
    template<typename EventType>
    void unsubscribe(std::shared_ptr<IEventSubscriber> subscriber) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto& subscriberList = subscribers_[typeid(EventType).name()];
        subscriberList.erase(
            std::remove_if(subscriberList.begin(), subscriberList.end(),
                           [&](const std::shared_ptr<IEventSubscriber>& s){
                               return s == subscriber;
                           }),
            subscriberList.end()
        );
        ERP::Logger::Logger::getInstance().debug("EventBus: Subscriber for event type '" + std::string(typeid(EventType).name()) + "' removed.");
    }

    /**
     * @brief Publishes an event to all subscribed handlers.
     * The event is dispatched to handlers interested in its specific type.
     * @param event A shared pointer to the event object.
     */
    void publish(std::shared_ptr<Event> event);

    // Delete copy constructor and assignment operator to enforce singleton
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    EventBus();

    std::map<std::string, std::vector<std::shared_ptr<IEventSubscriber>>> subscribers_;
    std::mutex mutex_;
};

} // namespace EventBus
} // namespace ERP
#endif // MODULES_EVENTBUS_EVENTBUS_H