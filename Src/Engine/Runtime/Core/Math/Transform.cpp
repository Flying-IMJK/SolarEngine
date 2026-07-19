#include "Transform.h"
#include "Matrix3x3.h"

#include "Runtime/Core/Types/Strings/String.h"
//-------------------------------------------------------------------------

namespace SE
{
    Transform Transform::Identity(Float3(0, 0, 0));

    Transform::Transform(const Float3& position, const Matrix3x3& rotationScale)
        : Translation(position)
    {
        rotationScale.Decompose(Scale, Orientation);
    }

    String Transform::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    bool Transform::IsIdentity() const
    {
        return Translation.IsZero() && Orientation.IsIdentity() && Scale.IsOne();
    }

    bool Transform::IsNanOrInfinity() const
    {
        return Translation.IsNanOrInfinity() || Orientation.IsNanOrInfinity() || Scale.IsNanOrInfinity();
    }

    Matrix Transform::GetRotation() const
    {
        Matrix result;
        Matrix::RotationQuaternion(Orientation, result);
        return result;
    }

    void Transform::GetRotation(Matrix& result) const
    {
        Matrix::RotationQuaternion(Orientation, result);
    }

    void Transform::SetRotation(const Matrix& value)
    {
        Quaternion::RotationMatrix(value, Orientation);
    }

    Matrix Transform::GetWorld() const
    {
        Matrix result;
        Matrix::Transformation(Scale, Orientation, Translation, result);
        return result;
    }

    void Transform::GetWorld(Matrix& result) const
    {
        Matrix::Transformation(Scale, Orientation, Translation, result);
    }

    Transform Transform::Add(const Float3& translation) const
    {
        Transform result;
        result.Orientation = Orientation;
        result.Scale = Scale;
        Float3::Add(Translation, translation, result.Translation);
        return result;
    }

    Transform Transform::Add(const Transform& other) const
    {
        Transform result;
        Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
        result.Orientation.Normalize();
        Float3::Multiply(Scale, other.Scale, result.Scale);
        Float3::Add(Translation, other.Translation, result.Translation);
        return result;
    }

    Transform Transform::Subtract(const Transform& other) const
    {
        Transform result;
        Float3::Subtract(Translation, other.Translation, result.Translation);
        const Quaternion invRotation = other.Orientation.Conjugated();
        Quaternion::Multiply(Orientation, invRotation, result.Orientation);
        result.Orientation.Normalize();
        Float3::Divide(Scale, other.Scale, result.Scale);
        return result;
    }

    void Transform::LocalToWorld(const Transform& other, Transform& result) const
    {
        //Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
        const float a = Orientation.y * other.Orientation.z - Orientation.z * other.Orientation.y;
        const float b = Orientation.z * other.Orientation.x - Orientation.x * other.Orientation.z;
        const float c = Orientation.x * other.Orientation.y - Orientation.y * other.Orientation.x;
        const float d = Orientation.x * other.Orientation.x + Orientation.y * other.Orientation.y + Orientation.z * other.Orientation.z;
        result.Orientation.x = Orientation.x * other.Orientation.w + other.Orientation.x * Orientation.w + a;
        result.Orientation.y = Orientation.y * other.Orientation.w + other.Orientation.y * Orientation.w + b;
        result.Orientation.z = Orientation.z * other.Orientation.w + other.Orientation.z * Orientation.w + c;
        result.Orientation.w = Orientation.w * other.Orientation.w - d;

        //result.Orientation.Normalize();
        const float length = result.Orientation.Length();
        if (length > Math::ZeroTolerance)
        {
            const float inv = 1.0f / length;
            result.Orientation.x *= inv;
            result.Orientation.y *= inv;
            result.Orientation.z *= inv;
            result.Orientation.w *= inv;
        }

        //Float3::Multiply(Scale, other.Scale, result.Scale);
        result.Scale = Float3(Scale.x * other.Scale.x, Scale.y * other.Scale.y, Scale.z * other.Scale.z);

        //Float3 tmp; Float3::Multiply(other.Translation, Scale, tmp);
        Float3 tmp = Float3(other.Translation.x * Scale.x, other.Translation.y * Scale.y, other.Translation.z * Scale.z);

        //Float3::Transform(tmp, Orientation, tmp);
        const float x = Orientation.x + Orientation.x;
        const float y = Orientation.y + Orientation.y;
        const float z = Orientation.z + Orientation.z;
        const float wx = Orientation.w * x;
        const float wy = Orientation.w * y;
        const float wz = Orientation.w * z;
        const float xx = Orientation.x * x;
        const float xy = Orientation.x * y;
        const float xz = Orientation.x * z;
        const float yy = Orientation.y * y;
        const float yz = Orientation.y * z;
        const float zz = Orientation.z * z;
        tmp = Float3(
            tmp.x * (1.0f - yy - zz) + tmp.y * (xy - wz) + tmp.z * (xz + wy),
            tmp.x * (xy + wz) + tmp.y * (1.0f - xx - zz) + tmp.z * (yz - wx),
            tmp.x * (xz - wy) + tmp.y * (yz + wx) + tmp.z * (1.0f - xx - yy));

        //Float3::Add(tmp, Translation, result.Translation);
        result.Translation = Float3(tmp.x + Translation.x, tmp.y + Translation.y, tmp.z + Translation.z);
    }

