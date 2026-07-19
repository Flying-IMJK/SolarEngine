
add_subdirectory(TypeConverters)

RUNTIME_ADD(
        Math.h
        Math.cpp
        Matrix.h
        Matrix.cpp
        Matrix3x3.h
        Matrix3x3.cpp
        Quaternion.h
        Quaternion.cpp
        Transform.h
        Transform.cpp
        Random.h
        AABBTree.h
        AABBTree.cpp

        Vector2.h
        Vector2.cpp
        Vector3.h
        Vector3.cpp
        Vector4.h
        Vector4.cpp
        Half.h
        Half.cpp

        Color.h
        Color.cpp

        NumericRange.h
        NumericRange.cpp
        Curves.h
        FloatCurve.h
        FloatCurve.cpp
        SIMD.h

        Line.h
        Line.cpp
        Plane.h
        Plane.cpp
        BoundingVolumes.h
        BoundingVolumes.cpp

        Rectangle.h
        Rectangle.cpp

        CollisionsHelper.h
        CollisionsHelper.cpp


#        CS
        BoundingBox.cs
        BoundingFrustum.cs
        BoundingSphere.cs
        AlphaBlendMode.cs
        CollisionsHelper.cs
        Color.cs
        Color.Palette.cs
        Color32.cs
        ColorHSV.cs
        Double.cs
        Float.cs
        FloatR10G10B10A2.cs
        FloatR11G11B10.cs
        Half.cs
        HalfUtils.cs
        Int.cs
        Margin.cs
        Mathd.cs
        MathFields.cs
        Mathf.cs
        Matrix.cs
        Matrix2x2.cs
        Matrix3x3.cs
        OrientedBoundingBox.cs
        Plane.cs
        Quaternion.cs
        Ray.cs
        Rectangle.cs
        SphericalHarmonics.cs
        Transform.cs
        Vector2.cs
        Vector3.cs
        Vector4.cs
        Viewport.cs
)