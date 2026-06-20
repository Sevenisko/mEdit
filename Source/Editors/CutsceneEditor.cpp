#include <Editors/CutsceneEditor.h>
#include <IO/BinaryReader.hpp>
#include <IO/ChgFile.h>
#include <I3D/I3D_model.h>
#include <I3D/I3D_light.h>
#include <I3D/I3D_sound.h>
#include <I3D/I3D_sector.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <FA6.h>
#include <FileDialog.hpp>

// Log functions are defined in BaseEditor.cpp
extern void editorLog(const char* fmt, ...);
extern void editorLogWarning(const char* fmt, ...);
extern void editorLogError(const char* fmt, ...);

// ============================================================================
// Camera interpolation helpers (matching game's implementation)
// ============================================================================

// Piecewise ease in/out function: quadratic ease-in, linear middle, quadratic ease-out
static float CameraEase(float t, float easeOut, float easeIn) {
    float sum = easeOut + easeIn;
    if(sum < 0.001f) return t;
    if(sum > 1.0f) {
        float inv = 1.0f / sum;
        easeOut *= inv;
        easeIn *= inv;
    }
    float k = 1.0f / (2.0f - easeOut - easeIn);
    if(t < easeOut)
        return k / easeOut * t * t;
    float boundary = 1.0f - easeIn;
    if(t > boundary)
        return 1.0f - k / easeIn * (1.0f - t) * (1.0f - t);
    return k * (2.0f * t - easeOut);
}

// Hermite spline interpolation: p0/p1 = endpoints, t0/t1 = tangent vectors
static S_vector HermiteInterp(const S_vector& p0, const S_vector& p1,
                              const S_vector& t0, const S_vector& t1, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;  // start point weight
    float h01 = -2.0f * t3 + 3.0f * t2;          // end point weight
    float h10 = t3 - 2.0f * t2 + t;              // start tangent weight
    float h11 = t3 - t2;                          // end tangent weight
    return S_vector(
        h00 * p0.x + h01 * p1.x + h10 * t0.x + h11 * t1.x,
        h00 * p0.y + h01 * p1.y + h10 * t0.y + h11 * t1.y,
        h00 * p0.z + h01 * p1.z + h10 * t0.z + h11 * t1.z
    );
}

// ============================================================================
// Menu commands specific to CutsceneEditor
// ============================================================================

enum CutsceneMenuCommand {
    CCMD_VIEW_CAMERA_PATH = 200,
    CCMD_VIEW_FOCUS_PATH,
    CCMD_VIEW_OBJECT_MARKERS,
};

// ============================================================================
// File loading
// ============================================================================

