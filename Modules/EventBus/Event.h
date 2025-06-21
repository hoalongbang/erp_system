// Modules/EventBus/Event.h
#ifndef MODULES_EVENTBUS_EVENT_H
#define MODULES_EVENTBUS_EVENT_H
#include <string>       // For std::string
#include <any>          // For std::any (C++17) to hold event data
#include <chrono>       // For timestamp
#include <memory>       // For std::shared_ptr (for managing event objects)

namespace ERP {
namespace EventBus {
/**
 * @brief Base class for all events in the system.
 * All specific events will inherit from this class.
 * It provides a unique timestamp for each event and a virtual method to get the event type name.
 */
class Event {
public:
    /**
     * @brief Default constructor.
     * Initializes the event's timestamp to the current system time.
     */
    Event() : timestamp_(std::chrono::system_clock::now()) {}
    /**
     * @brief Default virtual destructor.
     * Ensures proper cleanup of derived event classes.
     */
    virtual ~Event() = default;
    /**
     * @brief Gets the timestamp when the event was created.
     * @return The event's creation timestamp.
     */
    std::chrono::system_clock::time_point getTimestamp() const {
        return timestamp_;
    }
    /**
     * @brief Pure virtual method to get the unique type name of the event.
     * Derived classes must implement this to return a string identifying their type.
     * @return A string representing the event's type name.
     */
    virtual std::string getEventType() const = 0;
private:
    std::chrono::system_clock::time_point timestamp_; /**< The timestamp when the event was created. */
};


// --- Specific Event Types (Examples) ---

// User Events
struct UserLoggedInEvent : public Event {
    std::string userId;
    std::string username;
    std::string sessionId;
    std::string ipAddress;
    UserLoggedInEvent(std::string userId, std::string username, std::string sessionId, std::string ipAddress)
        : userId(std::move(userId)), username(std::move(username)), sessionId(std::move(sessionId)), ipAddress(std::move(ipAddress)) {}
    std::string getEventType() const override { return "UserLoggedIn"; }
};

struct UserLoggedOutEvent : public Event {
    std::string userId;
    std::string sessionId;
    UserLoggedOutEvent(std::string userId, std::string sessionId)
        : userId(std::move(userId)), sessionId(std::move(sessionId)) {}
    std::string getEventType() const override { return "UserLoggedOut"; }
};

// Product Events
struct ProductCreatedEvent : public Event {
    std::string productId;
    std::string productName;
    ProductCreatedEvent(std::string productId, std::string productName)
        : productId(std::move(productId)), productName(std::move(productName)) {}
    std::string getEventType() const override { return "ProductCreated"; }
};

struct ProductUpdatedEvent : public Event {
    std::string productId;
    std::string productName;
    ProductUpdatedEvent(std::string productId, std::string productName)
        : productId(std::move(productId)), productName(std::move(productName)) {}
    std::string getEventType() const override { return "ProductUpdated"; }
};

// Sales Order Events
struct SalesOrderCreatedEvent : public Event {
    std::string orderId;
    std::string orderNumber;
    SalesOrderCreatedEvent(std::string orderId, std::string orderNumber)
        : orderId(std::move(orderId)), orderNumber(std::move(orderNumber)) {}
    std::string getEventType() const override { return "SalesOrderCreated"; }
};

struct SalesOrderStatusChangedEvent : public Event {
    std::string orderId;
    int newStatus; // Use int for enum conversion
    SalesOrderStatusChangedEvent(std::string orderId, int newStatus)
        : orderId(std::move(orderId)), newStatus(newStatus) {}
    std::string getEventType() const override { return "SalesOrderStatusChanged"; }
};

// Inventory Events
struct InventoryLevelChangedEvent : public Event {
    std::string productId;
    std::string warehouseId;
    std::string locationId;
    double oldQuantity;
    double newQuantity;
    std::string transactionType; // e.g., "GoodsReceipt", "GoodsIssue", "Adjustment"
    InventoryLevelChangedEvent(std::string productId, std::string warehouseId, std::string locationId,
                               double oldQuantity, double newQuantity, std::string transactionType)
        : productId(std::move(productId)), warehouseId(std::move(warehouseId)), locationId(std::move(locationId)),
          oldQuantity(oldQuantity), newQuantity(newQuantity), transactionType(std::move(transactionType)) {}
    std::string getEventType() const override { return "InventoryLevelChanged"; }
};

// Journal Entry Events
struct JournalEntryPostedEvent : public Event {
    std::string journalEntryId;
    JournalEntryPostedEvent(std::string journalEntryId)
        : journalEntryId(std::move(journalEntryId)) {}
    std::string getEventType() const override { return "JournalEntryPosted"; }
};

// Config Events
struct ConfigUpdatedEvent : public Event {
    std::string configKey;
    std::string configValue; // New value (can be masked if sensitive)
    ConfigUpdatedEvent(std::string configKey, std::string configValue)
        : configKey(std::move(configKey)), configValue(std::move(configValue)) {}
    std::string getEventType() const override { return "ConfigUpdated"; }
};

// Device Events
struct DeviceRegisteredEvent : public Event {
    std::string deviceId;
    std::string deviceIdentifier;
    int deviceType; // Use int for enum conversion
    DeviceRegisteredEvent(std::string deviceId, std::string deviceIdentifier, int deviceType)
        : deviceId(std::move(deviceId)), deviceIdentifier(std::move(deviceIdentifier)), deviceType(deviceType) {}
    std::string getEventType() const override { return "DeviceRegistered"; }
};

struct DeviceConnectionStatusChangedEvent : public Event {
    std::string deviceId;
    int newStatus; // Use int for enum conversion
    std::string message;
    DeviceConnectionStatusChangedEvent(std::string deviceId, int newStatus, std::string message)
        : deviceId(std::move(deviceId)), newStatus(newStatus), message(std::move(message)) {}
    std::string getEventType() const override { return "DeviceConnectionStatusChanged"; }
};

struct DeviceEventRecordedEvent : public Event {
    std::string deviceId;
    int eventType; // Use int for enum conversion
    std::string description;
    DeviceEventRecordedEvent(std::string deviceId, int eventType, std::string description)
        : deviceId(std::move(deviceId)), eventType(eventType), description(std::move(description)) {}
    std::string getEventType() const override { return "DeviceEventRecorded"; }
};

// Integration Config Events
struct IntegrationConfigCreatedEvent : public Event {
    std::string configId;
    std::string systemCode;
    std::string systemName;
    IntegrationConfigCreatedEvent(std::string configId, std::string systemCode, std::string systemName)
        : configId(std::move(configId)), systemCode(std::move(systemCode)), systemName(std::move(systemName)) {}
    std::string getEventType() const override { return "IntegrationConfigCreated"; }
};

struct IntegrationConfigUpdatedEvent : public Event {
    std::string configId;
    std::string systemCode;
    std::string systemName;
    IntegrationConfigUpdatedEvent(std::string configId, std::string systemCode, std::string systemName)
        : configId(std::move(configId)), systemCode(std::move(systemCode)), systemName(std::move(systemName)) {}
    std::string getEventType() const override { return "IntegrationConfigUpdated"; }
};

struct IntegrationConfigStatusChangedEvent : public Event {
    std::string configId;
    int newStatus; // Use int for enum conversion
    IntegrationConfigStatusChangedEvent(std::string configId, int newStatus)
        : configId(std::move(configId)), newStatus(newStatus) {}
    std::string getEventType() const override { return "IntegrationConfigStatusChanged"; }
};

// Supplier Events
struct SupplierCreatedEvent : public Event {
    std::string supplierId;
    std::string supplierName;
    SupplierCreatedEvent(std::string supplierId, std::string supplierName)
        : supplierId(std::move(supplierId)), supplierName(std::move(supplierName)) {}
    std::string getEventType() const override { return "SupplierCreated"; }
};

struct SupplierUpdatedEvent : public Event {
    std::string supplierId;
    std::string supplierName;
    SupplierUpdatedEvent(std::string supplierId, std::string supplierName)
        : supplierId(std::move(supplierId)), supplierName(std::move(supplierName)) {}
    std::string getEventType() const override { return "SupplierUpdated"; }
};

struct SupplierStatusChangedEvent : public Event {
    std::string supplierId;
    int newStatus;
    SupplierStatusChangedEvent(std::string supplierId, int newStatus)
        : supplierId(std::move(supplierId)), newStatus(newStatus) {}
    std::string getEventType() const override { return "SupplierStatusChanged"; }
};


// Picking Request Events
struct PickingRequestCreatedEvent : public Event {
    std::string requestId;
    PickingRequestCreatedEvent(std::string requestId) : requestId(std::move(requestId)) {}
    std::string getEventType() const override { return "PickingRequestCreated"; }
};

struct PickingRequestStatusChangedEvent : public Event {
    std::string requestId;
    int newStatus;
    PickingRequestStatusChangedEvent(std::string requestId, int newStatus) : requestId(std::move(requestId)), newStatus(newStatus) {}
    std::string getEventType() const override { return "PickingRequestStatusChanged"; }
};


} // namespace EventBus
} // namespace ERP
#endif // MODULES_EVENTBUS_EVENT_H