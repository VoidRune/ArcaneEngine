#pragma once
#include "VkLocal.h"

namespace Arc
{
	class QueryPool
	{
	public:
		QueryPool(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t maxTimestampCount);
		~QueryPool();

		uint32_t AddTimestamp(VkCommandBuffer cmd, VkPipelineStageFlagBits pipelineStage);
		void QueryResults();
		bool IsReady() { return m_IsReady == VK_SUCCESS; };
		void ResetQueryPool(VkCommandBuffer cmd);

		std::vector<uint64_t>& GetQueryResults() { return m_QueryResults; };
	private:
		VkDevice m_Device;

		VkQueryPool m_QueryPool;
		std::vector<uint64_t> m_QueryResults;

		uint32_t m_TimestampIndex;
		uint32_t m_MaxTimestampCount;
		float m_TimestampPeriod;
		uint32_t m_IsReady;
	};
}