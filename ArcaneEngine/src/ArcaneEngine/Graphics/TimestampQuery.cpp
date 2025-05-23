#include "TimestampQuery.h"
#include "VulkanCore/VulkanLocal.h"
#include "VulkanCore/VulkanHandleCreation.h"
#include "ArcaneEngine/Core/Log.h"
#include <vulkan/vulkan_core.h>

namespace Arc
{
	TimestampQuery::TimestampQuery(DeviceHandle device, PhysicalDeviceHandle physicalDevice)
	{
		m_LogicalDevice = device;
		m_MaxTimestamps = 128;
		m_TimestampCount = 0;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties((VkPhysicalDevice)physicalDevice, &properties);
		m_TimestampPeriod = (double)properties.limits.timestampPeriod / 1e6;

		QueryPoolInfo info = {
			.logicalDevice = device,
			.maxTimestampCount = m_MaxTimestamps,
		};
		m_QueryPool = CreateQueryPoolHandle(info);
	}

	TimestampQuery::~TimestampQuery()
	{
		vkDestroyQueryPool((VkDevice)m_LogicalDevice, (VkQueryPool)m_QueryPool, nullptr);
	}

	void TimestampQuery::QueryResults()
	{
		if (m_TimestampCount <= 0)
		{
			return;
		}
		std::vector<uint64_t> queriedTimestamps(m_TimestampCount);
		VkResult result = vkGetQueryPoolResults((VkDevice)m_LogicalDevice,
			(VkQueryPool)m_QueryPool, 0,
			m_TimestampCount, sizeof(uint64_t) * queriedTimestamps.size(),
			queriedTimestamps.data(), sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
		// TODO remove VK_QUERY_RESULT_WAIT_BIT

		if (result == VK_SUCCESS)
		{
			for (const auto& pair : m_Timestamps)
			{
				const auto& name = pair.first;
				const auto& timestamp = pair.second;

				uint64_t start = queriedTimestamps[timestamp.startTimeIndex];
				uint64_t end = queriedTimestamps[timestamp.endTimeIndex];

				float timeMs = (end - start) * m_TimestampPeriod;
				//ARC_LOG("GPU Execution Time for {}: {}", name, timeMs);


				auto it = m_AccumulatedTimestamps.find(name);
				if (it != m_AccumulatedTimestamps.end())
				{
					m_AccumulatedTimestamps[name].count++;
					m_AccumulatedTimestamps[name].timeSum += timeMs;
				}
				else
				{
					AccumulatedTime t;
					t.count = 1;
					t.timeSum = timeMs;
					m_AccumulatedTimestamps[name] = t;
				}
				auto a = m_AccumulatedTimestamps[name];
				ARC_LOG("Average GPU Execution Time for {}: {}", name, a.timeSum / (double)a.count);
			}

			m_TimestampCount = 0;
			m_Timestamps.clear();
		}
	}

	TimestampQuery::ScopedTimer TimestampQuery::AddScopedTimer(const std::string& name, CommandBuffer* cmd)
	{
		if (m_Timestamps.empty())
		{
			vkCmdResetQueryPool((VkCommandBuffer)cmd->GetHandle(), (VkQueryPool)m_QueryPool, 0, m_MaxTimestamps);
		}

		return TimestampQuery::ScopedTimer(*this, cmd->GetHandle(), name);
	}

	TimestampQuery::ScopedTimer::ScopedTimer(TimestampQuery& query, CommandBufferHandle cmd, const std::string& name)
		: query(query), cmd(cmd), name(name)
	{
		isValid = true;
		if (query.m_Timestamps.find(name) != query.m_Timestamps.end() ||
			query.m_TimestampCount + 2 > query.m_MaxTimestamps)
		{
			isValid = false;
			return;
		}

		Timestamp timestamp = { query.m_TimestampCount, query.m_TimestampCount + 1 };
		query.m_Timestamps[name] = timestamp;
		query.m_TimestampCount += 2;

		vkCmdWriteTimestamp((VkCommandBuffer)cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, (VkQueryPool)query.m_QueryPool, timestamp.startTimeIndex);
	}

	TimestampQuery::ScopedTimer::~ScopedTimer()
	{
		if (!isValid)
		{
			return;
		}

		auto it = query.m_Timestamps.find(name);
		if (it != query.m_Timestamps.end())
		{
			vkCmdWriteTimestamp((VkCommandBuffer)cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, (VkQueryPool)query.m_QueryPool, it->second.endTimeIndex);
		}
	}
}