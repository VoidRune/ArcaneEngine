#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/CommandBuffer.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <string>
#include <unordered_map>

namespace Arc
{
	class TimestampQuery
	{
	public:
		TimestampQuery(DeviceHandle device, PhysicalDeviceHandle physicalDevice);
		~TimestampQuery();


		struct ScopedTimer {
			ScopedTimer(TimestampQuery& query, CommandBufferHandle cmd, const std::string& name);
			~ScopedTimer();
		private:
			TimestampQuery& query;
			CommandBufferHandle cmd;
			std::string name;
			bool isValid;
		};
		ScopedTimer AddScopedTimer(const std::string& name, CommandBuffer* cmd);
		void QueryResults();

	private:

		struct Timestamp
		{
			uint32_t startTimeIndex;
			uint32_t endTimeIndex;
		};

		QueryPoolHandle m_QueryPool;
		DeviceHandle m_LogicalDevice;
		uint32_t m_MaxTimestamps;
		uint32_t m_TimestampCount;
		double m_TimestampPeriod;
		std::unordered_map<std::string, Timestamp> m_Timestamps;
	};
}