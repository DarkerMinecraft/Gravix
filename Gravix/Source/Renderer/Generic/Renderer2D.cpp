#include "pch.h"
#include "Renderer2D.h"

#include "Core/UUID.h"

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Texture.h"
#include "Renderer/Generic/Types/Mesh.h"
#include "Debug/Instrumentor.h"
#include <glm/ext/matrix_transform.hpp>

namespace Gravix
{

	struct Renderer2DData
	{
		const uint32_t MaxQuads = 10000;
		const uint32_t MaxQuadVertices = MaxQuads * 4;
		const uint32_t MaxQuadIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		const uint32_t MaxCircles = 10000;
		const uint32_t MaxCircleVertices = MaxCircles * 4;
		const uint32_t MaxCircleIndices = MaxCircles * 6;

		Ref<Material> QuadMaterial;
		Ref<Texture2D> WhiteTexture;
		Ref<Mesh> QuadMesh;

		Ref<Material> CircleMaterial;
		Ref<Mesh> CircleMesh;

		DynamicStruct QuadPushConstants;
		std::vector<DynamicStruct> QuadVertexBuffer;

		DynamicStruct CirclePushConstants;
		std::vector<DynamicStruct> CircleVertexBuffer;

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

		uint32_t CircleIndexCount = 0;
	};

	static Renderer2DData* s_Data;

	void Renderer2D::Init(Ref<Framebuffer> renderTarget)
	{
		s_Data = new Renderer2DData();
		// Create a 1x1 white texture
		uint32_t whitePixel = 0xffffffff; // RGBA
		Buffer buffer;
		buffer.Data = reinterpret_cast<uint8_t*>(&whitePixel);
		buffer.Size = 4;

		s_Data->WhiteTexture = Texture2D::Create(buffer, 1, 1);

		// Create a default textured material
		MaterialSpecification matSpec{};
		matSpec.DebugName = "DefaultQuadTexture";
		matSpec.ShaderFilePath = "Assets/shaders/quad.slang";
		matSpec.BlendingMode = Blending::Alphablend;
		matSpec.RenderTarget = renderTarget;
		matSpec.EnableDepthTest = true;

		s_Data->QuadMaterial = Material::Create(matSpec);

		s_Data->QuadPushConstants = s_Data->QuadMaterial->GetPushConstantStruct();
		s_Data->QuadMesh = Mesh::Create(s_Data->QuadMaterial->GetVertexSize());

		matSpec.DebugName = "DefaultCircle";
		matSpec.ShaderFilePath = "Assets/shaders/circle.slang";

		s_Data->CircleMaterial = Material::Create(matSpec);
		s_Data->CirclePushConstants = s_Data->CircleMaterial->GetPushConstantStruct();
		s_Data->CircleMesh = Mesh::Create(s_Data->CircleMaterial->GetVertexSize());

		std::vector<uint32_t> quadIndices(s_Data->MaxQuadIndices);
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data->MaxQuadIndices; i += 6)
		{
			// First triangle
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			// Second triangle
			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;  // Each quad uses 4 vertices
		}

		s_Data->QuadMesh->SetIndices(quadIndices);

		std::vector<uint32_t> circleIndices(s_Data->MaxQuadIndices);
		offset = 0;
		for (uint32_t i = 0; i < s_Data->MaxQuadIndices; i += 6)
		{
			// First triangle
			circleIndices[i + 0] = offset + 0;
			circleIndices[i + 1] = offset + 1;
			circleIndices[i + 2] = offset + 2;

			// Second triangle
			circleIndices[i + 3] = offset + 2;
			circleIndices[i + 4] = offset + 3;
			circleIndices[i + 5] = offset + 0;

			offset += 4;  // Each quad uses 4 vertices
		}

		s_Data->CircleMesh->SetIndices(circleIndices);

