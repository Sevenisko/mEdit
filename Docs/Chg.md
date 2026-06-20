# Mafia `.chg` (Differences/Change) File Format

## Overview

The `.chg` format is used by the Mafia game engine to store **scene differences** (delta changes) that modify the base mission scene at runtime. These files can add new 3D frames, attach actors to frames, create actors on existing frames, and inject scripts/programs. They are loaded by `C_game::LoadDifferencesData()` and cleaned up by `C_game::ClearDiffData()`.

Files are located in the `diff/` subdirectory relative to the game's data path.

---

## File Structure

The file uses a **chunk-based** binary format. All multi-byte values are **little-endian**.

### Chunk Header (6 bytes)

Every chunk (including the root) begins with a 6-byte header:

| Offset | Size | Type   | Description                              |
|--------|------|--------|------------------------------------------|
| 0x00   | 2    | uint16 | Chunk ID                                 |
| 0x02   | 4    | uint32 | Data size (bytes following this header)   |

### Root Chunk

| Field     | Value  | Description                  |
|-----------|--------|------------------------------|
| Chunk ID  | `1234` | Magic number / format marker |
| Data size | varies | Total size of all sub-chunks |

The root chunk (ID `1234` / `0x04D2`) wraps the entire file contents. Inside it, sub-chunks appear sequentially.

---

## Sub-Chunk Types

### Chunk 100 -- Frame Definition

Creates or modifies a 3D frame (`I3D_frame`) in the scene. This is the primary chunk type for adding geometry, lights, sounds, and models to the scene.

#### Common Frame Data

Read in the following order:

| # | Size | Type            | Description                                          |
|---|------|-----------------|------------------------------------------------------|
| 1 | 4    | uint32          | Frame type (`I3D_FRAME_TYPE` enum, see below)        |
| 2 | 12   | S_vector (3xf32)| Position (x, y, z) -- local matrix translation       |
| 3 | 12   | S_vector (3xf32)| Scale (x, y, z)                                      |
| 4 | 16   | S_quat (4xf32)  | Rotation quaternion (x, y, z, w)                     |
| 5 | 4    | uint32          | Frame name string length (N)                         |
| 6 | N    | char[N]         | Frame name (null-terminated in memory, not on disk)  |
| 7 | 4    | uint32          | Parent frame name string length (M)                  |
| 8 | M    | char[M]         | Parent frame name (null-terminated in memory)        |

**Behavior:**
- The engine first attempts `FindFrame(name)` in the current scene.
- If the frame exists **and** already has actor data attached, the chunk is **skipped**.
- If the frame exists without actor data, it is **reused** (position/scale/rotation updated).
- If the frame does not exist, a **new frame** is created via `I3DDriver::CreateFrame(type)`.
- The parent frame is found by name; if not found, the scene's primary sector is used.
- The frame is linked to its parent via `I3D_frame::LinkTo(parent)`.
- The new frame pointer is stored in `C_game::m_vDiffFrames`.

After the common data, **type-specific data** follows based on the frame type:

---

#### Frame Type-Specific Data

##### `FRAME_LIGHT` (type = 2)

| # | Size | Type            | Description                                      |
|---|------|-----------------|--------------------------------------------------|
| 1 | 4    | uint32          | Light type                                       |
| 2 | 12   | S_vector (3xf32)| Light color (r, g, b)                            |
| 3 | 4    | float           | Range                                            |
| 4 | 1    | uint8           | On/Off flag (0 = off, 1 = on)                    |
| 5 | 4    | float           | Cone angle inner                                 |
| 6 | 4    | float           | Cone angle outer                                 |
| 7 | 4    | float           | Specular exponent inner (or falloff near)        |
| 8 | 4    | float           | Specular exponent outer (or falloff far)         |
| 9 | 4    | uint32          | Light mode flags                                 |
| 10| 4    | uint32          | Sector link count (K)                            |
| 11| ...  | (repeated K times) | Sector links (see below)                      |

**Sector Link entry** (repeated K times):

| Size | Type    | Description                  |
|------|---------|------------------------------|
| 4    | uint32  | Sector name length (L)       |
| L+1  | char[]  | Sector name (null-terminated)|

After all sector links, the light calls `Update()` internally.

##### `FRAME_SOUND` (type = 4)

