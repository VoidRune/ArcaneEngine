#include "QueryPool.h"

namespace Arc
{
	QueryPool::QueryPool(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t maxTimestampCount)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		m_Device = device;
		m_TimestampIndex = 0;
		m_MaxTimestampCount = maxTimestampCount;
		m_TimestampPeriod = properties.limits.timestampPeriod;
		m_IsReady = VK_SUCCESS;
		m_QueryResults.resize(2 * maxTimestampCount);

		VkQueryPoolCreateInfo queryPoolCreateInfo{};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;
		queryPoolCreateInfo.flags = 0;

		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = maxTimestampCount; // REVIEW

		VK_CHECK(vkCreateQueryPool(m_Device, &queryPoolCreateInfo, nullptr, &m_QueryPool));
		vkResetQueryPool(m_Device, m_QueryPool, 0, maxTimestampCount);
	}

	uint32_t QueryPool::AddTimestamp(VkCommandBuffer cmd, VkPipelineStageFlagBits pipelineStage)
	{

		if (m_IsReady == VK_SUCCESS)
			vkCmdWriteTimestamp(cmd, pipelineStage, m_QueryPool, m_TimestampIndex);

		m_TimestampIndex++;
		return m_TimestampIndex - 1;
	}

	void QueryPool::QueryResults()
	{
		std::vector<uint64_t> tempResults(m_MaxTimestampCount);
		m_IsReady = vkGetQueryPoolResults(m_Device,
			m_QueryPool, 0, 
			m_TimestampIndex, 2 * sizeof(uint64_t) * m_TimestampIndex,
			m_QueryResults.data(), 2 * sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

		//vkResetQueryPool(_device, _queryPool, 0, _maxTimestampCount);
		//if (result == VK_SUCCESS)
		//{
		//	memcpy(_queryResults.data(), tempResults.data(), sizeof(uint64_t) * _maxTimestampCount);
		//}
	}

	void QueryPool::ResetQueryPool(VkCommandBuffer cmd)
	{
		if (m_IsReady == VK_SUCCESS)
			vkCmdResetQueryPool(cmd, m_QueryPool, 0, m_MaxTimestampCount);
		//vkResetQueryPool(Core::Get()->GetLogicalDevice(), _queryPool, 0, 2);
		m_TimestampIndex = 0;
	}

	QueryPool::~QueryPool()
	{
		vkDestroyQueryPool(m_Device, m_QueryPool, nullptr);
	}
}