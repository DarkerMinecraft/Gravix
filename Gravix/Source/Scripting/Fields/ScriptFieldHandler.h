#pragma once

#include "Scripting/Fields/ScriptField.h"
#include <mono/metadata/object.h>

namespace Gravix
{

	// Helper class to handle script field get/set operations
	class ScriptFieldHandler
	{
	public:
		// Get field value from Mono instance
		static bool GetField(MonoObject* instance, MonoClassField* monoField, ScriptFieldType fieldType, ScriptFieldValue& outValue);

		// Set field value on Mono instance
		static bool SetField(MonoObject* instance, MonoClassField* monoField, ScriptFieldType fieldType, const ScriptFieldValue& value);
	};

}
