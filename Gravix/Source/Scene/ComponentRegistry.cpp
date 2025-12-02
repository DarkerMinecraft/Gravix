#include "pch.h"
#include "ComponentRegistry.h"

#include "Scene/ComponentRenderers/TagComponentRenderer.h"
#include "Scene/ComponentRenderers/TransformComponentRenderer.h"
#include "Scene/ComponentRenderers/CameraComponentRenderer.h"
#include "Scene/ComponentRenderers/SpriteRendererComponentRenderer.h"
#include "Scene/ComponentRenderers/ScriptComponentRenderer.h"
#include "Scene/ComponentRenderers/Rigidbody2DComponentRenderer.h"
#include "Scene/ComponentRenderers/BoxCollider2DComponentRenderer.h"
#include "Scene/ComponentRenderers/CircleRendererComponentRenderer.h"
#include "Scene/ComponentRenderers/CircleCollider2DComponentRenderer.h"
#include "Scene/ComponentRenderers/ComponentOrderComponentRenderer.h"

namespace Gravix
{

	void ComponentRegistry::RegisterAllComponents()
	{
		TagComponentRenderer::Register();
		TransformComponentRenderer::Register();
		CameraComponentRenderer::Register();
		SpriteRendererComponentRenderer::Register();
		ScriptComponentRenderer::Register();
		Rigidbody2DComponentRenderer::Register();
		BoxCollider2DComponentRenderer::Register();
		CircleRendererComponentRenderer::Register();
		CircleCollider2DComponentRenderer::Register();
		ComponentOrderComponentRenderer::Register();
	}

}