| # | Size | Type            | Description                                      |
|---|------|-----------------|--------------------------------------------------|
| 1 | 4    | uint32          | Sound file name length (N)                       |
| 2 | N    | char[N]         | Sound file name (null-terminated in memory)      |
| 3 | 4    | float/uint32    | Sound parameters (applied via SetVolume/similar) |
| 4 | 1    | uint8           | On/Off flag                                      |
| 5 | 4    | float           | Sound parameter 1 (e.g. frequency)               |
| 6 | 4    | float           | Sound parameter 2                                |
| 7 | 4    | float           | Sound parameter 3                                |
| 8 | 4    | float           | Sound parameter 4                                |
| 9 | 4    | float           | Range near                                       |
| 10| 4    | float           | Range far                                        |
| 11| 4    | float           | Cone angle                                       |
| 12| 4    | float           | Ambient volume                                   |
| 13| 1    | uint8           | Loop flag (0 = no loop, 1 = loop)                |
| 14| 4    | float           | Volume                                           |

The sound file is loaded through the engine's sound/animation cache system.

##### `FRAME_MODEL` (type = 9)

| # | Size | Type            | Description                                      |
|---|------|-----------------|--------------------------------------------------|
| 1 | 4    | uint32          | Model file name length (N)                       |
| 2 | N    | char[N]         | Model file name (null-terminated in memory)      |

The model is loaded from the model cache (`sModelCache`). The engine searches registered model directories for the file name. If the model is already cached, it is reused.

---

### Chunk 200 -- Actor on New Frame

Creates a new actor and attaches it to the **most recently created frame** from a preceding Chunk 100.

| # | Size | Type    | Description                                              |
|---|------|---------|----------------------------------------------------------|
| 1 | 4    | uint32  | Actor type (`EActorType` enum, see below)                |
| 2 | ...  | varies  | Actor-specific data (via `C_actor::LoadData(C_chunk*)`)  |

**Behavior:**
- Requires a valid frame from the preceding Chunk 100 (`v136`).
- **Legacy mapping:** If actor type == `2` (old `ACT_Player`), it is remapped to `ACT_Human` (`27`) and loaded via `ReadOldCharStruct()` instead of `LoadData()`.
- After loading, `C_actor::GameInit()` is called.
- The actor is added to both `C_game::m_vActors` and `C_game::m_vDiffActors`.
- The actor is registered with `C_mission::AddActor()`.

---

### Chunk 400 -- Actor on Existing Frame

Creates a new actor and attaches it to an **already existing frame** found by name.

| # | Size | Type    | Description                                    |
|---|------|---------|------------------------------------------------|
| 1 | 4    | uint32  | Frame name string length (N)                   |
| 2 | N    | char[N] | Frame name to search for                       |
| 3 | 4    | uint32  | Actor type (`EActorType` enum)                 |

**Behavior:**
- Finds the frame by name via `I3D_scene::FindFrame(name)`.
- If the frame is **not found** or **already has actor data**, the chunk is skipped.
- Creates actor via `C_mission::CreateActor(type)`.
- Calls `C_actor::Init(frame)` to bind the actor to the frame.
- If init succeeds: calls `GameInit()`, adds to actor lists, adds to `m_vDiffActors`.
- Sets actor's `m_IsInited` flag.

---

### Chunk 500 -- Script / Program

Loads one or more script programs to run as part of the difference data.

**Structure:** This chunk contains one or more sub-entries, each read in a loop while `C_chunk::HasRemainingData()` returns non-zero:

For each program:
1. A `C_program` object is allocated (104 bytes).
2. `C_program::LoadSourceCode(C_chunk*)` reads the script bytecode from the chunk.
3. `C_program::Init()` initializes the program.
4. The program is added to `C_game::m_vDiffPrograms` (which shares the `m_vPrograms` vector).

---

## Enumerations

### I3D_FRAME_TYPE

| Value | Name            | Description            |
|-------|-----------------|------------------------|
| 0     | FRAME_NULL      | Null frame             |
| 1     | FRAME_VISUAL    | Visual/mesh object     |
| 2     | FRAME_LIGHT     | Light source           |
| 3     | FRAME_CAMERA    | Camera                 |
| 4     | FRAME_SOUND     | Sound emitter          |
| 5     | FRAME_SECTOR    | Scene sector           |
| 6     | FRAME_DUMMY     | Dummy/helper frame     |
| 7     | FRAME_TARGET    | Target frame           |
| 8     | FRAME_USER      | User frame             |
| 9     | FRAME_MODEL     | Model container        |
| 10    | FRAME_JOINT     | Joint                  |
| 11    | FRAME_VOLUME    | Volume                 |
| 12    | FRAME_OCCLUDER  | Occluder               |
| 13    | FRAME_SCENE     | Scene root             |
| 14    | FRAME_AREA      | Area                   |
| 15    | FRAME_SHADOW    | Shadow                 |
| 16    | FRAME_LANDSCAPE | Landscape              |
| 17    | FRAME_EMITOR    | Particle emitter       |

