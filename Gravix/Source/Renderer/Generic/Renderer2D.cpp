#include "pch.h"
#include "Renderer2D.h"

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Shader.h"
#include "Renderer/Generic/Types/Pipeline.h"
#include "Renderer/Generic/Types/Texture.h"
#include "Renderer/Generic/Types/Mesh.h"

#include "Asset/Importers/ShaderImporter.h"

#include "Debug/Instrumentor.h"

namespace Gravix
{

	struct Renderer2DData
	{
		const uint32_t MaxQuads = 2000;  // Reduced from 10000 for better memory usage
		const uint32_t MaxQuadVertices = MaxQuads * 4;
		const uint32_t MaxQuadIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		const uint32_t MaxCircles = 2000;  // Reduced from 10000 for better memory usage
		const uint32_t MaxCircleVertices = MaxCircles * 4;
		const uint32_t MaxCircleIndices = MaxCircles * 6;

		const uint32_t MaxLines = 2000;  // Reduced from 10000 for better memory usage
		const uint32_t MaxLineVertices = MaxLines * 2;
		const float LineWidth = 2.0f;

		Ref<Material> QuadMaterial;
		Ref<Texture2D> WhiteTexture;
		Ref<Mesh> QuadMesh;

		Ref<Material> CircleMaterial;
		Ref<Mesh> CircleMesh;

		Ref<Material> LineMaterial;
		Ref<Mesh> LineMesh;

		DynamicStruct QuadPushConstants;
		std::vector<DynamicStruct> QuadVertexBuffer;

		DynamicStruct CirclePushConstants;
		std::vector<DynamicStruct> CircleVertexBuffer;

		DynamicStruct LinePushConstants;
		std::vector<DynamicStruct> LineVertexBuffer;

		// Cached vertex structs to avoid allocations in hot path
		DynamicStruct CachedQuadVertex;
		DynamicStruct CachedCircleVertex;
		DynamicStruct CachedLineVertex;

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
		uint32_t LineVertexCount = 0;
	};

	static Ref<Renderer2DData> s_Data;

	void Renderer2D::Init(Ref<Framebuffer> renderTarget)
	{
		s_Data = CreateRef<Renderer2DData>();
		// Create a 1x1 white texture
		uint32_t whitePixel = 0xffffffff; // RGBA
		Buffer buffer;
		buffer.Data = reinterpret_cast<uint8_t*>(&whitePixel);
		buffer.Size = 4;

		s_Data->WhiteTexture = Texture2D::Create(buffer, 1, 1);

		// Create quad material with new system
		{
			Ref<Shader> quadShader = ShaderImporter::LoadFromFile("Assets/shaders/quad.slang");

			PipelineConfiguration quadPipelineConfig{};
			quadPipelineConfig.BlendingMode = Blending::Alpha;
			quadPipelineConfig.EnableDepthTest = true;
			quadPipelineConfig.DepthCompareOp = CompareOp::Less;
			quadPipelineConfig.GraphicsTopology = Topology::TriangleList;

			Ref<Pipeline> quadPipeline = CreateRef<Pipeline>(quadPipelineConfig);

			s_Data->QuadMaterial = Material::Create(quadShader, quadPipeline);
			s_Data->QuadMaterial->SetFramebuffer(renderTarget);
			s_Data->QuadPushConstants = s_Data->QuadMaterial->GetPushConstantStruct();
			s_Data->CachedQuadVertex = s_Data->QuadMaterial->GetVertexStruct();
			s_Data->QuadMesh = Mesh::Create(s_Data->QuadMaterial->GetVertexSize(), s_Data->MaxQuadVertices, s_Data->MaxQuadIndices);
		}

		// Create circle material with new system
		{
			Ref<Shader> circleShader = ShaderImporter::LoadFromFile("Assets/shaders/circle.slang");

			PipelineConfiguration circlePipelineConfig{};
			circlePipelineConfig.BlendingMode = Blending::Alpha;
			circlePipelineConfig.EnableDepthTest = true;
			circlePipelineConfig.DepthCompareOp = CompareOp::Less;
			circlePipelineConfig.GraphicsTopology = Topology::TriangleList;

			Ref<Pipeline> circlePipeline = CreateRef<Pipeline>(circlePipelineConfig);

			s_Data->CircleMaterial = Material::Create(circleShader, circlePipeline);
			s_Data->CircleMaterial->SetFramebuffer(renderTarget);
			s_Data->CirclePushConstants = s_Data->CircleMaterial->GetPushConstantStruct();
			s_Data->CachedCircleVertex = s_Data->CircleMaterial->GetVertexStruct();
			s_Data->CircleMesh = Mesh::Create(s_Data->CircleMaterial->GetVertexSize(), s_Data->MaxCircleVertices, s_Data->MaxCircleIndices);
		}

		// Create line material with new system
		{
			Ref<Shader> lineShader = ShaderImporter::LoadFromFile("Assets/shaders/line.slang");

			PipelineConfiguration linePipelineConfig{};
			linePipelineConfig.BlendingMode = Blending::Alpha;
			linePipelineConfig.EnableDepthTest = true;
			linePipelineConfig.DepthCompareOp = CompareOp::Less;
			linePipelineConfig.GraphicsTopology = Topology::LineList;
			linePipelineConfig.LineWidth = s_Data->LineWidth;

			Ref<Pipeline> linePipeline = CreateRef<Pipeline>(linePipelineConfig);

			s_Data->LineMaterial = Material::Create(lineShader, linePipeline);
			s_Data->LineMaterial->SetFramebuffer(renderTarget);
			s_Data->LineMesh = Mesh::Create(s_Data->LineMaterial->GetVertexSize(), s_Data->MaxLineVertices, 0);
			s_Data->LinePushConstants = s_Data->LineMaterial->GetPushConstantStruct();
			s_Data->CachedLineVertex = s_Data->LineMaterial->GetVertexStruct();
		}

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

		// Reserve small initial capacity, buffers will grow dynamically as needed
		// This avoids both upfront over-allocation and many small reallocations
		s_Data->QuadVertexBuffer.reserve(400);   // 100 quads worth
		s_Data->CircleVertexBuffer.reserve(400); // 100 circles worth
		s_Data->LineVertexBuffer.reserve(200);   // 100 lines worth
	}

