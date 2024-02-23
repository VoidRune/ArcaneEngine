#include "BufferStream.h"

namespace Arc
{
	//==============================================================================
	/// BufferStreamWriter
	BufferStreamWriter::BufferStreamWriter(Buffer targetBuffer, uint64_t position)
		: m_TargetBuffer(targetBuffer), m_BufferPosition(position)
	{
	}

	bool BufferStreamWriter::WriteData(const char* data, size_t size)
	{
		bool valid = m_BufferPosition + size <= m_TargetBuffer.Size;
		if (!valid)
			return false;

		memcpy(m_TargetBuffer.As<uint8_t>() + m_BufferPosition, data, size);
		m_BufferPosition += size;
		return true;
	}

	void BufferStreamWriter::WriteBuffer(Buffer buffer, bool writeSize)
	{
		if (writeSize)
			WriteData((char*)&buffer.Size, sizeof(uint32_t));

		WriteData((char*)buffer.Data, buffer.Size);
	}

	void BufferStreamWriter::WriteZero(uint64_t size)
	{
		char zero = 0;
		for (uint64_t i = 0; i < size; i++)
			WriteData(&zero, 1);
	}

	void BufferStreamWriter::WriteString(const std::string& string)
	{
		size_t size = string.size();
		WriteData((char*)&size, sizeof(size_t));
		WriteData((char*)string.data(), sizeof(char) * string.size());
	}

	void BufferStreamWriter::WriteString(std::string_view string)
	{
		size_t size = string.size();
		WriteData((char*)&size, sizeof(size_t));
		WriteData((char*)string.data(), sizeof(char) * string.size());
	}

	//==============================================================================
	/// BufferStreamReader
	BufferStreamReader::BufferStreamReader(Buffer targetBuffer, uint64_t position)
		: m_TargetBuffer(targetBuffer), m_BufferPosition(position)
	{
	}

	bool BufferStreamReader::ReadData(char* destination, size_t size)
	{
		bool valid = m_BufferPosition + size <= m_TargetBuffer.Size;
		if (!valid)
			return false;

		memcpy(destination, m_TargetBuffer.As<uint8_t>() + m_BufferPosition, size);
		m_BufferPosition += size;
		return true;
	}

	bool BufferStreamReader::ReadBuffer(Buffer& buffer, uint32_t size)
	{
		buffer.Size = size;
		if (size == 0)
		{
			if (!ReadData((char*)&buffer.Size, sizeof(uint32_t)))
				return false;
		}

		buffer.Allocate(buffer.Size);
		return ReadData((char*)buffer.Data, buffer.Size);
	}

	bool BufferStreamReader::ReadString(std::string& string)
	{
		size_t size;
		if (!ReadData((char*)&size, sizeof(size_t)))
			return false;

		string.resize(size);
		return ReadData((char*)string.data(), sizeof(char) * size);
	}

}