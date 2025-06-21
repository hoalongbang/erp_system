// Modules/Product/DAO/ProductDAO.cpp
#include "Modules/Product/DAO/ProductDAO.h"
#include "Modules/Utils/DTOUtils.h"    // For common DTO to map conversions
#include "Modules/Utils/DateUtils.h"   // For date/time formatting
#include "Modules/Utils/StringUtils.h" // For enum to string conversion
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <typeinfo>          // Required for std::bad_any_cast

namespace ERP {
    namespace Product {
        namespace DAOs {

            // Use nlohmann json namespace
            using json = nlohmann::json;

            ProductDAO::ProductDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Product::DTO::ProductDTO>(connectionPool, "products") { // Pass table name to base constructor
            }

            std::map<std::string, std::any> ProductDAO::toMap(const ERP::Product::DTO::ProductDTO& product) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(product); // Use DTOUtils for BaseDTO fields

                data["name"] = product.name;
                data["product_code"] = product.productCode;
                data["category_id"] = product.categoryId;
                data["base_unit_of_measure_id"] = product.baseUnitOfMeasureId;
                ERP::DAOHelpers::putOptionalString(data, "description", product.description);
                ERP::DAOHelpers::putOptionalDouble(data, "purchase_price", product.purchasePrice);
                ERP::DAOHelpers::putOptionalString(data, "purchase_currency", product.purchaseCurrency);
                ERP::DAOHelpers::putOptionalDouble(data, "sale_price", product.salePrice);
                ERP::DAOHelpers::putOptionalString(data, "sale_currency", product.saleCurrency);
                ERP::DAOHelpers::putOptionalString(data, "image_url", product.imageUrl);
                ERP::DAOHelpers::putOptionalDouble(data, "weight", product.weight);
                ERP::DAOHelpers::putOptionalString(data, "weight_unit", product.weightUnit);
                data["type"] = static_cast<int>(product.type);
                ERP::DAOHelpers::putOptionalString(data, "manufacturer", product.manufacturer);
                ERP::DAOHelpers::putOptionalString(data, "supplier_id", product.supplierId);
                ERP::DAOHelpers::putOptionalString(data, "barcode", product.barcode);


                // Serialize nested DTOs (attributes, pricingRules, unitConversionRules) to JSON strings
                try {
                    json attributesJson = json::array();
                    for (const auto& attr : product.attributes) {
                        json attrJson;
                        attrJson["name"] = attr.name;
                        attrJson["value"] = attr.value;
                        if (attr.unit.has_value()) attrJson["unit"] = *attr.unit;
                        // Serialize nested metadata as well
                        if (!attr.metadata.empty()) {
                            attrJson["metadata"] = json::parse(ERP::Utils::DTOUtils::mapToJsonString(attr.metadata));
                        }
                        attributesJson.push_back(attrJson);
                    }
                    data["attributes_json"] = attributesJson.dump();
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductDAO: toMap - Error serializing attributes: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error serializing product attributes.");
                    data["attributes_json"] = "";
                }

                try {
                    json pricingRulesJson = json::array();
                    for (const auto& rule : product.pricingRules) {
                        json ruleJson;
                        ruleJson["id"] = rule.id;
                        ruleJson["type"] = static_cast<int>(rule.type);
                        ruleJson["value"] = rule.value;
                        if (rule.minQuantity.has_value()) ruleJson["min_quantity"] = *rule.minQuantity;
                        if (rule.maxQuantity.has_value()) ruleJson["max_quantity"] = *rule.maxQuantity;
                        if (rule.currency.has_value()) ruleJson["currency"] = *rule.currency;
                        if (rule.effectiveDate.has_value()) ruleJson["effective_date"] = ERP::Utils::DateUtils::formatDateTime(*rule.effectiveDate, ERP::Common::DATETIME_FORMAT);
                        if (rule.expirationDate.has_value()) ruleJson["expiration_date"] = ERP::Utils::DateUtils::formatDateTime(*rule.expirationDate, ERP::Common::DATETIME_FORMAT);
                        if (rule.customerGroupId.has_value()) ruleJson["customer_group_id"] = *rule.customerGroupId;
                        if (rule.description.has_value()) ruleJson["description"] = *rule.description;
                         // Serialize nested metadata
                        if (!rule.metadata.empty()) {
                            ruleJson["metadata"] = json::parse(ERP::Utils::DTOUtils::mapToJsonString(rule.metadata));
                        }
                        pricingRulesJson.push_back(ruleJson);
                    }
                    data["pricing_rules_json"] = pricingRulesJson.dump();
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductDAO: toMap - Error serializing pricing rules: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error serializing product pricing rules.");
                    data["pricing_rules_json"] = "";
                }

