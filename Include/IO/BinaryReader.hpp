#pragma once

#include <string>
#include <cstdint>
#include <rw_data.h>
#include <I3D/I3D_math.h>
#include <Utils.h>

class BinaryReader {
public:
	BinaryReader(const std::string& filename) {
		m_FileDescriptor = dtaOpen(filename.c_str(), 0);

		m_CurOffset = 0;
	}

	~BinaryReader() {
		if (m_FileDescriptor != -1) {
			dtaClose(m_FileDescriptor);
		}
	}

	bool IsOpen() const {
		return m_FileDescriptor != -1;
	}

	int GetDescriptor() const {
		return m_FileDescriptor;
	}

	size_t GetCurPos() const {
		return m_CurOffset;
	}

	size_t NumReadBytes() const { return m_ReadBytes;
	}

	size_t Seek(size_t offset, int mode) {
		//debugPrintf("Seek from %d: %d offset (%s), %d prev offset, %d cur offset", m_FileDescriptor, offset, m_Mode == SEEK_CUR ? "SEEK_CUR" : (m_Mode == SEEK_SET ? "SEEK_SET" : "SEEK_END"), prevOffset, curOffset);

		m_CurOffset = dtaSeek(m_FileDescriptor, offset, mode);

		return m_CurOffset;
	}

	int Read(void* buf, size_t size) {
		int readBytes = dtaRead(m_FileDescriptor, buf, size);

		int offset = m_CurOffset;

		m_CurOffset += readBytes;

		m_ReadBytes += readBytes;

		//debugPrintf("Read from %d: %d bytes, %d prev offset, %d cur offset", m_FileDescriptor, readBytes, offset, m_CurOffset);

		return readBytes;
	}

	uint8_t ReadUInt8() {
		uint8_t uValue;
		Read(&uValue, sizeof(uint8_t));
		return uValue;
	}

	int8_t ReadInt8() {
		int8_t iValue;
		Read(&iValue, sizeof(int8_t));
		return iValue;
	}

	uint16_t ReadUInt16() {
		uint16_t uValue;
		Read(&uValue, sizeof(uint16_t));
		return uValue;
	}

	int16_t ReadInt16() {
		int16_t iValue;
		Read(&iValue, sizeof(int16_t));
		return iValue;
	}

	uint32_t ReadUInt32() {
		uint32_t uValue;
		Read(&uValue, sizeof(uint32_t));
		return uValue;
	}

	int32_t ReadInt32() {
		int32_t iValue;
		Read(&iValue, sizeof(int32_t));
		return iValue;
	}

	uint64_t ReadUInt64() {
		uint64_t uValue;
		Read(&uValue, sizeof(uint64_t));
		return uValue;
	}

	int64_t ReadInt64() {
		int64_t iValue;
		Read(&iValue, sizeof(int64_t));
		return iValue;
	}

	float ReadSingle() {
		float fValue;
		Read(&fValue, sizeof(float));
		return fValue;
	}

	double ReadDouble() {
		double dValue;
		Read(&dValue, sizeof(double));
		return dValue;
	}

	bool ReadBoolean() {
		uint8_t uValue = ReadUInt8();
		return uValue != 0;
	}

	S_vector2 ReadVec2() {
		S_vector2 vValue;
		vValue.x = ReadSingle();
		vValue.y = ReadSingle();
		return vValue;
	}

	S_vector ReadVec3() {
		S_vector vValue;
		vValue.x = ReadSingle();
		vValue.y = ReadSingle();
		vValue.z = ReadSingle();
		return vValue;
	}

	S_vector4 ReadVec4() {
		S_vector4 vValue;
		vValue.x = ReadSingle();
		vValue.y = ReadSingle();
		vValue.z = ReadSingle();
		vValue.w = ReadSingle();
		return vValue;
	}

	S_quat ReadQuat() {
		S_quat qValue;
		qValue.x = ReadSingle();
		qValue.y = ReadSingle();
		qValue.z = ReadSingle();
		qValue.w = ReadSingle();
		return qValue;
	}

	S_matrix ReadMatrix() {
		S_matrix mValue;
		mValue.e[0] = ReadSingle();
		mValue.e[1] = ReadSingle();
		mValue.e[2] = ReadSingle();
		mValue.e[3] = ReadSingle();

		mValue.e[4] = ReadSingle();
		mValue.e[5] = ReadSingle();
		mValue.e[6] = ReadSingle();
		mValue.e[7] = ReadSingle();

		mValue.e[8] = ReadSingle();
		mValue.e[9] = ReadSingle();
		mValue.e[10] = ReadSingle();
		mValue.e[11] = ReadSingle();

		mValue.e[12] = ReadSingle();
		mValue.e[13] = ReadSingle();
		mValue.e[14] = ReadSingle();
		mValue.e[15] = ReadSingle();

		return mValue;
	}

	std::string ReadNullTerminatedString() {
		std::string str;
		char c = '\xFF';
		while (c != 0) {
			Read(&c, 1);
			if (c != 0) {
				str += c;
			}
		}
		return str;
	}

	std::string ReadFixedString(size_t uSize) {
		std::string str;
		std::vector<char> buffer(uSize);
		Read(buffer.data(), uSize);
		str.assign(buffer.begin(), buffer.end());
		return str;
	}

	std::string ReadString() {
		uint32_t uSize = ReadUInt32();
		std::string str;
		std::vector<char> buffer(uSize);
		Read(buffer.data(), uSize);
		str.assign(buffer.begin(), buffer.end());
		return str;
	}

	std::string ReadString8() {
		uint8_t uSize = ReadUInt8();
		std::string str;
		std::vector<char> buffer(uSize);
		Read(buffer.data(), uSize);
		str.assign(buffer.begin(), buffer.end());
		return str;
	}

private:
	int m_FileDescriptor = -1;
	size_t m_CurOffset = 0;
    size_t m_ReadBytes = 0;
};