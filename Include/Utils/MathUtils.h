#pragma once

#include <I3D/I3D_math.h>
#include <I3D/Visuals/I3D_object.h>

#define SPEED_MPH_TO_KMH(input) input * 0.621371f
#define SPEED_KMH_TO_MPH(input) input / 0.621371f

#define SPEED_GAME_TO_KMH(input) input * 3.0f
#define SPEED_GAME_TO_MPH(input) SPEED_KMH_TO_MPH(SPEED_GAME_TO_KMH(input))

#define SPEED_KMH_TO_GAME(input) input / 3.0f
#define SPEED_MPH_TO_GAME(input) SPEED_KMH_TO_GAME(SPEED_MPH_TO_KMH(input))

#define RAD(degrees) degrees*(PI / 180)
#define DEG(radians) radians * (180 / PI)

#define Clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

static float NormalizeAngle(float angle) {
    while(angle < -360.0f)
        angle += 360.0f;
    while(angle > 360.0f)
        angle -= 360.0f;
    return angle;
}

static float LerpAngle(float start, float end, float t) {
    start = NormalizeAngle(start);
    end = NormalizeAngle(end);
    float delta = end - start;

    if(fabs(delta) > 180.0) {
        if(delta > 0) {
            start += 360.0;
        } else {
            end += 360.0;
        }
    }

    float result = start + t * (end - start);
    return NormalizeAngle(result);
}

static float Lerpf(float start, float end, float t) { return start + t * (end - start); }

static S_vector LerpVec(const S_vector& start, const S_vector& end, float t) {
    return S_vector(Lerpf(start.x, end.x, t), Lerpf(start.y, end.y, t), Lerpf(start.z, end.z, t));
}

static S_vector EulerFromDir(const S_vector& dir) {
    S_vector euler;

    float hyp = sqrtf(dir.x * dir.x + dir.z * dir.z);
    euler.y = DEG(atan2f(-dir.y, hyp));

    euler.x = DEG(atan2f(dir.x, dir.z));

    return euler;
}

static S_vector DirFromEuler(const S_vector& euler) {
    S_vector dir;

    float yawRad = RAD(euler.x);
    float pitchRad = RAD(euler.y);

    dir.x = cosf(pitchRad) * sinf(yawRad);
    dir.y = sinf(pitchRad);
    dir.z = cosf(pitchRad) * cosf(yawRad);

    return dir;
}

static S_vector EulerFromQuat(const S_quat& rot) {
    S_vector euler;

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (rot.w * rot.x + rot.y * rot.z);
    float cosr_cosp = 1.0f - 2.0f * (rot.x * rot.x + rot.y * rot.y);
    euler.x = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (rot.w * rot.y - rot.z * rot.x);
    if(I3DFabs(sinp) >= 1.0f - MRG_ZERO) {
        euler.y = copysignf(PI / 2.0f, sinp); // Handle singularity at �90 degrees
    } else {
        euler.y = asinf(sinp);
    }

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (rot.w * rot.z + rot.x * rot.y);
    float cosy_cosp = 1.0f - 2.0f * (rot.y * rot.y + rot.z * rot.z);
    euler.z = atan2f(siny_cosp, cosy_cosp);

    // Convert radians to degrees
    euler.x = euler.x * 180.0f / PI;
    euler.y = euler.y * 180.0f / PI;
    euler.z = euler.z * 180.0f / PI;

    return euler;
}

static S_quat QuatFromEuler(const S_vector& euler) {
    // Convert degrees to radians
    float roll = euler.x * PI / 180.0f;
    float pitch = euler.y * PI / 180.0f;
    float yaw = euler.z * PI / 180.0f;

    // Half angles for quaternion math
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    S_quat quat;
    quat.w = cy * cp * cr + sy * sp * sr;
    quat.x = cy * cp * sr - sy * sp * cr;
    quat.y = sy * cp * sr + cy * sp * cr;
    quat.z = sy * cp * cr - cy * sp * sr;

    // Normalize to prevent drift
    float mag = sqrtf(quat.w * quat.w + quat.x * quat.x + quat.y * quat.y + quat.z * quat.z);
    if(mag > MRG_ZERO) {
        quat.w /= mag;
        quat.x /= mag;
        quat.y /= mag;
        quat.z /= mag;
    } else {
        quat = S_quat(); // Return identity quaternion if magnitude is too small
    }

    return quat;
}

