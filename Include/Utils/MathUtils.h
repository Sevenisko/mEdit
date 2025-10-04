#pragma once

#include <I3D/I3D_math.h>

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