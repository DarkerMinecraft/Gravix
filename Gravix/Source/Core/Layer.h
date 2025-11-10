#pragma once

#include "RefCounted.h"
#include "Events/Event.h"

namespace Gravix
{

	/**
	 * @brief Abstract base class for application layers
	 *
	 * Layers provide a modular way to organize application logic. Each layer
	 * receives update, render, and event callbacks from the application.
	 *
	 * Layers are stacked and processed in order during each frame:
	 * 1. Event processing (top to bottom, can be handled/blocked)
	 * 2. Update (bottom to top)
	 * 3. Render (bottom to top)
	 * 4. ImGui render (bottom to top)
	 *
	 * Common layer types:
	 * - Game/simulation layer (gameplay logic)
	 * - Editor layer (editor tools and UI)
	 * - Debug overlay layer (profiling, stats)
	 *
	 * @par Usage Example:
	 * @code
	 * class GameLayer : public Layer
	 * {
	 * public:
	 *     void OnUpdate(float deltaTime) override
	 *     {
	 *         // Update game logic
	 *         m_Player.Update(deltaTime);
	 *     }
	 *
	 *     void OnRender() override
	 *     {
	 *         // Render game objects
	 *         Renderer2D::BeginScene();
	 *         Renderer2D::DrawSprite(m_Player.GetTransform(), m_Player.GetTexture());
	 *         Renderer2D::EndScene();
	 *     }
	 *
	 *     void OnEvent(Event& e) override
	 *     {
	 *         // Handle input events
	 *         if (e.GetEventType() == EventType::KeyPressed)
	 *         {
	 *             auto& keyEvent = static_cast<KeyPressedEvent&>(e);
	 *             m_Player.HandleInput(keyEvent.GetKeyCode());
	 *         }
	 *     }
	 * };
	 *
	 * // Add to application
	 * app.PushLayer<GameLayer>();
	 * @endcode
	 */
	class Layer : public RefCounted
	{
	public:
		/**
		 * @brief Default constructor
		 */
		Layer() = default;

		/**
		 * @brief Virtual destructor
		 */
		virtual ~Layer() = default;

		/**
		 * @brief Handle events dispatched to this layer
		 * @param event Event to process (keyboard, mouse, window, etc.)
		 *
		 * Events propagate through layers from top to bottom. Mark the event
		 * as handled to prevent it from reaching lower layers.
		 */
		virtual void OnEvent(Event& event) {}

		/**
		 * @brief Update layer logic with delta time
		 * @param deltaTime Time elapsed since last frame in seconds
		 *
		 * Called once per frame for game logic, physics, animations, etc.
		 * All layers are updated before any rendering occurs.
		 */
		virtual void OnUpdate(float deltaTime) {}

		/**
		 * @brief Render layer content
		 *
		 * Called once per frame after all layers have been updated.
		 * Use this for 2D/3D rendering, not for UI (use OnImGuiRender instead).
		 */
		virtual void OnRender() {}

		/**
		 * @brief Render ImGui UI elements for this layer
		 *
		 * Called once per frame after OnRender. Use ImGui commands to build
		 * UI elements (windows, panels, widgets, etc.).
		 *
		 * @note ImGui rendering is automatically batched and submitted
		 */
		virtual void OnImGuiRender() {}
	};

}