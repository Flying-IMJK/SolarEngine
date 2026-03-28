#pragma once

#include "DebugDrawPrimitive.h"
#include "DebugDrawFont.h"

#include <mutex>
#include <list>

namespace SE
{
    class DebugDrawGroup
    {
    private:
        std::mutex m_Mutex;

        std::string m_name;

        std::list<DebugDrawPoint>    m_points;
        std::list<DebugDrawLine>     m_lines;
        std::list<DebugDrawTriangle> m_triangles;
        std::list<DebugDrawQuad>     m_quads;
        std::list<DebugDrawBox>      m_boxes;
        std::list<DebugDrawCylinder> m_cylinders;
        std::list<DebugDrawSphere>   m_spheres;
        std::list<DebugDrawCapsule>  m_capsules;
        std::list<DebugDrawText>     m_texts;

    public:
        virtual ~DebugDrawGroup();
        void initialize();
        void clear();
        void clearData();
        void setName(const std::string& name);
        const std::string& getName() const;
        
        void addPoint(const Float3& position, 
                      const Float4& color, 
                      const float    life_time = k_debug_draw_one_frame, 
                      const bool     no_depth_test = true);

        void addLine(const Float3& point0,
                     const Float3& point1,
                     const Float4& color0,
                     const Float4& color1,
                     const float    life_time = k_debug_draw_one_frame,
                     const bool     no_depth_test = true);

        void addTriangle(const Float3& point0,
                         const Float3& point1,
                         const Float3& point2,
                         const Float4& color0,
                         const Float4& color1,
                         const Float4& color2,
                         const float    life_time = k_debug_draw_one_frame,
                         const bool     no_depth_test = true,
                         const FillMode fillmod = _FillMode_wireframe);

        void addQuad(const Float3& point0,
                     const Float3& point1,
                     const Float3& point2,
                     const Float3& point3,
                     const Float4& color0,
                     const Float4& color1,
                     const Float4& color2,
                     const Float4& color3,
                     const float    life_time = k_debug_draw_one_frame,
                     const bool     no_depth_test = true,
                     const FillMode fillmode = _FillMode_wireframe);

        void addBox(const Float3& center_point,
                    const Float3& half_extends,
                    const Float4& rotate,
                    const Float4& color,
                    const float    life_time = k_debug_draw_one_frame,
                    const bool     no_depth_test = true);

        void addSphere(const Float3& center, 
                       const float    radius,
                       const Float4& color, 
                       const float    life_time, 
                       const bool     no_depth_test = true);

        void addCylinder(const Float3& center,
                         const float    radius,
                         const float    height,
                         const Float4& rotate,
                         const Float4& color,
                         const float    life_time = k_debug_draw_one_frame, 
                         const bool     no_depth_test = true);

        void addCapsule(const Float3& center,
                        const Float4& rotation,
                        const Float3& scale,
                        const float    radius,
                        const float    height,
                        const Float4& color,
                        const float    life_time = k_debug_draw_one_frame,
                        const bool     no_depth_test = true);

        void addText(const std::string& content,
                     const Float4&     color,
                     const Float3&     coordinate,
                     const int          size,
                     const bool         is_screen_text,
                     const float        life_time = k_debug_draw_one_frame);

        void removeDeadPrimitives(float delta_time);
        void mergeFrom(DebugDrawGroup* group);

        size_t getPointCount(bool no_depth_test) const;
        size_t getLineCount(bool no_depth_test) const;
        size_t getTriangleCount(bool no_depth_test) const;
        size_t getUniformDynamicDataCount() const;

        void writePointData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void writeLineData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void writeTriangleData(std::vector<DebugDrawVertex>& vertexs, bool no_depth_test);
        void writeUniformDynamicDataToCache(std::vector<std::pair<Matrix4x4, Float4> >& datas);
        void writeTextData(std::vector<DebugDrawVertex>& vertexs, DebugDrawFont* font, Matrix4x4 m_proj_view_matrix);

        size_t getSphereCount(bool no_depth_test) const;
        size_t getCylinderCount(bool no_depth_test) const;
        size_t getCapsuleCount(bool no_depth_test) const;
        size_t getTextCharacterCount() const;

    };
}