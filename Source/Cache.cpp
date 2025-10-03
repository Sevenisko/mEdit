#include <Cache.h>

std::unordered_map<std::string, I3D_model*> Cache::g_ModelCache;

I3D_model* Cache::Open(const std::string& fileName) {
    I3D_model* newModel = (I3D_model*)I3DGetDriver()->CreateFrame(FRAME_MODEL);
    if(g_ModelCache.find(fileName) != g_ModelCache.end()) {
        g_ModelCache[fileName]->AddRef();
        newModel->Duplicate(g_ModelCache[fileName]);

        return newModel;
    } else {
        LS3D_RESULT res = newModel->Open(("Models\\" + fileName).c_str());
        if(I3D_SUCCESS(res)) {
            g_ModelCache.insert({fileName, newModel});
            return newModel;
        } else {
            newModel->Release();
            return nullptr;
        }
    }
}

bool Cache::Fetch(const std::string& fileName, I3D_model* model) {
    if(!model) return false;

    if(g_ModelCache.find(fileName) == g_ModelCache.end()) {
        I3D_model* newModel = (I3D_model*)I3DGetDriver()->CreateFrame(FRAME_MODEL);
        LS3D_RESULT res = newModel->Open(("Models\\" + fileName).c_str());
        if(I3D_SUCCESS(res)) {
            g_ModelCache.insert({fileName, newModel});
        } else {
            newModel->Release();
            return false;
        }
    }

    I3D_model* cachedModel = g_ModelCache[fileName];

    //std::string name = cachedModel->GetName();
    S_vector pos = cachedModel->GetPos();
    S_quat rot = cachedModel->GetRot();
    S_vector scale = cachedModel->GetScale();

    model->Duplicate(cachedModel);
    //model->SetName(name.c_str());
    model->SetPos(pos);
    model->SetRot(rot);
    model->SetScale(scale);

    return true;
}