bool CutsceneEditor::LoadRecordFile(const std::string& repPath, const std::string& scenePath) {
    // Load the 3D scene (geometry + object modifications + city parts)
    if(!LoadScene(scenePath)) {
        editorLogError("Failed to load scene from %s!", scenePath.c_str());
        return false;
    }

    editorLog("Loading: scene2.bin");
    LoadScene2Bin(scenePath + "\\scene2.bin");

    editorLog("Loading: cache.bin");
    LoadCacheBin(scenePath + "\\cache.bin");

    // Optionally load .chg diff file
    int result = MessageBoxA(GetWindowHandle(),
                             "Would you like to load a .chg (differences) file?\n\nThis will apply scene delta changes (new frames, modified transforms).",
                             "Load Differences",
                             MB_ICONQUESTION | MB_YESNO);

    if(result == IDYES) {
        FileDialog chgDialog(FileDialog::Mode::OpenFile, "Select differences file", {{"Differences File (*.chg)", "*.chg"}});
        if(chgDialog.Show(GetWindowHandle()) && chgDialog.GetResult().valid) { LoadChgFileAndApply(chgDialog.GetResult().path); }
    }

    // Open the .rep file
    BinaryReader reader(repPath);
    if(!reader.IsOpen()) {
        editorLogError("Failed to open record file: %s", repPath.c_str());
        return false;
    }

    // Read and validate header
    reader.Read(&m_RepHeader, sizeof(RepHeader));
    if(m_RepHeader.magicByte != REP_MAGIC) {
        editorLogError("Invalid .rep file (magic: %d, expected: %d)", m_RepHeader.magicByte, REP_MAGIC);
        return false;
    }

    editorLog("Loaded .rep header: %d objects, %d camera chunks, %d focus chunks",
        m_RepHeader.countOfObjectDefinitionBlocks,
        m_RepHeader.countOfCameraChunks,
        m_RepHeader.countOfCameraFocusChunks);

    // ---- Animation Blocks ----
    if(m_RepHeader.sizeOfAnimationBlocks > 0) {
        uint32_t animCount = 0;
        uint32_t animUnk = 0;
        reader.Read(&animCount, 4);
        reader.Read(&animUnk, 4);

        m_AnimationBlocks.resize(animCount);
        for(uint32_t i = 0; i < animCount; i++) {
            reader.Read(&m_AnimationBlocks[i], sizeof(AnimationBlock));
            editorLog("  Anim[%d]: ID=%d name='%s'", i, m_AnimationBlocks[i].animationID, m_AnimationBlocks[i].animationName);
        }
        editorLog("  Animations: %d", animCount);
    }

    // ---- Object Definition Blocks ----
    m_ObjectDefs.resize(m_RepHeader.countOfObjectDefinitionBlocks);
    for(uint32_t i = 0; i < m_RepHeader.countOfObjectDefinitionBlocks; i++) {
        reader.Read(&m_ObjectDefs[i], sizeof(ObjectDefinitionBlock));
    }

    // ---- Frame Data Blob ----
    m_FrameDataBlob.resize(m_RepHeader.sizeOfObjectDefinitionsSection);
    if(m_RepHeader.sizeOfObjectDefinitionsSection > 0) {
        reader.Read(m_FrameDataBlob.data(), m_RepHeader.sizeOfObjectDefinitionsSection);
    }

    // ---- Camera Chunks ----
    m_CameraChunks.resize(m_RepHeader.countOfCameraChunks);
    for(uint32_t i = 0; i < m_RepHeader.countOfCameraChunks; i++) {
        reader.Read(&m_CameraChunks[i], sizeof(CameraChunk));
    }

    // ---- Camera Focus Chunks ----
    m_CameraFocusChunks.resize(m_RepHeader.countOfCameraFocusChunks);
    for(uint32_t i = 0; i < m_RepHeader.countOfCameraFocusChunks; i++) {
        reader.Read(&m_CameraFocusChunks[i], sizeof(CameraFocusChunk));
    }

    // ---- Script Events Section ----
    if(m_RepHeader.sizeOfScriptEventsSequence > 0) {
        uint32_t subtitleBlockSize, zatmizeBlockSize, scriptsSize, wavSize;
        reader.Read(&subtitleBlockSize, 4);
        reader.Read(&zatmizeBlockSize, 4);
        reader.Read(&scriptsSize, 4);
        reader.Read(&wavSize, 4);

        // Subtitle entries (12 bytes each: time, duration, stringID)
        if(subtitleBlockSize > 0) {
            // Skip subtitle data for now (no text database available)
            std::vector<uint8_t> subtitleTemp(subtitleBlockSize);
            reader.Read(subtitleTemp.data(), subtitleBlockSize);
        }

        // Zatmize (fade events)
        if(zatmizeBlockSize > 0) {
            uint32_t count = zatmizeBlockSize / sizeof(ZatmizeData);
            m_FadeEvents.resize(count);
            reader.Read(m_FadeEvents.data(), zatmizeBlockSize);
        }

        // Script events
        if(scriptsSize > 0) {
            uint32_t count = scriptsSize / sizeof(ScriptsData);
            m_ScriptEvents.resize(count);
            reader.Read(m_ScriptEvents.data(), scriptsSize);
        }

        // Wav events
        if(wavSize > 0) {
            uint32_t count = wavSize / sizeof(WavData);
            m_WavEvents.resize(count);
            reader.Read(m_WavEvents.data(), wavSize);
        }

        editorLog("  Events: %d fades, %d scripts, %d wavs",
            (int)m_FadeEvents.size(), (int)m_ScriptEvents.size(), (int)m_WavEvents.size());
    }

    // ---- Dialog Section ----
    if(m_RepHeader.sizeOfDialogSection > 0) {
        uint32_t dialogCount, dialogUnk, lastObjCount;
        reader.Read(&dialogCount, 4);
        reader.Read(&dialogUnk, 4);
        reader.Read(&lastObjCount, 4);

        m_DialogEntries.resize(dialogCount);
        for(uint32_t i = 0; i < dialogCount; i++) {
            reader.Read(&m_DialogEntries[i], sizeof(PreLastObject));
        }

        m_LastObjects.resize(lastObjCount);
        for(uint32_t i = 0; i < lastObjCount; i++) {
            reader.Read(&m_LastObjects[i], sizeof(LastObject));
        }

        editorLog("  Dialog: %d entries, %d last objects", dialogCount, lastObjCount);
    }

    // ---- Calculate total duration (ms) ----
    // timeActivation = end time (ms) for each object's keyframe iteration
    m_TotalTicks = 0;
    for(const auto& def : m_ObjectDefs) {
        if(def.timeActivation > m_TotalTicks) m_TotalTicks = def.timeActivation;
    }
    if(!m_CameraChunks.empty()) {
        uint32_t lastCamTime = m_CameraChunks.back().frameA;
        if(lastCamTime > m_TotalTicks) m_TotalTicks = lastCamTime;
    }
    if(!m_CameraFocusChunks.empty()) {
        uint32_t lastFocTime = m_CameraFocusChunks.back().frameA_Focus;
        if(lastFocTime > m_TotalTicks) m_TotalTicks = lastFocTime;
    }

    // ---- Match objects to 3D scene frames ----
    m_MatchedObjects.clear();
    for(auto& def : m_ObjectDefs) {
        MatchedObject match;
        match.def = &def;
        match.type = GetObjectType(def);
        match.frame = m_Scene->FindFrame(def.name, ENUMF_ALL);

        if(match.frame) {
            editorLog("  Matched: %s -> frame '%s' (%s)", def.name, match.frame->GetName(), GetObjectTypeName(match.type));
        } else {
            editorLogWarning("  Unmatched: %s (%s)", def.name, GetObjectTypeName(match.type));
        }

        m_MatchedObjects.push_back(match);
    }

    // Initialize per-object playback state (stride tables, data pointers)
    for(auto& match : m_MatchedObjects) {
        InitObjectPlayback(match);
    }

    // ---- Setup timeline sequence ----
    // Timeline uses milliseconds; estimate start from first keyframe timestamp
    m_Timeline.editor = this;
    m_Timeline.itemStarts.resize(m_ObjectDefs.size());
    m_Timeline.itemEnds.resize(m_ObjectDefs.size());
    for(size_t i = 0; i < m_ObjectDefs.size(); i++) {
        // Start = first keyframe timestamp (read from data blob if available)
        uint32_t startTime = 0;
        if(m_ObjectDefs[i].size > 8 && m_ObjectDefs[i].offset + 8 < m_FrameDataBlob.size()) {
            // First keyframe timestamp is at offset + 8 (after 8-byte header)
            startTime = *(const uint32_t*)(m_FrameDataBlob.data() + m_ObjectDefs[i].offset + 8);
        }
        m_Timeline.itemStarts[i] = (int)startTime;
        m_Timeline.itemEnds[i] = (int)m_ObjectDefs[i].timeActivation;
    }

    m_MissionPath = repPath;
    m_CurrentTick = 0;
    m_PlaybackState = PLAYBACK_STOPPED;

    char buf[256];
    sprintf(buf, "Cutscene Editor - %s", repPath.c_str());
    m_IGraph->SetAppName(buf);

    OnSceneLoaded();

    editorLog("Record file loaded: %d ms total (%.1f seconds)", m_TotalTicks, m_TotalTicks / 1000.0f);
    return true;
}

// ============================================================================
// Timeline Sequencer interface
// ============================================================================

void CutsceneEditor::TimelineSequence::Get(int index, int** start, int** end, int* type, unsigned int* color) {
    if(!editor || index < 0 || index >= (int)editor->m_ObjectDefs.size()) return;

    if(start) *start = &itemStarts[index];
    if(end) *end = &itemEnds[index];

    RecordObjectType objType = GetObjectType(editor->m_ObjectDefs[index]);
    if(type) *type = (int)objType;

    if(color) {
        switch(objType) {
        case RecordObjectType::Car:   *color = 0xFF4488DD; break;
        case RecordObjectType::Human: *color = 0xFF44DD88; break;
        case RecordObjectType::Actor: *color = 0xFFDD8844; break;
        default:                      *color = 0xFF888888; break;
        }
    }
}

// ============================================================================
// Playback
// ============================================================================

void CutsceneEditor::TickPlayback() {
    if(m_PlaybackState != PLAYBACK_PLAYING) return;

    // Accumulate time in milliseconds
    m_TickAccumulator += m_DeltaTime * 1000.0f * m_PlaybackSpeed;
    uint32_t advance = (uint32_t)m_TickAccumulator;
    m_TickAccumulator -= (float)advance;
    m_CurrentTick += advance;

    if(m_CurrentTick >= m_TotalTicks) {
        m_CurrentTick = m_TotalTicks;
        m_PlaybackState = PLAYBACK_STOPPED;
    }
}

