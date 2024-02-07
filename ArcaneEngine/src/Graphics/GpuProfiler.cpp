#include "GpuProfiler.h"

namespace Arc
{

	GpuProfiler::GpuProfiler(VkDevice device, VkPhysicalDevice physicalDevice) :
		m_QueryPool(device, physicalDevice, 128)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		m_TimestampPeriod = (double)properties.limits.timestampPeriod;
		m_TimestampPeriod /= 1e6;
	}

	GpuProfiler::~GpuProfiler()
	{

	}

	void GpuProfiler::BeginFrameProfiling(VkCommandBuffer cmd)
	{
		m_CurrentCmd = cmd;
		m_QueryPool.ResetQueryPool(cmd);
		m_ProfilerTasksInternal.clear();
	}

	void GpuProfiler::EndFrameProfiling()
	{
		if (m_ProfilerTasksInternal.size() > 0)
		{
			m_QueryPool.QueryResults();
			if (!m_QueryPool.IsReady())
				return;

			m_ProfilerTasks.clear();
			std::vector<uint64_t> res = m_QueryPool.GetQueryResults();

			for (int i = 0; i < m_ProfilerTasksInternal.size(); i++)
			{
				ProfilerTaskInternal t = m_ProfilerTasksInternal[i];
				ProfilerTask task;
				task.taskName = t.taskName;
				task.timeInMs = (res[2 * t.endTimeIndex] - res[2 * t.startTimeIndex]) * m_TimestampPeriod;
				m_ProfilerTasks.push_back(task);
			}
		}
	}

	uint32_t GpuProfiler::StartTask(const std::string& name)
	{
		ProfilerTaskInternal task;
		task.taskName = name;
		task.startTimeIndex = m_QueryPool.AddTimestamp(m_CurrentCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		uint32_t index = m_ProfilerTasksInternal.size();
		m_ProfilerTasksInternal.push_back(task);

		return index;
	}

	void GpuProfiler::EndTask(uint32_t taskId)
	{
		m_ProfilerTasksInternal[taskId].endTimeIndex = m_QueryPool.AddTimestamp(m_CurrentCmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}
}