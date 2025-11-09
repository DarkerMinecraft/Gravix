#pragma once

#include "Renderer/Generic/Camera.h"

namespace Gravix 
{

	enum class ProjectionType
	{
		Perspective = 0,
		Orthographic = 1
	};

	class SceneCamera : public Camera
	{
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;
		
		void SetOrthographic(float size, float nearClip, float farClip);
		void SetPerspective(float verticalFOV, float nearClip, float farClip);
		void SetViewportSize(uint32_t width, uint32_t height);

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float orthographicSize) { m_OrthographicSize = orthographicSize; RecalculateProjection(); }
		float GetOrthographicNearClip() const { return m_OrthographicNear; }
		void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
		float GetOrthographicFarClip() const { return m_OrthographicFar; }
		void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

		float GetPerspectiveFOV() const { return m_PerspectiveFOV; }
		void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }
		float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
		void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
		float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
		void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }
	private:
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		float m_OrthographicSize = 10.0f, m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

		float m_PerspectiveFOV = 45.0f;
		float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

		float m_AspectRatio = 1.778f; // Default to 16:9 aspect ratio
	};

}