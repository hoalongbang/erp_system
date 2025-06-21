// Modules/Database/DBConnection.cpp
// This file is intentionally left mostly empty as DBConnection is an abstract base class.
// Its methods are pure virtual and implemented by derived classes like SQLiteConnection.
// It serves as a central point to include common headers needed for derived classes.

#include "DBConnection.h"
// No direct includes of Logger or ErrorHandler here to maintain low-level independence.
// Derived classes or higher-level components will handle logging/error handling.

namespace ERP {
namespace Database {

// Constructor definition (can be empty for an abstract class, but good practice to define)
// No need to initialize dbConnection_ here as it's protected and handled by derived classes.
// Derived classes will typically take a connection object in their constructor.
// This file is mostly for defining the abstract interface.

// No implementation for pure virtual methods here.
// All concrete implementations for DB operations (open, close, execute, query, etc.)
// are provided in derived classes like SQLiteConnection.

} // namespace Database
} // namespace ERP