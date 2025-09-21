#include "pch.h"
#include "Scheduler.h"

namespace Gravix
{
	
	void Scheduler::Init(uint32_t threadCount /*= std::thread::hardware_concurrency()*/)
	{
		enki::TaskSchedulerConfig config;
		config.numTaskThreadsToCreate = threadCount > 0 ? threadCount - 1 : 0; // Main thread is also a task thread

		m_TaskScheduler.Initialize(config);
	}

	
}