void CutsceneEditor::InitObjectPlayback(MatchedObject& match) {
    auto& ps = match.playback;
    ps = {}; // Reset

    if(!match.def || match.def->size <= 8) return;
    if(match.def->offset + match.def->size > m_FrameDataBlob.size()) return;

    const uint8_t* base = m_FrameDataBlob.data() + match.def->offset;

    // Decode stride table from chkData[2] (reinterpret 2x uint64 as 4x uint32)
    const uint32_t* rawStrides = reinterpret_cast<const uint32_t*>(match.def->chkData);
    ps.strides[0] = rawStrides[0];
    ps.strides[1] = rawStrides[1];
    ps.strides[2] = rawStrides[2];
    ps.strides[3] = rawStrides[3];

    ps.pStart = base + 8;                          // Skip 8-byte per-actor data header
    ps.pEnd = base + match.def->size;
    ps.pCurrent = ps.pStart;
    ps.pNext = ps.pStart;
    ps.endTime = match.def->timeActivation;         // timeActivation = end time (ms)
    ps.finished = (match.def->chkDataEmpty != 0);   // Initial bFinished flag
}

void CutsceneEditor::ApplyAnimation(MatchedObject& match, uint32_t animID) {
    if(!match.frame || match.frame->GetType() != FRAME_MODEL) return;

    I3D_model* model = (I3D_model*)match.frame;

    // The raw animID field is packed (game's human apply callback at 0x56B670):
    //   Bits 0-10  (& 0x7FF):  animation reference (biased by 1024)
    //   Bit  13    (0x2000):   loop mode (set = options 2, clear = options 1)
    //   Bit  14    (0x4000):   blend direction
    uint32_t animRef = animID & 0x7FF;

    // Compare extracted ref — caller should only call us when animation actually changed
    // but guard here too. Store the extracted ref, NOT the raw field value.
    if((int)animRef == match.currentAnimID) return;

    // Stop and release previous animation
    if(match.currentAnimSet) {
        model->StopAnimation(0);
        model->SetAnimation(NULL, 0, 1.0f, 0);
        model->ResetAllPoses();
        match.currentAnimSet->Release();
        match.currentAnimSet = nullptr;
    }

    match.currentAnimID = (int)animRef;

    if(m_AnimationBlocks.empty()) return;
    if(animRef < 1024) return;
    uint32_t animIndex = animRef - 1024;
    bool loopAnim = (animID & 0x2000) != 0;

    // Look up by animationID field match, then fallback to direct array index
    const char* animName = nullptr;
    for(const auto& block : m_AnimationBlocks) {
        if(block.animationID == animIndex) {
            animName = block.animationName;
            break;
        }
    }
    if(!animName && animIndex < (uint32_t)m_AnimationBlocks.size()) {
        animName = m_AnimationBlocks[animIndex].animationName;
    }

    if(!animName || !animName[0]) {
        editorLogWarning("Animation lookup failed: raw 0x%X, ref %d, index %d (%d blocks)",
            animID, animRef, animIndex, (int)m_AnimationBlocks.size());
        return;
    }

    // Null-terminate safely (animationName is a fixed 48-char buffer)
    char safeName[49];
    memcpy(safeName, animName, 48);
    safeName[48] = '\0';

    // Try opening the animation — first with raw name, then with "Anims\\" prefix
    I3D_animation_set* anim = m_3DDriver->CreateAnimationSet();
    bool opened = false;

    // Try raw name (might already include path like "Anims\\walk01.i3d")
    if(anim->Open(safeName) == I3D_OK) {
        opened = true;
    }

    // Try with "Anims\\" prefix
    if(!opened) {
        char animPath[256];
        sprintf(animPath, "Anims\\%s", safeName);
        if(anim->Open(animPath) == I3D_OK) {
            opened = true;
        }
    }

    if(!opened) {
        editorLogWarning("Failed to load animation: '%s' (raw ID %d)", safeName, animID);
        anim->Release();
        return;
    }

    int animOptions = loopAnim ? 2 : 1;
    model->SetAnimation(anim, 0, 1.0f, animOptions);
    model->SetAnimTime(0, 0);
    model->StartAnimation(0);
    match.currentAnimSet = anim;

    editorLog("Animation: %s -> '%s' (index %d, %s)", match.def->name, safeName, animIndex, loopAnim ? "loop" : "once");
}

void CutsceneEditor::ApplyFrameData(uint32_t timeMs) {
    for(auto& match : m_MatchedObjects) {
        if(!match.frame || !match.def) continue;

        auto& ps = match.playback;
        if(ps.finished || !ps.pCurrent || !ps.pNext) continue;

        // Advance through keyframes until we pass current time
        // Each keyframe: [timestamp(u32), type(u32), ...data]
        // Stride = strides[type] gives total keyframe size
        while(ps.pNext + 8 <= ps.pEnd) {
            uint32_t nextTimestamp = *(const uint32_t*)ps.pNext;
            if(timeMs < nextTimestamp) break;

            ps.pCurrent = ps.pNext;
            uint32_t type = *(const uint32_t*)(ps.pCurrent + 4);
            if(type > 3) { ps.finished = true; break; }

            uint32_t stride = ps.strides[type];
            if(stride == 0) { ps.finished = true; break; }

            ps.pNext = ps.pCurrent + stride;

            uint32_t curTimestamp = *(const uint32_t*)ps.pCurrent;
            if(curTimestamp >= ps.endTime) {
                ps.finished = true;
                break;
            }
        }

        // Apply position and rotation from current keyframe (with interpolation toward next)
        // All types share the same layout after the 8-byte header:
        //   +0x08: position (S_vector, 12 bytes)
        //   +0x14: rotation (S_quat, 16 bytes)
        // But only if the stride is large enough to contain pos+rot data.
        // Unknown-type objects have stride 16 (only 8 bytes of payload) — skip those.
        uint32_t type = *(const uint32_t*)(ps.pCurrent + 4);
        uint32_t curStride = (type <= 3) ? ps.strides[type] : 0;
        const uint32_t minPosRotSize = 8 + sizeof(S_vector) + sizeof(S_quat); // 36 bytes

        if(type >= 1 && curStride >= minPosRotSize && ps.pCurrent + minPosRotSize <= ps.pEnd) {
            const S_vector& curPos = *(const S_vector*)(ps.pCurrent + 8);
            const S_quat& curRot = *(const S_quat*)(ps.pCurrent + 8 + sizeof(S_vector));

            // Interpolate between current and next keyframe
            S_vector interpPos = curPos;
            S_quat interpRot = curRot;

            if(ps.pNext != ps.pCurrent && ps.pNext + 8 <= ps.pEnd) {
                uint32_t nextType = *(const uint32_t*)(ps.pNext + 4);
                uint32_t nextStride = (nextType <= 3) ? ps.strides[nextType] : 0;

                if(nextType >= 1 && nextStride >= minPosRotSize && ps.pNext + minPosRotSize <= ps.pEnd) {
                    uint32_t curTime = *(const uint32_t*)ps.pCurrent;
                    uint32_t nextTime = *(const uint32_t*)ps.pNext;

                    if(nextTime > curTime && timeMs >= curTime) {
                        float t = (float)(timeMs - curTime) / (float)(nextTime - curTime);
                        if(t > 1.0f) t = 1.0f;

                        const S_vector& nextPos = *(const S_vector*)(ps.pNext + 8);
                        const S_quat& nextRot = *(const S_quat*)(ps.pNext + 8 + sizeof(S_vector));

                        // Lerp position
                        interpPos.x = curPos.x + (nextPos.x - curPos.x) * t;
                        interpPos.y = curPos.y + (nextPos.y - curPos.y) * t;
                        interpPos.z = curPos.z + (nextPos.z - curPos.z) * t;

                        // Slerp rotation (shortest path)
                        interpRot = curRot.Slerp(nextRot, t, true);
                    }
                }
            }

            match.frame->SetPos(interpPos);
            match.frame->SetRot(interpRot);

            // Human keyframes carry animationID at offset +36 (after pos+rot)
            // Type 1 (stride 40): pos+rot+animID
            // Type 2 (stride 44): pos+rot+animID+animOffset
            if(match.type == RecordObjectType::Human) {
                const uint8_t* animIDPtr = ps.pCurrent + 8 + sizeof(S_vector) + sizeof(S_quat);
                if(animIDPtr + 4 <= ps.pEnd) {
                    uint32_t rawAnimField = *(const uint32_t*)animIDPtr;
                    uint32_t animRef = rawAnimField & 0x7FF;

                    // Only load a new animation if the extracted reference changed
                    if((int)animRef != match.currentAnimID) {
                        ApplyAnimation(match, rawAnimField);
                    }

                    // Type 2/3: ALWAYS snap animation time, even if same animation
                    // Game does: SetAnimTime(channel, 40 * (animOffset & 0xFFF))
                    if(type >= 2 && animIDPtr + 8 <= ps.pEnd && match.frame->GetType() == FRAME_MODEL) {
                        I3D_model* mdl = (I3D_model*)match.frame;
                        uint32_t rawOffset = *(const uint32_t*)(animIDPtr + 4);
                        int animTime = 40 * (int)(rawOffset & 0xFFF);

                        // If animation finished (play-once ended), restart it
                        // before snapping time — otherwise SetAnimTime is ignored
                        if(!mdl->AnimIsOn(0)) {
                            mdl->StartAnimation(0);
                        }
                        mdl->SetAnimTime(0, animTime);
                    }
                }
            }

            match.frame->Update();
        }
    }
}

