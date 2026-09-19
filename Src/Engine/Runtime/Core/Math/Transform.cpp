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
        const float a = Orientation.Y * other.Orientation.Z - Orientation.Z * other.Orientation.Y;
        const float b = Orientation.Z * other.Orientation.X - Orientation.X * other.Orientation.Z;
        const float c = Orientation.X * other.Orientation.Y - Orientation.Y * other.Orientation.X;
        const float d = Orientation.X * other.Orientation.X + Orientation.Y * other.Orientation.Y + Orientation.Z * other.Orientation.Z;
        result.Orientation.X = Orientation.X * other.Orientation.W + other.Orientation.X * Orientation.W + a;
        result.Orientation.Y = Orientation.Y * other.Orientation.W + other.Orientation.Y * Orientation.W + b;
        result.Orientation.Z = Orientation.Z * other.Orientation.W + other.Orientation.Z * Orientation.W + c;
        result.Orientation.W = Orientation.W * other.Orientation.W - d;

        //result.Orientation.Normalize();
        const float length = result.Orientation.Length();
        if (length > Math::ZeroTolerance)
        {
            const float inv = 1.0f / length;
            result.Orientation.X *= inv;
            result.Orientation.Y *= inv;
            result.Orientation.Z *= inv;
            result.Orientation.W *= inv;
        }

        //Float3::Multiply(Scale, other.Scale, result.Scale);
        result.Scale = Float3(Scale.X * other.Scale.X, Scale.Y * other.Scale.Y, Scale.Z * other.Scale.Z);

        //Float3 tmp; Float3::Multiply(other.Translation, Scale, tmp);
        Float3 tmp = Float3(other.Translation.X * Scale.X, other.Translation.Y * Scale.Y, other.Translation.Z * Scale.Z);

        //Float3::Transform(tmp, Orientation, tmp);
        const float x = Orientation.X + Orientation.X;
        const float y = Orientation.Y + Orientation.Y;
        const float z = Orientation.Z + Orientation.Z;
        const float wx = Orientation.W * x;
        const float wy = Orientation.W * y;
        const float wz = Orientation.W * z;
        const float xx = Orientation.X * x;
        const float xy = Orientation.X * y;
        const float xz = Orientation.X * z;
        const float yy = Orientation.Y * y;
        const float yz = Orientation.Y * z;
        const float zz = Orientation.Z * z;
        tmp = Float3(
            tmp.X * (1.0f - yy - zz) + tmp.Y * (xy - wz) + tmp.Z * (xz + wy),
            tmp.X * (xy + wz) + tmp.Y * (1.0f - xx - zz) + tmp.Z * (yz - wx),
            tmp.X * (xz - wy) + tmp.Y * (yz + wx) + tmp.Z * (1.0f - xx - yy));

        //Float3::Add(tmp, Translation, result.Translation);
        result.Translation = Float3(tmp.X + Translation.X, tmp.Y + Translation.Y, tmp.Z + Translation.Z);
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
        if (invScale.X != 0.0f)
            invScale.X = 1.0f / invScale.X;
        if (invScale.Y != 0.0f)
            invScale.Y = 1.0f / invScale.Y;
        if (invScale.Z != 0.0f)
            invScale.Z = 1.0f / invScale.Z;
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
        if (invScale.X != 0.0f)
            invScale.X = 1.0f / invScale.X;
        if (invScale.Y != 0.0f)
            invScale.Y = 1.0f / invScale.Y;
        if (invScale.Z != 0.0f)
            invScale.Z = 1.0f / invScale.Z;
        const Quaternion invRotation = Orientation.Conjugated();
        result = point - Translation;
        Float3::Transform(result, invRotation, result);
        result *= invScale;
    }

    void Transform::WorldToLocalVector(const Float3& vector, Float3& result) const
    {
        Float3 invScale = Scale;
        if (invScale.X != 0.0f)
            invScale.X = 1.0f / invScale.X;
        if (invScale.Y != 0.0f)
            invScale.Y = 1.0f / invScale.Y;
        if (invScale.Z != 0.0f)
            invScale.Z = 1.0f / invScale.Z;
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
