#include "pch.h"
#include "ShaderCompiler.h"

#include "Utils/ShaderReflector.h"
#include "Utils/SlangTypeUtils.h"

#include <cstring>

using namespace slang;
namespace Gravix
{

	ShaderCompiler::ShaderCompiler()
	{
		createGlobalSession(m_GlobalSession.writeRef());
	}

	bool ShaderCompiler::CompileShader(const std::filesystem::path& filePath, std::vector<std::vector<uint32_t>>* spirvCodes, ShaderReflection* reflection)
	{
		GX_CORE_INFO("Compiling shader: {0}", filePath.string());

		SessionDesc sessionDesc{};
		TargetDesc targetDesc{};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = m_GlobalSession->findProfile("spirv_1_5");

		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		std::vector<CompilerOptionEntry> options;
		options.push_back({
			CompilerOptionName::VulkanUseEntryPointName,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		options.push_back({
			CompilerOptionName::Optimization,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		options.push_back({
			CompilerOptionName::GLSLForceScalarLayout,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		Slang::ComPtr<ISession> session;
		m_GlobalSession->createSession(sessionDesc, session.writeRef());

		Slang::ComPtr<IModule> slangModule;
		{
			Slang::ComPtr<IBlob> diagnosticBlob;
			slangModule = session->loadModule(filePath.string().c_str(), diagnosticBlob.writeRef());
			if (diagnosticBlob && diagnosticBlob->getBufferSize() > 0)
			{
				std::string message = (char*)diagnosticBlob->getBufferPointer();
				GX_CORE_CRITICAL("Failed to load shader: {0}", message);
				return false;
			}
		}

		if (!slangModule) return false;

		// --- Explicitly Collect Entry Points for Linking ---
		std::vector<IComponentType*> componentTypes;
		componentTypes.push_back(slangModule); // Start with the module itself

		uint32_t definedEntryPointCount = slangModule->getDefinedEntryPointCount();
		Slang::ComPtr<IEntryPoint> entryPoints[32]; // Max 32 entry points, adjust if needed
		for (uint32_t i = 0; i < definedEntryPointCount; i++)
		{
			slangModule->getDefinedEntryPoint(i, entryPoints[i].writeRef());
			componentTypes.push_back(entryPoints[i]);
		}

		// --- Create a Composite Program and Link It ---
		Slang::ComPtr<slang::IComponentType> composedProgram;
		{
			Slang::ComPtr<IBlob> diagnosticsBlob;
			SlangResult result = session->createCompositeComponentType(
				componentTypes.data(),
				componentTypes.size(),
				composedProgram.writeRef(),
				diagnosticsBlob.writeRef());

			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				// Handle diagnostics...
				if (SLANG_FAILED(result)) return false;
			}
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		{
			Slang::ComPtr<IBlob> diagnosticsBlob;
			SlangResult result = composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());

			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				// Handle diagnostics...
				if (SLANG_FAILED(result)) return false;
			}
		}

		if (!linkedProgram) return false;

		// --- Extract Reflection Data using ShaderReflector ---
		slang::ProgramLayout* layout = linkedProgram->getLayout();
		ShaderReflector::ExtractPushConstants(layout, reflection);
		ShaderReflector::ExtractStructs(layout, reflection);
		ShaderReflector::ExtractStructsFromPointers(layout, reflection);

		// --- Loop through the linked program's entry points ---
		uint32_t entryPointCount = layout->getEntryPointCount();

		for (uint32_t i = 0; i < entryPointCount; i++)
		{
			// Get the reflection for the entry point at index 'i'.
			slang::EntryPointReflection* entryPointReflection = layout->getEntryPointByIndex(i);
			if (!entryPointReflection) continue;

			ShaderStage shaderStage = SlangTypeUtils::SlangStageToShaderStage(entryPointReflection->getStage());
			GX_CORE_INFO("Processing Entry Point: {0} ({1})", entryPointReflection->getName(), SlangTypeUtils::ShaderStageToString(shaderStage));

			Slang::ComPtr<slang::IBlob> spirvCode;
			{
				Slang::ComPtr<slang::IBlob> diagnosticsBlob;
				// Get the code for the entry point at index 'i'.
				SlangResult result = linkedProgram->getEntryPointCode(
					i, // Use the loop index 'i' here
					0,
					spirvCode.writeRef(),
					diagnosticsBlob.writeRef());

				if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
				{
					std::string message = (char*)diagnosticsBlob->getBufferPointer();
					GX_CORE_CRITICAL("Failed to get entry point code: {0}", message);
					if (SLANG_FAILED(result)) continue;
				}
			}

			if (!spirvCode || spirvCode->getBufferSize() == 0) continue;

			std::vector<uint32_t> spirv(spirvCode->getBufferSize() / sizeof(uint32_t));
			std::memcpy(spirv.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

			if (spirv.empty() || spirv[0] != 0x07230203) // SPIR-V Magic Number
			{
				GX_CORE_ERROR("Invalid SPIR-V magic number for entry point {0}", entryPointReflection->getName());
				continue;
			}

			spirvCodes->push_back(spirv);
			reflection->SetShaderName(filePath.stem().string());
			reflection->AddEntryPoint({ std::string(entryPointReflection->getName()), shaderStage });
			if (shaderStage == ShaderStage::Compute)
				ShaderReflector::ExtractComputeDispatchInfo(entryPointReflection, reflection);
			else if (shaderStage == ShaderStage::Vertex)
				ShaderReflector::ExtractVertexAttributes(entryPointReflection, reflection);
		}

		return true;
	}

}
