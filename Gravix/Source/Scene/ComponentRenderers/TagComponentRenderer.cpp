#include "pch.h"
#include "TagComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include <imgui.h>
#endif

namespace Gravix
{

	void TagComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<TagComponent>(
			"Tag",
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
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
	void TagComponentRenderer::Serialize(YAML::Emitter& out, TagComponent& c)
	{
		out << YAML::Key << "Name" << YAML::Value << c.Name;
		out << YAML::Key << "CreationIndex" << YAML::Value << c.CreationIndex;
	}

	void TagComponentRenderer::Deserialize(TagComponent& c, const YAML::Node& node)
	{
		if (node["CreationIndex"])
			c.CreationIndex = node["CreationIndex"].as<uint32_t>();
	}

	void TagComponentRenderer::OnImGuiRender(TagComponent& c, ComponentUserSettings*)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Bold "Tag" label with input on the same line
		ImGui::PushFont(io.Fonts->Fonts[1]);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Tag");
		ImGui::PopFont();
		ImGui::SameLine();

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strcpy_s(buffer, c.Name.c_str());
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputText("##TagComponentName", buffer, sizeof(buffer)))
		{
			c.Name = std::string(buffer);
		}
	}
#endif

	void TagComponentRenderer::BinarySerialize(BinarySerializer& serializer, TagComponent& c)
	{
		serializer.Write(c.Name);
		serializer.Write(c.CreationIndex);
	}

	void TagComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, TagComponent& c)
	{
		c.Name = deserializer.Read<std::string>();
		c.CreationIndex = deserializer.Read<uint32_t>();
	}

}
