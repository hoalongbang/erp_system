// Modules/Product/DTO/Product.h
#ifndef MODULES_PRODUCT_DTO_PRODUCT_H
#define MODULES_PRODUCT_DTO_PRODUCT_H
#include <string>
#include <optional>
#include <chrono>
#include <vector> // For nested DTOs (if any)
#include <map>    // For std::map<string, any> replacement of QJsonObject
#include <any>    // For std::any replacement of QJsonObject

#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Common/Common.h"    // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"     // ĐÃ SỬA: Dùng tên tệp trực tiếp
// NEW: Include nested DTOs
#include "DataObjects/CommonDTOs/ProductAttributeDTO.h"
#include "DataObjects/CommonDTOs/ProductPricingRuleDTO.h"
#include "DataObjects/CommonDTOs/ProductUnitConversionRuleDTO.h"

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở
namespace ERP {
    namespace Product {
        namespace DTO {
            /**
             * @brief Enum for Product Type.
             */
            enum class ProductType {
                FINISHED_GOOD = 0, // Thành phẩm
                RAW_MATERIAL = 1,  // Nguyên vật liệu
                WORK_IN_PROCESS = 2, // Bán thành phẩm
                SERVICE = 3,       // Dịch vụ
                ASSEMBLY = 4,      // Sản phẩm lắp ráp
                KIT = 5            // Bộ sản phẩm
            };
            /**
             * @brief DTO for Product entity.
             */
            struct ProductDTO : public BaseDTO {
                std::string name;
                std::string productCode; // Unique code for the product
                std::string categoryId;  // Foreign key to CategoryDTO
                std::string baseUnitOfMeasureId; // Foreign key to UnitOfMeasureDTO
                std::optional<std::string> description;
                std::optional<double> purchasePrice;
                std::optional<std::string> purchaseCurrency;
                std::optional<double> salePrice;
                std::optional<std::string> saleCurrency;
                std::optional<std::string> imageUrl;
                std::optional<double> weight;
                std::optional<std::string> weightUnit;
                ProductType type; // ProductType enum
                std::optional<std::string> manufacturer; // NEW
                std::optional<std::string> supplierId;   // NEW: Link to SupplierDTO
                std::optional<std::string> barcode;      // NEW: EAN, UPC, etc.

                std::vector<ERP::DataObjects::ProductAttributeDTO> attributes; // NEW: Flexible product attributes
                std::vector<ERP::DataObjects::ProductPricingRuleDTO> pricingRules; // NEW: Complex pricing logic
                std::vector<ERP::DataObjects::ProductUnitConversionRuleDTO> unitConversionRules; // NEW: Unit conversions

                ProductDTO() : BaseDTO(), name(""), productCode(ERP::Utils::generateUUID()), type(ProductType::FINISHED_GOOD) {}
                virtual ~ProductDTO() = default;

                // Helper to convert enum to string
                std::string getTypeString() const {
                    switch (type) {
                        case ProductType::FINISHED_GOOD: return "Finished Good";
                        case ProductType::RAW_MATERIAL: return "Raw Material";
                        case ProductType::WORK_IN_PROCESS: return "Work-in-Process";
                        case ProductType::SERVICE: return "Service";
                        case ProductType::ASSEMBLY: return "Assembly";
                        case ProductType::KIT: return "Kit";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Product
} // namespace ERP
#endif // MODULES_PRODUCT_DTO_PRODUCT_H