### EActorType

| Value | Name             | C++ Class               |
|-------|------------------|--------------------------|
| 1     | ACT_Hidden       | C_actor                  |
| 2     | ACT_Player       | *(legacy, remapped to 27)* |
| 3     | ACT_HumanBase    | --                       |
| 4     | ACT_Car          | C_car                    |
| 5     | ACT_Script       | C_detector               |
| 6     | ACT_Door         | C_door                   |
| 8     | ACT_Railway      | C_railway                |
| 9     | ACT_VillaObject  | C_villa                  |
| 10    | ACT_Bottle       | C_bottle                 |
| 12    | ACT_Traffic      | C_traffic_car            |
| 18    | ACT_Pedestrians  | C_traffic_generator      |
| 20    | ACT_Bridge       | C_bridge                 |
| 21    | ACT_Dog          | C_dog                    |
| 22    | ACT_Airplane     | C_airplane               |
| 24    | ACT_Turnout      | C_turnout                |
| 25    | ACT_Pumper       | C_pumper                 |
| 27    | ACT_Human        | C_entity (C_human)       |
| 28    | ACT_RaceCamera   | C_racecam                |
| 30    | ACT_Wagon        | C_wagon                  |
| 31    | ACT_Irenka       | C_irenka                 |
| 32    | ACT_PublicPhysics| C_public_physics         |
| 33    | ACT_Shot         | C_actor_shot             |
| 34    | ACT_Clock        | C_clock                  |
| 35    | ACT_Physical     | C_box_new                |
| 36    | ACT_Truck        | C_car_ex                 |
| 37    | ACT_ItemDrop     | --                       |
| 38    | ACT_RailGenerator| C_rail_generator         |

---

## C_game Class -- Relevant Members

Based on reverse engineering of the constructor (`0x5472F0`), destructor (`0x547A60`), `LoadDifferencesData`, `ClearDiffData`, and `SaveGameSave`:

| Offset   | Size | Type                     | Name                 | Description                                      |
|----------|------|--------------------------|----------------------|--------------------------------------------------|
| +0x0000  | 68   | pad                      | `pad0`               | Scene tracking array (52-byte entries per frame)  |
| +0x0044  | 4    | C_mission*               | `m_pMission`         | Pointer to the mission object                    |
| +0x0048  | 4    | int                      | `m_iState`           | Game state                                       |
| +0x004C  | 148  | G_Camera                 | `m_sCamera`          | Camera controller                                |
| +0x00E0  | 8    | pad                      | `pad2`               | Unknown (contains ptr at +4 used as "human ref") |
| +0x00E8  | 4    | C_human*                 | `m_pHuman`           | Player's human actor                             |
| +0x00EC  | 16   | vector\<C_actor*\>       | `m_vActors`          | All active actors in the game                    |
| +0x00FC  | 16   | vector\<C_actor*\>       | `m_vActors2`         | Secondary actor list (offset 60\*4=240)          |
| +0x010C  | 16   | vector\<C_actor*\>       | `m_vActors3`         | Tertiary actor list (offset 64\*4=256)           |
| +0x011C  | 16   | vector\<C_actor*\>       | `m_vActors4`         | Actor list (offset 73\*4=292)                    |
| +0x0130  | 16   | vector\<C_actor*\>       | `m_vTempActors`      | Temporary actors (spawned/despawned dynamically) |
| +0x0140  | 10412| pad                      | `pad4`               | Large block: fires, shadows, sounds, etc.        |
| +0x29EC  | 32   | C_using_object           | `m_sUsingObject`     | Interactive object tracking                      |
| +0x2ADC  | 16   | vector\<I3D_frame*\>     | **`m_vDiffFrames`**  | Frames created by LoadDifferencesData            |
| +0x2AEC  | 16   | vector\<C_actor*\>       | **`m_vDiffActors`**  | Actors created by LoadDifferencesData            |
| +0x2AFC  | 16   | vector\<C_program*\>     | `m_vPrograms`        | Active script programs (diff programs added here too) |
| +0x2B0C  | 4    | int                      | `m_iUnk`             | Unknown value (saved in savegame byte 0)         |
| +0x2B10  | 656  | pad                      | `pad6`               | C_sledovacka, traffic, shift, etc.               |
| +0x2B18  | --   | C_sledovacka             | `m_sSledovacka`      | Camera follow system (at offset +0x2B18=11032)   |
| +0x2DA0  | 4    | void*                    | `m_pSchvestky`       | Schvestky subsystem pointer                      |
| +0x363E  | 1    | bool                     | `m_bUpdateScore`     | Score update flag                                |
| +0x3642  | 1    | bool                     | `m_bScoreOn`         | Score display on/off                             |
| +0x3644  | 4    | int                      | `m_iGameScore`       | Current game score                               |

