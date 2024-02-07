#pragma once
#include "Vulkan/QueryPool.h"

namespace Arc
{
	class GpuProfiler
	{
	public:
		GpuProfiler(VkDevice device, VkPhysicalDevice physicalDevice);
		~GpuProfiler();

		void BeginFrameProfiling(VkCommandBuffer cmd);
		void EndFrameProfiling();
		uint32_t StartTask(const std::string& name);
		void EndTask(uint32_t taskId);

		struct ProfilerTask
		{
			double timeInMs;
			std::string taskName;
		};
		std::vector<ProfilerTask>& GetResults() { return m_ProfilerTasks; };

	private:
		QueryPool m_QueryPool;
		double m_TimestampPeriod;


		struct ProfilerTaskInternal
		{
			uint32_t startTimeIndex;
			uint32_t endTimeIndex;
			std::string taskName;
		};
		VkCommandBuffer m_CurrentCmd;
		std::vector<ProfilerTaskInternal> m_ProfilerTasksInternal;
		std::vector<ProfilerTask> m_ProfilerTasks;

	};


}