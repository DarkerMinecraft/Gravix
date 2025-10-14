#include "pch.h"
#include "Renderer2D.h"

#include "Renderer/Generic/Material.h"
#include "Renderer/Generic/Texture.h"
#include "Renderer/Generic/MeshBuffer.h"

namespace Gravix
{

	struct Renderer2DData
	{
		const uint32_t MaxQuads = 10000;
		const uint32_t MaxVertices = MaxQuads * 4;
		const uint32_t MaxIndices = MaxQuads * 6;

		Ref<Material> TexturedMaterial;
		Ref<Texture2D> WhiteTexture;
		Ref<MeshBuffer> QuadMesh;

		DynamicStruct PushConstants;
		std::vector<DynamicStruct> QuadVertexBuffer;

		static constexpr std::array<glm::vec2, 4> QuadTextureCoords = 
		{
			glm::vec2(0.0f, 0.0f),  // Vertex 0: bottom-left
			glm::vec2(1.0f, 0.0f),  // Vertex 1: bottom-right
			glm::vec2(1.0f, 1.0f),  // Vertex 2: top-right
			glm::vec2(0.0f, 1.0f)   // Vertex 3: top-left
		};

		uint32_t QuadIndexCount = 0;
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
		matSpec.RenderTarget = renderTarget;

		s_Data->TexturedMaterial = Material::Create(matSpec);

		s_Data->PushConstants = s_Data->TexturedMaterial->GetPushConstantStruct();

		s_Data->QuadMesh = MeshBuffer::Create(s_Data->TexturedMaterial->GetReflectedStruct("Vertex"), s_Data->MaxVertices, s_Data->MaxIndices);

		std::vector<uint32_t> indicesVec(s_Data->MaxIndices);
		std::span<uint32_t> indices(indicesVec);
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
	}

	void Renderer2D::BeginScene(Command& cmd, const glm::mat4& viewProjection)
	{
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();
		s_Data->QuadMesh->ClearVertices();

		cmd.SetActiveMaterial(s_Data->TexturedMaterial);
		cmd.BindResource(0, 0, s_Data->WhiteTexture);

		s_Data->PushConstants.Set("viewProjMatrix", viewProjection);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		DynamicStruct vertex = s_Data->TexturedMaterial->GetVertexStruct();

		for (int i = 0; i < 4; i++)
		{
			glm::vec3 finalPos = position + glm::vec3(
				(i == 0 || i == 3) ? 0.0f : size.x,
				(i == 0 || i == 1) ? 0.0f : size.y,
				0.0f);

			vertex.Set<glm::vec3>("position", finalPos);
			vertex.Set<glm::vec2>("uv", s_Data->QuadTextureCoords[i]);
			vertex.Set<glm::vec4>("color", color);
			vertex.Set<float>("texIndex", 0);
			vertex.Set<float>("tilingFactor", 1);

			s_Data->QuadVertexBuffer.push_back(vertex);
		}

		s_Data->QuadIndexCount += 6;
	}


	void Renderer2D::EndScene(Command& cmd)
	{
		s_Data->QuadMesh->SetVertices(s_Data->QuadVertexBuffer);
		s_Data->PushConstants.Set("vertex", s_Data->QuadMesh->GetVertexBufferAddress());

		Flush(cmd);
	}

	void Renderer2D::Flush(Command& cmd)
	{
		cmd.BindMaterial(s_Data->PushConstants.Data());
		cmd.BeginRendering();
		cmd.BindMesh(s_Data->QuadMesh);
		cmd.DrawIndexed(s_Data->QuadIndexCount);
		cmd.EndRendering();
	}

	void Renderer2D::Destroy()
	{
		delete s_Data;
	}

}