                try {
                    json unitConversionRulesJson = json::array();
                    for (const auto& conversion : product.unitConversionRules) {
                        json conversionJson;
                        conversionJson["from_unit_id"] = conversion.fromUnitOfMeasureId;
                        conversionJson["to_unit_id"] = conversion.toUnitOfMeasureId;
                        conversionJson["conversion_factor"] = conversion.conversionFactor;
                        if (conversion.notes.has_value()) conversionJson["notes"] = *conversion.notes;
                         // Serialize nested metadata
                        if (!conversion.metadata.empty()) {
                            conversionJson["metadata"] = json::parse(ERP::Utils::DTOUtils::mapToJsonString(conversion.metadata));
                        }
                        unitConversionRulesJson.push_back(conversionJson);
                    }
                    data["unit_conversion_rules_json"] = unitConversionRulesJson.dump();
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductDAO: toMap - Error serializing unit conversion rules: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error serializing unit conversion rules.");
                    data["unit_conversion_rules_json"] = "";
                }

                return data;
            }

            ERP::Product::DTO::ProductDTO ProductDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Product::DTO::ProductDTO product;
                ERP::Utils::DTOUtils::fromMap(data, product); // Populate BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "name", product.name);
                    ERP::DAOHelpers::getPlainValue(data, "product_code", product.productCode);
                    ERP::DAOHelpers::getPlainValue(data, "category_id", product.categoryId);
                    ERP::DAOHelpers::getPlainValue(data, "base_unit_of_measure_id", product.baseUnitOfMeasureId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "description", product.description);
                    ERP::DAOHelpers::getOptionalDoubleValue(data, "purchase_price", product.purchasePrice);
                    ERP::DAOHelpers::getOptionalStringValue(data, "purchase_currency", product.purchaseCurrency);
                    ERP::DAOHelpers::getOptionalDoubleValue(data, "sale_price", product.salePrice);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sale_currency", product.saleCurrency);
                    ERP::DAOHelpers::getOptionalStringValue(data, "image_url", product.imageUrl);
                    ERP::DAOHelpers::getOptionalDoubleValue(data, "weight", product.weight);
                    ERP::DAOHelpers::getOptionalStringValue(data, "weight_unit", product.weightUnit);
                    
                    int typeInt;
                    if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) product.type = static_cast<ERP::Product::DTO::ProductType>(typeInt);
                    
                    ERP::DAOHelpers::getOptionalStringValue(data, "manufacturer", product.manufacturer);
                    ERP::DAOHelpers::getOptionalStringValue(data, "supplier_id", product.supplierId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "barcode", product.barcode);

                    // Deserialize nested DTOs from JSON strings
                    if (data.count("attributes_json") && data.at("attributes_json").type() == typeid(std::string)) {
                        std::string jsonStr = std::any_cast<std::string>(data.at("attributes_json"));
                        if (!jsonStr.empty()) {
                            try {
                                json attributesJson = json::parse(jsonStr);
                                for (const auto& attrJson : attributesJson) {
                                    ERP::DataObjects::ProductAttributeDTO attr;
                                    ERP::DAOHelpers::getPlainValue(attrJson, "name", attr.name);
                                    ERP::DAOHelpers::getPlainValue(attrJson, "value", attr.value);
                                    ERP::DAOHelpers::getOptionalStringValue(attrJson, "unit", attr.unit);
                                    if (attrJson.contains("metadata") && attrJson["metadata"].is_string()) {
                                         attr.metadata = ERP::Utils::DTOUtils::jsonStringToMap(attrJson["metadata"].get<std::string>());
                                    }
                                    product.attributes.push_back(attr);
                                }
                            } catch (const std::exception& e) {
                                ERP::Logger::Logger::getInstance().error("ProductDAO: fromMap - Error deserializing attributes: " + std::string(e.what()));
                                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error deserializing product attributes.");
                            }
                        }
                    }

