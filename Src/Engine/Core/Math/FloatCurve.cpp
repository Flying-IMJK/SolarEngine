#include "FloatCurve.h"
#include "Curves.h"
#include "Core/Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE
{
    #ifdef SE_DEVELOPMENT
    // ID generator needed for curve editor
    uint16 FloatCurve::s_pointIdentifierGenerator = 0;
    #endif

    //-------------------------------------------------------------------------

    float FloatCurve::Evaluate( float parameter ) const
    {
        float result = 0;

        if ( m_points.IsEmpty() )
        {
            return result;
        }

        if ( m_points.Count() == 1 )
        {
            return m_points[0].m_value;
        }

        //-------------------------------------------------------------------------

        FloatRange const parameterRange = GetParameterRange();

        if ( parameterRange.ContainsExclusive( parameter ) )
        {
            int32_t const numPoints = GetNumPoints();
            int32_t const numCurves = numPoints - 1;
            for ( int32_t i = 0; i < numCurves; i++ )
            {
                int32_t const startIdx = i;
                int32_t const endIdx = i + 1;

                // If the parameter is within this curve, evaluate it
                if ( parameter >= m_points[startIdx].m_parameter && parameter <= m_points[endIdx].m_parameter )
                {
                    float const T = ( parameter - m_points[startIdx].m_parameter ) / ( m_points[endIdx].m_parameter - m_points[startIdx].m_parameter );
                    // result = Math::CubicHermite::GetPoint( m_points[startIdx].m_value, m_points[startIdx].m_outTangent, m_points[endIdx].m_value, m_points[endIdx].m_inTangent, T );
                    break;
                }
            }
        }
        else // Outside curve range
        {
            if ( parameter <= parameterRange.begin )
            {
                result = m_points[0].m_value;
            }
            else
            {
                result = m_points.Last().m_value;
            }
        }

        return result;
    }

    void FloatCurve::AddPoint( float parameter, float value, float inTangent, float outTangent )
    {
        m_points.Add( { parameter, value, inTangent, outTangent } );
        SortPoints();

        #ifdef SE_DEVELOPMENT
        RegeneratePointIDs();
        #endif
    }

    void FloatCurve::EditPoint( int32_t pointIdx, float parameter, float value )
    {
        ENGINE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_parameter = parameter;
        m_points[pointIdx].m_value = value;
        SortPoints();
    }

    void FloatCurve::SetPointTangentMode( int32_t pointIdx, TangentMode mode )
    {
        ENGINE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_tangentMode = mode;
    }

    void FloatCurve::SetPointOutTangent( int32_t pointIdx, float tangent )
    {
        ENGINE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );
        m_points[pointIdx].m_outTangent = tangent;
    }

    void FloatCurve::SetPointInTangent( int32_t pointIdx, float tangent )
    {
        m_points[pointIdx].m_inTangent = tangent;
    }

    void FloatCurve::RemovePoint( int32_t pointIdx )
    {
        ENGINE_ASSERT( pointIdx >= 0 && pointIdx < GetNumPoints() );

        m_points.RemoveAt(pointIdx );

        #ifdef SE_DEVELOPMENT
        RegeneratePointIDs();
        #endif
    }

    //-------------------------------------------------------------------------
    // Core Type Requirements
    //-------------------------------------------------------------------------

    bool FloatCurve::operator==( FloatCurve const& rhs ) const
    {
        if ( m_points.Count() != rhs.m_points.Count() )
        {
            return false;
        }

        int32_t numPoints = GetNumPoints();
        for ( int32_t i = 0; i < numPoints; i++ )
        {
            if ( m_points[i] != rhs.m_points[i] )
            {
                return false;
            }
        }

        return true;
    }

    #ifdef SE_DEVELOPMENT
    void FloatCurve::RegeneratePointIDs()
    {
        for ( auto& point : m_points )
        {
            point.m_ID = ++s_pointIdentifierGenerator;
        }
    }
    #endif

    bool FloatCurve::FromString( String const& inStr, FloatCurve& outCurve )
    {
        outCurve.Clear();

        // Read number of points
        //-------------------------------------------------------------------------

		int32 startIdx = 0;
        int32 endIdx = inStr.FindFirstOf( ',', startIdx );
        if ( endIdx == INVALID_INDEX)
        {
            return false;
        }

        char* pCaret = nullptr;
        uint32 numPoints;
		StringUtils::ParseHex(inStr.Get(), &numPoints);
		//std::str strtoulw( &inStr[startIdx], &pCaret, 0 );

        //-------------------------------------------------------------------------

        for ( auto i = 0u; i < numPoints; i++ )
        {
            Point& p = outCurve.m_points.AddOne();

            if ( pCaret[0] != ',' ) { return false; }
            p.m_parameter = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_value = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_inTangent = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_outTangent = std::strtof( ++pCaret, &pCaret );

            if ( pCaret[0] != ',' ) { return false; }
            p.m_tangentMode = (TangentMode) std::strtoul( ++pCaret, &pCaret, 0 );
        }

        outCurve.SortPoints();

        #ifdef SE_DEVELOPMENT
        outCurve.RegeneratePointIDs();
        #endif

        return true;
    }

    String FloatCurve::ToString() const
    {
        String curveStr;
        curveStr.Resize( GetNumPoints() * 30 ); // rough over-estimate of 30 characters per point
        curveStr = String::Format(SE_TEXT("{0}"), (uint32) GetNumPoints() );

        for ( auto& point : m_points )
        {
            String pointStr = String::Format(SE_TEXT(",{:f},{:f},{:f},{:f},{:u}"), point.m_parameter, point.m_value, point.m_inTangent, point.m_outTangent, (uint8_t)point.m_tangentMode);

            curveStr.Append(pointStr);
        }

        return curveStr;
    }
}