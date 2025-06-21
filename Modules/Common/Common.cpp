// Modules/Common/Common.cpp
// This file is intended to contain implementations of any non-template/non-inline functions
// declared in Common.h.
// Currently, all functions in Common.h are inline and defined directly within the header.
// This file is kept for structural completeness and consistency within the module,
// and for future non-inline function implementations.

#include "Common.h" // Include the corresponding header file

namespace ERP {
namespace Common {

// Example of where a non-inline function's implementation would go if it were declared in Common.h:
/*
std::string entityStatusToString(EntityStatus status) {
    // Implementation details would go here
    // This is already inline in Common.h for convenience, but could be moved here if complex.
    switch (status) {
        case EntityStatus::ACTIVE: return "Active";
        case EntityStatus::INACTIVE: return "Inactive";
        // ...
        default: return "Unknown";
    }
}
*/

// No other non-inline function implementations needed currently.

} // namespace Common
} // namespace ERP