void CutsceneEditor::ApplyCameraData(uint32_t timeMs) {
    if(!m_CameraFollow || m_CameraChunks.empty()) return;

    const int nCamPos = (int)m_CameraChunks.size();
    const int nCamFoc = (int)m_CameraFocusChunks.size();

    // ---- Camera Position: match game's MainUpdate search logic ----
    // Phase 1: Find current valid keyframe (last with interpFlags & 3 != 0 && frameA <= time)
    int curKey = -1;
    for(int i = 0; i < nCamPos; i++) {
        if((m_CameraChunks[i].unkType & 3) != 0 && m_CameraChunks[i].frameA <= timeMs)
            curKey = i;
    }
    if(curKey < 0) return;

    // Check for end marker (FOV == 500.0f)
    if(m_CameraChunks[curKey].FOV == 500.0f)
        return;

    // Phase 2: Find next valid keyframe (first with interpFlags & 3 != 0 && frameA > time)
    int nextKey = -1;
    for(int i = curKey; i < nCamPos; i++) {
        if((m_CameraChunks[i].unkType & 3) != 0 && m_CameraChunks[i].frameA > timeMs) {
            nextKey = i;
            break;
        }
    }

    // Phase 3: Adjust indices based on interpFlags bit 1 (sub-entry offset)
    // When bit 1 is set, the actual interpolation data is at adjacent chunk offsets
    int keyAIdx = curKey;
    if((m_CameraChunks[curKey].unkType & 2) != 0 && curKey + 2 < nCamPos)
        keyAIdx = curKey + 2;

    int keyBIdx = nextKey;
    if(nextKey >= 0 && (m_CameraChunks[nextKey].unkType & 2) != 0 && nextKey + 1 < nCamPos)
        keyBIdx = nextKey + 1;

    S_vector camPos;
    float camFov = 0.0f;
    float camRoll = 0.0f;

    if(keyBIdx < 0 || keyBIdx >= nCamPos) {
        // Past last keyframe — use static position
        camPos = m_CameraChunks[keyAIdx].position;
        camFov = m_CameraChunks[keyAIdx].FOV;
        camRoll = m_CameraChunks[keyAIdx].roll;
    } else {
        const CameraChunk& k0 = m_CameraChunks[keyAIdx];
        const CameraChunk& k1 = m_CameraChunks[keyBIdx];

        // Game uses frameB for interpolation parameter
        float rawT = 0.0f;
        if(k1.frameB != k0.frameB)
            rawT = (float)(timeMs - k0.frameB) / (float)(k1.frameB - k0.frameB);
        if(rawT < 0.0f) rawT = 0.0f;
        if(rawT > 1.0f) rawT = 1.0f;

        float t = CameraEase(rawT, k0.easeIn, k1.easeOut);

        // Hermite spline position interpolation
        camPos = HermiteInterp(k0.position, k1.position, k0.curve, k1.subCurve, t);

        // FOV and roll: weighted blend (h00/h01 basis only)
        float h1 = 2.0f*t*t*t - 3.0f*t*t + 1.0f;
        float h2 = -2.0f*t*t*t + 3.0f*t*t;
        camFov = h1 * k0.FOV + h2 * k1.FOV;
        camRoll = h1 * k0.roll + h2 * k1.roll;
    }

    m_CameraPos = camPos;

    if(camFov > 0.0f && camFov < 3.14f) {
        m_CameraFOV = camFov;
        if(m_Camera) m_Camera->SetFOV(camFov);
    }

    // ---- Camera Focus/Target: same interpFlags-aware search ----
    if(nCamFoc > 0) {
        int curFoc = -1;
        for(int i = 0; i < nCamFoc; i++) {
            if((m_CameraFocusChunks[i].unkType & 3) != 0 && m_CameraFocusChunks[i].frameA_Focus <= timeMs)
                curFoc = i;
        }

        if(curFoc >= 0) {
            int nextFoc = -1;
            for(int i = curFoc; i < nCamFoc; i++) {
                if((m_CameraFocusChunks[i].unkType & 3) != 0 && m_CameraFocusChunks[i].frameA_Focus > timeMs) {
                    nextFoc = i;
                    break;
                }
            }

            // Adjust for bit 1
            int focAIdx = curFoc;
            if((m_CameraFocusChunks[curFoc].unkType & 2) != 0 && curFoc + 2 < nCamFoc)
                focAIdx = curFoc + 2;

            int focBIdx = nextFoc;
            if(nextFoc >= 0 && (m_CameraFocusChunks[nextFoc].unkType & 2) != 0 && nextFoc + 1 < nCamFoc)
                focBIdx = nextFoc + 1;

            S_vector focPos;

            if(focBIdx < 0 || focBIdx >= nCamFoc) {
                focPos = m_CameraFocusChunks[focAIdx].position;
            } else {
                const CameraFocusChunk& f0 = m_CameraFocusChunks[focAIdx];
                const CameraFocusChunk& f1 = m_CameraFocusChunks[focBIdx];

                float rawT = 0.0f;
                if(f1.frameB_Focus != f0.frameB_Focus)
                    rawT = (float)(timeMs - f0.frameB_Focus) / (float)(f1.frameB_Focus - f0.frameB_Focus);
                if(rawT < 0.0f) rawT = 0.0f;
                if(rawT > 1.0f) rawT = 1.0f;

                float t = CameraEase(rawT, f0.easeIn, f1.easeOut);
                focPos = HermiteInterp(f0.position, f1.position, f0.curve, f1.subCurve, t);
            }

            // Direction = focus - position (matching game's camera direction computation)
            S_vector dir = focPos - camPos;
            float len = dir.Magnitude();
            if(len > 0.001f) {
                dir *= (1.0f / len);
                m_CameraRot.x = atan2f(dir.x, dir.z) * (180.0f / 3.14159265f);
                m_CameraRot.y = -asinf(dir.y) * (180.0f / 3.14159265f);
            }
        }
    }

    // Re-apply camera transform (BaseEditor::Update already ran before OnUpdate)
    if(m_Camera) {
        m_Camera->SetPos(m_CameraPos);
        S_vector forwardDir = DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0});
        m_Camera->SetDir(forwardDir, camRoll);
        m_Camera->Update();
    }
}

