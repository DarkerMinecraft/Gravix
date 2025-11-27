#include "pch.h"
#include "TextureImporter.h"

#include "Project/Project.h"

#include <stb_image.h>

namespace Gravix
{

	Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadTexture2D(Project::GetAssetDirectory() / metadata.FilePath);
	}

	Ref<Texture2D> TextureImporter::LoadTexture2D(const std::filesystem::path& path)
	{
		int width, height, channels;
		Buffer data = LoadTexture2DToBuffer(path, &width, &height, &channels);

		TextureSpecification spec;
		spec.DebugName = path.filename().string();
		Ref<Texture2D> texture = Texture2D::Create(data, width, height, spec);
		data.Release();

		return texture;
	}

	Buffer TextureImporter::LoadTexture2DToBuffer(const std::filesystem::path& path, int* width, int* height, int* channels)
	{
		// Force RGBA format for consistency
		stbi_set_flip_vertically_on_load(false); // Vulkan has inverted Y compared to OpenGL

		Buffer data;
		data.Data = stbi_load(path.string().c_str(), width, height, channels, STBI_rgb_alpha);

		if (!(stbi_uc*)data.Data)
		{
			GX_CORE_ERROR("Failed to load texture: {0} - {1}", path.string(), stbi_failure_reason());

			*width = 16;
			*height = 16;
			*channels = 4;

			// Allocate error texture using malloc to match Buffer::Release() which uses free()
			const size_t pixelCount = 16 * 16;
			uint32_t* pixels = (uint32_t*)malloc(sizeof(uint32_t) * pixelCount);

			uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
			uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

			for (int x = 0; x < 16; x++) {
				for (int y = 0; y < 16; y++) {
					pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
				}
			}

			data.Data = reinterpret_cast<uint8_t*>(pixels);
			data.Size = sizeof(uint32_t) * pixelCount;
		}
		else { data.Size = *width * *height * *channels; }

		return data;
	}

}