	void Renderer2D::BeginScene(Command& cmd, Camera& camera, const glm::mat4& transformMatrix)
	{
		GX_PROFILE_FUNCTION();

		s_Data->TextureSlotIndex = 1;
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		s_Data->CircleIndexCount = 0;
		s_Data->CircleVertexBuffer.clear();

		s_Data->LineVertexCount = 0;
		s_Data->LineVertexBuffer.clear();

		s_Data->TextureSlots[0] = s_Data->WhiteTexture;
		s_Data->QuadPushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
		s_Data->CirclePushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
		s_Data->LinePushConstants.Set("viewProjMatrix", camera.GetProjection() * glm::inverse(transformMatrix));
	}

	void Renderer2D::BeginScene(Command& cmd, EditorCamera& camera)
	{
		GX_PROFILE_FUNCTION();

		s_Data->TextureSlotIndex = 1;
		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBuffer.clear();

		s_Data->CircleIndexCount = 0;
		s_Data->CircleVertexBuffer.clear();

		s_Data->LineVertexCount = 0;
		s_Data->LineVertexBuffer.clear();

		s_Data->TextureSlots[0] = s_Data->WhiteTexture;
		s_Data->QuadPushConstants.Set("viewProjMatrix", camera.GetViewProjection());
		s_Data->CirclePushConstants.Set("viewProjMatrix", camera.GetViewProjection());
		s_Data->LinePushConstants.Set("viewProjMatrix", camera.GetViewProjection());
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

		for (int i = 0; i < 4; i++)
		{
			// Calculate vertex position relative to center
			glm::vec4 finalPos = transformMatrix * s_Data->QuadVertexOffsets[i];

			s_Data->CachedQuadVertex.Set("position", finalPos);
			s_Data->CachedQuadVertex.Set("uv", s_Data->QuadTextureCoords[i]);
			s_Data->CachedQuadVertex.Set("color", color);
			s_Data->CachedQuadVertex.Set("texIndex", textureIndex);
			s_Data->CachedQuadVertex.Set("tilingFactor", tilingFactor);
			s_Data->CachedQuadVertex.Set("entityID", entityID);

			s_Data->QuadVertexBuffer.push_back(s_Data->CachedQuadVertex);
		}

		s_Data->QuadIndexCount += 6;
	}

