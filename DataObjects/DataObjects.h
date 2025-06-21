// DataObjects/DataObjects.h
#ifndef DATAOBJECTS_DATAOBJECTS_H
#define DATAOBJECTS_DATAOBJECTS_H
// This file serves as a central point to include all common DTOs.
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <map>
#include <any>
// Include Common module for basic enums like EntityStatus
#include "Common.h"
// Include BaseDTO for common fields
#include "BaseDTO.h"

// NEW: Common DTOs used across multiple modules
#include "DataObjects/CommonDTOs/ContactPersonDTO.h"
#include "DataObjects/CommonDTOs/AddressDTO.h"
#include "DataObjects/CommonDTOs/ProductAttributeDTO.h"         // NEW
#include "DataObjects/CommonDTOs/ProductPricingRuleDTO.h"       // NEW
#include "DataObjects/CommonDTOs/ProductUnitConversionRuleDTO.h" // NEW

// Product Module DTOs
#include "Product.h"
// Customer Module DTOs
#include "Customer.h"
// Supplier Module DTOs
#include "Supplier.h"
// User Module DTOs
#include "User.h"
// Catalog Module DTOs
#include "Category.h"
#include "Location.h"
#include "UnitOfMeasure.h"
#include "Warehouse.h"
#include "Role.h"
#include "Permission.h"
// Warehouse Module DTOs
#include "Inventory.h"
#include "InventoryTransaction.h"
#include "InventoryCostLayer.h"
#include "PickingRequest.h"
#include "PickingDetail.h"
#include "StocktakeRequest.h"
#include "StocktakeDetail.h"
// Material Module DTOs
#include "ReceiptSlip.h"
#include "ReceiptSlipDetail.h"
#include "IssueSlip.h"
#include "IssueSlipDetail.h"
#include "MaterialRequestSlip.h"
#include "MaterialRequestSlipDetail.h"
#include "MaterialIssueSlip.h"
#include "MaterialIssueSlipDetail.h"
// Sales Module DTOs
#include "SalesOrder.h"
#include "SalesOrderDetail.h"
#include "Invoice.h"
#include "InvoiceDetail.h"
#include "Payment.h"
#include "Shipment.h"
#include "ShipmentDetail.h"
#include "Quotation.h"
// Finance Module DTOs
#include "AccountReceivableBalance.h"
#include "AccountReceivableTransaction.h"
#include "GeneralLedgerAccount.h"
#include "GLAccountBalance.h"
#include "JournalEntry.h"
// Security Module DTOs
#include "Session.h"
// Database Module DTOs
#include "DatabaseConfig.h"
#endif // DATAOBJECTS_DATAOBJECTS_H