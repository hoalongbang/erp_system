// Modules/Config/DAO/ConfigDAO.h
#ifndef MODULES_CONFIG_DAO_CONFIGDAO_H
#define MODULES_CONFIG_DAO_CONFIGDAO_H
#include "DAOBase/DAOBase.h" // Include the new templated DAOBase
#include "Modules/Config/DTO/Config.h" // For ConfigDTO

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace ERP {
namespace Config {
namespace DAOs {
/**
 * @brief DAO class for Config entity.
 * Handles database operations for ConfigDTO.
 */
class ConfigDAO : public ERP::DAOBase::DAOBase<ERP::Config::DTO::ConfigDTO> {
public:
    /**
     * @brief Constructs a ConfigDAO.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit ConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ConfigDAO() override = default;

protected:
    /**
     * @brief Converts a ConfigDTO object into a data map for database storage.
     * This is an override of the pure virtual method in DAOBase.
     * @param config ConfigDTO object.
     * @return Converted data map.
     */
    std::map<std::string, std::any> toMap(const ERP::Config::DTO::ConfigDTO& config) const override;
    /**
     * @brief Converts a database data map into a ConfigDTO object.
     * This is an override of the pure virtual method in DAOBase.
     * @param data Database data map.
     * @return Converted ConfigDTO object.
     */
    ERP::Config::DTO::ConfigDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};
} // namespace DAOs
} // namespace Config
} // namespace ERP
#endif // MODULES_CONFIG_DAO_CONFIGDAO_H