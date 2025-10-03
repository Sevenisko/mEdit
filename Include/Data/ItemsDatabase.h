#pragma once

#include <vector>
#include <string>

class ItemsDatabase {
  public:
    enum ItemFlags {
        ITEM_IS_LOCKED = 0x1,
        ITEM_IS_GUN = 0x2,
        ITEM_IS_MELEE = 0x4,
        ITEM_IS_BIG = 0x20,
        ITEM_IS_SNIPER = 0x40,
        ITEM_IS_EXPLOSIVE = 0xA0,
    };

    enum ItemSetting { IS_RejectCartridgeTime, IS_ReloadPause, IS_LoadCartridgeTime, IS_ByteSkip, IS_ShootDelay };

    struct ItemProperty {
        ItemSetting eType;
        int* iValue;
    };

    struct Item {
        char szInternalName[32];
        ItemFlags eFlags;
        char szModelName[32];
        uint8_t pad0[8];
        int iReloadAnimID;
        uint8_t pad1[4];
        int iShootSoundID;
        int iReloadSoundID;
        int iItemType;
        int iMagCapacity;
        int iTotalAmmo;
        uint8_t pad2[12];
        float fRangeOfFire;
        float fDamage;
        uint8_t pad3[8];
        float fRecoil;
        float fAccuracy;
        uint8_t properties[48];
    };

    bool Load(const std::string& szFilePath);
    bool Save();

    void Clear() { m_vItems.clear(); }

    size_t NumItems() const { return m_vItems.size(); }

    Item* GetItem(int id) {
        if(id < 0 || static_cast<size_t>(id) >= m_vItems.size()) return nullptr;

        return &m_vItems[id];
    }

  private:
    std::string m_szFileName;
    std::vector<Item> m_vItems;
};