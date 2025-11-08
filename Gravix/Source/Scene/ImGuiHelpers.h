#pragma once

#include <glm/glm.hpp>

namespace Gravix
{

	// ImGui helper class for rendering component properties with a consistent layout
	class ImGuiHelpers
	{
	public:
		// Begin a property row with label on left and control on right
		static void BeginPropertyRow(const char* label, float columnWidth = 120.0f);

		// End the property row
		static void EndPropertyRow();

		// Draw a Vec3 control with X/Y/Z buttons and drag floats
		static void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 120.0f);
	};

}
