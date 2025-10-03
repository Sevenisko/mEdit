#pragma once

#include <string>
#include <unordered_map>
#include <LS3D.h>
#include <I3D/I3D_driver.h>
#include <I3D/I3D_model.h>

class Cache {
  public:
    static I3D_model* Open(const std::string& fileName);
    static bool Fetch(const std::string& fileName, I3D_model* model);

  private:
    static std::unordered_map<std::string, I3D_model*> g_ModelCache;
};