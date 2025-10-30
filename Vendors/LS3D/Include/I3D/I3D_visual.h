#pragma once

#include <I3D/I3D_frame.h>

struct POS_NORM {
    S_vector pos;
    S_vector normal;
};

class I3D_void_interface {
    virtual int __stdcall NumRegions();
    virtual int __stdcall SetWeight(int, int, float);
    virtual float __stdcall GetWeight(int, int);
};

class I3D_visual : public I3D_frame, public I3D_void_interface {
  public:
    virtual ~I3D_visual() {}
    virtual void UpdateV() override;
    virtual void __stdcall SetSpace(bool);
    virtual void __stdcall SetShadowMode(int mode);
    virtual int __stdcall GetShadowMode();
    virtual void __stdcall SetProjectorMode(int mode);
    virtual int __stdcall GetProjectorMode();
    virtual void _stdcall UpdateBoundProc();
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(void* fgStruct);

    inline I3D_VISUAL_TYPE GetVisualType() const {
        return *(I3D_VISUAL_TYPE*)((int)this + 0x1D4);
    }
  private:
    __declspec(dllexport) void __stdcall UpdateWBoundProc();
};

static const char* GetVisualTypeName(I3D_VISUAL_TYPE type) {
    switch(type) {
    case VISUAL_LIT_OBJECT: return "Lit Object";
    case VISUAL_PROJECTOR: return "Projector";
    case VISUAL_SINGLE_MESH: return "Single Mesh";
    case VISUAL_MORPH: return "Morph";
    case VISUAL_PART_ELEMENT_BASE: return "Particle Element Base";
    case VISUAL_BILLBOARD: return "Billboard";
    case VISUAL_PART_ELEMENT: return "Particle Element";
    case VISUAL_MIRROR: return "Mirror";
    case VISUAL_LAND_PATCH: return "Land Patch";
    case VISUAL_SINGLE_MORPH: return "Single Morph";
    case VISUAL_LENSFLARE: return "Lens Flare";
    case VISUAL_OBJECT: return "Object";
    default: return "Unknown";
    }
}