#pragma once

#include "Buffer.h"

#include <filesystem>
#include <fstream>

namespace Arc
{
	//==============================================================================
	/// BufferStreamWriter
	class BufferStreamWriter
	{
	public:
		BufferStreamWriter(Buffer targetBuffer, uint64_t position = 0);
		BufferStreamWriter(const BufferStreamWriter&) = delete;

		bool IsStreamGood() const { return (bool)m_TargetBuffer; }
		uint64_t GetStreamPosition() { return m_BufferPosition; }
		void SetStreamPosition(uint64_t position) { m_BufferPosition = position; }
		bool WriteData(const char* data, size_t size);

		void WriteBuffer(Buffer buffer, bool writeSize = true);
		void WriteZero(uint64_t size);
		void WriteString(const std::string& string);
		void WriteString(std::string_view string);

		template<typename T>
		void WriteRaw(const T& type)
		{
			bool success = WriteData((char*)&type, sizeof(T));
		}

		// Returns Buffer with currently written size
		Buffer GetBuffer() const { return Buffer(m_TargetBuffer, m_BufferPosition); }
	private:
		Buffer m_TargetBuffer;
		uint64_t m_BufferPosition = 0;
	};

	//==============================================================================
	/// BufferStreamReader
	class BufferStreamReader
	{
	public:
		BufferStreamReader(Buffer targetBuffer, uint64_t position = 0);
		BufferStreamReader(const BufferStreamReader&) = delete;

		bool IsStreamGood() const { return (bool)m_TargetBuffer; }
		uint64_t GetStreamPosition() { return m_BufferPosition; }
		void SetStreamPosition(uint64_t position) { m_BufferPosition = position; }
		bool ReadData(char* destination, size_t size);

		bool ReadBuffer(Buffer& buffer, uint32_t size = 0);
		bool ReadString(std::string& string);

		template<typename T>
		bool ReadRaw(T& type)
		{
			bool success = ReadData((char*)&type, sizeof(T));
			return success;
		}

		// Returns Buffer with currently read size
		Buffer GetBuffer() const { return Buffer(m_TargetBuffer, m_BufferPosition); }
	private:
		Buffer m_TargetBuffer;
		uint64_t m_BufferPosition = 0;
	};

} // namespace Walnut
