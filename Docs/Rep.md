# Mafia 1 - Cutscene / Records System

> Reverse-engineered documentation of the `C_records` class and the `.rep` replay format.
> Based on analysis of the game executable (LS3D engine, MSVC x86).

---

## 1. Overview

Mafia uses a **record/replay system** (`C_records`) for in-game cutscenes. The system captures and plays back actor transforms (position + rotation), camera movements, script events, dialog lines, and video overlays. Everything is driven by a single `.rep` binary file per cutscene.

The global singleton `g_pRecordsManager` (address `0x647900`) owns the entire lifecycle: loading, playback, serialization, and teardown.

---

## 2. File Format (`.rep`)

### 2.1 Header (96 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `magicByte` | Must be **5681** (`0x1631`) |
| 0x04 | 4 | `sizeOfAnimationBlocks` | Byte size of the animation name table |
| 0x08 | 4 | `sizeOfObjectDefinitionsSection` | Byte size of the raw frame-data blob |
| 0x0C | 4 | `countOfObjectDefinitionBlocks` | Number of 108-byte object definition blocks |
| 0x10 | 4 | `fixedCCSequence` | Unknown / reserved |
| 0x14 | 4 | `countOfCameraChunks` | Number of camera position keyframes |
| 0x18 | 4 | `countOfCameraFocusChunks` | Number of camera focus-target keyframes |
| 0x1C | 4 | `sizeOfScriptEventsSequence` | Byte size of the script/fade/wav section |
| 0x20 | 4 | `sizeOfDialogSection` | Byte size of the dialog section |
| 0x24 | 60 | `padding` | Zeroed / unused |

### 2.2 Section Layout (sequential after header)

```
[Header 96B]
[Animation Block Data]          <- sizeOfAnimationBlocks bytes
[Object Definition Blocks]      <- 108 * countOfObjectDefinitionBlocks bytes
[Object Definition Data]        <- sizeOfObjectDefinitionsSection bytes (raw frame data)
[Camera Chunks]                 <- 64 * countOfCameraChunks bytes
[Camera Focus Chunks]           <- 56 * countOfCameraFocusChunks bytes
[Script Events Section]         <- sizeOfScriptEventsSequence bytes
[Dialog Section]                <- sizeOfDialogSection bytes
```

### 2.3 Animation Block Data

Starts immediately after the header. Contains an animation name lookup table.

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `countOfAnimationBlocks` |
| 0x04 | 4 | `unknown` |
| 0x08 | 52 * count | Array of `AnimationBlock` entries |

