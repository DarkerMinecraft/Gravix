#include "pch.h"
#include "ScriptFieldHandler.h"

#include <mono/metadata/class.h>

namespace Gravix
{

	bool ScriptFieldHandler::GetField(MonoObject* instance, MonoClassField* monoField, ScriptFieldType fieldType, ScriptFieldValue& outValue)
	{
		if (!instance || !monoField)
			return false;

		outValue.Type = fieldType;

		switch (fieldType)
		{
		case ScriptFieldType::Float:
		{
			float value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Double:
		{
			double value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Int:
		{
			int32_t value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::UInt:
		{
			uint32_t value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Long:
		{
			int64_t value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Short:
		{
			int16_t value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Byte:
		{
			uint8_t value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Char:
		{
			char value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Bool:
		{
			bool value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Vector2:
		{
			glm::vec2 value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Vector3:
		{
			glm::vec3 value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Vector4:
		{
			glm::vec4 value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		case ScriptFieldType::Entity:
		{
			UUID value;
			mono_field_get_value(instance, monoField, &value);
			outValue.SetValue(value);
			break;
		}
		}

		return true;
	}

	bool ScriptFieldHandler::SetField(MonoObject* instance, MonoClassField* monoField, ScriptFieldType fieldType, const ScriptFieldValue& value)
	{
		if (!instance || !monoField)
			return false;

		switch (fieldType)
		{
		case ScriptFieldType::Float:
		{
			float val = value.GetValue<float>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Double:
		{
			double val = value.GetValue<double>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Int:
		{
			int32_t val = value.GetValue<int32_t>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::UInt:
		{
			uint32_t val = value.GetValue<uint32_t>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Long:
		{
			int64_t val = value.GetValue<int64_t>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Short:
		{
			int16_t val = value.GetValue<int16_t>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Byte:
		{
			uint8_t val = value.GetValue<uint8_t>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Char:
		{
			char val = value.GetValue<char>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Bool:
		{
			bool val = value.GetValue<bool>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Vector2:
		{
			glm::vec2 val = value.GetValue<glm::vec2>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Vector3:
		{
			glm::vec3 val = value.GetValue<glm::vec3>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Vector4:
		{
			glm::vec4 val = value.GetValue<glm::vec4>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		case ScriptFieldType::Entity:
		{
			UUID val = value.GetValue<UUID>();
			mono_field_set_value(instance, monoField, &val);
			break;
		}
		}

		return true;
	}

}