	void Renderer2D::DrawCircle(const glm::mat4& transformMatrix, uint32_t entityID, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/, float thickness /*= 0.1f*/, float fade /*= 0.005f*/)
	{
		for (int i = 0; i < 4; i++)
		{
			// Calculate vertex position relative to center
			glm::vec4 finalPos = transformMatrix * s_Data->QuadVertexOffsets[i];

			s_Data->CachedCircleVertex.Set("worldPosition", finalPos);
			s_Data->CachedCircleVertex.Set("localPosition", s_Data->QuadVertexOffsets[i] * 2.0f);
			s_Data->CachedCircleVertex.Set("color", color);
			s_Data->CachedCircleVertex.Set("thickness", thickness);
			s_Data->CachedCircleVertex.Set("fade", fade);
			s_Data->CachedCircleVertex.Set("entityID", entityID);

			s_Data->CircleVertexBuffer.push_back(s_Data->CachedCircleVertex);
		}

		s_Data->CircleIndexCount += 6;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/)
	{
		s_Data->CachedLineVertex.Set("color", color);

		s_Data->CachedLineVertex.Set("position", p0);
		s_Data->LineVertexBuffer.push_back(s_Data->CachedLineVertex);
		s_Data->CachedLineVertex.Set("position", p1);
		s_Data->LineVertexBuffer.push_back(s_Data->CachedLineVertex);

		s_Data->LineVertexCount += 2;
	}

	void Renderer2D::DrawQuadOutline(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);


		DrawLine(p0, p1, color);
		DrawLine(p1, p2, color);
		DrawLine(p2, p3, color);
		DrawLine(p3, p0, color);
	}

	void Renderer2D::DrawQuadOutline(const glm::mat4& transformMatrix, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/)
	{
		glm::vec3 lineVertices[4]; 
		for(int i = 0; i < 4; i++)
			lineVertices[i] = transformMatrix * s_Data->QuadVertexOffsets[i];
			
		DrawLine(lineVertices[0], lineVertices[1], color);
		DrawLine(lineVertices[1], lineVertices[2], color);
		DrawLine(lineVertices[2], lineVertices[3], color);
		DrawLine(lineVertices[3], lineVertices[0], color);
	}

	void Renderer2D::DrawCircleOutline(const glm::mat3& position, const glm::vec2& size, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/)
	{
		float pi = 3.14159265359f;
		for (int i = 0; i < 15; i++) 
		{
			float theta0 = (i / 15.0f) * 2.0f * pi;
			float theta1 = ((i + 1) / 15.0f) * 2.0f * pi;

			glm::vec3 p0 = position * glm::vec3(cos(theta0) * size.x * 0.5f, sin(theta0) * size.y * 0.5f, 1.0f);
			glm::vec3 p1 = position * glm::vec3(cos(theta1) * size.x * 0.5f, sin(theta1) * size.y * 0.5f, 1.0f);
			DrawLine(p0, p1, color);
		}
	}

	void Renderer2D::DrawCircleOutline(const glm::mat4& transformMatrix, const glm::vec4& color /*= { 1.0f, 1.0f, 1.0f, 1.0f }*/)
	{
		constexpr int segments = 32; // higher = smoother outline
		constexpr float pi = 3.14159265359f;

		for (int i = 0; i < segments; i++)
		{
			float theta0 = (i / (float)segments) * 2.0f * pi;
			float theta1 = ((i + 1) / (float)segments) * 2.0f * pi;

			// Local circle with radius 0.5 � scaling will come from transformMatrix
			glm::vec4 localP0 = glm::vec4(cos(theta0) * 0.5f, sin(theta0) * 0.5f, 0.0f, 1.0f);
			glm::vec4 localP1 = glm::vec4(cos(theta1) * 0.5f, sin(theta1) * 0.5f, 0.0f, 1.0f);

			// Apply full transform (position, rotation, scale)
			glm::vec3 p0 = glm::vec3(transformMatrix * localP0);
			glm::vec3 p1 = glm::vec3(transformMatrix * localP1);

			// Draw line segment
			DrawLine(p0, p1, color);
		}
	}

	void Renderer2D::EndScene(Command& cmd)
	{
		GX_PROFILE_FUNCTION();

		s_Data->QuadMesh->SetVertices(s_Data->QuadVertexBuffer);
		s_Data->QuadPushConstants.Set("vertex", s_Data->QuadMesh->GetVertexBufferAddress());

		s_Data->CircleMesh->SetVertices(s_Data->CircleVertexBuffer);
		s_Data->CirclePushConstants.Set("vertex", s_Data->CircleMesh->GetVertexBufferAddress());

		s_Data->LineMesh->SetVertices(s_Data->LineVertexBuffer);
		s_Data->LinePushConstants.Set("vertex", s_Data->LineMesh->GetVertexBufferAddress());

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

		cmd.SetActiveMaterial(s_Data->LineMaterial);
		cmd.SetLineWidth(s_Data->LineWidth);
		cmd.BindMaterial(s_Data->LinePushConstants.Data());
		cmd.Draw(s_Data->LineVertexCount);
	}

	void Renderer2D::Destroy()
	{
		s_Data = nullptr; // Automatically cleaned up by Ref<>
	}

}