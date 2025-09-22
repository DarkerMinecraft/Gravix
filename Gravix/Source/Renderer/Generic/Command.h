#pragma once

#include "Renderer/CommandImpl.h"

namespace Gravix
{

	class Command
	{
	public:
		Command(const std::string& commandName);
		virtual ~Command();
	private:
		CommandImpl* m_Impl = nullptr;
	private:
		void Initialize(const std::string& commandName);
	};
}