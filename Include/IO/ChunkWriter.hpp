#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "BinaryWriter.hpp"
#include "Chunk.h"

class ChunkWriter {
  public:
    ChunkWriter(BinaryWriter* writer) : m_Writer(writer) {
        assert(writer && writer->IsOpen());
        m_ChunkStack.push_back({0, 0}); // Root level
    }

    BinaryWriter* GetWriter() const { return m_Writer; }

    // Start a new chunk and write its header
    void Ascend(ChunkType type) {
        // Write header with placeholder size
        ChunkHeader header{type, 0};
        size_t headerPos = m_Writer->GetCurPos();
        m_Writer->Write(&header, sizeof(ChunkHeader));

        // Record chunk level data
        m_ChunkStack.push_back({type, headerPos, static_cast<size_t>(m_Writer->GetCurPos())});
    }

    void operator+=(ChunkType type) { Ascend(type); } // Type will be set later if needed

    // Close current chunk and update its size in the header
    void Descend() {
        assert(!m_ChunkStack.empty() && "Cannot descend: no chunk to close");

        auto& chunkLevel = m_ChunkStack.back();
        size_t currentPos = m_Writer->GetCurPos();
        size_t chunkSize = currentPos - chunkLevel.DataStartPos;

        // Update header with correct size
        m_Writer->Seek(chunkLevel.HeaderPos, SEEK_SET);
        ChunkHeader header;
        header.Type = m_ChunkStack.back().Type; // Preserve original type
        header.Size = static_cast<uint32_t>(chunkSize + sizeof(ChunkHeader));
        m_Writer->Write(&header, sizeof(ChunkHeader));

        // Restore file position
        m_Writer->Seek(currentPos, SEEK_SET);

        m_ChunkStack.pop_back();
    }

    void operator--() { Descend(); }

    void Write(void* ptr, size_t size) { m_Writer->Write(ptr, size); }

    template<typename T> void Write(const T& val) { Write((void*)&val, sizeof(T)); }

    void WriteNullTerminatedString(const std::string& str) { m_Writer->WriteNullTerminatedString(str); }

    void WriteString(const std::string& str) { m_Writer->WriteString(str); }

    void WriteFixedString(const std::string& str, size_t size) { m_Writer->WriteFixedString(str, size); }

    void WriteFloatChunk(ChunkType type, float value) {
        Ascend(type);
        Write<float>(value);
        Descend();
    }

    void WriteIntChunk(ChunkType type, int value) {
        Ascend(type);
        Write<int>(value);
        Descend();
    }

    void WriteUIntChunk(ChunkType type, uint32_t value) {
        Ascend(type);
        Write<uint32_t>(value);
        Descend();
    }

    void WriteVec2Chunk(ChunkType type, const S_vector2& vec) {
        Ascend(type);
        Write<S_vector2>(vec);
        Descend();
    }

    void WriteVec3Chunk(ChunkType type, const S_vector& vec) {
        Ascend(type);
        Write<S_vector>(vec);
        Descend();
    }

    void WriteVec4Chunk(ChunkType type, const S_vector4& vec) {
        Ascend(type);
        Write<S_vector4>(vec);
        Descend();
    }

    void WriteQuatChunk(ChunkType type, const S_quat& quat) {
        Ascend(type);
        Write<S_quat>(quat);
        Descend();
    }

    void WriteMatrixChunk(ChunkType type, const S_matrix& mat) {
        Ascend(type);
        Write<S_matrix>(mat);
        Descend();
    }

    void WriteStringChunk(ChunkType type, const std::string& str) {
        Ascend(type);
        WriteNullTerminatedString(str);
        Descend();
    }

    bool IsOpen() const { return m_Writer->IsOpen(); }

    operator bool() const { return IsOpen(); }

  private:
    BinaryWriter* m_Writer{nullptr};
    std::vector<OutputChunkLevelData> m_ChunkStack{};
};