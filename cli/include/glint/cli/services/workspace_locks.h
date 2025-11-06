// Machine Summary Block
// {"file":"cli/include/glint/cli/services/workspace_locks.h","purpose":"Manages workspace lockfiles for modules and asset packs.","exports":["glint::cli::services::ModuleLockEntry","glint::cli::services::AssetLockEntry","glint::cli::services::ModuleRegistry","glint::cli::services::AssetRegistry"],"depends_on":["<filesystem>","<optional>","<string>","<vector>"],"notes":["modules_lock_management","asset_lock_management","deterministic_write_order"]}
// Human Summary
// Provides helpers to load, mutate, and persist `modules.lock` and `assets.lock` files within a Glint workspace.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/**
 * @file workspace_locks.h
 * @brief Helpers for reading and writing workspace module/asset lockfiles.
 */

namespace glint::cli::services {

/// @brief Records a module entry stored in `modules.lock`.
struct ModuleLockEntry {
    std::string name;       ///< Module identifier.
    std::string version;    ///< Module semantic version.
    bool enabled = true;    ///< Whether the module is active.
    std::string hash;       ///< Optional module digest (for determinism validation).
};

/// @brief Records an asset pack entry stored in `assets.lock`.
struct AssetLockEntry {
    std::string name;       ///< Asset pack identifier.
    std::string version;    ///< Version string pinned by the lock.
    std::string status;     ///< Status indicator (`installed`, `pending`, etc.).
    std::string hash;       ///< Optional integrity digest.
};

/**
 * @brief In-memory representation of `modules.lock` that supports safe mutation and persistence.
 */
class ModuleRegistry {
public:
    ModuleRegistry();

    /**
     * @brief Load the registry from `<workspace>/modules/modules.lock`.
     * @param workspaceRoot Workspace root directory.
     * @return Populated registry. Missing lockfiles yield an empty registry.
     */
    static ModuleRegistry load(const std::filesystem::path& workspaceRoot);

    /// @brief Persist registry state back to `<workspace>/modules/modules.lock`.
    void save() const;

    /// @brief Set (or update) a module entry, replacing existing data by name.
    void upsert(const ModuleLockEntry& entry);

    /// @brief Enable or disable the named module.
    void setEnabled(const std::string& moduleName, bool enabled);

    /// @brief Remove a module from the registry (no-op if missing).
    void remove(const std::string& moduleName);

    /// @brief Retrieve a module entry by name.
    std::optional<ModuleLockEntry> find(const std::string& moduleName) const;

    /// @brief Access the internal list (ordered alphabetically when saved).
    const std::vector<ModuleLockEntry>& modules() const { return m_modules; }

private:
    std::filesystem::path m_lockPath;
    std::vector<ModuleLockEntry> m_modules;
};

/**
 * @brief In-memory representation of `assets.lock` with helpers for mutation and persistence.
 */
class AssetRegistry {
public:
    AssetRegistry();

    /**
     * @brief Load the registry from `<workspace>/assets/assets.lock`.
     */
    static AssetRegistry load(const std::filesystem::path& workspaceRoot);

    /// @brief Persist registry state back to `<workspace>/assets/assets.lock`.
    void save() const;

    /// @brief Insert or replace an asset pack entry.
    void upsert(const AssetLockEntry& entry);

    /// @brief Update the status of an asset pack (e.g., installed, pending).
    void setStatus(const std::string& name, const std::string& status);

    /// @brief Retrieve an asset pack entry by name.
    std::optional<AssetLockEntry> find(const std::string& name) const;

    /// @brief Return all asset entries.
    const std::vector<AssetLockEntry>& assets() const { return m_assets; }

private:
    std::filesystem::path m_lockPath;
    std::vector<AssetLockEntry> m_assets;
};

} // namespace glint::cli::services
