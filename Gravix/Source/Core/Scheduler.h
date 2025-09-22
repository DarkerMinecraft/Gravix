#pragma once

#include "TaskScheduler.h"

namespace Gravix 
{

	struct RunPinnedTaskLoop : public enki::IPinnedTask
	{
		void Execute() override
		{
			while (TaskSch->GetIsRunning() && ExecuteTasks)
			{
				TaskSch->WaitForNewPinnedTasks(); // this thread will 'sleep' until there are new pinned tasks
				TaskSch->RunPinnedTasks();
			}
		}

		enki::TaskScheduler* TaskSch;
		bool ExecuteTasks = true;
	};

	class Scheduler 
	{
	public:
		Scheduler() = default;
		~Scheduler() = default;

		// Initialize with specified number of threads (default to number of hardware threads)
		void Init(uint32_t threadCount = std::thread::hardware_concurrency());

		//void CreateRunPinnedTaskLoop(const RunPinnedTaskLoop& runTask) { m_TaskScheduler.AddPinnedTask(runTask); }

		enki::TaskScheduler& GetTaskScheduler() { return m_TaskScheduler; }
	private:
		enki::TaskScheduler m_TaskScheduler;
	};
}