Each **AnimationBlock** (52 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `animationID` |
| 0x04 | 48 | `animationName` (null-terminated string) |

### 2.4 Object Definition Block (108 bytes each)

Each block describes one recorded actor/object. The game matches these to live actors by name.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 36 | `Name` | Actor/object name (lookup key) |
| 0x24 | 36 | `NameTypeFlags` | Actor type string (e.g. `"car"`, `"hum"`, `"act"`) |
| 0x48 | 8 | `chkData[0]` | Type identification hash (uint64) |
| 0x50 | 8 | `chkData[1]` | Type identification hash (uint64) |
| 0x58 | 4 | `chkDataEmpty` | Stored but not used for type matching |
| 0x5C | 4 | `timeActivation` | Tick/time when this object becomes active |
| 0x60 | 4 | `definitionID` | Must match the actor's internal type ID |
| 0x64 | 4 | `size` | Size of this object's frame data in the data blob |
| 0x68 | 4 | `offset` | Byte offset into the Object Definition Data section |

**Type identification** uses `chkData` hashes, not string comparison on `NameTypeFlags`:

| chkData[0] | chkData[1] | Type |
|------------|------------|------|
| `188978561032` | `240518168632` | **Car** |
| `171798691848` | `240518168620` | **Human** |
| `395136991240` | `0` | **Actor** (generic) |
| `68719476744` | `0` | **Unknown** |

### 2.5 Frame Data Types

Frame data is stored as sequential keyframes. The structure varies by actor type.

**Human Type A** (`type == 1`, 28 bytes per frame):

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 12 | `position` (Vector3: x, y, z) |
| 0x0C | 16 | `rotation` (Quaternion: w, x, y, z) |
| 0x1C | 4 | `animationID` |

**Human Type B** (`type == 2`, 32 bytes per frame):

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 12 | `position` (Vector3) |
| 0x0C | 16 | `rotation` (Quaternion) |
| 0x1C | 4 | `animationID` |
| 0x20 | 4 | `animationOffset` (keyframe offset: `value & 0xFFF`) |

**Car** (44 bytes per frame):

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `frameA` |
| 0x04 | 4 | `type` |
| 0x08 | 12 | `position` (Vector3) |
| 0x14 | 16 | `rotation` (Quaternion) |
| 0x24 | 4 | `unk1` |
| 0x28 | 4 | `unk2` |

**Actor (generic)** (72 bytes per frame):

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `frameA` |
| 0x04 | 4 | `type` |
| 0x08 | 12 | `position` (Vector3) |
| 0x14 | 16 | `rotation` (Quaternion) |
| 0x1C | 24 | `unk1..unk6` (6 floats) |
| 0x34 | 32 | `unk[8]` (8 uint32s) |

Each frame in the Human types starts with a sub-header (`frameA`, `type`) that selects between Type A and Type B dynamically per keyframe.

### 2.6 Camera Chunk (64 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `frameA` |
| 0x04 | 4 | `frameB` |
| 0x08 | 4 | `unkType` |
| 0x0C | 12 | `position` (x, y, z) |
| 0x18 | 12 | `curve` (curveX, curveY, curveZ) |
| 0x24 | 12 | `subCurve` (subCurveX, subCurveY, subCurveZ) |
| 0x30 | 4 | `rollView` |
| 0x34 | 4 | `FOV` |
| 0x38 | 8 | `unk1` |

### 2.7 Camera Focus Chunk (56 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `frameA_Focus` |
| 0x04 | 4 | `frameB_Focus` |
| 0x08 | 4 | `unkType` |
| 0x0C | 12 | `position` (x, y, z) |
| 0x18 | 12 | `curve` |
| 0x24 | 12 | `subCurve` |
| 0x30 | 8 | `unk2` |

### 2.8 Script Events Section

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `unkBlockSize` |
| 0x04 | 4 | `zatmizeBlockSize` |
| 0x08 | 4 | `scriptsSize` |
| 0x0C | 4 | `wavSize` |

Followed by:

**ZatmizeData** (32 bytes each, count = `zatmizeBlockSize / 32`) - screen fade events:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `unk1` |
| 0x04 | 4 | `fadeA` (float) |
| 0x08 | 4 | `fadeB` (float) |
| 0x0C | 4 | `unk2` |
| 0x10 | 16 | `data` |

**ScriptsData** (40 bytes each, count = `scriptsSize / 40`) - timed script triggers:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `time` (activation tick) |
| 0x04 | 36 | `name` (script name, null-terminated) |

**WavData** (40 bytes each, count = `wavSize / 40`) - timed sound events:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `time` (activation tick) |
| 0x04 | 4 | `state` |
| 0x08 | 32 | `name` (wav filename, null-terminated) |

### 2.9 Dialog Section

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `count` (number of PreLastObject entries) |
| 0x04 | 4 | `unk` |
| 0x08 | 4 | `size` (number of LastObject entries) |

**PreLastObject** (36 bytes each) - dialog line triggers:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `time` |
| 0x04 | 4 | `charID` (character/speaker ID) |
| 0x08 | 4 | `stringID` (dialog string table index) |
| 0x0C | 24 | `name` |

**LastObject** (40 bytes each) - additional timed entries:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `timeA` |
| 0x04 | 4 | `timeB` |
| 0x08 | 32 | `name` |
| 0x28 | 1 | `unk0` |
| 0x29 | 1 | `unk1` |
| 0x2A | 2 | `unk2` |

---

## 3. Class Architecture

### 3.1 C_records (global singleton at `0x647900`)

The object is ~300 bytes and contains two embedded sub-objects:

```
C_records (vtable: off_625AD8)
+0x000  vtable*
+0x004  ... state/flag fields ...
+0x008  activeActors[]       (vector<void*>, begin/end pointers)
+0x014  pausedActors[]       (vector<void*>, begin/end pointers)
+0x038  ... internal data ...
+0x044  cameraChunkData*     (allocated on Open)
+0x048  cameraFocusData*     (allocated on Open)
+0x04C  ... camera playback state (indices, counts) ...
+0x068  ... script event pointers ...
+0x074  actorStatesBackup*   (for Unload restore)
+0x078  scriptEventsData*
+0x07C  dialogData*
+0x080  C_record_actors      (sub-object, vtable: off_625AC0)
+0x090  C_record_defs        (sub-object, definition manager)
+0x0E4  playbackState        (0=stop, 1=play, 2=record)
+0x0E8  prevPlaybackState
+0x128  loadedFlag
+0x129  readyFlag
```

### 3.2 C_record_actors (sub-object at +128)

Manages the actor lookup table. Each entry is 56 bytes: `[4B handle][48B name][4B extra]`.

```
+0x00  vtable* (off_625AC0)
+0x04  actorCount
+0x08  actorEntries*        (56 bytes per entry)
+0x0C  capacity
```

### 3.3 C_record_defs (sub-object at +144)

Manages record definition entries (what data is bound to which actor).

```
+0x00  dataBlock*           (master data pointer)
+0x08  definitions[]        (vector of 88-byte entries, begin ptr)
+0x0C  definitions_end      (end ptr)
+0x18  actorEntries[]       (vector of 40-byte entries, begin ptr)
+0x1C  actorEntries_end     (end ptr)
```

**88-byte definition entry** layout:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 4 | `state` (0 = needs load, 1 = loaded/active) |
| 0x04 | 4 | `typeID` (matches `definitionID` from file) |
| 0x08 | 36 | `actorTypeName` (secondary lookup key) |
| 0x2C | 36 | `objectName` (primary lookup key, matched via strcmp) |
| 0x50 | 4 | `actorPtr` (pointer to live game actor, or 0) |
| 0x54 | 4 | `recordDataPtr` (pointer to 92-byte record data object) |

### 3.4 Record Data Object (92 bytes, 0x5C)

Allocated per matched actor. Holds the frame data pointer and playback cursor.

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 20 | Control state (5 dwords, initialized to 0) |
| 0x14 | 4 | `definitionID` |
| 0x18 | 4 | `frameDataPtr` (pointer to raw frame data) |
| 0x1C | 4 | `playbackCursor` (current read position, starts = frameDataPtr) |
| 0x20 | 32 | `name` (object name string) |
| 0x44 | 16 | `chkData` (type identification, copied from ObjectDefinitionBlock) |
| 0x54 | 4 | `timeActivation` |

---

## 4. Lifecycle & Data Flow

### 4.1 Initialization (`0x59CC30`)

Called once at startup. Constructs the global `C_records` object:
1. Initializes the `C_record_actors` sub-object at +128
2. Initializes the `C_record_defs` sub-object at +144
3. Sets vtable to `off_625AD8`
4. Sets initial playback state to 4 (idle)
5. Zeroes all section pointers

### 4.2 Actor Registration

Before a cutscene can play, actors must be registered:

1. **`AddActor(name, actorPtr)`** (`0x59C630`) — Adds a 40-byte entry `{name[36], actorPtr}` to the definition manager's actor list. This maps a string name to a live game object.

2. **`RegisterActor(actorPtr)`** (`0x598FB0`) — Adds the actor pointer to the manager's active actor vector. Sets the actor's flags (`|= 0x10`), disables its AI visibility callback, and enables its recording callback.

### 4.3 Loading — Full Open (`0x59A3C0`)

This is the primary cutscene loading path:

```
Open(filename)
  |
  +-- dtaOpen(filename)
  +-- Read 96-byte header, verify magic == 5681
  +-- ReleaseAll() on existing definitions
  +-- Read animation block data (via actor table sub-object)
  +-- Allocate & read:
  |     - Object definition blocks (108B * count)
  |     - Object definition data section (raw frame data)
  |     - Camera chunks (64B * count)
  |     - Script events section
  |     - Dialog section
  +-- For each ObjectDefinitionBlock:
  |     +-- If name == "<none>" -> create unbound record
  |     +-- Else -> match against registered actors by name
  |     +-- On match: allocate 92B record data object
  |     +--           copy frame data pointer + chkData + timing
  |     +--           call RegisterDefinition()
  +-- Setup camera chunk pointers (position, focus, counts)
  +-- Setup dialog data (allocate + copy sub-sections)
  +-- Save initial actor states:
  |     For each registered actor of type MODEL(6):
  |       Save position (Vector3) + rotation (Quaternion)
  |     For each actor of type HUMAN(3) or PHYSICAL(27):
  |       Record reference for scale reset on unload
  +-- Set dword_647A58 = 5 (global "cutscene active" flag)
  +-- SetPlaybackState(0) then SetPlaybackState(1) -> start playback
  +-- Hide HUD indicators, disable player control
```

### 4.4 Loading — Definitions Only (`0x59C640` -> `0x59DCD0`)

Used when adding definitions to an already-running session:

```
LoadFromFile(filename)
  |
  +-- dtaOpen, read header, verify magic
  +-- dtaSeek past animation blocks
  +-- Read object definition blocks + data section only
  +-- Match each block against existing actor entries by:
  |     1. strcmp(actorEntry.name, block.Name)
  |     2. Check block.definitionID == actor's internal typeID
  +-- Create 92B record data objects for matches
  +-- RegisterDefinition() for each
  +-- ClearDefinitions() (reset playback cursors)
  +-- SetPlaybackState(1) -> resume
```

### 4.5 Playback State Machine (`0x5989A0`)

```
SetPlaybackState(newState):
  previousState = currentState
  currentState = newState

  switch(newState):
    case 0 (STOP):
      For all active actors:  clear flag 0x10, disable recording callback, disable playback callback
      For all paused actors:  clear flag 0x10, disable recording callback
      loadedFlag = 0

    case 1 (PLAY):
      For all active actors:  set flag 0x10, disable playback, enable recording callback
      For all paused actors:  set flag 0x10, enable recording callback

    case 2 (RECORD):
      For all active actors:  clear flag 0x10, disable recording, enable playback callback
      For all paused actors:  set flag 0x10, enable recording callback
```

The actor flag `0x10` controls whether the actor is under cutscene control (hidden from normal gameplay logic).

### 4.6 Playback Tick

During active playback, each frame:

1. **Frame data consumption**: For each definition entry with `state == 1`, the `playbackCursor` in the 92-byte record data object advances through the frame data buffer. The current keyframe's `position` and `rotation` are read.

2. **Actor transform application**: The position is written to the actor's I3D_frame (`SetPos`), and the quaternion rotation is normalized and applied (`SetRot`). The frame's dirty flags are set (`|= 0x40000000`) to force a world matrix recalculation.

3. **Camera update** (`0x59E1F0`): If a video overlay (`IShow`) is active, it's synchronized. Camera chunks provide per-frame camera position, rotation curves, FOV, and roll values.

4. **Script events**: Timed script triggers fire when the current playback tick matches the event's `time` field. This includes:
   - **Zatmize (fade)** events — screen fade in/out
   - **Script** events — trigger named game scripts
   - **Wav** events — play sound effects
   - **Dialog** events — trigger `DabingSpeak()` for character voice lines with subtitle IDs

### 4.7 Unload (`0x59C6A0`)

Restores the game state after a cutscene ends:

```
Unload()
  |
  +-- ReleaseRecordData() - destroy all 92B record data objects
  +-- Clear actor table sub-object
  +-- SetPlaybackState(0) then SetPlaybackState(4) -> fully stop
  +-- ReleaseAll() definitions
  +-- CallVtable[10](this) -> cleanup callback
  +-- Free camera chunk data, script events, dialog data
  +-- Zero all section pointers
  +-- Restore saved actor states:
  |     For each backed-up actor of type MODEL(6):
  |       Restore position + rotation from saved values
  |       Normalize quaternion, set dirty flags
  |       Call Update() on the frame
  |     For each HUMAN(3) / PHYSICAL(27):
  |       Reset scale to 1.0
  +-- Free the backup state array
  +-- Re-enable player control, show HUD
  +-- Set loadedFlag = 1
```

### 4.8 Serialization

The system supports saving/loading the current state to memory buffers (for savegames):

- **`GetSaveSize()`** (`0x59B210`) — Calculates the byte size needed. Only includes definitions with `actorTypeName == "<NONE>"` (unbound records).

- **`SaveToBuffer(buffer)`** (`0x59B2A0`) — Writes the count of saveable definitions, then for each: size of frame data, object name (36B), chkData fields, definitionID, typeID, and the raw frame data bytes.

- **`LoadFromBuffer(buffer)`** (`0x59B430`) — Reverses the above: reads definitions from the buffer, reconstructs 92-byte record data objects, and calls `RegisterDefinition()` for each.

- **`SaveToFile(path)`** / **`SaveToFileCompact(path)`** (`0x59B570` / `0x59B5B0`) — Write to disk via `fopen("wb")`, delegating to the actor table sub-object's serialization method and a vtable-dispatched write method.

---

## 5. Key Functions Reference

| Address | Name | Description |
|---------|------|-------------|
| `0x59CC30` | `C_records::Init` | Global constructor, initializes singleton |
| `0x59A3C0` | `C_records::Open` | Full .rep load + start playback |
| `0x59C640` | `C_records::LoadFromFile` | Load object definitions only |
| `0x59C6A0` | `C_records::Unload` | Stop + restore + cleanup |
| `0x59C620` | `C_records::Clear` | Clear all definitions |
| `0x59C630` | `C_records::AddActor` | Register an actor by name |
| `0x59E2D0` | `C_records::IsPlaying` | Check if cutscene is active |
| `0x5989A0` | `C_records::SetPlaybackState` | State machine (0/1/2/4) |
| `0x598FB0` | `C_records::RegisterActor` | Add actor to active list |
| `0x59C650` | `C_records::DetachActor` | Remove actor from definitions |
| `0x59C660` | `C_records::AttachActor` | Bind actor to definition by name |
| `0x59C580` | `C_records::FindActorIndex` | Lookup actor handle by name |
| `0x59C5C0` | `C_records::GetActorData` | Get actor entry by index |
| `0x59C5D0` | `C_records::GetActorRecord` | Get actor's last field by handle |
| `0x59B210` | `C_records::GetSaveSize` | Calculate savegame buffer size |
| `0x59B2A0` | `C_records::SaveToBuffer` | Serialize to memory |
| `0x59B430` | `C_records::LoadFromBuffer` | Deserialize from memory |
| `0x59B570` | `C_records::SaveToFile` | Serialize to disk |
| `0x59B5B0` | `C_records::SaveToFileCompact` | Serialize to disk (compact) |
| `0x59C4E0` | `C_records::ReleaseRecordData` | Free all record data objects |
| `0x5993D0` | `C_records::CollectActors` | Build combined actor list |
| `0x5991E0` | `C_records::IsActorRegistered` | Check if actor is in any list |
| `0x599240` | `C_records::SetActorVisibility` | Move actor between active/paused |
| `0x599370` | `C_records::IsActorHidden` | Check if actor is in paused list |
| `0x59E020` | `C_records::CreateAnimation` | Load animation set by index |
| `0x59E110` | `C_records::SetIndicators` | Configure HUD indicator state |
| `0x59E140` | `C_records::PlayDialog` | Trigger voice line (DabingSpeak) |
| `0x59E170` | `C_records::StopDialog` | Stop active voice playback |
| `0x59E180` | `C_records::SetupVideo` | Create IShow video overlay |
| `0x59E1E0` | `C_records::SetVideoTime` | Set video playback start time |
| `0x59E1F0` | `C_records::UpdateVideo` | Sync video to cutscene tick |
| `0x59CA90` | `C_records::SetupDialogData` | Allocate + copy dialog sub-sections |
| `0x59C170` | `C_records::LoadAnimation` | Load .5ds animation for actor |
| `0x59C2F0` | `C_records::InitActorFrames` | Initialize frame references |
| `0x59A240` | `C_records::ResetPlayback` | Reset playback cursor for an actor |
| `0x59D740` | `C_record_defs::RegisterDefinition` | Insert/update definition entry |
| `0x59D0E0` | `C_record_defs::FindByName` | Lookup definition by object name |
| `0x59D4D0` | `C_record_defs::Clear` | Reset definition array (end = begin) |
| `0x59D2C0` | `C_record_defs::ReleaseAll` | Free all record data + reset |
| `0x59DCD0` | `C_record_defs::LoadFromFile` | Parse definitions from .rep file |
| `0x59DF40` | `C_record_defs::DetachActor` | Unbind actor from definitions |
| `0x59DFB0` | `C_record_defs::AttachActor` | Bind actor to matching definition |
| `0x59B740` | `C_record_actors::FindByName` | Lookup actor index by name string |
| `0x59B7C0` | `C_record_actors::GetByIndex` | Get actor entry data by index |
| `0x59BB50` | `C_record_actors::Clear` | Free actor entry array |
| `0x59B890` | `C_record_actors::Reset` | Reset actor table state |
| `0x59BAB0` | `C_record_actors::ReadFromFile` | Read actor table from file handle |
| `0x59B5F0` | `C_record_actors::ctor` | Constructor |
| `0x59CF70` | `C_record_defs::ctor` | Constructor |
| `0x5C25E0` | `C_record_data::ResetCursor` | Set playback cursor to null |
| `0x5C25F0` | `C_record_data::GetDataSize` | Calculate consumed data size |

---

## 6. Global State

| Address | Name | Description |
|---------|------|-------------|
| `0x647900` | `g_pRecordsManager` | Pointer to global C_records singleton |
| `0x647A4C` | `g_pActorStateBackup` | Saved actor positions/rotations for restore on Unload |
| `0x647A50` | `g_pVideoOverlay` | Active IShow video overlay instance |
| `0x647A54` | `g_videoStartTime` | Video playback start timestamp |
| `0x647A58` | `g_cutsceneActiveFlag` | Non-zero while a cutscene is playing (set to 5 on Open) |

---

## 7. Integration with Game Systems

- **Actor system**: Actors under cutscene control have flag `0x10` set, which disables their normal AI/physics tick. Their transforms are driven entirely by the record data.

- **Camera**: The game camera is overridden by camera chunks during playback. Position is interpolated using curve/subCurve spline data. FOV and roll are applied per-chunk.

- **Dialog/Dabing**: Voice lines are triggered via `DabingSpeak(charID, stringID, 1, 3)` at the specified tick. The dabing system handles lip-sync and subtitle display.

- **HUD**: Indicators are hidden during cutscenes (`g_pIndicators.m_uFlags` bit manipulation). Player input is disabled via `sub_5B7ED0(game, 1, 1)`.

- **Savegames**: Only "unbound" definitions (those with actor type name `"<NONE>"`) are serialized to savegame buffers. Bound definitions are reconstructed from the .rep file on load.
