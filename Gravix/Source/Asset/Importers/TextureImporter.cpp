#include "pch.h"
#include "TextureImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Gravix 
{

	Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		// Force RGBA format for consistency
		stbi_set_flip_vertically_on_load(false); // Vulkan has inverted Y compared to OpenGL

		int width, height, channels;
		stbi_uc* data = stbi_load(metadata.FilePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		
		Buffer buf;
		if (!data)
		{
			GX_CORE_ERROR("Failed to load texture: {0} - {1}", metadata.FilePath.string(), stbi_failure_reason());

			m_Width = 4;
			m_Height = 4;
			m_Channels = 4;

			uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
			uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
			std::array<uint32_t, 16 * 16> pixels; //for 16x16 checkerboard texture
			for (int x = 0; x < 16; x++) {
				for (int y = 0; y < 16; y++) {
					pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
				}
			}

			buf.Data = reinterpret_cast<uint8_t*>(pixels.data());
			buf.Size = sizeof(uint32_t) * pixels.size();;
		}
		else 
		{
			buf.Data = data;
			buf.Size = width * height * m_Channels;
		}

		TextureSpecification spec;
		spec.DebugName = metadata.FilePath.filename().string();
		Ref<Texture2D> texture = Texture2D::Create(buf, width, height, spec);
		
		stbi_image_free(data);
		return texture;
	}

}