		memset(s_Data->TextureSlots.data(), 0, s_Data->TextureSlots.size() * sizeof(uint32_t));
	}

	void Renderer2D::BeginScene(Command& cmd, Camera& camera, const glm::mat4& transformMatrix)
	{
		GX_PROFILE_FUNCTION();

		s_Data->TextureSlotIndex = 1;
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		s_Data->CircleIndexCount = 0;
		s_Data->CircleVertexBuffer.clear();

		s_Data->TextureSlots[0] = s_Data->WhiteTexture;
		s_Data->QuadPushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
		s_Data->CirclePushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
	}

	void Renderer2D::BeginScene(Command& cmd, EditorCamera& camera)
	{
		GX_PROFILE_FUNCTION();

		s_Data->TextureSlotIndex = 1;
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		s_Data->CircleIndexCount = 0;
		s_Data->CircleVertexBuffer.clear();

		s_Data->TextureSlots[0] = s_Data->WhiteTexture;
		s_Data->QuadPushConstants.Set("viewProjMatrix", camera.GetViewProjection());
		s_Data->CirclePushConstants.Set("viewProjMatrix", camera.GetViewProjection());
	}

	void Renderer2D::DrawQuad(const glm::mat4& transformMatrix, uint32_t entityID, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/, Ref<Texture2D> texture /*= nullptr*/, float tilingFactor /*= 1.0f*/)
	{
		float textureIndex = 0.0f;
		if (texture != nullptr)
		{
			// Check if texture is already in a slot
			for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++)
			{
				if (*s_Data->TextureSlots[i].get() == *texture.get())
				{
					textureIndex = (float)i;
					break;
				}
			}

			// Add new texture if not found
			if (textureIndex == 0.0f)
			{
				// Check if we have room for more textures
				if (s_Data->TextureSlotIndex < s_Data->MaxTextureSlots)
				{
					textureIndex = (float)s_Data->TextureSlotIndex;
					s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
					s_Data->TextureSlotIndex++;
				}
				// If no room, use white texture (fallback)
			}
		}

		DynamicStruct vertex = s_Data->QuadMaterial->GetVertexStruct();

		for (int i = 0; i < 4; i++)
		{
			// Calculate vertex position relative to center
			glm::vec4 finalPos = transformMatrix * s_Data->QuadVertexOffsets[i];

			vertex.Set("position", finalPos);
			vertex.Set("uv", s_Data->QuadTextureCoords[i]);
			vertex.Set("color", color);
			vertex.Set("texIndex", textureIndex);
			vertex.Set("tilingFactor", tilingFactor);
			vertex.Set("entityID", entityID);

			s_Data->QuadVertexBuffer.push_back(vertex);
		}

		s_Data->QuadIndexCount += 6;
	}

	void Renderer2D::DrawCircle(const glm::mat4& transformMatrix, uint32_t entityID, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/, float thickness /*= 0.1f*/, float fade /*= 0.005f*/)
	{
		DynamicStruct vertex = s_Data->CircleMaterial->GetVertexStruct();

		for (int i = 0; i < 4; i++)
		{
			// Calculate vertex position relative to center
			glm::vec4 finalPos = transformMatrix * s_Data->QuadVertexOffsets[i];

			vertex.Set("worldPosition", finalPos);
			vertex.Set("localPosition", s_Data->QuadVertexOffsets[i] * 2.0f);
			vertex.Set("color", color);
			vertex.Set("thickness", thickness);
			vertex.Set("fade", fade);
			vertex.Set("entityID", entityID);

			s_Data->CircleVertexBuffer.push_back(vertex);
		}

		s_Data->CircleIndexCount += 6;
	}

	void Renderer2D::EndScene(Command& cmd)
	{
		GX_PROFILE_FUNCTION();

		s_Data->QuadMesh->SetVertices(s_Data->QuadVertexBuffer);
		s_Data->QuadPushConstants.Set("vertex", s_Data->QuadMesh->GetVertexBufferAddress());

		s_Data->CircleMesh->SetVertices(s_Data->CircleVertexBuffer);
		s_Data->CirclePushConstants.Set("vertex", s_Data->CircleMesh->GetVertexBufferAddress());

		Flush(cmd);
	}

	void Renderer2D::Flush(Command& cmd)
	{
		cmd.SetActiveMaterial(s_Data->QuadMaterial);
		for (uint32_t i = 0; i < s_Data->TextureSlotIndex; i++)
			cmd.BindResource(0, i, s_Data->TextureSlots[i]);
		cmd.BindMaterial(s_Data->QuadPushConstants.Data());
		cmd.BindMesh(s_Data->QuadMesh);
		cmd.DrawIndexed(s_Data->QuadIndexCount);
		cmd.SetActiveMaterial(s_Data->CircleMaterial);
		cmd.BindMaterial(s_Data->CirclePushConstants.Data());
		cmd.BindMesh(s_Data->CircleMesh);
		cmd.DrawIndexed(s_Data->CircleIndexCount);
	}

	void Renderer2D::Destroy()
	{
		delete s_Data;
	}

}