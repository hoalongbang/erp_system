// Modules/EventBus/IEventSubscriber.h
#ifndef MODULES_EVENTBUS_IEVENTSUBSCRIBER_H
#define MODULES_EVENTBUS_IEVENTSUBSCRIBER_H

#include <string>       // For std::string
#include <memory>       // For std::shared_ptr

// Rút gọn includes
#include "Event.h" // Include the base Event class

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
             * Ensures proper cleanup of derived classes through the base pointer.
             */
            virtual ~IEventSubscriber() = default;

            /**
             * @brief This method is called when an event that the subscriber is interested in is published.
             * Derived classes should cast the event to their specific type before processing.
             * @param event A shared pointer to the published event.
             */
            virtual void handleEvent(std::shared_ptr<Event> event) = 0;

            /**
             * @brief Gets the type name of the event that this subscriber is interested in.
             * This allows the EventBus to efficiently dispatch events only to relevant subscribers.
             * @return The string representing the event's type name (e.g., "UserLoggedIn", "ProductCreated").
             */
            virtual std::string getEventType() const = 0;
        };

    } // namespace EventBus
} // namespace ERP

#endif // MODULES_EVENTBUS_IEVENTSUBSCRIBER_H