void CutsceneEditor::SeekToTime(uint32_t timeMs) {
    m_CurrentTick = timeMs;
    m_TickAccumulator = 0.0f;

    // Reset all playback cursors and re-advance to the target time
    for(auto& match : m_MatchedObjects) {
        // Reset animation state so ApplyFrameData will re-apply the correct animation
        match.currentAnimID = -1;
        InitObjectPlayback(match);
    }

    ApplyFrameData(timeMs);
    if(m_CameraFollow) {
        ApplyCameraData(timeMs);
    }
}

// ============================================================================
// .chg diff support
// ============================================================================

extern std::map<I3D_model*, std::string> g_ModelsMap;
extern std::map<I3D_sound*, std::string> g_SoundsMap;

bool CutsceneEditor::LoadChgFileAndApply(const std::string& chgPath) {
    if (!LoadChgFile(chgPath, m_ChgFile)) {
        editorLogError("Failed to load .chg file: %s", chgPath.c_str());
        return false;
    }

    for (auto& entry : m_ChgFile.entries) {
        if (entry.type != ChgEntry::ENTRY_FRAME_DEFINITION) continue;

        auto& fd = entry.frameDef;
        I3D_frame* frame = m_Scene->FindFrame(fd.frameName.c_str(), ENUMF_ALL);

        if (frame) {
            // Update existing frame
            frame->SetPos(fd.position);
            frame->SetScale(fd.scale);
            frame->SetRot(fd.rotation);
            editorLog("ChgApply: Updated existing frame '%s'", fd.frameName.c_str());
            continue;
        }

        // Create new frame based on type
        switch (fd.frameType) {
        case CHG_FT_MODEL: {
            I3D_model* model = (I3D_model*)m_3DDriver->CreateFrame(FRAME_MODEL);
            if (model->Open(("Models\\" + fd.modelData.modelFileName).c_str()) == I3D_OK) {
                g_ModelsMap[model] = fd.modelData.modelFileName;
            }
            frame = model;
            break;
        }
        case CHG_FT_LIGHT: {
            I3D_light* light = (I3D_light*)m_3DDriver->CreateFrame(FRAME_LIGHT);
            light->SetLightType((I3D_LIGHTTYPE)fd.lightData.lightType);
            light->SetColor2(fd.lightData.color);
            light->SetRange(0.0f, fd.lightData.range);
            light->SetCone(fd.lightData.coneInner, fd.lightData.coneOuter);
            light->SetMode(fd.lightData.lightMode);
            light->SetOn(fd.lightData.isOn != 0);

            // Add to sectors
            for (const auto& sectorName : fd.lightData.sectorLinks) {
                I3D_frame* sectorFrame = m_Scene->FindFrame(sectorName.c_str(), ENUMF_ALL);
                if (sectorFrame && sectorFrame->GetType() == FRAME_SECTOR) {
                    ((I3D_sector*)sectorFrame)->AddLight(light);
                }
            }
            frame = light;
            break;
        }
        case CHG_FT_SOUND: {
            I3D_sound* sound = (I3D_sound*)m_3DDriver->CreateFrame(FRAME_SOUND);
            if (sound->Open(("Sounds\\" + fd.soundData.soundFileName).c_str(), 0, nullptr, nullptr) == I3D_OK) {
                g_SoundsMap[sound] = fd.soundData.soundFileName;
            }
            sound->SetVolume(fd.soundData.volume);
            sound->SetRange(fd.soundData.rangeNear, fd.soundData.rangeFar, 0, 0);
            sound->SetLoop(fd.soundData.loop != 0);
            sound->SetOn(fd.soundData.isOn != 0);
            frame = sound;
            break;
        }
        default: {
            frame = m_3DDriver->CreateFrame((I3D_FRAME_TYPE)fd.frameType);
            break;
        }
        }

        if (frame) {
            frame->SetName(fd.frameName.c_str());
            frame->SetPos(fd.position);
            frame->SetScale(fd.scale);
            frame->SetRot(fd.rotation);

            m_Scene->AddFrame(frame);

            // Link to parent or primary sector
            I3D_frame* parent = nullptr;
            if (!fd.parentName.empty()) {
                parent = m_Scene->FindFrame(fd.parentName.c_str(), ENUMF_ALL);
            }
            frame->LinkTo(parent ? parent : m_PrimarySector);

            m_DiffFrames.push_back(frame);
            editorLog("ChgApply: Created diff frame '%s' (type %d)", fd.frameName.c_str(), fd.frameType);
        }
    }

    editorLog("ChgApply: Applied %d entries, created %d new frames",
        (int)m_ChgFile.entries.size(), (int)m_DiffFrames.size());
    return true;
}

void CutsceneEditor::CleanupDiffFrames() {
    for (auto* frame : m_DiffFrames) {
        if (frame) {
            m_Scene->DeleteFrame(frame);
            frame->LinkTo(NULL);
            frame->Release();
        }
    }
    m_DiffFrames.clear();
}