    void Transform::LocalToWorldVector(const Float3& vector, Float3& result) const
    {
        Float3 tmp = vector * Scale;
        Float3::Transform(tmp, Orientation, result);
    }

    void Transform::LocalToWorld(const Float3& point, Float3& result) const
    {
        Float3 tmp = point * Scale;
        Float3::Transform(tmp, Orientation, tmp);
        Float3::Add(tmp, Translation, result);
    }

    void Transform::WorldToLocal(const Transform& other, Transform& result) const
    {
        Float3 invScale = Scale;
        if (invScale.x != 0.0f)
            invScale.x = 1.0f / invScale.x;
        if (invScale.y != 0.0f)
            invScale.y = 1.0f / invScale.y;
        if (invScale.z != 0.0f)
            invScale.z = 1.0f / invScale.z;
        const Quaternion invRotation = Orientation.Conjugated();
        Quaternion::Multiply(invRotation, other.Orientation, result.Orientation);
        result.Orientation.Normalize();
        Float3::Multiply(other.Scale, invScale, result.Scale);
        const Float3 tmp = other.Translation - Translation;
        Float3::Transform(tmp, invRotation, result.Translation);
        Float3::Multiply(result.Translation, invScale, result.Translation);
    }

    void Transform::WorldToLocal(const Float3& point, Float3& result) const
    {
        Float3 invScale = Scale;
        if (invScale.x != 0.0f)
            invScale.x = 1.0f / invScale.x;
        if (invScale.y != 0.0f)
            invScale.y = 1.0f / invScale.y;
        if (invScale.z != 0.0f)
            invScale.z = 1.0f / invScale.z;
        const Quaternion invRotation = Orientation.Conjugated();
        result = point - Translation;
        Float3::Transform(result, invRotation, result);
        result *= invScale;
    }

    void Transform::WorldToLocalVector(const Float3& vector, Float3& result) const
    {
        Float3 invScale = Scale;
        if (invScale.x != 0.0f)
            invScale.x = 1.0f / invScale.x;
        if (invScale.y != 0.0f)
            invScale.y = 1.0f / invScale.y;
        if (invScale.z != 0.0f)
            invScale.z = 1.0f / invScale.z;
        const Quaternion invRotation = Orientation.Conjugated();
        Float3::Transform(vector, invRotation, result);
        result *= invScale;
    }

    void Transform::WorldToLocal(const Quaternion& rotation, Quaternion& result) const
    {
        Quaternion orientation = Orientation;
        orientation.Conjugate();
        Quaternion::Multiply(orientation, rotation, orientation);
        orientation.Normalize();
        result = orientation;
    }

    Float3 Transform::GetRight() const
    {
        return Float3::Transform(Float3::Right, Orientation);
    }

    Float3 Transform::GetLeft() const
    {
        return Float3::Transform(Float3::Left, Orientation);
    }

    Float3 Transform::GetUp() const
    {
        return Float3::Transform(Float3::Up, Orientation);
    }

    Float3 Transform::GetDown() const
    {
        return Float3::Transform(Float3::Down, Orientation);
    }

    Float3 Transform::GetForward() const
    {
        return Float3::Transform(Float3::Forward, Orientation);
    }

    Float3 Transform::GetBackward() const
    {
        return Float3::Transform(Float3::Backward, Orientation);
    }

    Transform Transform::Lerp(const Transform& t1, const Transform& t2, float amount)
    {
        Transform result;
        Float3::Lerp(t1.Translation, t2.Translation, amount, result.Translation);
        Quaternion::Slerp(t1.Orientation, t2.Orientation, amount, result.Orientation);
        Float3::Lerp(t1.Scale, t2.Scale, amount, result.Scale);
        return result;
    }

    void Transform::Lerp(const Transform& t1, const Transform& t2, float amount, Transform& result)
    {
        Float3::Lerp(t1.Translation, t2.Translation, amount, result.Translation);
        Quaternion::Slerp(t1.Orientation, t2.Orientation, amount, result.Orientation);
        Float3::Lerp(t1.Scale, t2.Scale, amount, result.Scale);
    }

}