**Total struct size: 14016 bytes (0x36C0)**

### Key Difference-Related Vectors

| Vector            | Byte Offset | DWORD Index | Description                                    |
|-------------------|-------------|-------------|------------------------------------------------|
| `m_vDiffFrames`   | +0x2ADC     | [2743]      | I3D_frame pointers created by diff loading     |
| `m_vDiffActors`   | +0x2AEC     | [2747]      | C_actor pointers created by diff loading       |
| `m_vPrograms`     | +0x2AFC     | [2751]      | Shared: global programs + diff programs        |

Each vector follows the standard layout:
```
+0x00: allocator (4 bytes, typically unused)
+0x04: first     (4 bytes, pointer to first element)
+0x08: last      (4 bytes, pointer past last element)
+0x0C: end       (4 bytes, pointer to end of allocated storage)
```

---

## C_game Methods -- Reversed Names

| Address    | Mangled Name                                      | Clean Signature                                              |
|------------|---------------------------------------------------|--------------------------------------------------------------|
| 0x5472F0   | `__ct__6C_gameFv`                                 | `C_game::C_game()`                                           |
| 0x547A60   | `__dt__6C_gameFv`                                 | `C_game::~C_game()`                                          |
| 0x5A0810   | `Init__6C_gameF`                                  | `C_game::Init()`                                             |
| 0x5A3C60   | `Done__6C_gameFv`                                 | `C_game::Done()`                                             |
| 0x5A51C0   | `Tick__6C_gameFUi`                                | `C_game::Tick(uint32_t dt)`                                  |
| 0x5AE480   | `LoadDifferencesData__6C_gameFPCc`                | `C_game::LoadDifferencesData(const char *filename)`          |
| 0x5AF860   | `ClearDiffData__6C_gameFv`                        | `C_game::ClearDiffData()`                                    |
| 0x5AF840   | `GetOrAllocFrameUserData__6C_gameFP9I3D_frame`    | `C_game::GetOrAllocFrameUserData(I3D_frame *frame)`          |
| 0x5AFCA0   | `UnregisterFrameFromTracking__6C_gameFP9I3D_frame`| `C_game::UnregisterFrameFromTracking(I3D_frame *frame)`      |
| 0x5AFE40   | `InvalidateActor__6C_gameFP7C_actorb`             | `C_game::InvalidateActor(C_actor *actor, bool skipPrograms)` |
| 0x5AFD50   | `GlobalProgramRun__6C_gameFPc`                    | `C_game::GlobalProgramRun(char *progName)`                   |
| 0x5B47D0   | `SaveGameGetSize__6C_gameFb`                      | `C_game::SaveGameGetSize(bool full)`                         |
| 0x5B4920   | `SaveGameSave__6C_gameFPPvb`                      | `C_game::SaveGameSave(void **buf, bool full)`                |
| 0x5A07E0   | `SetHuman__6C_gameFP7C_actor`                     | `C_game::SetHuman(C_actor *human)`                           |
| 0x5A77C0   | `AddTemporaryActor__6C_gameFP7C_actor`            | `C_game::AddTemporaryActor(C_actor *actor)`                  |
| 0x5A79A0   | `RemoveTemporaryActor__6C_gameFP7C_actor`         | `C_game::RemoveTemporaryActor(C_actor *actor)`               |
| 0x5ADD10   | `LoadSemaphores__6C_gameFv`                       | `C_game::LoadSemaphores()`                                   |
| 0x5ADEE0   | `LoadCityMusicAreas__6C_gameFv`                   | `C_game::LoadCityMusicAreas()`                               |
| 0x5B69D0   | `LoadRoadBlocks__6C_gameFv`                       | `C_game::LoadRoadBlocks()`                                   |
| 0x5B6B10   | `LoadTaxiPoints__6C_gameFv`                       | `C_game::LoadTaxiPoints()`                                   |
| 0x5B6E00   | `LoadTelephones__6C_gameFv`                       | `C_game::LoadTelephones()`                                   |
| 0x5B90B0   | `LoadScreamAreas__6C_gameFv`                      | `C_game::LoadScreamAreas()`                                  |