// ============================================================================
// BaseEditor overrides
// ============================================================================

void CutsceneEditor::OnUpdate() {
    TickPlayback();

    if(m_SceneLoaded && m_TotalTicks > 0) {
        ApplyFrameData(m_CurrentTick);
        if(m_CameraFollow) {
            ApplyCameraData(m_CurrentTick);
        }

        // Tick animations on models that have active animations
        if(m_PlaybackState == PLAYBACK_PLAYING) {
            int tickMs = (int)(m_DeltaTime * 1000.0f * m_PlaybackSpeed);
            for(auto& match : m_MatchedObjects) {
                if(match.currentAnimSet && match.frame && match.frame->GetType() == FRAME_MODEL) {
                    I3D_model* model = (I3D_model*)match.frame;
                    model->Tick(tickMs);
                    model->Update();
                }
            }
        }
    }
}

void CutsceneEditor::OnRender3D() {
    if(!m_SceneLoaded) return;

    // Draw interpolated camera position spline (interpFlags-aware)
    if(m_DrawCameraPath && m_CameraChunks.size() > 1) {
        std::vector<BaseEditor::LineVertex> verts;
        std::vector<WORD> indices;

        const int SEGS = 16;
        WORD vi = 0;

        // Build list of valid keyframe indices (interpFlags & 3 != 0)
        std::vector<int> validKeys;
        for(int i = 0; i < (int)m_CameraChunks.size(); i++) {
            if((m_CameraChunks[i].unkType & 3) != 0)
                validKeys.push_back(i);
        }

        for(size_t ki = 0; ki + 1 < validKeys.size(); ki++) {
            int cur = validKeys[ki];
            int nxt = validKeys[ki + 1];

            // Adjust for interpFlags bit 1
            int aIdx = cur;
            if((m_CameraChunks[cur].unkType & 2) != 0 && cur + 2 < (int)m_CameraChunks.size())
                aIdx = cur + 2;
            int bIdx = nxt;
            if((m_CameraChunks[nxt].unkType & 2) != 0 && nxt + 1 < (int)m_CameraChunks.size())
                bIdx = nxt + 1;

            const CameraChunk& k0 = m_CameraChunks[aIdx];
            const CameraChunk& k1 = m_CameraChunks[bIdx];

            for(int s = 0; s <= SEGS; s++) {
                float rawT = (float)s / (float)SEGS;
                float t = CameraEase(rawT, k0.easeIn, k1.easeOut);
                S_vector pos = HermiteInterp(k0.position, k1.position, k0.curve, k1.subCurve, t);

                DWORD col = 0x80FF4444;
                verts.push_back({pos.x, pos.y, pos.z, col});
                if(s > 0) {
                    indices.push_back(vi - 1);
                    indices.push_back(vi);
                }
                vi++;
            }
        }

        S_matrix mat;
        mat.Identity();

        m_IGraph->SetWorldMatrix(mat);
        DrawBatchedLines(verts, indices, {1, 0.25f, 0.25f}, 0x80);
    }

    // Draw interpolated camera focus/target spline (interpFlags-aware)
    if(m_DrawFocusPath && m_CameraFocusChunks.size() > 1) {
        std::vector<BaseEditor::LineVertex> verts;
        std::vector<WORD> indices;

        const int SEGS = 16;
        WORD vi = 0;

        std::vector<int> validKeys;
        for(int i = 0; i < (int)m_CameraFocusChunks.size(); i++) {
            if((m_CameraFocusChunks[i].unkType & 3) != 0)
                validKeys.push_back(i);
        }

        for(size_t ki = 0; ki + 1 < validKeys.size(); ki++) {
            int cur = validKeys[ki];
            int nxt = validKeys[ki + 1];

            int aIdx = cur;
            if((m_CameraFocusChunks[cur].unkType & 2) != 0 && cur + 2 < (int)m_CameraFocusChunks.size())
                aIdx = cur + 2;
            int bIdx = nxt;
            if((m_CameraFocusChunks[nxt].unkType & 2) != 0 && nxt + 1 < (int)m_CameraFocusChunks.size())
                bIdx = nxt + 1;

            const CameraFocusChunk& f0 = m_CameraFocusChunks[aIdx];
            const CameraFocusChunk& f1 = m_CameraFocusChunks[bIdx];

            for(int s = 0; s <= SEGS; s++) {
                float rawT = (float)s / (float)SEGS;
                float t = CameraEase(rawT, f0.easeIn, f1.easeOut);
                S_vector pos = HermiteInterp(f0.position, f1.position, f0.curve, f1.subCurve, t);

                DWORD col = 0x804444FF;
                verts.push_back({pos.x, pos.y, pos.z, col});
                if(s > 0) {
                    indices.push_back(vi - 1);
                    indices.push_back(vi);
                }
                vi++;
            }
        }

        S_matrix mat;
        mat.Identity();

        m_IGraph->SetWorldMatrix(mat);
        DrawBatchedLines(verts, indices, {0.25f, 0.25f, 1}, 0x80);
    }

    // Draw markers at active objects
    if(m_DrawObjectMarkers) {
        for(auto& match : m_MatchedObjects) {
            if(!match.frame) continue;
            if(m_CurrentTick < match.def->timeActivation) continue;

            S_vector pos = match.frame->GetWorldPos();
            I3D_bbox bbox;
            bbox.min = pos - S_vector(0.3f, 0.3f, 0.3f);
            bbox.max = pos + S_vector(0.3f, 0.3f, 0.3f);

            S_vector color;
            switch(match.type) {
            case RecordObjectType::Car:   color = {0.27f, 0.53f, 0.87f}; break;
            case RecordObjectType::Human: color = {0.27f, 0.87f, 0.53f}; break;
            case RecordObjectType::Actor: color = {0.87f, 0.53f, 0.27f}; break;
            default:                      color = {0.5f, 0.5f, 0.5f}; break;
            }

            S_matrix mat;
            mat.Identity();

            m_IGraph->SetWorldMatrix(mat);

            DrawWireframeBox(bbox, color, 0x80);
        }
    }
}

