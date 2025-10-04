#pragma once
#include "common.h"

struct IDirect3DIndexBuffer8;
class C_IBuffer;

struct COMPLETE_IB_STRUCT {
    C_IBuffer* m_pBuffer;
    IDirect3DIndexBuffer8* m_pD3DBuffer;
    uint32_t m_uBufferId;
};

class C_IBuffer { 
public:
    C_IBuffer() { }
    ~C_IBuffer() { }

    BYTE* buffer;
    uint8_t pad0[4];
    uint32_t usage;
    D3DPOOL pool;
    IDirect3DIndexBuffer8* indexBuffer;
    uint8_t pad1[32];
    uint32_t numIndices;
    uint8_t pad2[4];
    uint32_t index;
};

class C_IBManager {
public:
    C_IBManager() { }
    ~C_IBManager() { }

    bool m_Alloc(uint32_t uUnk1, uint32_t uUnk2, COMPLETE_IB_STRUCT* pIBStructOut);
    bool m_Free(COMPLETE_IB_STRUCT* pIBStruct);
    void m_DebugStatus2LOG(); // NOTE(DavoSK): used in debug engine, we can backport it back when we do implementation
    void m_FreeAllIBuffers();
    void m_FreeAllUnusedIBuffers();
    void m_FreeAllVRAMIBuffers();
    void m_UnlockAllIBuffers();
    friend class IGraph;
};