---

## ClearDiffData Cleanup Process

When `C_game::ClearDiffData()` is called, it performs the following in order:

1. **Remove diff actors** (`m_vDiffActors`):
   - For each actor: removes it from `m_vActors`, `m_vActors2`, `m_vActors3`, and `m_vActors4`.
   - Calls `InvalidateActor()` to clean up all references.
   - Calls actor's virtual `Done()` method.
   - Calls `C_mission::DelActor()` to unregister from the mission.

2. **Remove diff frames** (`m_vDiffFrames`):
   - For each frame: checks if it has attached actor data.
   - If actor data exists: removes actor from all lists, calls `GameDone()`, `DelActor()`.
   - Calls `UnregisterFrameFromTracking()` to remove from scene tracking.
   - Handles special case for `FRAME_LIGHT` (type 2): removes light from linked sectors.
   - Calls `I3D_scene::RemoveFrame()` to remove from the 3D scene.

3. **Remove diff programs** (`m_vPrograms`):
   - For each program added by diff: calls `C_program::Done()`.
   - Deletes the program object.

---

## Binary Layout Example

```
File: diff\example.chg

[Root Chunk Header]
  04 D2              Chunk ID = 1234 (magic)
  XX XX XX XX        Data size

  [Sub-chunk: Frame]
    00 64            Chunk ID = 100
    XX XX XX XX      Data size

    09 00 00 00      Frame type = FRAME_MODEL (9)
    XX XX XX XX      Position X (float)
    XX XX XX XX      Position Y (float)
    XX XX XX XX      Position Z (float)
    XX XX XX XX      Scale X (float)
    XX XX XX XX      Scale Y (float)
    XX XX XX XX      Scale Z (float)
    XX XX XX XX      Rotation quat X (float)
    XX XX XX XX      Rotation quat Y (float)
    XX XX XX XX      Rotation quat Z (float)
    XX XX XX XX      Rotation quat W (float)
    0B 00 00 00      Frame name length = 11
    6D 79 5F 6D      "my_m"
    6F 64 65 6C      "odel"
    5F 30 31         "_01"
    06 00 00 00      Parent name length = 6
    73 65 63 74      "sect"
    6F 72              "or"

    [Model-specific: model filename]
    0C 00 00 00      Model name length = 12
    6D 79 6D 6F      "mymo"
    64 65 6C 2E      "del."
    34 64 73 00      "4ds\0"

  [Sub-chunk: Actor on Frame]
    00 C8            Chunk ID = 200
    XX XX XX XX      Data size

    1B 00 00 00      Actor type = ACT_Human (27)
    [... actor-specific data via LoadData ...]

  [Sub-chunk: Actor on Existing Frame]
    01 90            Chunk ID = 400
    XX XX XX XX      Data size

    0A 00 00 00      Frame name length = 10
    [... frame name ...]
    06 00 00 00      Actor type = ACT_Door (6)

  [Sub-chunk: Script]
    01 F4            Chunk ID = 500
    XX XX XX XX      Data size

    [... C_program source code data ...]
```

---

## Notes

- The chunk system uses a stack-based descent/ascend mechanism via `C_chunk`. Each `Descend()` call pops the current chunk end marker, effectively closing the chunk scope.
- String data in the file is **not null-terminated** on disk. The engine reads the length-prefixed string and manually null-terminates it in a stack buffer.
- The `HIBYTE(v138[0])` flag in `LoadDifferencesData` tracks whether a frame was **newly created** (1) or **reused from existing scene** (0). Newly created frames have their reference released after being linked to the scene, since the scene graph takes ownership.
- The `.chg` files are typically loaded during mission initialization and cleared on mission transitions.
