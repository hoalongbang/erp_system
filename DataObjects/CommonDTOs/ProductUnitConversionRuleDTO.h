// DataObjects/CommonDTOs/ProductUnitConversionRuleDTO.h
#ifndef DATAOBJECTS_COMMONDTOS_PRODUCTUNITCONVERSIONRULE_H
#define DATAOBJECTS_COMMONDTOS_PRODUCTUNITCONVERSIONRULE_H
#include <string>
#include <optional>
#include <map>
#include <any>

namespace ERP {
namespace DataObjects {
/**
 * @brief DTO for a product unit conversion rule.
 * Defines how quantities of a product can be converted between different units of measure.
 */
struct ProductUnitConversionRuleDTO {
    std::string fromUnitOfMeasureId;    /**< ID of the source UnitOfMeasure. */
    std::string toUnitOfMeasureId;      /**< ID of the target UnitOfMeasure. */
    double conversionFactor;            /**< Factor to multiply 'from' quantity to get 'to' quantity.
                                             (e.g., 1 box = 12 pieces, factor = 12). */
    std::optional<std::string> notes;   /**< Any notes about the conversion. */
    std::map<std::string, std::any> metadata; /**< Additional conversion-specific parameters. */

    ProductUnitConversionRuleDTO() = default;
    ProductUnitConversionRuleDTO(const std::string& fromUnitId, const std::string& toUnitId, double factor,
                                 const std::optional<std::string>& notes = std::nullopt,
                                 const std::map<std::string, std::any>& metadata = {})
        : fromUnitOfMeasureId(fromUnitId), toUnitOfMeasureId(toUnitId), conversionFactor(factor),
          notes(notes), metadata(metadata) {}
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_COMMONDTOS_PRODUCTUNITCONVERSIONRULE_H