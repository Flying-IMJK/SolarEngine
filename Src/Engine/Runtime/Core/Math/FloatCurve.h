#pragma once
#include "Math.h"
#include "NumericRange.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Core/Types/Strings/String.h"

//-------------------------------------------------------------------------
// Interpolation Curve
//-------------------------------------------------------------------------
// A sequence of piece wise cubic hermite splines -
// This curve is useful for when you want remap one float value to another

namespace SE
{
    class SE_API_RUNTIME FloatCurve
    {
#ifdef SE_DEVELOPMENT
        static uint16 s_pointIdentifierGenerator;
#endif

    public:
        // Set curve state from a string
        static bool FromString(String const &inStr, FloatCurve &outCurve);

        // The tangent options per point
        enum TangentMode : uint8
        {
            Free,
            Locked,
        };

        // A 1D curve point - the ID is needed for the tooling to distinguish points from one another
        struct Point
        {
            inline bool operator==(Point const &rhs) const
            {
                return m_parameter == rhs.m_parameter && m_value == rhs.m_value && m_inTangent == rhs.m_inTangent && m_outTangent == rhs.m_outTangent && m_tangentMode == rhs.m_tangentMode;
            }

            inline bool operator!=(Point const &rhs) const { return !operator==(rhs); }

            float m_parameter;
            float m_value;
            float m_inTangent = 1;
            float m_outTangent = 1;
            TangentMode m_tangentMode = TangentMode::Free;

#ifdef SE_DEVELOPMENT
            uint16 m_ID; // Not serialized, runtime generated through edit operations
#endif
        };

    public:
        // Curve query
        //-------------------------------------------------------------------------

        inline int32 GetNumPoints() const { return (int32)m_points.Count(); }
        Point const &GetPoint(int32 pointIdx) const
        {
            ENGINE_ASSERT(pointIdx >= 0 && pointIdx < GetNumPoints());
            return m_points[pointIdx];
        }

        // Get the range for the parameters that this curve covers
        inline FloatRange GetParameterRange() const
        {
            FloatRange range;
            for (auto i = 0u; i < m_points.Count(); i++)
            {
                range.begin = Math::Min(range.begin, m_points[i].m_parameter);
                range.end = Math::Max(range.end, m_points[i].m_parameter);
            }
            return range;
        }

        // Get the range for the values that this curve covers
        // Note: this will evaluate the curve to find the actual value range and so is pretty expensive!
        inline FloatRange GetValueRange() const
        {
            constexpr static float const numPointsToEvaluate = 150;
            FloatRange const parameterRange = GetParameterRange();
            float const stepT = parameterRange.GetLength() / numPointsToEvaluate;

            FloatRange valueRange(Evaluate(0.0f));
            for (auto i = 1; i < numPointsToEvaluate; i++)
            {
                float const t = parameterRange.begin + (i * stepT);
                valueRange.GrowRange(Evaluate(t));
            }

            return valueRange;
        }

        // Evaluate the curve and return the value for the specified input parameter
        // If the parameter supplied is outside the parameter range the value returned will be that of the nearest extremity point
        float Evaluate(float parameter) const;

        // Curve manipulation
        //-------------------------------------------------------------------------

        void AddPoint(float parameter, float value, float inTangent = 1.0f, float outTangent = 1.0f);
        void EditPoint(int32 pointIdx, float parameter, float value);
        void SetPointTangentMode(int32 pointIdx, TangentMode mode);
        void SetPointOutTangent(int32 pointIdx, float tangent);
        void SetPointInTangent(int32 pointIdx, float tangent);
        void RemovePoint(int32 pointIdx);
        void Clear() { m_points.Clear(); }

#ifdef SE_DEVELOPMENT
        void RegeneratePointIDs();
#endif

        // Serialization
        //-------------------------------------------------------------------------

        // Set the string state from a string
        // Warning! This will crash on invalid strings, if you are not sure if the string is valid, use the static function supplied
        inline void FromString(String const &inStr)
        {
            bool result = FloatCurve::FromString(inStr, *this);
            ENGINE_ASSERT(result);
        }

        // Returns the curve state as a string
        String ToString() const;

        // Operators
        //-------------------------------------------------------------------------

        // Equality operators needed for core types
        bool operator==(FloatCurve const &rhs) const;

        // Equality operators needed for core types
        inline bool operator!=(FloatCurve const &rhs) const { return !operator==(rhs); }

    private:
        inline void SortPoints()
        {
            Function<bool(const Point &, const Point &)> SortPredicate = [](const Point &a, const Point &b)
            {
                return a.m_parameter < b.m_parameter;
            };

            Sorting::QuickSort(m_points, SortPredicate);
        }

    private:
        List<Point> m_points; // Space for 4 curves
    };
}