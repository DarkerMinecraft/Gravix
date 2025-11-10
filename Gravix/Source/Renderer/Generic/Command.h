#pragma once

#include "Types/Framebuffer.h"
#include "Renderer/CommandImpl.h"

namespace Gravix
{

	/**
	 * @brief Graphics command buffer for recording rendering operations
	 *
	 * The Command class provides a high-level interface for recording GPU rendering
	 * commands. It abstracts the underlying graphics API (Vulkan) and provides a
	 * clean, type-safe interface for:
	 * - Material and shader binding
	 * - Resource binding (textures, framebuffers)
	 * - Mesh rendering (indexed/non-indexed, instanced)
	 * - Render pass management
	 * - ImGui rendering
	 *
	 * Commands are recorded and then submitted to the GPU for execution.
	 * The class follows the Vulkan model of explicit command recording with
	 * BeginRendering/EndRendering pairs.
	 *
	 * **Rendering Pipeline:**
	 * 1. Create Command with target framebuffer
	 * 2. BeginRendering() - Start render pass
	 * 3. SetActiveMaterial() - Choose shader/pipeline
	 * 4. BindResource() - Bind textures/uniforms
	 * 5. BindMaterial() - Apply material with push constants
	 * 6. BindMesh() - Set vertex/index buffers
	 * 7. DrawIndexed() or Draw() - Issue draw call
	 * 8. EndRendering() - Finish render pass
	 *
	 * @par Usage Example:
	 * @code
	 * // Create command buffer for main framebuffer
	 * Command cmd(mainFramebuffer);
	 *
	 * // Begin rendering
	 * cmd.BeginRendering();
	 *
	 * // Draw a sprite
	 * cmd.SetActiveMaterial(spriteMaterial);
	 * cmd.BindResource(0, spriteTexture);
	 * cmd.BindMaterial();
	 * cmd.BindMesh(quadMesh);
	 * cmd.DrawIndexed(6); // 6 indices for 2 triangles (quad)
	 *
	 * // Finish rendering
	 * cmd.EndRendering();
	 * @endcode
	 *
	 * @see Material, Mesh, Framebuffer
	 */
	class Command
	{
	public:
		/**
		 * @brief Construct a command buffer
		 * @param framebuffer Target framebuffer (nullptr for swapchain)
		 * @param presentIndex Swapchain image index (if rendering to screen)
		 * @param shouldCopy Whether to copy previous frame contents
		 *
		 * Creates a command buffer for recording rendering operations.
		 * If framebuffer is nullptr, renders to the swapchain (screen).
		 */
		Command(Ref<Framebuffer> framebuffer = nullptr, uint32_t presentIndex = 0, bool shouldCopy = true);

		/**
		 * @brief Destructor
		 */
		virtual ~Command();

		/**
		 * @brief Set the active material for subsequent draw calls
		 * @param material Material containing shader and pipeline state
		 *
		 * Binds the material's shader program and sets pipeline state
		 * (blend mode, cull mode, topology, etc.).
		 */
		void SetActiveMaterial(Material* material);

		/**
		 * @brief Set the active material (Ref version)
		 * @param material Material reference
		 */
		void SetActiveMaterial(Ref<Material> material) { SetActiveMaterial(material.get()); }

		/**
		 * @brief Bind a framebuffer attachment as a shader resource
		 * @param binding Shader binding point (descriptor set slot)
		 * @param buffer Framebuffer containing the texture
		 * @param index Attachment index within the framebuffer
		 * @param sampler True to bind as sampled texture, false for storage
		 */
		void BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler = false);

		/**
		 * @brief Bind a framebuffer attachment (Ref version)
		 */
		void BindResource(uint32_t binding, Ref<Framebuffer> buffer, uint32_t index, bool sampler = false) { BindResource(binding, buffer.get(), index, sampler); }

		/**
		 * @brief Bind a texture to a shader binding point
		 * @param binding Shader binding point (descriptor set slot)
		 * @param index Array index if binding to texture array
		 * @param texture 2D texture to bind
		 */
		void BindResource(uint32_t binding, uint32_t index, Texture2D* texture);

