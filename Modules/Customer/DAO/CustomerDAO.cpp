// Modules/Customer/DAO/CustomerDAO.cpp
#include "CustomerDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption (Dùng tên tệp trực tiếp)
#include "DAOHelpers.h" // Ensure DAOHelpers is included for utility functions
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions

#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp> // NEW: Include for JSON library
#include <optional> // Include for std::optional
#include <any>      // Include for std::any
#include <type_traits> // For std::is_same (used by DAOHelpers)

namespace ERP {
    namespace Customer { // Updated namespace
        namespace DAOs {

            // NEW: Use nlohmann namespace for JSON
            using json = nlohmann::json;

            // Updated constructor to call base class constructor
            CustomerDAO::CustomerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Customer::DTO::CustomerDTO>(connectionPool, "customers") { //
                Logger::Logger::getInstance().info("CustomerDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool CustomerDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> CustomerDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool CustomerDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool CustomerDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool CustomerDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> CustomerDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int CustomerDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }


            // Method to convert DTO to map
            std::map<std::string, std::any> CustomerDAO::toMap(const ERP::Customer::DTO::CustomerDTO& customer) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields
                data["id"] = customer.id;
                data["status"] = static_cast<int>(customer.status);
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(customer.createdAt, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", customer.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", customer.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", customer.updatedBy);
                // CustomerDTO specific fields
                data["name"] = customer.name;
                ERP::DAOHelpers::putOptionalString(data, "tax_id", customer.taxId);
                ERP::DAOHelpers::putOptionalString(data, "notes", customer.notes);
                ERP::DAOHelpers::putOptionalString(data, "default_payment_terms", customer.defaultPaymentTerms);
                ERP::DAOHelpers::putOptionalDouble(data, "credit_limit", customer.creditLimit); // Assuming putOptionalDouble for optional double

                // Handle nested DTOs (contactPersons and addresses) by serializing to JSON
                try {
                    json contactPersonsJson = json::array();
                    for (const auto& cp : customer.contactPersons) {
                        json cpJson;
                        cpJson["id"] = cp.id;
                        cpJson["first_name"] = cp.firstName;
                        if (cp.lastName.has_value()) cpJson["last_name"] = *cp.lastName;
                        if (cp.email.has_value()) cpJson["email"] = *cp.email;
                        if (cp.phoneNumber.has_value()) cpJson["phone_number"] = *cp.phoneNumber;
                        if (cp.position.has_value()) cpJson["position"] = *cp.position;
                        cpJson["is_primary"] = cp.isPrimary;
                        contactPersonsJson.push_back(cpJson);
                    }
                    data["contact_persons_json"] = contactPersonsJson.dump(); // Store as JSON string
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("CustomerDAO::toMap - Error serializing contact_persons: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CustomerDAO: Error serializing contact persons.");
                    data["contact_persons_json"] = ""; // Store empty or handle as error
                }
                try {
                    json addressesJson = json::array();
                    for (const auto& addr : customer.addresses) {
                        json addrJson;
                        addrJson["id"] = addr.id;
                        addrJson["street"] = addr.street;
                        addrJson["city"] = addr.city;
                        addrJson["state_province"] = addr.stateProvince;
                        addrJson["postal_code"] = addr.postalCode;
                        addrJson["country"] = addr.country;
                        if (addr.addressType.has_value()) addrJson["address_type"] = *addr.addressType;
                        addrJson["is_primary"] = addr.isPrimary;
                        addressesJson.push_back(addrJson);
                    }
                    data["addresses_json"] = addressesJson.dump(); // Store as JSON string
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("CustomerDAO::toMap - Error serializing addresses: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CustomerDAO: Error serializing addresses.");
                    data["addresses_json"] = ""; // Store empty or handle as error
                }
                return data;
            }
            // Method to convert map to DTO
            ERP::Customer::DTO::CustomerDTO CustomerDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Customer::DTO::CustomerDTO customer;
                try {
                    // BaseDTO fields
                    ERP::DAOHelpers::getPlainValue(data, "id", customer.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        customer.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    } else {
                        customer.status = ERP::Common::EntityStatus::UNKNOWN; // Default status
                    }
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", customer.createdAt);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", customer.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", customer.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", customer.updatedBy);
                    // CustomerDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", customer.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "tax_id", customer.taxId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", customer.notes);
                    ERP::DAOHelpers::getOptionalStringValue(data, "default_payment_terms", customer.defaultPaymentTerms);
                    ERP::DAOHelpers::getOptionalDoubleValue(data, "credit_limit", customer.creditLimit);
                    // Handle nested DTOs (contactPersons and addresses) by deserializing from JSON
                    if (data.count("contact_persons_json") && data.at("contact_persons_json").type() == typeid(std::string)) {
                        try {
                            json contactPersonsJson = json::parse(std::any_cast<std::string>(data.at("contact_persons_json")));
                            for (const auto& cpJson : contactPersonsJson) {
                                ERP::DataObjects::ContactPersonDTO cp;
                                ERP::DAOHelpers::getPlainValue(cpJson, "id", cp.id);
                                ERP::DAOHelpers::getPlainValue(cpJson, "first_name", cp.firstName);
                                ERP::DAOHelpers::getOptionalStringValue(cpJson, "last_name", cp.lastName);
                                ERP::DAOHelpers::getOptionalStringValue(cpJson, "email", cp.email);
                                ERP::DAOHelpers::getOptionalStringValue(cpJson, "phone_number", cp.phoneNumber);
                                ERP::DAOHelpers::getOptionalStringValue(cpJson, "position", cp.position);
                                ERP::DAOHelpers::getPlainValue(cpJson, "is_primary", cp.isPrimary); // is_primary is bool
                                customer.contactPersons.push_back(cp);
                            }
                        } catch (const std::exception& e) {
                            Logger::Logger::getInstance().error("CustomerDAO::fromMap - Error deserializing contact_persons: " + std::string(e.what()));
                            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CustomerDAO: Error deserializing contact persons.");
                        }
                    }
                    if (data.count("addresses_json") && data.at("addresses_json").type() == typeid(std::string)) {
                        try {
                            json addressesJson = json::parse(std::any_cast<std::string>(data.at("addresses_json")));
                            for (const auto& addrJson : addressesJson) {
                                ERP::DataObjects::AddressDTO addr;
                                ERP::DAOHelpers::getPlainValue(addrJson, "id", addr.id);
                                ERP::DAOHelpers::getPlainValue(addrJson, "street", addr.street);
                                ERP::DAOHelpers::getPlainValue(addrJson, "city", addr.city);
                                ERP::DAOHelpers::getPlainValue(addrJson, "state_province", addr.stateProvince);
                                ERP::DAOHelpers::getPlainValue(addrJson, "postal_code", addr.postalCode);
                                ERP::DAOHelpers::getPlainValue(addrJson, "country", addr.country);
                                ERP::DAOHelpers::getOptionalStringValue(addrJson, "address_type", addr.addressType);
                                ERP::DAOHelpers::getPlainValue(addrJson, "is_primary", addr.isPrimary); // is_primary is bool
                                customer.addresses.push_back(addr);
                            }
                        } catch (const std::exception& e) {
                            Logger::Logger::getInstance().error("CustomerDAO::fromMap - Error deserializing addresses: " + std::string(e.what()));
                            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CustomerDAO: Error deserializing addresses.");
                        }
                    }
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("CustomerDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "CustomerDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("CustomerDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CustomerDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return customer;
            }
        } // namespace DAOs
    } // namespace Customer
} // namespace ERP