#pragma once

#include "Core/UUID.h"
#include "Core/RefCounted.h"

namespace Gravix
{

	/**
	 * @brief Type alias for asset identifiers
	 *
	 * Each asset is uniquely identified by a UUID (Universally Unique Identifier).
	 * This handle is used to reference assets throughout the engine and is
	 * persistent across serialization.
	 */
	using AssetHandle = UUID;

	/**
	 * @brief Enumeration of supported asset types
	 *
	 * Defines the different types of assets that can be loaded, managed,
	 * and serialized by the asset system.
	 */
	enum class AssetType
	{
		None = 0,     ///< Invalid/unknown asset type
		Scene,        ///< Scene asset (.gravix file)
		Texture2D,    ///< 2D texture asset (.png, .jpg, etc.)
		Material,     ///< Material asset (.gmat file with shader and parameters)
		Script,       ///< C# script asset (.cs file)
		Shader,       ///< Shader asset (.slang file compiled to SPIR-V)
		Pipeline,     ///< Pipeline asset (.pipeline YAML file with rendering configuration)
	};

	/**
	 * @brief Current loading state of an asset
	 *
	 * Assets are loaded asynchronously in multiple stages:
	 * 1. NotLoaded - Asset is known but not yet loaded
	 * 2. Loading - Asset data is being loaded from disk (CPU side)
	 * 3. ReadyForGPU - CPU loading complete, waiting for GPU upload
	 * 4. Loaded - Fully loaded and ready for use
	 * 5. Failed - Loading failed due to missing file, invalid data, etc.
	 */
	enum AssetState
	{
		NotLoaded,    ///< Asset not yet loaded
		Loading,      ///< Currently loading from disk (async)
		ReadyForGPU,  ///< CPU data loaded, pending GPU upload
		Loaded,       ///< Fully loaded and ready to use
		Failed        ///< Loading failed (file not found, invalid format, etc.)
	};

	/**
	 * @brief Convert asset type enum to string representation
	 * @param type Asset type to convert
	 * @return String name of the asset type (e.g., "Scene", "Texture2D")
	 */
	std::string_view AssetTypeToString(AssetType type);

	/**
	 * @brief Convert string to asset type enum
	 * @param typeStr String representation of asset type
	 * @return Corresponding AssetType enum value
	 */
	AssetType StringToAssetType(const std::string& typeStr);

	/**
	 * @brief Abstract base class for all engine assets
	 *
	 * The Asset class is the foundation of Gravix's asset system. All loadable
	 * resources (textures, scenes, materials, scripts) derive from this class.
	 *
	 * Key features:
	 * - Unique identification via AssetHandle (UUID)
	 * - Reference counting for automatic memory management
	 * - Type identification via GetAssetType()
	 * - Async loading support
	 * - Serialization support
	 *
	 * Assets are managed by AssetManager implementations:
	 * - EditorAssetManager - Development-time, loads from disk
	 * - RuntimeAssetManager - Packaged games, loads from asset packs
	 *
	 * @par Usage Example:
	 * @code
	 * // Load a texture asset
	 * AssetHandle texHandle = AssetManager::GetAssetHandleFromPath("Assets/Textures/player.png");
	 * Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(texHandle);
	 *
	 * if (texture && texture->IsLoaded())
	 * {
	 *     // Use the texture
	 *     sprite.SetTexture(texture);
	 * }
	 * @endcode
	 *
	 * @see AssetManager, EditorAssetManager, RuntimeAssetManager
	 */
	class Asset : public RefCounted
	{
	public:
		/**
		 * @brief Virtual destructor
		 */
		virtual ~Asset() = default;

		/**
		 * @brief Get the type of this asset
		 * @return Asset type enum value
		 *
		 * Derived classes must implement this to return their specific type.
		 */
		virtual AssetType GetAssetType() const = 0;

		/**
		 * @brief Get the unique handle for this asset
		 * @return Asset handle (UUID)
		 */
		AssetHandle Handle() const { return m_Handle; }

	private:
		AssetHandle m_Handle; ///< Unique identifier for this asset
	};

}