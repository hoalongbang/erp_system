// Modules/Database/DTO/DatabaseConfig.h
#ifndef MODULES_DATABASE_DTO_DATABASECONFIG_H
#define MODULES_DATABASE_DTO_DATABASECONFIG_H
#include <string>   // For std::string
#include <optional> // For std::optional

namespace ERP {
namespace Database {
namespace DTO {
/**
 * @brief Enum for supported database types.
 */
enum class DatabaseType {
    SQLite,
    PostgreSQL, // Example for future expansion
    MySQL       // Example for future expansion
};
/**
 * @brief DTO for Database Configuration.
 * Contains all necessary parameters to establish a database connection.
 */
struct DatabaseConfig {
    DatabaseType type;          /**< Type of the database (e.g., SQLite, PostgreSQL). */
    std::string database;       /**< Database name or file path (for SQLite). */
    std::optional<std::string> host; /**< Hostname or IP address for network databases. */
    std::optional<int> port;         /**< Port number for network databases. */
    std::optional<std::string> username; /**< Username for database authentication. */
    std::optional<std::string> password; /**< Password for database authentication. */
    int maxConnections = 10;    /**< MỚI: Maximum number of connections in the pool. */
    int connectionTimeoutSeconds = 30; /**< MỚI: Timeout for acquiring a connection from the pool. */
    // Default constructor
    DatabaseConfig() : type(DatabaseType::SQLite) {}
};
} // namespace DTO
} // namespace Database
} // namespace ERP
#endif // MODULES_DATABASE_DTO_DATABASECONFIG_H