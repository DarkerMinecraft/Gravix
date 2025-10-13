#include "pch.h"
#include "Renderer2D.h"

#include "Renderer/Generic/Material.h"
#include "Renderer/Generic/Texture.h"
#include "Renderer/Generic/MeshBuffer.h"

namespace Gravix 
{

	struct Renderer2DStorage 
	{
		Ref<Material> TexturedMaterial;
		Ref<Texture2D> WhiteTexture;
		Ref<MeshBuffer> QuadMesh;
		DynamicStruct Vertex;
		DynamicStruct PushConstants;
	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init() 
	{
		s_Data = new Renderer2DStorage();
		// Create a 1x1 white texture
		uint32_t whitePixel = 0xffffffff; // RGBA
		s_Data->WhiteTexture = Texture2D::Create(&whitePixel, 1, 1);

		// Create a default textured material
		MaterialSpecification matSpec;
		matSpec.DebugName = "DefaultTexturedMaterial";
		matSpec.ShaderFilePath = "Assets/shaders/texture.slang";

		s_Data->TexturedMaterial = Material::Create(matSpec);

		s_Data->QuadMesh = MeshBuffer::Create();
	}

	void Renderer2D::Destroy() 
	{
		delete s_Data;
	}

}