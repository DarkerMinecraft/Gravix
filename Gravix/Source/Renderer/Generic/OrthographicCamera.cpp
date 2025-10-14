#include "pch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Gravix
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_ViewMatrix(1.0f)
	{
		m_ViewProjMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::UpdateProjectionMatrix(uint32_t width, uint32_t height)
	{
		if (width != m_ProjWidth || height != m_ProjHeight)
		{
			GX_CORE_INFO("Updating orthographic camera projection matrix to width: {0}, height: {1}", width, height);

			m_ProjWidth = width;
			m_ProjHeight = height;

			// Use a normalized coordinate system (-1 to +1 range)
			float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
			m_ProjectionMatrix = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);

			m_ViewProjMatrix = m_ProjectionMatrix * m_ViewMatrix;
		}
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));
		m_ViewMatrix = glm::inverse(transform);

		m_ViewProjMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}