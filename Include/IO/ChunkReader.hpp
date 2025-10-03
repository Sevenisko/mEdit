#pragma once
#include "BinaryReader.hpp"
#include <string>
#include <vector>
#include "Chunk.h"

class ChunkReader {
  public:
    ChunkReader(BinaryReader* reader) : m_Reader(reader), m_Size(dtaGetSize(m_Reader->GetDescriptor())) { assert(reader && reader->IsOpen()); }

    BinaryReader* GetReader() const { return m_Reader; }

    // Enter new chunk and return its type
    ChunkType Rascend() {
        m_ReadBytes = 0;
        ChunkHeader header{};
        m_Reader->Read(&header, sizeof(ChunkHeader));

        m_ChunkStack.push_back({m_Reader->GetCurPos(), header.Size - sizeof(ChunkHeader)});

        m_CurChunkLevel = &m_ChunkStack.back();

        return header.Type;
    }

    ChunkType operator++() { return Rascend(); }

    // Seek to end of current chunk
    void Descend() {
        const auto& chunkLevel = m_ChunkStack.back();
        m_Reader->Seek(chunkLevel.Pos + chunkLevel.ChunkSize, SEEK_SET);

        if(m_CurChunkLevel) {
            if(m_ReadBytes < m_CurChunkLevel->ChunkSize) { m_ReadBytes += (m_CurChunkLevel->ChunkSize - m_ReadBytes + sizeof(ChunkHeader)); }

            m_CurChunkLevel = nullptr;
        }

        m_ChunkStack.pop_back();
    }

    void operator--() { Descend(); }

    template<typename T> void Read(T* ptr, size_t size = sizeof(T)) {
        m_ReadBytes += size;
        m_Reader->Read(ptr, size);
    }

    template<typename T> T Read() {
        T val{};
        Read(&val);
        return val;
    }

    std::string ReadNullTerminatedString() { return m_Reader->ReadNullTerminatedString(); }

    std::string ReadString() { return m_Reader->ReadString(); }

    std::string ReadFixedString(size_t size) { return m_Reader->ReadFixedString(size); }

    inline size_t Size() const { return m_Reader->GetCurPos() + sizeof(ChunkHeader) <= m_Size; }

    inline size_t CurPos() const { return m_ReadBytes; }

    operator bool() const { return Size() > 0; }

  private:
    BinaryReader* m_Reader{nullptr};
    std::vector<InputChunkLevelData> m_ChunkStack{};
    InputChunkLevelData* m_CurChunkLevel = nullptr;
    size_t m_ReadBytes = 0;
    size_t m_Size = 0;
};