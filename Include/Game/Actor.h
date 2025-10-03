#pragma once

#include <cstdint>
#include <I3D/I3D_frame.h>
#include "Enums.h"
#include <IO/ChunkReader.hpp>
#include <IO/ChunkWriter.hpp>
#include <string>

struct GameScript {
    I3D_frame* frame = nullptr;
    std::string name = "";
    std::string script = "";
};

class Actor {
  public:
    ActorType GetType() const { return m_Type; }

    /*void SetName(const std::string& name) { m_Name = name; }
    std::string GetName() const { return m_Name; }*/

    void SetFrame(I3D_frame* frame) { m_Frame = frame; }
    I3D_frame* GetFrame() const { return m_Frame; }

    virtual void Duplicate(Actor* sourceActor) {
        //m_Name = sourceActor->m_Name;
        m_Type = sourceActor->m_Type;
        //m_Frame = sourceActor->m_Frame;
    }

    virtual bool OnLoad(ChunkReader* chunk) { return false; }
    virtual void OnSave(BinaryWriter* writer) {}

    virtual void OnInspectorGUI() {}

    virtual GameScript* GetScript() { return nullptr; }

    virtual size_t GetDataSize() { return 0; }

    bool Load(ChunkReader* chunk) {
        ChunkReader& c = *chunk;

        bool loaded = false;

        if(++c == CT_ACTOR) {
            while(c) {
                ChunkType type = ++c;

                switch(type) {
                case CT_ACTOR_NAME: m_Name = c.ReadNullTerminatedString(); break;

                case CT_ACTOR_TYPE: m_Type = c.Read<ActorType>(); break;

                case CT_ACTOR_DATA: loaded = OnLoad(chunk); break;
                }

                --c;
            }

            --c;
        }

        return loaded;
    }

    void Save(ChunkWriter* chunk) {
        chunk->Ascend(CT_ACTOR);
        {
            chunk->WriteStringChunk(CT_ACTOR_NAME, GetFrame()->GetName());
            chunk->WriteUIntChunk(CT_ACTOR_TYPE, GetType());
            if(GetDataSize() > 0) {
                chunk->Ascend(CT_ACTOR_DATA);
                {
                    OnSave(chunk->GetWriter());
                    chunk->Descend();
                }
            }

            chunk->Descend();
        }
    }

  protected:
    std::string m_Name = "";
    ActorType m_Type = ACTOR_UNKNOWN;
    I3D_frame* m_Frame = nullptr;
};