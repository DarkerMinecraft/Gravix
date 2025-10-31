#include "pch.h"
#include "Renderer2D.h"

#include "Core/UUID.h"

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Texture.h"
#include "Renderer/Generic/Types/Mesh.h"
#include <glm/ext/matrix_transform.hpp>

namespace Gravix
{

	struct Renderer2DData
	{
		const uint32_t MaxQuads = 10000;
		const uint32_t MaxVertices = MaxQuads * 4;
		const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		Ref<Material> TexturedMaterial;
		Ref<Texture2D> WhiteTexture;
		Ref<Mesh> QuadMesh;

		DynamicStruct PushConstants;
		std::vector<DynamicStruct> QuadVertexBuffer;

		static constexpr std::array<glm::vec2, 4> QuadTextureCoords = 
		{
			glm::vec2(0.0f, 0.0f),  // Vertex 0: bottom-left
			glm::vec2(1.0f, 0.0f),  // Vertex 1: bottom-right
			glm::vec2(1.0f, 1.0f),  // Vertex 2: top-right
			glm::vec2(0.0f, 1.0f)   // Vertex 3: top-left
		};

		// Define vertex offsets relative to center
		static constexpr std::array<glm::vec4, 4> QuadVertexOffsets =
		{
			glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),  // Vertex 0: bottom-left
			glm::vec4(0.5f, -0.5f, 0.0f, 1.0f),  // Vertex 1: bottom-right
			glm::vec4(0.5f,  0.5f, 0.0f, 1.0f),  // Vertex 2: top-right
			glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f)   // Vertex 3: top-left
		};

		uint32_t QuadIndexCount = 0;
		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture
	};

	static Renderer2DData* s_Data;

	void Renderer2D::Init(Ref<Framebuffer> renderTarget)
	{
		s_Data = new Renderer2DData();
		// Create a 1x1 white texture
		uint32_t whitePixel = 0xffffffff; // RGBA
		s_Data->WhiteTexture = Texture2D::Create(&whitePixel, 1, 1);

		// Create a default textured material
		MaterialSpecification matSpec;
		matSpec.DebugName = "DefaultTexturedMaterial";
		matSpec.ShaderFilePath = "Assets/shaders/texture.slang";
		matSpec.BlendingMode = Blending::Alphablend;
		matSpec.RenderTarget = renderTarget;
		matSpec.EnableDepthTest = true;
		matSpec.DepthCompareOp = CompareOp::LessOrEqual;

		s_Data->TexturedMaterial = Material::Create(matSpec);

		s_Data->PushConstants = s_Data->TexturedMaterial->GetPushConstantStruct();
		s_Data->QuadMesh = Mesh::Create(s_Data->TexturedMaterial->GetVertexSize());

		std::vector<uint32_t> indices(s_Data->MaxIndices);
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data->MaxIndices; i += 6)
		{
			// First triangle
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			// Second triangle
			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;  // Each quad uses 4 vertices
		}

		s_Data->QuadMesh->SetIndices(indices);

		memset(s_Data->TextureSlots.data(), 0, s_Data->TextureSlots.size() * sizeof(uint32_t));
	}

	void Renderer2D::BeginScene(Command& cmd, Camera& camera, const glm::mat4& transformMatrix)
	{
		s_Data->TextureSlotIndex = 1; 
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		cmd.SetActiveMaterial(s_Data->TexturedMaterial);
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;

		s_Data->PushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
	}

	void Renderer2D::BeginScene(Command& cmd, EditorCamera& camera)
	{
		s_Data->TextureSlotIndex = 1;
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		cmd.SetActiveMaterial(s_Data->TexturedMaterial);
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;

		s_Data->PushConstants.Set("viewProjMatrix", camera.GetViewProjection());
	}

	void Renderer2D::DrawQuad(const glm::mat4& transformMatrix, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/, Ref<Texture2D> texture /*= nullptr*/, float tilingFactor /*= 1.0f*/)
	{
		DynamicStruct vertex = s_Data->TexturedMaterial->GetVertexStruct();

		float textureIndex = 0.0f;
		if (texture != nullptr)
		{
			for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++)
			{
				if (*s_Data->TextureSlots[i].get() == *texture.get())
				{
					textureIndex = (float)i;
					break;
				}
			}

			if (textureIndex == 0.0)
			{
				textureIndex = (float)s_Data->TextureSlotIndex;
				s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
				s_Data->TextureSlotIndex++;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			// Calculate vertex position relative to center
			glm::vec4 finalPos = transformMatrix * s_Data->QuadVertexOffsets[i];

			vertex.Set("position", finalPos);
			vertex.Set("uv", s_Data->QuadTextureCoords[i]);
			vertex.Set("color", color);
			vertex.Set("texIndex", textureIndex);
			vertex.Set("tilingFactor", tilingFactor);

			s_Data->QuadVertexBuffer.push_back(vertex);
		}

		s_Data->QuadIndexCount += 6;
	}

	void Renderer2D::EndScene(Command& cmd)
	{
		std::sort(s_Data->QuadVertexBuffer.begin(), s_Data->QuadVertexBuffer.end(),
			[](DynamicStruct& a, DynamicStruct& b) {
				return a.Get<glm::vec3>("position").z < b.Get<glm::vec3>("position").z;
			});

		s_Data->QuadMesh->SetVertices(s_Data->QuadVertexBuffer);
		s_Data->PushConstants.Set("vertex", s_Data->QuadMesh->GetVertexBufferAddress());

		for(uint32_t i = 0; i < s_Data->TextureSlotIndex; i++)
			cmd.BindResource(0, i, s_Data->TextureSlots[i]);

		Flush(cmd);
	}

	void Renderer2D::Flush(Command& cmd)
	{
		cmd.BindMaterial(s_Data->PushConstants.Data());
		cmd.BindMesh(s_Data->QuadMesh);
		cmd.DrawIndexed(s_Data->QuadIndexCount);
	}

	void Renderer2D::Destroy()
	{
		delete s_Data;
	}

}