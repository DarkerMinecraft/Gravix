#pragma once

#include "Renderer/Generic/Camera.h"

namespace Gravix 
{

	class SceneCamera : public Camera 
	{
	public:
		SceneCamera() = default;
		virtual ~SceneCamera() = default;
		
		void SetOrthographic(float size, float nearClip, float farClip);
		void SetViewportSize(uint32_t width, uint32_t height);

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float orthographicSize) { m_OrthographicSize = orthographicSize; RecalculateProjection(); }
	private:
		void RecalculateProjection();
	private:
		float m_OrthographicSize = 10.0f, m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

		float m_AspectRatio;
	};

}