static void ExtractTransformComponents(const S_matrix* matrix, S_vector* translation, S_vector* rotation, S_vector* scale) {
    if(!matrix) return;

    // Extract translation (m_41, m_42, m_43)
    if(translation) {
        translation->x = matrix->m_41;
        translation->y = matrix->m_42;
        translation->z = matrix->m_43;
    }

    // Extract scale (length of first three columns)
    if(scale) {
        S_vector col1(matrix->m_11, matrix->m_21, matrix->m_31);
        S_vector col2(matrix->m_12, matrix->m_22, matrix->m_32);
        S_vector col3(matrix->m_13, matrix->m_23, matrix->m_33);
        scale->x = static_cast<float>(col1.Magnitude());
        scale->y = static_cast<float>(col2.Magnitude());
        scale->z = static_cast<float>(col3.Magnitude());

        // Handle near-zero scales to avoid division by zero
        if(scale->x < MRG_ZERO) scale->x = 1.0f;
        if(scale->y < MRG_ZERO) scale->y = 1.0f;
        if(scale->z < MRG_ZERO) scale->z = 1.0f;
    }

    // Extract rotation (convert to quaternion, then to Euler angles)
    if(rotation) {
        // Remove scale to get rotation matrix
        S_matrix rotMat = *matrix;
        if(scale) {
            rotMat.m_11 /= scale->x;
            rotMat.m_21 /= scale->x;
            rotMat.m_31 /= scale->x;
            rotMat.m_12 /= scale->y;
            rotMat.m_22 /= scale->y;
            rotMat.m_32 /= scale->y;
            rotMat.m_13 /= scale->z;
            rotMat.m_23 /= scale->z;
            rotMat.m_33 /= scale->z;
        }

        // Convert to quaternion
        S_quat quat;
        quat.Make(rotMat);

        // Convert quaternion to Euler angles (degrees)
        float sinr_cosp = 2.0f * (quat.w * quat.x + quat.y * quat.z);
        float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
        rotation->x = atan2f(sinr_cosp, cosr_cosp) * 180.0f / PI; // Roll

        float sinp = 2.0f * (quat.w * quat.y - quat.z * quat.x);
        if(I3DFabs(sinp) >= 1.0f - MRG_ZERO) {
            rotation->y = copysignf(PI / 2.0f, sinp) * 180.0f / PI; // Pitch
        } else {
            rotation->y = asinf(sinp) * 180.0f / PI;
        }

        float siny_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
        float cosy_cosp = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
        rotation->z = atan2f(siny_cosp, cosy_cosp) * 180.0f / PI; // Yaw
    }
}

static void PackTransformComponents(S_matrix* matrix, const S_vector* translation, const S_vector* rotation, const S_vector* scale) {
    if(!matrix) return;

    // Start with identity matrix
    matrix->Identity();

    // Apply rotation (from Euler angles to quaternion to matrix)
    if(rotation) {
        // Convert Euler angles (degrees) to quaternion
        float roll = rotation->x * PI / 180.0f;
        float pitch = rotation->y * PI / 180.0f;
        float yaw = rotation->z * PI / 180.0f;

        float cy = cosf(yaw * 0.5f);
        float sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f);
        float sp = sinf(pitch * 0.5f);
        float cr = cosf(roll * 0.5f);
        float sr = sinf(roll * 0.5f);

        S_quat quat;
        quat.w = cy * cp * cr + sy * sp * sr;
        quat.x = cy * cp * sr - sy * sp * cr;
        quat.y = sy * cp * sr + cy * sp * cr;
        quat.z = sy * cp * cr - cy * sp * sr;

        // Normalize quaternion
        float mag = sqrtf(quat.w * quat.w + quat.x * quat.x + quat.y * quat.y + quat.z * quat.z);
        if(mag > MRG_ZERO) {
            quat.w /= mag;
            quat.x /= mag;
            quat.y /= mag;
            quat.z /= mag;
        }

        // Convert quaternion to rotation matrix
        matrix->SetRot(quat);
    }

    // Apply scale
    if(scale) {
        matrix->m_11 *= scale->x;
        matrix->m_12 *= scale->y;
        matrix->m_13 *= scale->z;
        matrix->m_21 *= scale->x;
        matrix->m_22 *= scale->y;
        matrix->m_23 *= scale->z;
        matrix->m_31 *= scale->x;
        matrix->m_32 *= scale->y;
        matrix->m_33 *= scale->z;
    }

    // Apply translation
    if(translation) {
        matrix->m_41 = translation->x;
        matrix->m_42 = translation->y;
        matrix->m_43 = translation->z;
    }
}

static constexpr float EPS_AABB = 1e-6f;
static constexpr float FLT_INF = FLT_MAX;

