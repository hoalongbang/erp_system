// DataObjects/CommonDTOs/ProductAttributeDTO.h
#ifndef DATAOBJECTS_COMMONDTOS_PRODUCTATTRIBUTE_H
#define DATAOBJECTS_COMMONDTOS_PRODUCTATTRIBUTE_H
#include <string>
#include <optional>
#include <map>
#include <any> // For value

namespace ERP {
namespace DataObjects {
/**
 * @brief DTO for a generic product attribute.
 * Used for storing flexible attributes of a product (e.g., color, size, material).
 */
struct ProductAttributeDTO {
    std::string name;             /**< Name of the attribute (e.g., "Color", "Size"). */
    std::string value;            /**< Value of the attribute (e.g., "Red", "XL"). */
    std::optional<std::string> unit; /**< Unit for the value if applicable (e.g., "cm", "kg"). */
    // Optional metadata for the attribute itself (e.g., validation rules, display order)
    std::map<std::string, std::any> metadata;

    ProductAttributeDTO() = default;
    ProductAttributeDTO(const std::string& name, const std::string& value,
                        const std::optional<std::string>& unit = std::nullopt,
                        const std::map<std::string, std::any>& metadata = {})
        : name(name), value(value), unit(unit), metadata(metadata) {}
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_COMMONDTOS_PRODUCTATTRIBUTE_H