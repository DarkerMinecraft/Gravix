#pragma once

#include <string>

namespace Gravix::StringUtils
{

	// Convert camelCase to Title Case with spaces
	// Examples: "speed" -> "Speed", "jumpForce" -> "Jump Force", "maxHealthPoints" -> "Max Health Points"
	inline std::string CamelCaseToTitleCase(const std::string& input)
	{
		if (input.empty())
			return input;

		std::string result;
		result.reserve(input.length() + 10); // Reserve extra space for potential spaces

		// First character is always uppercase
		result += std::toupper(input[0]);

		for (size_t i = 1; i < input.length(); i++)
		{
			char c = input[i];

			// Insert space before uppercase letters (unless previous char was also uppercase)
			if (std::isupper(c))
			{
				// Check if we should add a space
				bool shouldAddSpace = true;

				// Don't add space if previous char was uppercase (for acronyms like "HTTPRequest" -> "HTTP Request")
				if (i > 0 && std::isupper(input[i - 1]))
				{
					// But do add space if next char is lowercase (e.g., "HTTPServer" -> "HTTP Server")
					if (i + 1 < input.length() && std::islower(input[i + 1]))
						shouldAddSpace = true;
					else
						shouldAddSpace = false;
				}

				if (shouldAddSpace)
					result += ' ';
			}

			result += c;
		}

		return result;
	}

}
