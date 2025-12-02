#include "pch.h"
#include "SpriteRendererComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include "Asset/AssetManager.h"
#include "Project/Project.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#endif

namespace Gravix
{

	void SpriteRendererComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<SpriteRendererComponent>(
			"Sprite Renderer",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
#ifdef GRAVIX_EDITOR_BUILD
			Serialize,
			Deserialize,
			OnImGuiRender,
#endif
			BinarySerialize,
			BinaryDeserialize
		);
	}

#ifdef GRAVIX_EDITOR_BUILD

	void SpriteRendererComponentRenderer::Serialize(YAML::Emitter& out, SpriteRendererComponent& c)
	{
		out << YAML::Key << "Color" << YAML::Value << c.Color;
		out << YAML::Key << "Texture" << YAML::Value << (uint64_t)c.Texture;
		out << YAML::Key << "TilingFactor" << YAML::Value << c.TilingFactor;
	}

	void SpriteRendererComponentRenderer::Deserialize(SpriteRendererComponent& c, const YAML::Node& node)
	{
		c.Color = node["Color"].as<glm::vec4>();
		c.Texture = (AssetHandle)node["Texture"].as<uint64_t>();
		c.TilingFactor = node["TilingFactor"].as<float>();
	}

	void SpriteRendererComponentRenderer::OnImGuiRender(SpriteRendererComponent& c, ComponentUserSettings*)
	{
		// Color property
		ImGuiHelpers::BeginPropertyRow("Color");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::ColorEdit4("##Color", glm::value_ptr(c.Color));
		ImGuiHelpers::EndPropertyRow();

		// Texture property
		ImGuiHelpers::BeginPropertyRow("Texture");

		std::string label = "None";
		bool validTexture = false;
		if (c.Texture != 0)
		{
			if (AssetManager::IsValidAssetHandle(c.Texture) && AssetManager::GetAssetType(c.Texture) == AssetType::Texture2D)
			{
				const auto& metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(c.Texture);
				label = metadata.FilePath.filename().string();
				validTexture = true;
			}
			else
			{
				label = "Invalid";
			}
		}

		float availWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = validTexture ? availWidth - 30.0f : availWidth;

		ImGui::Button(label.c_str(), { buttonWidth, 0.0f });
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				AssetHandle textureHandle = *(AssetHandle*)payload->Data;
				if(AssetManager::GetAssetType(textureHandle) == AssetType::Texture2D)
					c.Texture = textureHandle;
			}
			ImGui::EndDragDropTarget();
		}

		if (validTexture)
		{
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if (ImGui::Button("X", { 26.0f, 0.0f }))
				c.Texture = 0;
		}

		ImGuiHelpers::EndPropertyRow();

		// Tiling Factor property
		ImGuiHelpers::BeginPropertyRow("Tiling Factor");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat("##TilingFactor", &c.TilingFactor);
		ImGuiHelpers::EndPropertyRow();
	}
#endif

	void SpriteRendererComponentRenderer::BinarySerialize(BinarySerializer& serializer, SpriteRendererComponent& c)
	{
		serializer.Write(c.Color);
		serializer.Write(static_cast<uint64_t>(c.Texture));
		serializer.Write(c.TilingFactor);
	}

	void SpriteRendererComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, SpriteRendererComponent& c)
	{
		c.Color = deserializer.Read<glm::vec4>();
		c.Texture = static_cast<AssetHandle>(deserializer.Read<uint64_t>());
		c.TilingFactor = deserializer.Read<float>();
	}

}
