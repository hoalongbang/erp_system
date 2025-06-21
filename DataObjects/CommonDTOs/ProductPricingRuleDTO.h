// DataObjects/CommonDTOs/ProductPricingRuleDTO.h
#ifndef DATAOBJECTS_COMMONDTOS_PRODUCTPRICINGRULE_H
#define DATAOBJECTS_COMMONDTOS_PRODUCTPRICINGRULE_H
#include <string>
#include <optional>
#include <chrono>
#include <map>
#include <any>

namespace ERP {
namespace DataObjects {

enum class PricingRuleType {
    FIXED_PRICE,        /**< Fixed price per unit. */
    VOLUME_DISCOUNT,    /**< Discount based on quantity purchased. */
    TIERED_PRICING,     /**< Different prices for different quantity tiers. */
    PROMOTIONAL_PRICE,  /**< Temporary promotional price. */
    WHOLESALE_PRICE,    /**< Price for wholesale customers. */
    RETAIL_PRICE,       /**< Price for retail customers. */
    CUSTOM              /**< Custom pricing rule. */
};

/**
 * @brief DTO for a product pricing rule.
 * Allows defining flexible pricing structures for products.
 */
struct ProductPricingRuleDTO {
    std::string id;
    PricingRuleType type;               /**< Type of pricing rule (e.g., FIXED_PRICE, VOLUME_DISCOUNT). */
    double value;                       /**< The price or discount value. */
    std::optional<double> minQuantity;  /**< Minimum quantity for this rule to apply (for volume/tiered). */
    std::optional<double> maxQuantity;  /**< Maximum quantity for this rule to apply (for tiered). */
    std::optional<std::string> currency; /**< Currency of the price/value. */
    std::optional<std::chrono::system_clock::time_point> effectiveDate; /**< When this rule becomes active. */
    std::optional<std::chrono::system_clock::time_point> expirationDate; /**< When this rule expires. */
    std::optional<std::string> customerGroupId; /**< Apply to specific customer group (optional). */
    std::optional<std::string> description;     /**< Description of the rule. */
    std::map<std::string, std::any> metadata;   /**< Additional rule-specific parameters (e.g., discount percentage). */

    ProductPricingRuleDTO() : id(ERP::Utils::generateUUID()), type(PricingRuleType::FIXED_PRICE), value(0.0) {} // Assuming generateUUID is available.
    
    // Helper to convert enum to string (or can use a central StringUtils)
    std::string getTypeString() const {
        switch (type) {
            case PricingRuleType::FIXED_PRICE: return "Fixed Price";
            case PricingRuleType::VOLUME_DISCOUNT: return "Volume Discount";
            case PricingRuleType::TIERED_PRICING: return "Tiered Pricing";
            case PricingRuleType::PROMOTIONAL_PRICE: return "Promotional Price";
            case PricingRuleType::WHOLESALE_PRICE: return "Wholesale Price";
            case PricingRuleType::RETAIL_PRICE: return "Retail Price";
            case PricingRuleType::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_COMMONDTOS_PRODUCTPRICINGRULE_H