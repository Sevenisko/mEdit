#pragma once

#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <string>
#include <glm.hpp>
#include <gtc/quaternion.hpp>

class MemoryStream {
public:
    MemoryStream() { }

    MemoryStream(uint8_t* pData, size_t uSize) {
        m_pData = pData;
        m_uSize = uSize;
        m_uCurPos = 0;
    }

    size_t Write(void* pBuf, size_t uSize) {
        if (m_uCurPos + uSize <= m_uSize) {
            memcpy(&m_pData[m_uCurPos], pBuf, uSize);

            m_uCurPos += uSize;

            return uSize;
        } else if ((m_uSize - (m_uCurPos + uSize)) != 0) {
            size_t size = (m_uSize - (m_uCurPos + uSize));

            memcpy(&m_pData[m_uCurPos], pBuf, size);

            m_uCurPos += size;

            return size;
        } else {
            return 0;
        }
    }

    void WriteUInt8(uint8_t value) {
        Write(&value, sizeof(uint8_t));
    }
    void WriteInt8(int8_t value) {
        Write(&value, sizeof(int8_t));
    }
    void WriteUInt16(uint16_t value) {
        Write(&value, sizeof(uint16_t));
    }
    void WriteInt16(int16_t value) {
        Write(&value, sizeof(int16_t));
    }
    void WriteUInt32(uint32_t value) {
        Write(&value, sizeof(uint32_t));
    }
    void WriteInt32(int32_t value) {
        Write(&value, sizeof(int32_t));
    }
    void WriteUInt64(uint64_t value) {
        Write(&value, sizeof(uint64_t));
    }
    void WriteInt64(int64_t value) {
        Write(&value, sizeof(int64_t));
    }
    void WriteSingle(float value) {
        Write(&value, sizeof(float));
    }
    void WriteDouble(double value) {
        Write(&value, sizeof(double));
    }
    void WriteVec2(const glm::vec2& value) {
        Write((void*)&value, sizeof(glm::vec2));
    }
    void WriteVec3(const glm::vec3& value) {
        Write((void*)&value, sizeof(glm::vec3));
    }
    void WriteQuat(const glm::quat& value) {
        Write((void*)&value, sizeof(glm::quat));
    }
    void WriteNullTerminatedString(const std::string& str) {
        for (char c : str) {
            Write(&c, 1);
        }
        char c = 0;
        Write(&c, 1);
    }
    void WriteFixedString(const std::string& str) {
        for (char c : str) {
            Write(&c, 1);
        }
    }
    void WriteString(const std::string& str) {
        uint32_t uSize = str.length();
        Write(&uSize, sizeof(uint32_t));

        for (char c : str) {
            Write(&c, 1);
        }
    }
    void WriteChunk(Chunk& sChunk) {
        WriteUInt16(sChunk.GetType());

        size_t uBufSize = sChunk.GetSize();

        WriteUInt32(uBufSize + 6);

        uint8_t* pBuf = new uint8_t[uBufSize];

        sChunk.Read(pBuf, uBufSize);

        sChunk.SetCurPos(0);

        Write(pBuf, uBufSize);

        delete[] pBuf;
    }

    size_t Read(void* pBuf, size_t uSize) {
        if (m_uCurPos + uSize <= m_uSize) {
            memcpy(pBuf, &m_pData[m_uCurPos], uSize);

            m_uCurPos += uSize;

            return uSize;
        } else if ((m_uSize - (m_uCurPos + uSize)) != 0) {
            size_t size = (m_uSize - (m_uCurPos + uSize));

            memcpy(pBuf, &m_pData[m_uCurPos], size);

            m_uCurPos += size;

            return size;
        } else {
            return 0;
        }
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
    glm::vec2 ReadVec2() {
        glm::vec2 vValue;

        Read(&vValue, sizeof(glm::vec2));

        return vValue;
    }
    glm::vec3 ReadVec3() {
        glm::vec3 vValue;

        Read(&vValue, sizeof(glm::vec3));

        return vValue;
    }
    glm::quat ReadQuat() {
        glm::quat qValue;

        Read(&qValue, sizeof(glm::quat));

        return qValue;
    }
    std::string ReadNullTerminatedString() {
        std::string str;

        char c = 255;

        while (c != 0) {
            Read(&c, 1);
            str += c;
        }

        return str;
    }
    std::string ReadFixedString(size_t uSize) {
        std::string str;

        char c;

        for (size_t u = 0; u < uSize; u++) {
            Read(&c, 1);

            str += c;
        }

        return str;
    }
    std::string ReadString() {
        uint32_t uSize;

        Read(&uSize, sizeof(uint32_t));

        std::string str;
        char c;

        for (uint32_t u = 0; u < uSize; u++) {
            Read(&c, 1);

            str += c;
        }

        return str;
    }
    bool ReadChunk(Chunk& sChunk) {
        uint16_t uType  = ReadUInt16();
        size_t uBufSize = ReadUInt32();

        uBufSize -= 6;

        sChunk.SetType(uType);
        sChunk.SetSize(uBufSize);
        sChunk.Alloc();

        uint8_t* pBuf = new uint8_t[uBufSize];

        size_t uReadBytes = Read(pBuf, uBufSize);

        sChunk.Write(pBuf, uBufSize);

        sChunk.SetCurPos(0);

        delete[] pBuf;

        if (uReadBytes != uBufSize) {
            m_uCurPos -= (6 + uReadBytes);
            return false;
        }

        return true;
    }

private:
    uint8_t* m_pData;
    size_t m_uSize;
    size_t m_uCurPos;
};