// Returns true if ray intersects AABB [min..max], sets tNear (entry), tFar (exit)
// - For picking: if true && tNear < bestDist, candidate!
// - Handles ray (t>=0), inside/outside, parallel rays perfectly.
// - ~5 cycles on modern CPU, branchless where possible.
inline bool RayAABB(const S_vector& origin,
                    const S_vector& dir, // Doesn't need to be normalized
                    const S_vector& boxMin,
                    const S_vector& boxMax,
                    float& tNear,
                    float& tFar) {
    // X slab
    if(fabsf(dir.x) < EPS_AABB) {
        if(origin.x < boxMin.x || origin.x > boxMax.x) return false;
        tNear = -FLT_INF;
        tFar = FLT_INF;
    } else {
        float invX = 1.0f / dir.x;
        float tx1 = (boxMin.x - origin.x) * invX;
        float tx2 = (boxMax.x - origin.x) * invX;
        tNear = fminf(tx1, tx2);
        tFar = fmaxf(tx1, tx2);
    }

    // Y slab
    float tyNear, tyFar;
    if(fabsf(dir.y) < EPS_AABB) {
        if(origin.y < boxMin.y || origin.y > boxMax.y) return false;
        tyNear = -FLT_INF;
        tyFar = FLT_INF;
    } else {
        float invY = 1.0f / dir.y;
        float ty1 = (boxMin.y - origin.y) * invY;
        float ty2 = (boxMax.y - origin.y) * invY;
        tyNear = fminf(ty1, ty2);
        tyFar = fmaxf(ty1, ty2);
    }

    // Z slab
    float tzNear, tzFar;
    if(fabsf(dir.z) < EPS_AABB) {
        if(origin.z < boxMin.z || origin.z > boxMax.z) return false;
        tzNear = -FLT_INF;
        tzFar = FLT_INF;
    } else {
        float invZ = 1.0f / dir.z;
        float tz1 = (boxMin.z - origin.z) * invZ;
        float tz2 = (boxMax.z - origin.z) * invZ;
        tzNear = fminf(tz1, tz2);
        tzFar = fmaxf(tz1, tz2);
    }

    // Overall interval
    tNear = fmaxf(tNear, fmaxf(tyNear, tzNear)); // Entry = max of nears
    tFar = fminf(tFar, fminf(tyFar, tzFar)); // Exit = min of fars

    // Hit?
    return (tNear <= tFar) && (tFar >= 0.0f);
}

// Convenience: Returns HIT DISTANCE (tNear, clamped >=0) or -1.0f (miss)
inline float RayAABB_HitDist(const S_vector& origin, const S_vector& dir, const S_vector& boxMin, const S_vector& boxMax, float maxDist = FLT_MAX) {
    float tNear, tFar;
    if(!RayAABB(origin, dir, boxMin, boxMax, tNear, tFar)) { return -1.0f; }
    float tHit = fmaxf(0.0f, tNear); // Clamp to ray start
    return (tHit <= fminf(tFar, maxDist)) ? tHit : -1.0f;
}

static bool RayTriangleIntersect(S_vector orig, S_vector dir, S_vector v0, S_vector v1, S_vector v2, float& t, float& u, float& vv) {
    static constexpr float EPS = 1e-6f;
    S_vector e1 = v1 - v0, e2 = v2 - v0;
    S_vector P = dir.Cross(e2);
    float det = e1.Dot(P);
    if(abs(det) < EPS) return false;
    float invDet = 1.0f / det;
    S_vector T = orig - v0;
    u = T.Dot(P) * invDet;
    if(u < 0 || u > 1) return false;
    S_vector Q = T.Cross(e1);
    vv = dir.Dot(Q) * invDet;
    if(vv < 0 || u + vv > 1) return false;
    t = e2.Dot(Q) * invDet;
    return t > EPS;
}

static float TestMeshRaycast(S_vector rayOrigin, S_vector rayDir, I3D_object* obj, float maxDist) {
    I3D_mesh_object* mesh = obj->GetMesh();
    I3D_mesh_level* lod = mesh->GetLOD(0); // Highest detail; or pick by dist-to-cam

    S_vertex_3d* verts = lod->LockVertices(D3DLOCK_READONLY); // Flags: 0=read?

    const S_matrix& worldMat = obj->GetWorldMat(); // Transforms local → world

    float closestT = maxDist;

    for (uint32_t vNum = 0; vNum < lod->m_uNumVertices; vNum += 3) {
        S_vector v0 = verts[vNum].pos * worldMat;
        S_vector v1 = verts[vNum + 1].pos * worldMat;
        S_vector v2 = verts[vNum + 2].pos * worldMat;

        float t, u, v;
        if(RayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, u, v) && t > 0 && t < closestT && u >= 0 && v >= 0 && u + v <= 1) { closestT = t; }
    }

    lod->UnlockVertices();
    return closestT < maxDist ? closestT : -1.0f;
}

struct ScreenPos {
    ImVec2 pos;
    float depth = 0.0f;
    bool valid = false;
};

static bool ProjectWorldToScreen(I3D_scene* scene, const S_vector& worldPos, ScreenPos& out) {
    S_vector4 clipSpace;
    LS3D_RESULT res = scene->TransformPoints(&worldPos, &clipSpace, 1);
    if(res != I3D_OK) return false;

    if(clipSpace.w <= 0.0f) return false;

    float ndcX = clipSpace.x / clipSpace.w;
    float ndcY = clipSpace.y / clipSpace.w;
    out.depth = clipSpace.z / clipSpace.w;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    out.pos.x = (ndcX * 0.5f + 0.5f) * displaySize.x;
    out.pos.y = (1.0f - (ndcY * 0.5f + 0.5f)) * displaySize.y;

    out.valid = (ndcX >= -1.01f && ndcX <= 1.01f && ndcY >= -1.01f && ndcY <= 1.01f && out.depth <= 1.0f && out.depth >= -0.01f);

    return out.valid;
}