void CutsceneEditor::OnRenderUI() {
    // ---- Objects Panel ----
    if(m_ShowObjects) {
        if(ImGui::Begin("Objects")) {
            ImGui::Text("Objects: %d", (int)m_ObjectDefs.size());
            ImGui::Separator();

            if(ImGui::BeginTable("##objects", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Matched", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableHeadersRow();

                for(int i = 0; i < (int)m_ObjectDefs.size(); i++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    bool selected = (m_SelectedObject == i);
                    if(ImGui::Selectable(m_ObjectDefs[i].name, selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        m_SelectedObject = i;
                        // Focus camera on matched frame
                        if(i < (int)m_MatchedObjects.size() && m_MatchedObjects[i].frame) {
                            MoveCameraToTarget(m_MatchedObjects[i].frame->GetWorldPos());
                        }
                    }

                    ImGui::TableNextColumn();
                    RecordObjectType type = GetObjectType(m_ObjectDefs[i]);
                    ImGui::TextUnformatted(GetObjectTypeName(type));

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", m_ObjectDefs[i].timeActivation);

                    ImGui::TableNextColumn();
                    if(i < (int)m_MatchedObjects.size() && m_MatchedObjects[i].frame) {
                        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Yes");
                    } else {
                        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "No");
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    // ---- Camera Panel ----
    if(m_ShowCamera) {
        if(ImGui::Begin("Camera")) {
            ImGui::Text("Position Keys: %d", (int)m_CameraChunks.size());
            ImGui::Text("Focus Keys: %d", (int)m_CameraFocusChunks.size());
            ImGui::Separator();

            ImGui::Checkbox("Follow Camera", &m_CameraFollow);

            // Show interpolated camera state
            ImGui::Separator();
            ImGui::Text("Interpolated Position: %.2f, %.2f, %.2f", m_CameraPos.x, m_CameraPos.y, m_CameraPos.z);
            ImGui::Text("Interpolated FOV: %.3f", m_CameraFOV);

            // Find current key span
            int keyIdx = -1;
            for(size_t i = 0; i < m_CameraChunks.size(); i++) {
                if(m_CameraChunks[i].frameA <= m_CurrentTick) keyIdx = (int)i;
                else break;
            }

            if(keyIdx >= 0) {
                const CameraChunk& k = m_CameraChunks[keyIdx];

                ImGui::Separator();
                ImGui::Text("Key %d/%d  (time: %d ms)", keyIdx, (int)m_CameraChunks.size(), k.frameA);
                ImGui::Text("Key Pos: %.2f, %.2f, %.2f", k.position.x, k.position.y, k.position.z);
                ImGui::Text("Key FOV: %.3f  Roll: %.3f", k.FOV, k.roll);
                ImGui::Text("Tangent Out: %.2f, %.2f, %.2f", k.curve.x, k.curve.y, k.curve.z);
                ImGui::Text("Tangent In:  %.2f, %.2f, %.2f", k.subCurve.x, k.subCurve.y, k.subCurve.z);
                ImGui::Text("Ease: %.3f / %.3f", k.easeIn, k.easeOut);
            }
        }
        ImGui::End();
    }

    // ---- Events Panel ----
    if(m_ShowEvents) {
        if(ImGui::Begin("Events")) {
            if(ImGui::BeginTabBar("##events_tabs")) {
                // Fades tab
                if(ImGui::BeginTabItem("Fades")) {
                    ImGui::Text("Count: %d", (int)m_FadeEvents.size());
                    if(ImGui::BeginTable("##fades", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("Fade A");
                        ImGui::TableSetupColumn("Fade B");
                        ImGui::TableSetupColumn("Unk1");
                        ImGui::TableHeadersRow();

                        for(auto& f : m_FadeEvents) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%.2f", f.fadeA);
                            ImGui::TableNextColumn(); ImGui::Text("%.2f", f.fadeB);
                            ImGui::TableNextColumn(); ImGui::Text("%d", f.unk1);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                // Scripts tab
                if(ImGui::BeginTabItem("Scripts")) {
                    ImGui::Text("Count: %d", (int)m_ScriptEvents.size());
                    if(ImGui::BeginTable("##scripts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("Time");
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableHeadersRow();

                        for(auto& s : m_ScriptEvents) {
                            bool nearCurrent = (m_CurrentTick >= s.time && m_CurrentTick < s.time + 30);
                            if(nearCurrent) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(255, 255, 0, 40));

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%d", s.time);
                            ImGui::TableNextColumn(); ImGui::TextUnformatted(s.name);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                // Sounds tab
                if(ImGui::BeginTabItem("Sounds")) {
                    ImGui::Text("Count: %d", (int)m_WavEvents.size());
                    if(ImGui::BeginTable("##wavs", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("Time");
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("State");
                        ImGui::TableHeadersRow();

                        for(auto& w : m_WavEvents) {
                            bool nearCurrent = (m_CurrentTick >= w.time && m_CurrentTick < w.time + 30);
                            if(nearCurrent) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(255, 255, 0, 40));

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%d", w.time);
                            ImGui::TableNextColumn(); ImGui::TextUnformatted(w.name);
                            ImGui::TableNextColumn(); ImGui::Text("%d", w.state);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                // Dialog tab
                if(ImGui::BeginTabItem("Dialog")) {
                    ImGui::Text("Dialog: %d, Last: %d", (int)m_DialogEntries.size(), (int)m_LastObjects.size());
                    if(ImGui::BeginTable("##dialog", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("Time");
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("Char ID");
                        ImGui::TableSetupColumn("String ID");
                        ImGui::TableHeadersRow();

                        for(auto& d : m_DialogEntries) {
                            bool nearCurrent = (m_CurrentTick >= d.time && m_CurrentTick < d.time + 30);
                            if(nearCurrent) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(255, 255, 0, 40));

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%d", d.time);
                            ImGui::TableNextColumn(); ImGui::TextUnformatted(d.name);
                            ImGui::TableNextColumn(); ImGui::Text("%d", d.charID);
                            ImGui::TableNextColumn(); ImGui::Text("%d", d.stringID);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                // Animations tab
                if(ImGui::BeginTabItem("Animations")) {
                    ImGui::Text("Count: %d", (int)m_AnimationBlocks.size());
                    if(ImGui::BeginTable("##anims", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("ID");
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableHeadersRow();

                        for(auto& a : m_AnimationBlocks) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%d", a.animationID);
                            ImGui::TableNextColumn(); ImGui::TextUnformatted(a.animationName);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    // ---- Timeline / Playback Controls ----
    if(ImGui::Begin("Timeline")) {
        // Playback controls
        if(m_PlaybackState == PLAYBACK_PLAYING) {
            if(ImGui::Button(ICON_FA_PAUSE " Pause")) {
                m_PlaybackState = PLAYBACK_PAUSED;
            }
        } else {
            if(ImGui::Button(ICON_FA_PLAY " Play")) {
                if(m_CurrentTick >= m_TotalTicks) SeekToTime(0);
                m_PlaybackState = PLAYBACK_PLAYING;
            }
        }
        ImGui::SameLine();
        if(ImGui::Button(ICON_FA_STOP " Stop")) {
            m_PlaybackState = PLAYBACK_STOPPED;
            SeekToTime(0);
        }
        ImGui::SameLine();

        // Speed control
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderFloat("Speed", &m_PlaybackSpeed, 0.1f, 4.0f, "%.1fx");
        ImGui::SameLine();

        // Time display (ms)
        int timeMs = (int)m_CurrentTick;
        ImGui::SetNextItemWidth(150.0f);
        if(ImGui::SliderInt("Time (ms)", &timeMs, 0, (int)m_TotalTicks)) {
            SeekToTime((uint32_t)timeMs);
        }
        ImGui::SameLine();
        ImGui::Text("%.1fs", m_CurrentTick / 1000.0f);

        ImGui::Separator();

        // Sequencer timeline
        m_TimelineCurrentFrame = (int)m_CurrentTick;
        ImSequencer::Sequencer(&m_Timeline, &m_TimelineCurrentFrame, &m_TimelineExpanded, &m_TimelineSelectedEntry, &m_TimelineFirstFrame,
            ImSequencer::SEQUENCER_CHANGE_FRAME);

        // If user scrubbed the sequencer, seek to the new time
        if((uint32_t)m_TimelineCurrentFrame != m_CurrentTick) {
            SeekToTime((uint32_t)m_TimelineCurrentFrame);
            if(m_PlaybackState == PLAYBACK_PLAYING) {
                m_PlaybackState = PLAYBACK_PAUSED;
            }
        }
    }
    ImGui::End();
}

void CutsceneEditor::OnDockLayout(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    ImGui::DockBuilderSetNodeSize(dockspace_id, vpSize);

    ImGuiID dock_left, dock_center;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, &dock_left, &dock_center);

    ImGuiID dock_right;
    ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Right, 0.25f, &dock_right, &dock_center);

    ImGuiID dock_bottom;
    ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Down, 0.30f, &dock_bottom, &dock_center);

    ImGuiID dock_right_top, dock_right_bottom;
    ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.50f, &dock_right_bottom, &dock_right_top);

    ImGuiID dock_bottom_timeline, dock_bottom_log;
    ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Right, 0.0f, &dock_bottom_log, &dock_bottom_timeline);

    ImGui::DockBuilderDockWindow("Objects", dock_left);
    ImGui::DockBuilderDockWindow("Viewport", dock_center);
    ImGui::DockBuilderDockWindow("Camera", dock_right_top);
    ImGui::DockBuilderDockWindow("Events", dock_right_bottom);
    ImGui::DockBuilderDockWindow("Timeline", dock_bottom_timeline);
    ImGui::DockBuilderDockWindow("Log", dock_bottom_log);

    ImGui::DockBuilderFinish(dockspace_id);
}

void CutsceneEditor::OnMenuSetup() {
    m_ViewMenu = CreatePopupMenu();

    AppendMenu(m_ViewMenu, MF_STRING, CCMD_VIEW_CAMERA_PATH, "Camera Path");
    CheckMenuItem(m_ViewMenu, CCMD_VIEW_CAMERA_PATH, MF_BYCOMMAND | MF_CHECKED);
    AppendMenu(m_ViewMenu, MF_STRING, CCMD_VIEW_FOCUS_PATH, "Focus Path");
    CheckMenuItem(m_ViewMenu, CCMD_VIEW_FOCUS_PATH, MF_BYCOMMAND | MF_CHECKED);
    AppendMenu(m_ViewMenu, MF_STRING, CCMD_VIEW_OBJECT_MARKERS, "Object Markers");
    CheckMenuItem(m_ViewMenu, CCMD_VIEW_OBJECT_MARKERS, MF_BYCOMMAND | MF_CHECKED);

    // Append to menu bar (File is already added by BaseEditor)
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_ViewMenu, "View");
}

void CutsceneEditor::OnMenuCommand(WORD cmd) {
    switch(cmd) {
    case CCMD_VIEW_CAMERA_PATH: {
        m_DrawCameraPath = !m_DrawCameraPath;
        CheckMenuItem(m_ViewMenu, CCMD_VIEW_CAMERA_PATH, MF_BYCOMMAND | (m_DrawCameraPath ? MF_CHECKED : MF_UNCHECKED));
        break;
    }
    case CCMD_VIEW_FOCUS_PATH: {
        m_DrawFocusPath = !m_DrawFocusPath;
        CheckMenuItem(m_ViewMenu, CCMD_VIEW_FOCUS_PATH, MF_BYCOMMAND | (m_DrawFocusPath ? MF_CHECKED : MF_UNCHECKED));
        break;
    }
    case CCMD_VIEW_OBJECT_MARKERS: {
        m_DrawObjectMarkers = !m_DrawObjectMarkers;
        CheckMenuItem(m_ViewMenu, CCMD_VIEW_OBJECT_MARKERS, MF_BYCOMMAND | (m_DrawObjectMarkers ? MF_CHECKED : MF_UNCHECKED));
        break;
    }
    }
}

void CutsceneEditor::OnFileOpen() {
    // First, select the .rep file
    FileDialog repDialog(FileDialog::Mode::OpenFile, "Select cutscene record file", {{"Record File (*.rep)", "*.rep"}});
    if(!repDialog.Show(GetWindowHandle())) return;
    if(!repDialog.GetResult().valid) return;
    std::string repPath = repDialog.GetResult().path;

    // Then, select the scene directory
    FileDialog sceneDialog(FileDialog::Mode::SelectDirectory, "Select mission directory (scene)", {});
    if(!sceneDialog.Show(GetWindowHandle())) return;
    if(!sceneDialog.GetResult().valid) return;
    std::string scenePath = sceneDialog.GetResult().path;

    if (!LoadRecordFile(repPath, scenePath)) return;
}

void CutsceneEditor::OnFileClose() {
    // No save prompt needed for cutscene viewer
}

void CutsceneEditor::OnSceneLoaded() {
    // Nothing extra needed for cutscene editor
}

void CutsceneEditor::OnClear() {
    // Release all active animation sets
    for(auto& match : m_MatchedObjects) {
        if(match.currentAnimSet) {
            if(match.frame && match.frame->GetType() == FRAME_MODEL) {
                I3D_model* model = (I3D_model*)match.frame;
                model->StopAnimation(0);
                model->SetAnimation(NULL, 0, 1.0f, 0);
                model->ResetAllPoses();
            }
            match.currentAnimSet->Release();
            match.currentAnimSet = nullptr;
        }
    }

    CleanupDiffFrames();
    m_ChgFile.Clear();

    m_PlaybackState = PLAYBACK_STOPPED;
    m_CurrentTick = 0;
    m_TotalTicks = 0;
    m_TickAccumulator = 0.0f;

    m_RepHeader = {};
    m_AnimationBlocks.clear();
    m_ObjectDefs.clear();
    m_FrameDataBlob.clear();
    m_CameraChunks.clear();
    m_CameraFocusChunks.clear();
    m_FadeEvents.clear();
    m_ScriptEvents.clear();
    m_WavEvents.clear();
    m_DialogEntries.clear();
    m_LastObjects.clear();
    m_MatchedObjects.clear();

    m_Timeline.itemStarts.clear();
    m_Timeline.itemEnds.clear();
    m_Timeline.editor = nullptr;

    m_SelectedObject = -1;
}
