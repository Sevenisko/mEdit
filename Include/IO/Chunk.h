#pragma once

using ChunkType = uint16_t;

#pragma pack(push, 1)
struct ChunkHeader {
    ChunkType Type;
    uint32_t Size;
};
#pragma pack(pop)

struct InputChunkLevelData {
    size_t Pos; // Start position of chunk data (after header)
    size_t ChunkSize; // Size of chunk data (excluding header)
};

struct OutputChunkLevelData {
    ChunkType Type;
    size_t HeaderPos; // Position of chunk header
    size_t DataStartPos; // Start position of chunk data (after header)
};

enum E_CHUNK_TYPE {
    // general sub-chunk types
    CT_VERSION = 0x0001,
    // word maj
    // word min
    // copyright info text
    CT_COPYRIGHT = 0x0002,
    // text
    CT_NAME = 0x0010,
    // null-terminated string
    CT_POSITION = 0x0020,
    // S_vector
    CT_DIRECTION = 0x0021,
    // S_vector
    CT_QUATERNION = 0x0022,
    // S_quat
    CT_SCALE = 0x002D,
    // S_vector
    CT_NUSCALE = CT_SCALE,
    // S_vector
    CT_FLOAT = 0x0024,
    CT_INT = 0x0025,
    CT_COLOR = 0x0026,
    // S_vector
    CT_USCALE = 0x002a,
    // float
    CT_PIVOT = 0x002b,
    // S_vector
    CT_WORLD_POSITION = 0x002c,
    // S_vector

    CT_ROTATION = 0x0030,
    // S_vector axis / float angle
    CT_THUMBNAIL = 0x0040,
    // image data

    // mission data
    CT_BASECHUNK = 0x4c53,

    CT_CAMERA_FOV = 0x3010,
    // float
    CT_CAMERA_RANGE = 0x3011,
    // float far_range
    CT_CAMERA_ORTHOGONAL = 0x3012,
    CT_CAMERA_ORTHO_SCALE = 0x3013,
    CT_CAMERA_EDIT_RANGE = 0x3014,
    // float far_range

    CT_SCENE_BGND_COLOR = 0x3200,
    // S_vector scene_background_color
    CT_SCENE_CLIPPING_RANGE = 0x3211,
    // float near_range, far_range

    CT_MODIFICATIONS = 0x4000,
    CT_MODIFICATION = 0x4010,
    // CT_NAME
    // CT_POSITION
    // CT_QUATERNION
    CT_FRAME_TYPE = 0x4011,
    CT_FRAME_SUBTYPE = 0x4012,
    // dword I3D_FRAME_TYPE
    CT_FRAME_FLAGS = 0x4014,
    // dword frame_flags
    CT_LINK = 0x4020,
    // CT_NAME   (parent name)
    CT_HIDDEN = 0x4033,

    CT_MODIFY_MODELFILENAME = 0x2012,
    // string

    CT_MODIFY_LIGHT = 0x4040, // light params, followed by params:
    CT_LIGHT_TYPE = 0x4041,
    // dword type
    // CT_COLOR color
    CT_LIGHT_POWER = 0x4042,
    // float power
    CT_LIGHT_CONE = 0x4043,
    // float cone_in, cone_out
    CT_LIGHT_RANGE = 0x4044,
    // float range_n, range_f
    CT_LIGHT_MODE = 0x4045,
    // dword m_Mode
    CT_LIGHT_SECTOR = 0x4046,
    // char[] sector_name
    CT_LIGHT_SPECULAR_COLOR = 0x4047, // obsolete
    CT_LIGHT_SPECULAR_POWER = 0x4048, // obsolete
    CT_MODIFY_SOUND = 0x4060,
    // CT_NAME file_name
    CT_SOUND_TYPE = 0x4061,
    // dword type
    CT_SOUND_VOLUME = 0x4062,
    // float volume
    CT_SOUND_OUTVOL = 0x4063,
    // float outside_volume
    CT_SOUND_CONE = 0x4064,
    // float cone_in, cone_out
    CT_SOUND_RANGE = 0x4068,
    // float range_n, range_f, range_fade
    CT_SOUND_LOOP = 0x4066,
    CT_SOUND_ENABLE = 0x4067,
    CT_SOUND_SECTOR = 0xb200,
    CT_SOUND_STREAMING = 0x4069,
    // char[] sector_name
    CT_MODIFY_VOLUME = 0x4070,
    CT_VOLUME_TYPE = 0x4071,
    // dword type
    CT_VOLUME_MATERIAL = 0x4072,
    // dword material_id
    CT_MODIFY_OCCLUDER = 0x4083,
    // dword num_vertices
    // S_vector[num_vertices] vertices
    // dword num_indices
    // uint16_t[num_indices] indices

    CT_MODIFY_VISUAL = 0x4090,
    // dword visual_type
    CT_VISUAL_MATERIAL = 0x4092,
    // dword material_id
    CT_MODIFY_LIT_OBJECT = 0x40a0,
    // lit-object data
    CT_MODIFY_SECTOR = 0x40b0,
    CT_SECTOR_ENVIRONMENT = 0x40b2,
    // word env_id
    CT_SECTOR_TEMPERATURE = 0x40b3,
    // float
    CT_MODIFY_VISUAL_PROPERTY = 0x40c0,
    // byte I3D_PROPERTYTYPE
    // word property_index
    // property data (variable length)
    CT_MODIFY_BRIGHTNESS = 0x40e0,
    // float brightness

    // cache.bin chunk types
    CT_CITY = 0x1f4,
    CT_PART = 0x03e8,
    CT_FRAME = 0x07d0,

    // game-specific types
    CT_ACTORS = 0xae20,
    CT_ACTOR = 0xae21,
    CT_ACTOR_TYPE = 0xae22,
    CT_ACTOR_NAME = 0xae23,
    CT_ACTOR_DATA = 0xae24,
    CT_SCRIPTS = 0xae50,
    CT_SCRIPT = 0xae51
};