                    if (data.count("pricing_rules_json") && data.at("pricing_rules_json").type() == typeid(std::string)) {
                        std::string jsonStr = std::any_cast<std::string>(data.at("pricing_rules_json"));
                        if (!jsonStr.empty()) {
                            try {
                                json pricingRulesJson = json::parse(jsonStr);
                                for (const auto& ruleJson : pricingRulesJson) {
                                    ERP::DataObjects::ProductPricingRuleDTO rule;
                                    ERP::DAOHelpers::getPlainValue(ruleJson, "id", rule.id);
                                    int ruleTypeInt;
                                    if (ERP::DAOHelpers::getPlainValue(ruleJson, "type", ruleTypeInt)) rule.type = static_cast<ERP::DataObjects::PricingRuleType>(ruleTypeInt);
                                    ERP::DAOHelpers::getPlainValue(ruleJson, "value", rule.value);
                                    ERP::DAOHelpers::getOptionalDoubleValue(ruleJson, "min_quantity", rule.minQuantity);
                                    ERP::DAOHelpers::getOptionalDoubleValue(ruleJson, "max_quantity", rule.maxQuantity);
                                    ERP::DAOHelpers::getOptionalStringValue(ruleJson, "currency", rule.currency);
                                    ERP::DAOHelpers::getOptionalTimeValue(ruleJson, "effective_date", rule.effectiveDate);
                                    ERP::DAOHelpers::getOptionalTimeValue(ruleJson, "expiration_date", rule.expirationDate);
                                    ERP::DAOHelpers::getOptionalStringValue(ruleJson, "customer_group_id", rule.customerGroupId);
                                    ERP::DAOHelpers::getOptionalStringValue(ruleJson, "description", rule.description);
                                    if (ruleJson.contains("metadata") && ruleJson["metadata"].is_string()) {
                                         rule.metadata = ERP::Utils::DTOUtils::jsonStringToMap(ruleJson["metadata"].get<std::string>());
                                    }
                                    product.pricingRules.push_back(rule);
                                }
                            } catch (const std::exception& e) {
                                ERP::Logger::Logger::getInstance().error("ProductDAO: fromMap - Error deserializing pricing rules: " + std::string(e.what()));
                                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error deserializing product pricing rules.");
                            }
                        }
                    }

                    if (data.count("unit_conversion_rules_json") && data.at("unit_conversion_rules_json").type() == typeid(std::string)) {
                        std::string jsonStr = std::any_cast<std::string>(data.at("unit_conversion_rules_json"));
                        if (!jsonStr.empty()) {
                            try {
                                json unitConversionRulesJson = json::parse(jsonStr);
                                for (const auto& conversionJson : unitConversionRulesJson) {
                                    ERP::DataObjects::ProductUnitConversionRuleDTO conversion;
                                    ERP::DAOHelpers::getPlainValue(conversionJson, "from_unit_id", conversion.fromUnitOfMeasureId);
                                    ERP::DAOHelpers::getPlainValue(conversionJson, "to_unit_id", conversion.toUnitOfMeasureId);
                                    ERP::DAOHelpers::getPlainValue(conversionJson, "conversion_factor", conversion.conversionFactor);
                                    ERP::DAOHelpers::getOptionalStringValue(conversionJson, "notes", conversion.notes);
                                    if (conversionJson.contains("metadata") && conversionJson["metadata"].is_string()) {
                                         conversion.metadata = ERP::Utils::DTOUtils::jsonStringToMap(conversionJson["metadata"].get<std::string>());
                                    }
                                    product.unitConversionRules.push_back(conversion);
                                }
                            } catch (const std::exception& e) {
                                ERP::Logger::Logger::getInstance().error("ProductDAO: fromMap - Error deserializing unit conversion rules: " + std::string(e.what()));
                                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductDAO: Error deserializing unit conversion rules.");
                            }
                        }
                    }

                } catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ProductDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "Failed to cast data for ProductDTO: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "Unexpected error in fromMap for ProductDTO: " + std::string(e.what()));
                }
                return product;
            }
        } // namespace DAOs
    } // namespace Product
} // namespace ERP