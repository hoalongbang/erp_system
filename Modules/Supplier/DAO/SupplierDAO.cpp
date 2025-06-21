// Modules/Supplier/DAO/SupplierDAO.cpp
#include "SupplierDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption
#include "DAOHelpers.h"        // For utility functions
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions

#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp> // For JSON library
#include <optional>
#include <any>
#include <type_traits>

namespace ERP {
    namespace Supplier {
        namespace DAOs {

            using json = nlohmann::json;

            // Updated constructor to call base class constructor
            SupplierDAO::SupplierDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Supplier::DTO::SupplierDTO>(connectionPool, "suppliers") { // Pass table name to base constructor
                Logger::Logger::getInstance().info("SupplierDAO: Initialized.");
            }

            // The 'create', 'get', 'update', 'remove', 'getById', 'count' methods are now implemented in DAOBase.

            // Method to convert DTO to map
            std::map<std::string, std::any> SupplierDAO::toMap(const ERP::Supplier::DTO::SupplierDTO& supplier) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields
                data["id"] = supplier.id;
                data["status"] = static_cast<int>(supplier.status);
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(supplier.createdAt, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", supplier.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", supplier.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", supplier.updatedBy);
                // SupplierDTO specific fields
                data["name"] = supplier.name;
                ERP::DAOHelpers::putOptionalString(data, "tax_id", supplier.taxId);
                ERP::DAOHelpers::putOptionalString(data, "notes", supplier.notes);
                ERP::DAOHelpers::putOptionalString(data, "default_payment_terms", supplier.defaultPaymentTerms);
                ERP::DAOHelpers::putOptionalString(data, "default_delivery_terms", supplier.defaultDeliveryTerms);

                // Handle nested DTOs (contactPersons and addresses) by serializing to JSON
                try {
                    json contactPersonsJson = json::array();
                    for (const auto& cp : supplier.contactPersons) {
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
                    data["contact_persons_json"] = contactPersonsJson.dump();
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("SupplierDAO::toMap - Error serializing contact_persons: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SupplierDAO: Error serializing contact persons.");
                    data["contact_persons_json"] = "";
                }
                try {
                    json addressesJson = json::array();
                    for (const auto& addr : supplier.addresses) {
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
                    data["addresses_json"] = addressesJson.dump();
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("SupplierDAO::toMap - Error serializing addresses: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SupplierDAO: Error serializing addresses.");
                    data["addresses_json"] = "";
                }
                return data;
            }
            // Method to convert map to DTO
            ERP::Supplier::DTO::SupplierDTO SupplierDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Supplier::DTO::SupplierDTO supplier;
                try {
                    // BaseDTO fields
                    ERP::DAOHelpers::getPlainValue(data, "id", supplier.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        supplier.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    } else {
                        supplier.status = ERP::Common::EntityStatus::UNKNOWN; // Default status
                    }
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", supplier.createdAt);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", supplier.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", supplier.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", supplier.updatedBy);
                    // SupplierDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", supplier.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "tax_id", supplier.taxId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", supplier.notes);
                    ERP::DAOHelpers::getOptionalStringValue(data, "default_payment_terms", supplier.defaultPaymentTerms);
                    ERP::DAOHelpers::getOptionalStringValue(data, "default_delivery_terms", supplier.defaultDeliveryTerms);

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
                                ERP::DAOHelpers::getPlainValue(cpJson, "is_primary", cp.isPrimary);
                                supplier.contactPersons.push_back(cp);
                            }
                        } catch (const std::exception& e) {
                            Logger::Logger::getInstance().error("SupplierDAO::fromMap - Error deserializing contact_persons: " + std::string(e.what()));
                            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SupplierDAO: Error deserializing contact persons.");
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
                                ERP::DAOHelpers::getPlainValue(addrJson, "is_primary", addr.isPrimary);
                                supplier.addresses.push_back(addr);
                            }
                        } catch (const std::exception& e) {
                            Logger::Logger::getInstance().error("SupplierDAO::fromMap - Error deserializing addresses: " + std::string(e.what()));
                            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SupplierDAO: Error deserializing addresses.");
                        }
                    }
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("SupplierDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SupplierDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("SupplierDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SupplierDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return supplier;
            }
        } // namespace DAOs
    } // namespace Supplier
} // namespace ERP