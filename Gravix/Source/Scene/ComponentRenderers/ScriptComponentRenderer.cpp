#include "pch.h"
#include "ScriptComponentRenderer.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Scene/ImGuiHelpers.h"
#include "Scripting/Core/ScriptEngine.h"
#include "Scripting/Fields/ScriptFieldRegistry.h"
#include "Project/Project.h"
#include "Utils/StringUtils.h"
#include <imgui.h>
#include <algorithm>
#endif

namespace Gravix
{

	void ScriptComponentRenderer::Register()
	{
		ComponentRegistry::Get().RegisterComponent<ScriptComponent>(
			"Script",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true, .AllowMultiple = true },
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
	void ScriptComponentRenderer::Serialize(YAML::Emitter& out, ScriptComponent& c)
	{
		out << YAML::Key << "Name" << YAML::Value << c.Name;
	}

	void ScriptComponentRenderer::Deserialize(ScriptComponent& c, const YAML::Node& node)
	{
		c.Name = node["Name"].as<std::string>();
	}

	void ScriptComponentRenderer::OnImGuiRender(ScriptComponent& c, ComponentUserSettings* userSettings)
	{
		ImGuiHelpers::BeginPropertyRow("Class");

		// Get all available script classes
		auto& entityClasses = ScriptEngine::GetEntityClasses();

		// Create a vector of class names for the combo
		std::vector<std::string> classNames;
		classNames.push_back("None"); // First option is "None"
		for (const auto& [className, scriptClass] : entityClasses)
		{
			classNames.push_back(className);
		}

		// Find current selection index
		int currentIndex = 0;
		if (!c.Name.empty())
		{
			for (int i = 0; i < classNames.size(); i++)
			{
				if (classNames[i] == c.Name)
				{
					currentIndex = i;
					break;
				}
			}
		}

		// Use a static map to store search buffers per component instance
		static std::unordered_map<void*, std::string> searchBuffers;
		void* componentID = &c;

		// Show combo box
		if (ImGui::BeginCombo("##Class", currentIndex == 0 ? "None" : c.Name.c_str()))
		{
			// Add search input at the top of the combo
			std::string& searchText = searchBuffers[componentID];
			char searchBuffer[256] = "";
			strncpy_s(searchBuffer, searchText.c_str(), sizeof(searchBuffer) - 1);

			ImGui::SetNextItemWidth(-1);
			if (ImGui::InputTextWithHint("##ScriptSearch", "Search...", searchBuffer, sizeof(searchBuffer)))
			{
				searchText = searchBuffer;
			}

			// Set keyboard focus to search input when combo opens
			if (ImGui::IsWindowAppearing())
				ImGui::SetKeyboardFocusHere(-1);

			ImGui::Separator();

			// Convert search to lowercase for case-insensitive matching
			std::string searchLower = searchText;
			std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

			for (int i = 0; i < classNames.size(); i++)
			{
				// Filter by search text if not empty
				if (!searchText.empty())
				{
					std::string nameLower = classNames[i];
					std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

					if (nameLower.find(searchLower) == std::string::npos)
						continue; // Skip if doesn't match search
				}

				bool isSelected = (currentIndex == i);
				if (ImGui::Selectable(classNames[i].c_str(), isSelected))
				{
					if (i == 0)
						c.Name = ""; // None selected
					else
						c.Name = classNames[i];

					// Clear search after selection
					searchBuffers[componentID].clear();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGuiHelpers::EndPropertyRow();

		// Display and edit script fields if a valid script is selected
		if (!c.Name.empty() && ScriptEngine::IsEntityClassExists(c.Name) && userSettings && userSettings->CurrentEntity)
		{
			auto& scriptClass = entityClasses[c.Name];
			const auto& fields = scriptClass->GetFields();

			if (!fields.empty())
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				auto& registry = ScriptEngine::GetFieldRegistry();
				UUID entityID = userSettings->CurrentEntity->GetID();

				for (const auto& [fieldName, field] : fields)
				{
					// Convert camelCase to Title Case for better readability
					std::string displayName = StringUtils::CamelCaseToTitleCase(fieldName);
					ImGuiHelpers::BeginPropertyRow(displayName.c_str());

					// Get stored field value from registry
					ScriptFieldValue* storedValue = registry.GetFieldValue(entityID, c.Name, fieldName);
					ScriptFieldValue fieldValue;
					bool hasValue = storedValue != nullptr;

					if (hasValue)
					{
						fieldValue = *storedValue;
					}
					else
					{
						// Initialize with default value from C# field definition
						fieldValue = field.DefaultValue;
					}

					// Ensure Type is always set correctly
					fieldValue.Type = field.Type;

					// Display UI based on field type
					bool modified = false;
					switch (field.Type)
					{
					case ScriptFieldType::Float:
					{
						float value = fieldValue.GetValue<float>();
						if (field.HasRange)
						{
							if (ImGui::SliderFloat("##value", &value, field.RangeMin, field.RangeMax))
							{
								fieldValue.SetValue(value);
								modified = true;
							}
						}
						else
						{
							if (ImGui::DragFloat("##value", &value, 0.1f))
							{
								fieldValue.SetValue(value);
								modified = true;
							}
						}
						break;
					}
					case ScriptFieldType::Int:
					{
						int value = fieldValue.GetValue<int32_t>();
						if (field.HasRange)
						{
							if (ImGui::SliderInt("##value", &value, (int)field.RangeMin, (int)field.RangeMax))
							{
								fieldValue.SetValue(value);
								modified = true;
							}
						}
						else
						{
							if (ImGui::DragInt("##value", &value))
							{
								fieldValue.SetValue(value);
								modified = true;
							}
						}
						break;
					}
					case ScriptFieldType::Bool:
					{
						bool value = fieldValue.GetValue<bool>();
						if (ImGui::Checkbox("##value", &value))
						{
							fieldValue.SetValue(value);
							modified = true;
						}
						break;
					}
					case ScriptFieldType::Vector2:
					{
						glm::vec2 value = fieldValue.GetValue<glm::vec2>();
						if (ImGui::DragFloat2("##value", &value.x, 0.1f))
						{
							fieldValue.SetValue(value);
							modified = true;
						}
						break;
					}
					case ScriptFieldType::Vector3:
					{
						glm::vec3 value = fieldValue.GetValue<glm::vec3>();
						if (ImGui::DragFloat3("##value", &value.x, 0.1f))
						{
							fieldValue.SetValue(value);
							modified = true;
						}
						break;
					}
					case ScriptFieldType::Vector4:
					{
						glm::vec4 value = fieldValue.GetValue<glm::vec4>();
						if (ImGui::DragFloat4("##value", &value.x, 0.1f))
						{
							fieldValue.SetValue(value);
							modified = true;
						}
						break;
					}
					case ScriptFieldType::Entity:
					{
						UUID entityRefID = fieldValue.GetValue<UUID>();
						Entity referencedEntity = entityRefID != 0 ? userSettings->CurrentEntity->GetScene()->GetEntityByUUID(entityRefID) : Entity{};

						// Display entity name or "None"
						std::string entityName = referencedEntity ? referencedEntity.GetName() : "None";
						ImGui::Button(entityName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

						// Drag-drop target for entities from Scene Hierarchy
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
							{
								UUID droppedEntityID = *(UUID*)payload->Data;
								fieldValue.SetValue(droppedEntityID);
								modified = true;
							}
							ImGui::EndDragDropTarget();
						}

						// Right-click to clear
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
						{
							fieldValue.SetValue(UUID(0));
							modified = true;
						}

						// Tooltip showing entity ID
						if (referencedEntity && ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Entity: %s\nUUID: %llu", entityName.c_str(), (uint64_t)entityRefID);
						}

						break;
					}
					default:
						ImGui::TextDisabled("Unsupported type");
						break;
					}

					// Save to registry if modified
					if (modified)
					{
						registry.SetFieldValue(entityID, c.Name, fieldName, fieldValue);

						// Persist registry to disk
						std::filesystem::path registryPath = Project::GetActive()->GetConfig().LibraryDirectory / "ScriptsRegistry.orbreg";
						registry.Serialize(registryPath);

						// Also update the live script instance if it's running
						auto* instances = ScriptEngine::GetEntityScriptInstances(entityID);
						if (instances)
						{
							for (auto& instance : *instances)
							{
								if (instance->GetScriptClass()->GetFullClassName() == c.Name)
								{
									ScriptEngine::SetFieldValue(instance->GetMonoObject(), field, fieldValue);
									break;
								}
							}
						}

						if (userSettings)
							userSettings->WasModified = true;
					}

					ImGuiHelpers::EndPropertyRow();
				}
			}
		}
	}
#endif

	void ScriptComponentRenderer::BinarySerialize(BinarySerializer& serializer, ScriptComponent& c)
	{
		serializer.Write(c.Name);
	}

	void ScriptComponentRenderer::BinaryDeserialize(BinaryDeserializer& deserializer, ScriptComponent& c)
	{
		c.Name = deserializer.Read<std::string>();
	}

}
