#pragma once

#include <LS3D.h>
#include <I3D/Visuals/I3D_object.h>
#include <I3D/I3D_scene.h>
#include <IBManager.h>

typedef enum LightmapType : uint8_t { LM_TYPE_VERTEX = 5, LM_TYPE_BITMAP = 6 } LightmapType;

#pragma pack(push, 1)
struct ColorRGBA {
    uint8_t r, g, b, a;
};

struct ColorRGB {
    uint8_t r, g, b;
};
#pragma pack(pop)

typedef enum BitmapType : uint8_t { LM_BITMAP_TYPE_BITMAP = 0, LM_BITMAP_TYPE_COLOR = 1 } BitmapType;

typedef enum : uint8_t { LM_VERTEX = 1 << 0, LM_BITMAP = 1 << 1, LM_BUILD = 1 << 2 } LightmapLevelFlags;

class I3D_mesh_object;

class lod {
  public:
    struct LinkCoord {
        uint16_t origVertexIndex;
        uint16_t bitmapIndex;
    };

    struct FaceMap {
        uint16_t* indices;
    };

    uint32_t vtable;
    uint8_t pad[44];
    uint32_t numColorsOrUVs;
    S_vector2* UVs;
    S_vector2* LitUVs;
    uint32_t unk2;
    LinkCoord* linkCoords;
    ColorRGBA* vertexColors;
    uint8_t pad2[8];

    struct Bitmap {
        uint8_t type;
        uint16_t unk;
        uint16_t width;
        uint16_t height;
        uint32_t size;
        uint32_t format;
        uint32_t unk2;
        union {
            ColorRGB* pixels;
            ColorRGBA color;
        };
        uint32_t unk3;
    }* bitmaps;

    uint32_t numBitmaps;
    void* lightMap;
    uint32_t numFaceGroups;
    FaceMap* faceMaps;
    C_IBuffer* indexBuffer;
    uint32_t unk4;
    uint32_t indexBufferOffset;
    uint32_t unk5;
};

class I3D_lit_object : public I3D_object {
  public:
    virtual LS3D_RESULT __stdcall Duplicate(const I3D_frame* frame) override;
    virtual ~I3D_lit_object() {}
    virtual void m_AddFGroups() override;
    virtual void m_RenderFGroup(void* fgStruct) override;
    virtual void __stdcall SetMesh(const I3D_mesh_object*, bool) override;
    virtual void __stdcall UpdateVertices(int) override;
    virtual void __stdcall UpdateVertices2() override;
    virtual void __stdcall DontUse(uint32_t);
    virtual void __stdcall DontUse2(uint32_t);
    virtual LS3D_RESULT __stdcall Construct(I3D_scene* scene, S_vector& pos, float scale, uint32_t flags, I3D_CALLBACK callback, void* userData);
    virtual void __stdcall Destruct(uint32_t);
    virtual LS3D_RESULT __stdcall Load(int fileDescriptor);
    virtual LS3D_RESULT __stdcall Save(int fileDescriptor);
    virtual LS3D_RESULT __stdcall SetMode(uint32_t);
    virtual void __stdcall SetActiveLevel(int);
    virtual uint32_t __stdcall GetTotalLMMemory();
    virtual int __stdcall GetNumRectInfos(int, int);
    virtual void* __stdcall GetRectInfos(int, int);
    virtual void __stdcall UpdateLMFromRectInfos(int, int);
    virtual uint32_t __stdcall GetRectInfoFlags(int, int, int);

    uint8_t GetNumLevels() const {
        uint8_t numLevels = 0;
        for(size_t i = 0; i < 6; ++i) {
            if(m_sLevels[i].active) { numLevels |= 1 << i; }
        }
        return numLevels;
    }

    // TODO: Find out why the output is unreadable by LS3D

