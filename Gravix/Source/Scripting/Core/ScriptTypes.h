#pragma once

#include <string>
#include <unordered_map>

extern "C"
{
	typedef struct _MonoType MonoType;
}

namespace Gravix
{

	enum class ScriptFieldType
	{
		None = 0,
		Float, Vector2, Vector3, Vector4,
		Int, UInt, Long, Bool, Double, Short, Byte,
		Char,
		Entity
	};

	class ScriptTypeUtils
	{
	public:
		static ScriptFieldType MonoTypeToScriptType(MonoType* monoType);
		static const char* ScriptFieldTypeToString(ScriptFieldType type);

	private:
		static const std::unordered_map<std::string, ScriptFieldType>& GetTypeMap();
	};

}
