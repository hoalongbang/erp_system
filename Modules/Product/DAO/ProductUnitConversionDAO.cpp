// Modules/Product/DAO/ProductUnitConversionDAO.cpp
#include "ProductUnitConversionDAO.h"
#include "DAOHelpers.h"     // Standard includes for mapping helpers
#include "Logger.h"         // Standard includes for logging
#include "ErrorHandler.h"   // Standard includes for error handling
#include "Common.h"         // Standard includes for common enums/constants
#include "DateUtils.h"      // Standard includes for date/time utilities
#include "DTOUtils.h"       // Standard includes for BaseDTO mapping utilities

#include <sstream>
#include <stdexcept>
#include <optional>
#include <any>
#include <type_traits> // For std::is_same

namespace ERP {
    namespace Product {
        namespace DAOs {
            ProductUnitConversionDAO::ProductUnitConversionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Product::DTO::ProductUnitConversionDTO>(connectionPool, "product_unit_conversions") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("ProductUnitConversionDAO: Initialized.");
            }

            std::map<std::string, std::any> ProductUnitConversionDAO::toMap(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(conversion); // BaseDTO fields

                data["product_id"] = conversion.productId;
                data["from_unit_of_measure_id"] = conversion.fromUnitOfMeasureId;
                data["to_unit_of_measure_id"] = conversion.toUnitOfMeasureId;
                data["conversion_factor"] = conversion.conversionFactor;
                ERP::DAOHelpers::putOptionalString(data, "notes", conversion.notes);

                return data;
            }

            ERP::Product::DTO::ProductUnitConversionDTO ProductUnitConversionDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Product::DTO::ProductUnitConversionDTO conversion;
                ERP::Utils::DTOUtils::fromMap(data, conversion); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "product_id", conversion.productId);
                    ERP::DAOHelpers::getPlainValue(data, "from_unit_of_measure_id", conversion.fromUnitOfMeasureId);
                    ERP::DAOHelpers::getPlainValue(data, "to_unit_of_measure_id", conversion.toUnitOfMeasureId);

                    // Handle conversion_factor with type checking for robustness
                    ERP::DAOHelpers::getPlainValue(data, "conversion_factor", conversion.conversionFactor);

                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", conversion.notes);
                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ProductUnitConversionDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ProductUnitConversionDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductUnitConversionDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductUnitConversionDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }

                return conversion;
            }

            bool ProductUnitConversionDAO::save(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) {
                // Calls the templated create method from DAOBase.
                return create(conversion);
            }

            std::optional<ERP::Product::DTO::ProductUnitConversionDTO> ProductUnitConversionDAO::findById(const std::string& id) {
                // Calls the templated getById method from DAOBase.
                return getById(id);
            }

            bool ProductUnitConversionDAO::update(const ERP::Product::DTO::ProductUnitConversionDTO& conversion) {
                // Calls the templated update method from DAOBase.
                return DAOBase<ERP::Product::DTO::ProductUnitConversionDTO>::update(conversion);
            }

            bool ProductUnitConversionDAO::remove(const std::string& id) {
                // Calls the templated remove method from DAOBase.
                return DAOBase<ERP::Product::DTO::ProductUnitConversionDTO>::remove(id);
            }

            std::vector<ERP::Product::DTO::ProductUnitConversionDTO> ProductUnitConversionDAO::findAll() {
                // Calls the templated findAll method from DAOBase.
                return DAOBase<ERP::Product::DTO::ProductUnitConversionDTO>::findAll();
            }

            std::vector<ERP::Product::DTO::ProductUnitConversionDTO> ProductUnitConversionDAO::getByProductId(const std::string& productId) {
                std::map<std::string, std::any> filters;
                filters["product_id"] = productId;
                return get(filters); // Use templated get
            }

            std::optional<ERP::Product::DTO::ProductUnitConversionDTO> ProductUnitConversionDAO::getConversion(
                const std::string& productId,
                const std::string& fromUnitId,
                const std::string& toUnitId) {
                std::map<std::string, std::any> filters;
                filters["product_id"] = productId;
                filters["from_unit_of_measure_id"] = fromUnitId;
                filters["to_unit_of_measure_id"] = toUnitId;

                std::vector<ERP::Product::DTO::ProductUnitConversionDTO> results = get(filters); // Use templated get
                if (!results.empty()) {
                    return results.front();
                }
                return std::nullopt;
            }

            int ProductUnitConversionDAO::countConversions(const std::map<std::string, std::any>& filters) {
                // Use templated count
                return count(filters);
            }

        } // namespace DAOs
    } // namespace Product
} // namespace ERP