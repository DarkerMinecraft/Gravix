#include "pch.h"
#include "ComponentOrderComponentRenderer.h"

namespace Gravix
{

	void ComponentOrderComponentRenderer::Register()
	{
		// ComponentOrderComponent - Hidden component for tracking component addition order
		ComponentRegistry::Get().RegisterComponent<ComponentOrderComponent>(
			"ComponentOrder",
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
#ifdef GRAVIX_EDITOR_BUILD
			Serialize,
			Deserialize,
			nullptr,  // No UI rendering for this hidden component
#endif
			BinarySerialize,
			BinaryDeserialize
		);
	}

#ifdef GRAVIX_EDITOR_BUILD
	void ComponentOrderComponentRenderer::Serialize(YAML::Emitter& out, ComponentOrderComponent& c)
	{
		out << YAML::Key << "Order" << YAML::Value << YAML::BeginSeq;
		for (const auto& typeIdx : c.ComponentOrder)
		{
			// Find the component name by type_index
			const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
			auto it = allComponents.find(typeIdx);
			if (it != allComponents.end())
			{
				out << it->second.Name;
			}
		}
		out << YAML::EndSeq;
	}

	void ComponentOrderComponentRenderer::Deserialize(ComponentOrderComponent& c, const YAML::Node& node)
	{
		c.ComponentOrder.clear();
		if (node["Order"])
		{
			for (const auto& item : node["Order"])
			{
				std::string componentName = item.as<std::string>();
				// Find the type_index by component name
				const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
				for (const auto& [typeIdx, info] : allComponents)
				{
					if (info.Name == componentName)
					{
						c.ComponentOrder.push_back(typeIdx);
						break;
					}
				}
			}
		}
	}
#endif

	void ComponentOrderComponentRenderer::BinarySerialize(BinarySerializer& serializer, ComponentOrderComponent& c)
	{
		// Write the number of component types in the order vector
		serializer.Write(static_cast<uint32_t>(c.ComponentOrder.size()));

		// Write each component name (we serialize names, not type_index, as type_index may differ across platforms)
		for (const auto& typeIdx : c.ComponentOrder)
		{
			const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
			auto it = allComponents.find(typeIdx);
			if (it != allComponents.end())
			{
				serializer.Write(it->second.Name);
			}
			else
			{
				serializer.Write(std::string("")); // Write empty string if component not found
			}
		}
	}

	void ComponentOrderComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, ComponentOrderComponent& c)
	{
		c.ComponentOrder.clear();

		// Read the number of component types
		uint32_t count = deserializer.Read<uint32_t>();

		// Read each component name and find its type_index
		for (uint32_t i = 0; i < count; i++)
		{
			std::string componentName = deserializer.Read<std::string>();

			if (!componentName.empty())
			{
				// Find the type_index by component name
				const auto& allComponents = ComponentRegistry::Get().GetAllComponents();
				for (const auto& [typeIdx, info] : allComponents)
				{
					if (info.Name == componentName)
					{
						c.ComponentOrder.push_back(typeIdx);
						break;
					}
				}
			}
		}
	}

}
