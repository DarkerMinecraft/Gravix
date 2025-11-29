#include "pch.h"
#include "ComponentRegistry.h"
#include "ImGuiHelpers.h"

#include "Components.h"

#include "Asset/AssetManager.h"
#include "Project/Project.h"
#include "Serialization/YAMLConverters.h"

#include "Scripting/ScriptEngine.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Gravix
{

	void ComponentRegistry::RegisterAllComponents()
	{
		RegisterComponent<TagComponent>(
			"Tag",
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, TagComponent& c)
			{
				out << YAML::Key << "Name" << YAML::Value << c.Name;
				out << YAML::Key << "CreationIndex" << YAML::Value << c.CreationIndex;
			},
			[](TagComponent& c, const YAML::Node& node)
			{
				if (node["CreationIndex"])
					c.CreationIndex = node["CreationIndex"].as<uint32_t>();
			},
			[](TagComponent& c)
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
		);

		RegisterComponent<TransformComponent>(
			"Transform",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, TransformComponent& c) 
			{
				out << YAML::Key << "Position" << YAML::Value << c.Position;
				out << YAML::Key << "Rotation" << YAML::Value << c.Rotation;
				out << YAML::Key << "Scale" << YAML::Value << c.Scale;
			},
			[](TransformComponent& c, const YAML::Node& node) 
			{
				c.Position = node["Position"].as<glm::vec3>();
				c.Rotation = node["Rotation"].as<glm::vec3>();
				c.Scale = node["Scale"].as<glm::vec3>();

				c.CalculateTransform();
			},
			[](TransformComponent& c)
			{
				// Draw the Transform component UI
				ImGuiHelpers::DrawVec3Control("Position", c.Position);
				ImGuiHelpers::DrawVec3Control("Rotation", c.Rotation);
				ImGuiHelpers::DrawVec3Control("Scale", c.Scale, 1.0f);
				// Update the transform matrix when values change
				c.CalculateTransform();
			}
		);

		RegisterComponent<ScriptComponent>(
			"Script",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true, .AllowMultiple = true },
			nullptr,
			[](YAML::Emitter& out, ScriptComponent& c)
			{
				out << YAML::Key << "Name" << YAML::Value << c.Name;
			},
			[](ScriptComponent& c, const YAML::Node& node)
			{
				c.Name = node["Name"].as<std::string>();
			},
			[](ScriptComponent& c)
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
			}
		);

		RegisterComponent<CameraComponent>(
			"Camera",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			[](CameraComponent& c, Scene* scene)
			{
				c.Camera.SetViewportSize(scene->GetViewportWidth(), scene->GetViewportHeight());
			},
			[](YAML::Emitter& out, CameraComponent& c)
			{
				auto& camera = c.Camera;
				out << YAML::Key << "Camera" << YAML::BeginMap;
				out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
				out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveFOV();
				out << YAML::Key << "PerspectiveNearClip" << YAML::Value << camera.GetPerspectiveNearClip();
				out << YAML::Key << "PerspectiveFarClip" << YAML::Value << camera.GetPerspectiveFarClip();
				out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
				out << YAML::Key << "OrthographicNearClip" << YAML::Value << camera.GetOrthographicNearClip();
				out << YAML::Key << "OrthographicFarClip" << YAML::Value << camera.GetOrthographicFarClip();
				out << YAML::EndMap;

				out << YAML::Key << "Primary" << YAML::Value << c.Primary;
				out << YAML::Key << "FixedAspectRatio" << YAML::Value << c.FixedAspectRatio;
			},
			[](CameraComponent& c, const YAML::Node& node)
			{
				auto& camera = c.Camera;
				const YAML::Node& cameraNode = node["Camera"];
				camera.SetProjectionType((ProjectionType)cameraNode["ProjectionType"].as<int>());
				camera.SetPerspectiveFOV(cameraNode["PerspectiveFOV"].as<float>());
				camera.SetPerspectiveNearClip(cameraNode["PerspectiveNearClip"].as<float>());
				camera.SetPerspectiveFarClip(cameraNode["PerspectiveFarClip"].as<float>());
				camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
				camera.SetOrthographicNearClip(cameraNode["OrthographicNearClip"].as<float>());
				camera.SetOrthographicFarClip(cameraNode["OrthographicFarClip"].as<float>());
				c.Primary = node["Primary"].as<bool>();
				c.FixedAspectRatio = node["FixedAspectRatio"].as<bool>();
			},
			[](CameraComponent& c)
			{
				const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
				const char* currentProjectionTypeString = projectionTypeStrings[(int)c.Camera.GetProjectionType()];

				auto& camera = c.Camera;

				// Primary checkbox
				ImGuiHelpers::BeginPropertyRow("Primary");
				ImGui::Checkbox("##Primary", &c.Primary);
				ImGuiHelpers::EndPropertyRow();

				// Projection type combo
				ImGuiHelpers::BeginPropertyRow("Projection");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##Projection", currentProjectionTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
						if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
						{
							currentProjectionTypeString = projectionTypeStrings[i];
							camera.SetProjectionType((ProjectionType)i);
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				ImGuiHelpers::EndPropertyRow();

				if (camera.GetProjectionType() == ProjectionType::Orthographic)
				{
					ImGuiHelpers::BeginPropertyRow("Size");
					float orthoSize = camera.GetOrthographicSize();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##Size", &orthoSize))
						camera.SetOrthographicSize(orthoSize);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Near Clip");
					float nearClip = camera.GetOrthographicNearClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##NearClip", &nearClip))
						camera.SetOrthographicNearClip(nearClip);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Far Clip");
					float farClip = camera.GetOrthographicFarClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##FarClip", &farClip))
						camera.SetOrthographicFarClip(farClip);
					ImGuiHelpers::EndPropertyRow();
				}

				if (camera.GetProjectionType() == ProjectionType::Perspective)
				{
					ImGuiHelpers::BeginPropertyRow("Vertical FOV");
					float fov = camera.GetPerspectiveFOV();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##VerticalFOV", &fov))
						camera.SetPerspectiveFOV(fov);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Near Clip");
					float nearClip = camera.GetPerspectiveNearClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##NearClip", &nearClip))
						camera.SetPerspectiveNearClip(nearClip);
					ImGuiHelpers::EndPropertyRow();

					ImGuiHelpers::BeginPropertyRow("Far Clip");
					float farClip = camera.GetPerspectiveFarClip();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat("##FarClip", &farClip))
						camera.SetPerspectiveFarClip(farClip);
					ImGuiHelpers::EndPropertyRow();
				}

				ImGuiHelpers::BeginPropertyRow("Fixed Aspect");
				ImGui::Checkbox("##FixedAspectRatio", &c.FixedAspectRatio);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		RegisterComponent<SpriteRendererComponent>(
			"Sprite Renderer",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, SpriteRendererComponent& c)
			{
				out << YAML::Key << "Color" << YAML::Value << c.Color;
				out << YAML::Key << "Texture" << YAML::Value << (uint64_t)c.Texture;
				out << YAML::Key << "TilingFactor" << YAML::Value << c.TilingFactor;
			},
			[](SpriteRendererComponent& c, const YAML::Node& node)
			{
				c.Color = node["Color"].as<glm::vec4>();
				c.Texture = (AssetHandle)node["Texture"].as<uint64_t>();
				c.TilingFactor = node["TilingFactor"].as<float>();
			},
			[](SpriteRendererComponent& c)
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
		);

		RegisterComponent<Rigidbody2DComponent>(
			"Rigidbody2D",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, Rigidbody2DComponent& c)
			{
				out << YAML::Key << "BodyType" << YAML::Value << (int)c.Type;
				out << YAML::Key << "FixedRotation" << YAML::Value << c.FixedRotation;
			},
			[](Rigidbody2DComponent& c, const YAML::Node& node)
			{
				c.Type = (Rigidbody2DComponent::BodyType)node["BodyType"].as<int>();
				c.FixedRotation = node["FixedRotation"].as<bool>();
			},
			[](Rigidbody2DComponent& c)
			{
				const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
				const char* currentBodyTypeString = bodyTypeStrings[(int)c.Type];

				// Body Type combo
				ImGuiHelpers::BeginPropertyRow("Body Type");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##BodyType", currentBodyTypeString))
				{
					for (int i = 0; i < 3; i++)
					{
						bool isSelected = (int)c.Type == i;
						if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
						{
							c.Type = (Rigidbody2DComponent::BodyType)i;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				ImGuiHelpers::EndPropertyRow();

				// Fixed Rotation checkbox
				ImGuiHelpers::BeginPropertyRow("Fixed Rotation");
				ImGui::Checkbox("##FixedRotation", &c.FixedRotation);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		RegisterComponent<BoxCollider2DComponent>(
			"BoxCollider2D",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, BoxCollider2DComponent& c)
			{
				out << YAML::Key << "Offset" << YAML::Value << c.Offset;
				out << YAML::Key << "Size" << YAML::Value << c.Size;
				out << YAML::Key << "Density" << YAML::Value << c.Density;
				out << YAML::Key << "Friction" << YAML::Value << c.Friction;
				out << YAML::Key << "Restitution" << YAML::Value << c.Restitution;
			},
			[](BoxCollider2DComponent& c, const YAML::Node& node)
			{
				c.Offset = node["Offset"].as<glm::vec2>();
				c.Size = node["Size"].as<glm::vec2>();
				c.Density = node["Density"].as<float>();
				c.Friction = node["Friction"].as<float>();
				c.Restitution = node["Restitution"].as<float>();
			},
			[](BoxCollider2DComponent& c)
			{
				// Offset property
				ImGuiHelpers::BeginPropertyRow("Offset");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat2("##Offset", glm::value_ptr(c.Offset), 0.01f);
				ImGuiHelpers::EndPropertyRow();

				// Size property
				ImGuiHelpers::BeginPropertyRow("Size");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat2("##Size", glm::value_ptr(c.Size), 0.01f);
				ImGuiHelpers::EndPropertyRow();

				// Density property
				ImGuiHelpers::BeginPropertyRow("Density");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Density", &c.Density, 0.01f, 0.0f, 100.0f);
				ImGuiHelpers::EndPropertyRow();

				// Friction property
				ImGuiHelpers::BeginPropertyRow("Friction");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Friction", &c.Friction, 0.01f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();

				// Restitution property
				ImGuiHelpers::BeginPropertyRow("Restitution");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Restitution", &c.Restitution, 0.01f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		RegisterComponent<CircleRendererComponent>(
			"Circle Renderer",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, CircleRendererComponent& c)
			{
				out << YAML::Key << "Color" << YAML::Value << c.Color;
				out << YAML::Key << "Thickness" << YAML::Value << c.Thickness;
				out << YAML::Key << "Fade" << YAML::Value << c.Fade;
			},
			[](CircleRendererComponent& c, const YAML::Node& node)
			{
				c.Color = node["Color"].as<glm::vec4>();
				c.Thickness = node["Thickness"].as<float>();
				c.Fade = node["Fade"].as<float>();
			},
			[](CircleRendererComponent& c)
			{
				// Color property
				ImGuiHelpers::BeginPropertyRow("Color");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::ColorEdit4("##Color", glm::value_ptr(c.Color));
				ImGuiHelpers::EndPropertyRow();

				// Thickness property
				ImGuiHelpers::BeginPropertyRow("Thickness");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Thickness", &c.Thickness, 0.01f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();

				// Fade property
				ImGuiHelpers::BeginPropertyRow("Fade");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Fade", &c.Fade, 0.001f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		RegisterComponent<CircleCollider2DComponent>(
			"CircleCollider2D",
			ComponentSpecification{ .HasNodeTree = true, .CanRemoveComponent = true },
			nullptr,
			[](YAML::Emitter& out, CircleCollider2DComponent& c)
			{
				out << YAML::Key << "Offset" << YAML::Value << c.Offset;
				out << YAML::Key << "Size" << YAML::Value << c.Size;
				out << YAML::Key << "Density" << YAML::Value << c.Density;
				out << YAML::Key << "Friction" << YAML::Value << c.Friction;
				out << YAML::Key << "Restitution" << YAML::Value << c.Restitution;
			},
			[](CircleCollider2DComponent& c, const YAML::Node& node)
			{
				c.Offset = node["Offset"].as<glm::vec2>();
				c.Size = node["Size"].as<glm::vec2>();
				c.Density = node["Density"].as<float>();
				c.Friction = node["Friction"].as<float>();
				c.Restitution = node["Restitution"].as<float>();
			},
			[](CircleCollider2DComponent& c)
			{
				// Offset property
				ImGuiHelpers::BeginPropertyRow("Offset");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat2("##Offset", glm::value_ptr(c.Offset), 0.01f);
				ImGuiHelpers::EndPropertyRow();

				// Size property
				ImGuiHelpers::BeginPropertyRow("Size");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat2("##Size", glm::value_ptr(c.Size), 0.01f);
				ImGuiHelpers::EndPropertyRow();

				// Density property
				ImGuiHelpers::BeginPropertyRow("Density");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Density", &c.Density, 0.01f, 0.0f, 100.0f);
				ImGuiHelpers::EndPropertyRow();

				// Friction property
				ImGuiHelpers::BeginPropertyRow("Friction");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Friction", &c.Friction, 0.01f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();

				// Restitution property
				ImGuiHelpers::BeginPropertyRow("Restitution");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::DragFloat("##Restitution", &c.Restitution, 0.01f, 0.0f, 1.0f);
				ImGuiHelpers::EndPropertyRow();
			}
		);

		// ComponentOrderComponent - Hidden component for tracking component addition order
		RegisterComponent<ComponentOrderComponent>(
			"ComponentOrder",
			ComponentSpecification{ .HasNodeTree = false, .CanRemoveComponent = false },
			nullptr,
			[](YAML::Emitter& out, ComponentOrderComponent& c)
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
			},
			[](ComponentOrderComponent& c, const YAML::Node& node)
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
			},
			nullptr  // No UI rendering for this hidden component
		);
	}

}