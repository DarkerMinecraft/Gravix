#pragma once

#include "Asset/Asset.h"
#include "Renderer/Specification.h"

#include <string>

namespace Gravix
{

	struct PipelineConfiguration
	{
		// Blending
		Blending BlendingMode = Blending::None;

		// Depth testing
		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		CompareOp DepthCompareOp = CompareOp::Less;

		// Rasterization
		Cull CullMode = Cull::None;
		FrontFace FrontFaceWinding = FrontFace::CounterClockwise;
		Fill FillMode = Fill::Solid;

		// Topology
		Topology GraphicsTopology = Topology::TriangleList;

		// Line width
		float LineWidth = 1.0f;

		// RenderTarget is NOT stored here - it's provided by the application at runtime
	};

	// Pipeline Asset - stores graphics pipeline configuration (YAML serialized)
	class Pipeline : public Asset
	{
	public:
		Pipeline() = default;
		Pipeline(const PipelineConfiguration& config);
		virtual ~Pipeline() = default;

		virtual AssetType GetAssetType() const override { return AssetType::Pipeline; }

		const PipelineConfiguration& GetConfiguration() const { return m_Configuration; }
		void SetConfiguration(const PipelineConfiguration& config) { m_Configuration = config; }

	private:
		PipelineConfiguration m_Configuration;

		friend class PipelineImporter;
	};

}