		/**
		 * @brief Bind a texture (Ref version with array index)
		 */
		void BindResource(uint32_t binding, uint32_t index, Ref<Texture2D> texture) { BindResource(binding, index, texture.get()); }

		/**
		 * @brief Bind a texture to binding point 0
		 * @param binding Shader binding point
		 * @param texture Texture to bind
		 */
		void BindResource(uint32_t binding, Texture2D* texture) { BindResource(binding, 0, texture); }

		/**
		 * @brief Bind a texture to binding point 0 (Ref version)
		 */
		void BindResource(uint32_t binding, Ref<Texture2D> texture) { BindResource(binding, 0, texture.get()); }

		/**
		 * @brief Bind the active material and optionally set push constants
		 * @param pushConstants Pointer to push constant data (or nullptr)
		 *
		 * Finalizes material binding and uploads push constants if provided.
		 * Must be called after SetActiveMaterial() and BindResource() calls.
		 */
		void BindMaterial(void* pushConstants = nullptr);

		/**
		 * @brief Dispatch a compute shader
		 *
		 * Issues a compute shader dispatch with the active material.
		 * Material must be a compute shader for this to work.
		 */
		void Dispatch();

		/**
		 * @brief Begin recording rendering commands
		 *
		 * Starts a render pass with the command buffer's target framebuffer.
		 * All draw calls must occur between BeginRendering() and EndRendering().
		 */
		void BeginRendering();

		/**
		 * @brief Bind vertex and index buffers from a mesh
		 * @param mesh Mesh containing geometry data
		 *
		 * Binds the mesh's vertex buffer(s) and index buffer for subsequent
		 * Draw/DrawIndexed calls.
		 */
		void BindMesh(Mesh* mesh);

		/**
		 * @brief Bind a mesh (Ref version)
		 */
		void BindMesh(Ref<Mesh> mesh) { BindMesh(mesh.get()); }

		/**
		 * @brief Draw non-indexed geometry
		 * @param vertexCount Number of vertices to draw
		 * @param instanceCount Number of instances (for instanced rendering)
		 * @param firstVertex Starting vertex index
		 * @param firstInstance Starting instance ID
		 *
		 * Issues a draw call without using the index buffer.
		 */
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

		/**
		 * @brief Draw indexed geometry
		 * @param indexCount Number of indices to draw
		 * @param instanceCount Number of instances (for instanced rendering)
		 * @param firstIndex Starting index in the index buffer
		 * @param vertexOffset Offset added to each index
		 * @param firstInstance Starting instance ID
		 *
		 * Issues a draw call using the bound index buffer.
		 * Most common for rendering meshes.
		 */
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

		/**
		 * @brief Draw ImGui UI elements
		 *
		 * Renders all ImGui draw data accumulated during the frame.
		 * Should be called after EndRendering() for the main scene.
		 */
		void DrawImGui();

		/**
		 * @brief End recording rendering commands
		 *
		 * Finishes the current render pass. Must match a BeginRendering() call.
		 */
		void EndRendering();

		/**
		 * @brief Resolve/copy framebuffer contents to another framebuffer
		 * @param dst Destination framebuffer
		 * @param shaderUse True if result will be sampled in shaders
		 *
		 * Performs a blit or resolve operation (for MSAA).
		 * Handles layout transitions automatically.
		 */
		void ResolveFramebuffer(Framebuffer* dst, bool shaderUse);

		/**
		 * @brief Resolve framebuffer (Ref version)
		 */
		void ResolveFramebuffer(Ref<Framebuffer> dst, bool shaderUse) { ResolveFramebuffer(dst.get(), shaderUse); }

	private:
		CommandImpl* m_Impl = nullptr; ///< Platform-specific implementation (Vulkan)

	private:
		/**
		 * @brief Initialize the command buffer
		 */
		void Initialize(Ref<Framebuffer> framebuffer, uint32_t presentIndex, bool shouldCopy);
	};
}