    LS3D_RESULT __stdcall CustomSave(BinaryWriter* writer) {
        if(!writer || !writer->IsOpen()) {
            debugPrintf("!! invalid or closed BinaryWriter   I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
            return I3DERR_FILESYSTEMERR;
        }

        uint8_t numLevels = GetNumLevels();

        I3D_lit_object* litObject = this;

        if(numLevels == 0) {
            debugPrintf("!! no active levels   I3D_lit_object::Save()  this:0x%x %s", this, GetName());
            return I3D_OK;
        }

        writer->WriteUInt8(numLevels);

        uint8_t activeCount = 0; // NEW: Track sequential active levels

        for(size_t levelIndex = 0; levelIndex < 6; levelIndex++) {
            const LightmapLevel* level = &m_sLevels[levelIndex];
            if(!level || !level->active) { continue; }

            activeCount++; // Increment for each active level

            if(!m_pMesh) {
                debugPrintf("!! no mesh   I3D_lit_object::Save()  this:0x%x %s", this, GetName());
                return I3DERR_NOTINITIALIZED;
            }

#pragma pack(push, 1)
            struct LevelHeader {
                uint8_t version;
                uint8_t type;
                uint32_t numLODs;
                float texelsPerUnit;
                float range;
                uint8_t level;
            } levelHeader;
#pragma pack(pop)

            levelHeader.version = 33;
            levelHeader.type = static_cast<uint8_t>(level->type);
            levelHeader.numLODs = m_pMesh->m_uNumLODs;
            levelHeader.texelsPerUnit = level->texelsPerUnit;
            levelHeader.range = level->range;
            levelHeader.level = activeCount; // CHANGED: Sequential 1-based, not levelIndex

            writer->Write(&levelHeader, sizeof(LevelHeader));

            for(uint32_t lodIndex = 0; lodIndex < m_pMesh->m_uNumLODs; ++lodIndex) {
                lod* lod = level->LODs[lodIndex];
                /*if(!lod || !lod->vertexColors || !lod->UVs || !lod->linkCoords || !lod->bitmaps || !lod->faceMaps || !lod->indexBuffer) {
                    debugPrintf("!! invalid LOD data   I3D_lit_object::CustomSave()  this:0x%x %s", this, GetName());
                    return I3DERR_NOTINITIALIZED;
                }*/

                auto meshLod = m_pMesh->GetLOD(lodIndex);

                writer->WriteUInt16(meshLod->m_uNumVertices);

                if(level->type == LM_TYPE_VERTEX) {
                    uint32_t numColors = lod->numColorsOrUVs;
                    writer->WriteUInt32(numColors);
                    if(!lod->vertexColors) {
                        debugPrintf("!! error writing data (VertColorList)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                        return I3DERR_FILECORRUPTED;
                    }
                    writer->Write(lod->vertexColors, numColors * sizeof(ColorRGBA));
                } else if(level->type == LM_TYPE_BITMAP) {
                    uint16_t numBitmaps = static_cast<uint16_t>(lod->numBitmaps);
                    uint16_t numFaceGroups = meshLod->m_uNumFGroups;

                    writer->WriteUInt16(numBitmaps);
                    writer->WriteUInt16(numFaceGroups);

                    lod::Bitmap* bitmaps = lod->bitmaps;
                    for(uint32_t i = 0; i < numBitmaps; ++i) {
                        lod::Bitmap& bitmap = bitmaps[i];

                        bool isSingleColor = bitmap.width == 3 && bitmap.height == 3;

                        writer->WriteUInt8(isSingleColor);

                        if(isSingleColor) { writer->Write((void*)&bitmap.color, sizeof(ColorRGBA)); }

                        writer->WriteUInt32(1);
                        writer->WriteUInt32(bitmap.width);
                        writer->WriteUInt32(bitmap.height);

                        if(!isSingleColor) {
                            if(!bitmap.pixels) {
                                debugPrintf("!! error writing data (RGB data)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                                return I3DERR_FILECORRUPTED;
                            }

                            writer->Write(bitmap.pixels, bitmap.size * sizeof(ColorRGB));

                            // TODO: Find out where we could delete the lightmap pixel data to prevent memory leaks

                            //delete[] bitmap.pixels;
                            //bitmap.pixels = nullptr;
                        }
                    }

                    uint32_t numUVs = lod->numColorsOrUVs;
                    writer->WriteUInt32(numUVs);

                    if(!lod->LitUVs) { // Renamed from UVs
                        debugPrintf("!! error writing data (OrigUV)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                        return I3DERR_FILECORRUPTED;
                    }

                    // Reverse the postLoadProcess adjustment to write pre-adjusted (per-bitmap) UVs
                    S_vector2* writeUVs = lod->LitUVs; // Default to existing
                    if(lod->lightMap) { // Atlas exists, so m_pLitUV is post-adjusted—reverse to per-bitmap UVs
                        // Atlas width/height from lightMap data (offsets match decomp: +40/+42 from inner ptr)
                        void* atlasData = *reinterpret_cast<void**>(lod->lightMap);
                        float atlasScaleX = 1.0f / static_cast<float>(*reinterpret_cast<uint16_t*>((uintptr_t)atlasData + 40));
                        float atlasScaleY = 1.0f / static_cast<float>(*reinterpret_cast<uint16_t*>((uintptr_t)atlasData + 42));

                        S_vector2* origUVs = new S_vector2[numUVs];
                        S_vector2* adjUV = lod->LitUVs;
                        lod::LinkCoord* lc = lod->linkCoords;
                        for(uint32_t v = 0; v < numUVs; ++v, ++adjUV, ++lc) {
                            uint32_t bi = lc->bitmapIndex;
                            if(bi >= lod->numBitmaps) {
                                debugPrintf("!! corrupted lightmap index in CustomSave() - bitmapIndex:%d >= numBitmaps:%d", bi, lod->numBitmaps);
                                delete[] origUVs;
                                return I3DERR_FILECORRUPTED;
                            }
                            lod::Bitmap& b = bitmaps[bi];

                            float f = *reinterpret_cast<float*>(&b.format);
                            float u2 = *reinterpret_cast<float*>(&b.unk2);
                            bool rotated = (f >= 2.0f);
                            if(rotated) { f -= 2.0f; }

                            float origX, origY;
                            if(rotated) {
                                origY = (adjUV->x - f - 0.5f * atlasScaleX) / ((static_cast<float>(b.height) - 1.0f) * atlasScaleX);
                                origX = (adjUV->y - u2 - 0.5f * atlasScaleY) / ((static_cast<float>(b.width) - 1.0f) * atlasScaleY);
                            } else {
                                origX = (adjUV->x - f - 0.5f * atlasScaleX) / ((static_cast<float>(b.width) - 1.0f) * atlasScaleX);
                                origY = (adjUV->y - u2 - 0.5f * atlasScaleY) / ((static_cast<float>(b.height) - 1.0f) * atlasScaleY);
                            }

                            origUVs[v].x = origX;
                            origUVs[v].y = origY;
                        }
                        writeUVs = origUVs;
                    }

                    writer->Write(writeUVs, numUVs * sizeof(S_vector2));

                    if(writeUVs != lod->LitUVs) {
                        delete[] writeUVs; // Clean up temp reversed UVs
                    }

                    if(!lod->linkCoords) {
                        debugPrintf("!! error writing data (VertInfo)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                        return I3DERR_FILECORRUPTED;
                    }

                    writer->Write(lod->linkCoords, numUVs * sizeof(lod::LinkCoord));

                    uint32_t numIndices = 3 * meshLod->NumFaces();
                    writer->WriteUInt32(numIndices);

                    if(!lod->indexBuffer) {
                        debugPrintf("!! error writing data (FaceList)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                        return I3DERR_FILECORRUPTED;
                    }

                    // NOTE: Outputs invalid indices = mesh is fucked
                    BYTE* buf = nullptr;
                    lod->indexBuffer->indexBuffer->Lock(0, 0, &buf, D3DLOCK_NOSYSLOCK);
                    buf += 2 * lod->indexBufferOffset;
                    writer->Write(buf, numIndices * sizeof(uint16_t));
                    lod->indexBuffer->indexBuffer->Unlock();

                    const lod::FaceMap* faceMaps = lod->faceMaps;
                    for(uint32_t i = 0; i < numFaceGroups; ++i) {
                        uint32_t numFaces = meshLod->GetFGroup(i)->numFaces;
                        writer->WriteUInt32(numFaces);

                        if(!faceMaps[i].indices) {
                            debugPrintf("!! error writing data (FG indexes)  I3D_lit_object::Save()  this:0x%x %s  handle:0x%x", this, GetName(), writer);
                            return I3DERR_FILECORRUPTED;
                        }

                        writer->Write(faceMaps[i].indices, numFaces * sizeof(uint16_t));
                    }
                }
            }
        }

        debugPrintf("written LM data - this:0x%x %s  handle:0x%x", this, GetName(), writer);
        return I3D_OK;
    }

  private:
    uint32_t unk;
    struct LightmapLevel {
        bool active;
        uint8_t pad[3];
        uint32_t type;
        float texelsPerUnit;
        float range;
        lod* LODs[10];
    } m_sLevels[6];
};