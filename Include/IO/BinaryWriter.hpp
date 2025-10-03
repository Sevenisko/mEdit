#pragma once

#include <string>
#include <cstdint>
#include <fstream>
#include <I3D/I3D_math.h>

class BinaryWriter {
  public:
    BinaryWriter(const std::string& fileName) { m_File.open(fileName, std::ios::binary); }
    ~BinaryWriter() {
        if(m_File.is_open()) { m_File.close(); }
    }

    bool IsOpen() const { return m_File.is_open(); }

    std::ofstream* GetStream() { return &m_File; }

    size_t GetCurPos() const { return m_CurFilePos; }

    size_t Seek(size_t offset, int mode) {
        std::ios::seekdir dir;

        switch(mode) {
        case SEEK_CUR: {
            dir = std::ios::cur;
            m_CurFilePos += offset;
        } break;
        case SEEK_SET: {
            dir = std::ios::beg;
            m_CurFilePos = offset;
        } break;
        case SEEK_END: {
            dir = std::ios::end;
        } break;
        }

        m_File.seekp(offset, dir);

        if(mode == SEEK_END) { 
            m_CurFilePos = m_File.tellp();
        }

        return m_CurFilePos;
    }

    void Write(void* buf, size_t size) {
        m_File.write((char*)buf, size);
        m_CurFilePos += size;
    }

    void WriteUInt8(uint8_t value) { Write(&value, sizeof(uint8_t)); }
    void WriteInt8(int8_t value) { Write(&value, sizeof(int8_t)); }
    void WriteUInt16(uint16_t value) { Write(&value, sizeof(uint16_t)); }
    void WriteInt16(int16_t value) { Write(&value, sizeof(int16_t)); }
    void WriteUInt32(uint32_t value) { Write(&value, sizeof(uint32_t)); }
    void WriteInt32(int32_t value) { Write(&value, sizeof(int32_t)); }
    void WriteUInt64(uint64_t value) { Write(&value, sizeof(uint64_t)); }
    void WriteInt64(int64_t value) { Write(&value, sizeof(int64_t)); }
    void WriteSingle(float value) { Write(&value, sizeof(float)); }
    void WriteDouble(double value) { Write(&value, sizeof(double)); }
    void WriteBoolean(bool value) { Write(&value, sizeof(bool)); }
    void WriteVec2(const S_vector2& value) {
        WriteSingle(value.x);
        WriteSingle(value.y);
    }
    void WriteVec3(const S_vector& value) {
        WriteSingle(value.x);
        WriteSingle(value.y);
        WriteSingle(value.z);
    }
    void WriteVec4(const S_vector4& value) {
        WriteSingle(value.x);
        WriteSingle(value.y);
        WriteSingle(value.z);
        WriteSingle(value.w);
    }
    void WriteQuat(const S_quat& value) {
        WriteSingle(value.w);
        WriteSingle(value.x);
        WriteSingle(value.y);
        WriteSingle(value.z);
    }
    void WriteMatrix(const S_matrix& value) {
        WriteSingle(value.m_11);
        WriteSingle(value.m_12);
        WriteSingle(value.m_13);
        WriteSingle(value.m_14);

        WriteSingle(value.m_21);
        WriteSingle(value.m_22);
        WriteSingle(value.m_23);
        WriteSingle(value.m_24);

        WriteSingle(value.m_31);
        WriteSingle(value.m_32);
        WriteSingle(value.m_33);
        WriteSingle(value.m_34);

        WriteSingle(value.m_41);
        WriteSingle(value.m_42);
        WriteSingle(value.m_43);
        WriteSingle(value.m_44);
    }
    void WriteNullTerminatedString(const std::string& str) {
        for(char c: str) {
            Write(&c, 1);
        }
        char c = 0;
        Write(&c, 1);
    }
    void WriteFixedString(const std::string& str, size_t size) {
        size_t uCurChar = 0;
        while(uCurChar < size) {
            if(uCurChar < str.size()) {
                Write((char*)&str[uCurChar], 1);
            } else {
                char c = '\0';
                Write(&c, 1);
            }

            uCurChar++;
        }
    }
    void WriteString(const std::string& str) {
        uint32_t uSize = str.length();
        Write(&uSize, sizeof(uint32_t));

        for(char c: str) {
            Write(&c, 1);
        }
    }
    void WriteString8(const std::string& str) {
        uint8_t uSize = 0;
        if(str.length() < 255) {
            uSize = static_cast<uint8_t>(str.length());
        } else {
            uSize = 255;
        }

        Write(&uSize, sizeof(uint8_t));

        for(char c: str) {
            Write(&c, 1);
        }
    }

  private:
    std::ofstream m_File;
    size_t m_CurFilePos = 0;
};