// Modules/Asset/DAO/AssetDAO.h
#ifndef MODULES_ASSET_DAO_ASSETDAO_H
#define MODULES_ASSET_DAO_ASSETDAO_H
#include "DAOBase/DAOBase.h" // Include the new templated DAOBase
#include "Modules/Asset/DTO/Asset.h" // For AssetDTO
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace ERP {
namespace Asset {
namespace DAOs {
/**
 * @brief DAO class for Asset entity.
 * Handles database operations for AssetDTO.
 */
class AssetDAO : public ERP::DAOBase::DAOBase<ERP::Asset::DTO::AssetDTO> {
public:
    /**
     * @brief Constructs an AssetDAO.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit AssetDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~AssetDAO() override = default;

protected:
    /**
     * @brief Converts an AssetDTO object into a data map for database storage.
     * This is an override of the pure virtual method in DAOBase.
     * @param asset AssetDTO object.
     * @return Converted data map.
     */
    std::map<std::string, std::any> toMap(const ERP::Asset::DTO::AssetDTO& asset) const override;
    /**
     * @brief Converts a database data map into an AssetDTO object.
     * This is an override of the pure virtual method in DAOBase.
     * @param data Database data map.
     * @return Converted AssetDTO object.
     */
    ERP::Asset::DTO::AssetDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase, so no need to declare here
    // However, for `save` in AssetDAO.cpp, it still uses a static `toMap` which might conflict.
    // The `toMap`/`fromMap` methods should be members of the templated DAO.
};
} // namespace DAOs
} // namespace Asset
} // namespace ERP
#endif // MODULES_ASSET_DAO_ASSETDAO_H