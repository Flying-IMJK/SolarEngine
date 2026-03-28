
#include "GUIGizmos.h"
#include "Core/Systems.h"
#include "Core/Math/Transform.h"

namespace SE::Gizmos
{

   constexpr static uint32_t const g_numCircleVertices = 16;
   static_assert( ( g_numCircleVertices % 4 ) == 0 );

   struct GizmosData
   {
      bool g_circleVerticesInitialized = false;
      Float4 g_circleVerticesXUp[g_numCircleVertices];
      Float4 g_circleVerticesYUp[g_numCircleVertices];
      Float4 g_circleVerticesZUp[g_numCircleVertices];

      //-------------------------------------------------------------------------
      // Actual length is 2 since we specify dimensions in a half-size and then scale
      uint32 const g_unitCubeNumVertices = 8;
      Float4 const g_unitCubeVertices[8] =
      {
         Float4( -1.0f, -1.0f, -1.0f, 1.0f ),    // BFL
         Float4( -1.0f, 1.0f, -1.0f, 1.0f ),     // TFL
         Float4( 1.0f, 1.0f, -1.0f, 1.0f ),      // TFR
         Float4( 1.0f, -1.0f, -1.0f, 1.0f ),     // BFR
         Float4( -1.0f, -1.0f, 1.0f, 1.0f ),     // BBL
         Float4( -1.0f, 1.0f, 1.0f, 1.0f ),      // TBL
         Float4( 1.0f, 1.0f, 1.0f, 1.0f ),       // TBR
         Float4( 1.0f, -1.0f, 1.0f, 1.0f )       // BBR
     };

      // Box Primitives
      uint32 const g_unitCubeSolidNumIndices = 36;
      const uint16 g_unitCubeSolidIndices[36] =
      {
         // Triangle faces for a solid box
         0, 1, 3, 3, 1, 2,
         3, 2, 7, 7, 2, 6,
         7, 6, 4, 4, 6, 5,
         4, 5, 0, 0, 5, 1,
         1, 5, 2, 2, 5, 6,
         4, 0, 7, 7, 0, 3
     };

      uint32 const g_unitCubeWireNumIndices = 24;
      const uint16 g_unitCubeWireIndices[24] =
      {
         // Lines for a wire-frame box
         0, 1, 1, 2, 2, 3, 3, 0,
         4, 5, 5, 6, 6, 7, 7, 4,
         0, 4, 1, 5, 2, 6, 3, 7,
     };

      List<ThreadCommandBuffer*>       threadCmd;
      CriticalSection                  cmdMutex;
   };

   GizmosData* m_gizmosData;


   class GizmosSystem : public ISystem
   {
      ENGINE_SYSTEM(GizmosSystem)
   public:

      GizmosSystem() : ISystem(SE_TEXT("GizmosSystem")) {}

   protected:
      bool OnInit() override
      {
         m_gizmosData = New<GizmosData>();
         return true;
      }
   };

   ENGINE_SYSTEM_REGISTER(GizmosSystem);

   ThreadCommandBuffer& GetThreadCmd()
   {
      Threading::ScopeLock Lock( m_gizmosData->cmdMutex );

      auto const threadID = Threading::GetCurrentThreadID();

      // Check for an already created buffer for this thread
      for ( auto& pThreadBuffer : m_gizmosData->threadCmd )
      {
         if ( pThreadBuffer->GetThreadID() == threadID )
         {
            return *pThreadBuffer;
         }
      }

      // Create a new buffer
      auto pThreadBuffer = New<ThreadCommandBuffer>( threadID );
      m_gizmosData->threadCmd.Add(pThreadBuffer);

      return *pThreadBuffer;
   }

   void GetFrameCommandBuffer(FrameCommandBuffer& frameCmd)
   {
      Threading::ScopeLock Lock( m_gizmosData->cmdMutex );
      for ( auto& pThreadBuffer : m_gizmosData->threadCmd )
      {
         frameCmd.AddThreadCommands( *pThreadBuffer );
         pThreadBuffer->Clear();
      }
   }

   void ClearCommandBuffer()
   {
      Threading::ScopeLock Lock( m_gizmosData->cmdMutex );
      for ( auto& pThreadBuffer : m_gizmosData->threadCmd )
      {
         pThreadBuffer->Clear();
      }
   }


   static void CreateCircleVertices( Float4* pVerts, Float3 planeVector, Float3 upAxis )
   {
      /*float const radiansPerEdge( Math::TWO_PI / g_numCircleVertices );
      Quaternion rotation;
      Quaternion::RotationAxis ( upAxis, radiansPerEdge, rotation);

      for ( auto i = 0; i < g_numCircleVertices; i++ )
      {
         planeVector = rotation * planeVector;
         pVerts[i] = planeVector.w();
      }*/
   }

   bool InitializeCircleVertices()
   {
      CreateCircleVertices( m_gizmosData->g_circleVerticesXUp, Float3::UnitY, Float3::UnitX );
      CreateCircleVertices( m_gizmosData->g_circleVerticesYUp, Float3::UnitX, Float3::UnitY );
      CreateCircleVertices( m_gizmosData->g_circleVerticesZUp, Float3::UnitX, Float3::UnitZ );
      return true;
   }


   void InternalDrawPoint( ThreadCommandBuffer& cmdList,
      Float3 const& position,
      Color const& color,
      float thickness,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( PointCommand( position, color, thickness), depthTestState );
   }

   void InternalDrawLine( ThreadCommandBuffer& cmdList,
      Float3 const& startPosition,
      Float3 const& endPosition,
      Color const& color,
      float lineThickness,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( LineCommand( startPosition, endPosition, color, lineThickness), depthTestState );
   }

   void InternalDrawLine( ThreadCommandBuffer& cmdList,
      Float3 const& startPosition,
      Float3 const& endPosition,
      Color const& startColor,
      Color const& endColor,
      float lineThickness,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( LineCommand( startPosition, endPosition, startColor, endColor, lineThickness), depthTestState );
   }

   void InternalDrawLine( ThreadCommandBuffer& cmdList,
      Float3 const& startPosition,
      Float3 const& endPosition,
      Color const& color,
      float startLineThickness,
      float endLineThickness,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( LineCommand( startPosition, endPosition, color, startLineThickness, endLineThickness), depthTestState );
   }

   void InternalDrawLine( ThreadCommandBuffer& cmdList,
      Float3 const& startPosition,
      Float3 const& endPosition,
      Color const& startColor,
      Color const& endColor,
      float startLineThickness,
      float endLineThickness,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( LineCommand( startPosition, endPosition, startColor, endColor, startLineThickness, endLineThickness), depthTestState );
   }

   void InternalDrawTriangle( ThreadCommandBuffer& cmdList,
      Float3 const& v0,
      Float3 const& v1,
      Float3 const& v2,
      Color const& color,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( TriangleCommand( v0, v1, v2, color), depthTestState );
   }

   void InternalDrawTriangle( ThreadCommandBuffer& cmdList,
      Float3 const& v0,
      Float3 const& v1,
      Float3 const& v2,
      Color const& color0,
      Color const& color1,
      Color const& color2,
      DepthTest depthTestState = DepthTest::Disable)
   {
      cmdList.AddCommand( TriangleCommand( v0, v1, v2, color0, color1, color2), depthTestState );
   }

   void InternalDrawText2D( ThreadCommandBuffer& cmdList,
      Float2 const& position,
      Char const* pText,
      Color const& color,
      FontSize size,
      TextAlignment alignment,
      DepthTest depthTestState)
   {
      cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, false), depthTestState );
   }

   void InternalDrawText3D( ThreadCommandBuffer& cmdList,
      Float3 const& position,
      Char const* pText,
      Color const& color,
      FontSize size,
      TextAlignment alignment,
      DepthTest depthTestState)
   {
      cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, false), depthTestState );
   }

   void InternalDrawTextBox2D( ThreadCommandBuffer& cmdList,
      Float2 const& position,
      Char const* pText,
      Color const& color,
      FontSize size,
      TextAlignment alignment,
      DepthTest depthTestState)
   {
      cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, true), depthTestState );
   }

   void InternalDrawTextBox3D( ThreadCommandBuffer& cmdList,
      Float3 const& position,
      Char const* pText,
      Color const& color,
      FontSize size,
      TextAlignment alignment,
      DepthTest depthTestState)
   {
      cmdList.AddCommand( TextCommand( position, pText, color, size, alignment, true), depthTestState);
   }

   void InternalDrawCylinderOrCapsule( bool isCapsule, Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness, DepthTest depthTestState )
   {
      /*Float3 const axisX = worldTransform.GetLeft();
      Float3 const axisY = worldTransform.GetUp();
      Float3 const axisZ = worldTransform.GetForward();

      Float3 const origin = worldTransform.Translation;
      Float3 const halfHeightOffset = ( axisZ * halfHeight );

      Float3 const cylinderCenterTop = origin + halfHeightOffset;
      Float3 const cylinderCenterBottom = origin - halfHeightOffset;

      // 8 lines
      //-------------------------------------------------------------------------

      Float3 xOffset = ( axisX * radius );
      Float3 lt0 = cylinderCenterTop + xOffset;
      Float3 lt1 = cylinderCenterTop - xOffset;
      Float3 lb0 = cylinderCenterBottom + xOffset;
      Float3 lb1 = cylinderCenterBottom - xOffset;

      DrawLine( lt0, lb0, color, thickness, depthTestState);
      DrawLine( lt1, lb1, color, thickness, depthTestState);

      Float3 yOffset = ( axisY * radius );
      Float3 lt2 = cylinderCenterTop + yOffset;
      Float3 lt3 = cylinderCenterTop - yOffset;
      Float3 lb2 = cylinderCenterBottom + yOffset;
      Float3 lb3 = cylinderCenterBottom - yOffset;

      DrawLine( lt2, lb2, color, thickness, depthTestState);
      DrawLine( lt3, lb3, color, thickness, depthTestState);

      Float3 xzOffset0 = Float3::Normalize( axisX + axisY ) * radius;
      Float3 lt4 = cylinderCenterTop + xzOffset0;
      Float3 lt5 = cylinderCenterTop - xzOffset0;
      Float3 lb4 = cylinderCenterBottom + xzOffset0;
      Float3 lb5 = cylinderCenterBottom - xzOffset0;

      DrawLine( lt4, lb4, color, thickness, depthTestState);
      DrawLine( lt5, lb5, color, thickness, depthTestState);

      Float3 xzOffset1 = Float3::Normalize( axisX - axisY ) * radius;
      Float3 lt6 = cylinderCenterTop + xzOffset1;
      Float3 lt7 = cylinderCenterTop - xzOffset1;
      Float3 lb6 = cylinderCenterBottom + xzOffset1;
      Float3 lb7 = cylinderCenterBottom - xzOffset1;

      DrawLine( lt6, lb6, color, thickness, depthTestState);
      DrawLine( lt7, lb7, color, thickness, depthTestState);

      // Caps
      //-------------------------------------------------------------------------

      DrawCircle( Transform( worldTransform.GetRotation(), cylinderCenterTop ), Axis::Z, radius, color, thickness, depthTestState);
      DrawCircle( Transform( worldTransform.GetRotation(), cylinderCenterBottom ), Axis::Z, radius, color, thickness, depthTestState);

      //-------------------------------------------------------------------------

      if ( isCapsule )
      {
         float const radiansPerEdge( Math::TWO_PI / g_numCircleVertices );

         auto DrawSemiCircle = [&]( Float3 const& startPoint, Float3 const& capCenterPoint, Float3 const& shapeOrigin )
         {
            Float3 planeVector = startPoint - capCenterPoint;
            Float3 rotationAxis = Float3::Cross (startPoint - capCenterPoint, startPoint - shapeOrigin );
            rotationAxis.Normalize();

            Quaternion rotation;
            Quaternion::RotationAxis( rotationAxis, radiansPerEdge, rotation);

            Float3 prevPlaneVector = planeVector;
            for ( auto c = 0; c < g_numCircleVertices / 2; c++ )
            {
               prevPlaneVector = planeVector;
               planeVector = rotation * planeVector;
               DrawLine( capCenterPoint + prevPlaneVector, capCenterPoint + planeVector, color, thickness, depthTestState);
            }
         };

         DrawSemiCircle( lt0, cylinderCenterTop, origin );
         DrawSemiCircle( lt2, cylinderCenterTop, origin );
         DrawSemiCircle( lt4, cylinderCenterTop, origin );
         DrawSemiCircle( lt6, cylinderCenterTop, origin );

         DrawSemiCircle( lb0, cylinderCenterBottom, origin );
         DrawSemiCircle( lb2, cylinderCenterBottom, origin );
         DrawSemiCircle( lb4, cylinderCenterBottom, origin );
         DrawSemiCircle( lb6, cylinderCenterBottom, origin );
      }*/
   }


   void DrawPoint(Float3 const& position, Color const& color, float thickness, DepthTest depthTestState)
   {
      InternalDrawPoint( GetThreadCmd(), position, color, thickness, depthTestState);
   }

   void DrawLine(Float3 const& startPosition, Float3 const& endPosition, Color const& color, float lineThickness, DepthTest depthTestState)
   {
      InternalDrawLine( GetThreadCmd(), startPosition, endPosition, color, lineThickness, depthTestState);
   }

   void DrawLine(Float3 const& startPosition,
      Float3 const& endPosition,
      Color const& startColor,
      Color const& endColor,
      float lineThickness,
      DepthTest depthTestState)
   {
      InternalDrawLine( GetThreadCmd(), startPosition, endPosition, startColor, endColor, lineThickness, depthTestState);
   }

   void DrawTriangle(Float3 const& v0, Float3 const& v1, Float3 const& v2, Color const& color, DepthTest depthTestState)
   {
      InternalDrawTriangle( GetThreadCmd(), v0, v1, v2, color, color, color, depthTestState);
   }

   void DrawTriangle(Float3 const& v0, Float3 const& v1, Float3 const& v2, Color const& color0, Float4 const& color1, Float4 const& color2, DepthTest depthTestState)
   {
      InternalDrawTriangle( GetThreadCmd(), v0, v1, v2, color0, color1, color2, depthTestState);
   }

   void DrawText2D(Float2 const& screenPosition, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, DepthTest depthTestState)
   {
      InternalDrawText2D( GetThreadCmd(), screenPosition, pText, color, size, alignment, depthTestState);
   }

   void DrawText3D(Float3 const& worldPosition, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, DepthTest depthTestState)
   {
      InternalDrawText3D( GetThreadCmd(), worldPosition, pText, color, size, alignment, depthTestState);
   }

   void DrawTextBox2D(Float2 const& screenPos, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, DepthTest depthTestState)
   {
      InternalDrawTextBox2D( GetThreadCmd(), screenPos, pText, color, size, alignment, depthTestState);
   }

   void DrawTextBox3D(Float3 const& worldPos, Char const* pText, Color const& color, FontSize size, TextAlignment alignment, DepthTest depthTestState)
   {
      InternalDrawTextBox3D( GetThreadCmd(), worldPos, pText, color, size, alignment, depthTestState);
   }

   void CommandBuffer::Reset()
   {
   }

   void FrameCommandBuffer::AddThreadCommands(ThreadCommandBuffer const& threadCommands)
   {
      // TODO:
      // Broad-phase culling
      // Sort transparent and depth test off primitives by distance to camera
      // Sort text by font

      opaqueDepthOn.Append( threadCommands.GetOpaqueDepthTestEnabledBuffer() );
      opaqueDepthOff.Append( threadCommands.GetOpaqueDepthTestDisabledBuffer() );
      transparentDepthOn.Append( threadCommands.GetTransparentDepthTestEnabledBuffer() );
      transparentDepthOff.Append( threadCommands.GetTransparentDepthTestDisabledBuffer() );
   }

   void DrawBox(Float3 const& position, Quaternion const& rotation, Float3 const& size, Color const& color, DepthTest depthTestState)
   {
      Transform transform = Transform(position, rotation, size);

      // Calculate transformed vertices
      Float3 verts[8] =
      {
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[0])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[1])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[2])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[3])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[4])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[5])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[6])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[7]))
     };

      // Register draw commands

      ThreadCommandBuffer& cmd = GetThreadCmd();
      for ( auto i = 0; i < 36; i += 3 )
      {
         InternalDrawTriangle( cmd,
            verts[m_gizmosData->g_unitCubeSolidIndices[i]],
            verts[m_gizmosData->g_unitCubeSolidIndices[i + 1]],
            verts[m_gizmosData->g_unitCubeSolidIndices[i + 2]], color, depthTestState );
      }
   }

   void DrawWireBox(Float3 const& position, Quaternion const& rotation, Float3 const& size, Color const& color, float lineThickness, DepthTest depthTestState)
   {
      Transform transform = Transform(position, rotation, size);

      Float3 verts[8] =
      {
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[0])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[1])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[2])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[3])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[4])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[5])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[6])),
         transform.LocalToWorld(Float3( m_gizmosData->g_unitCubeVertices[7]))
     };

      // Register draw commands

      for ( auto i = 0; i < 24; i += 2 )
      {
         InternalDrawLine( GetThreadCmd(),
            verts[m_gizmosData->g_unitCubeWireIndices[i]],
            verts[m_gizmosData->g_unitCubeWireIndices[i + 1]], color, lineThickness, depthTestState);
      }
   }

   void DrawCircle(Float3 const& worldPosition, Float3 const& upAxis, float radius, Color const& color, float lineThickness, DepthTest depthTestState)
   {
      /*if ( !m_gizmosData->g_circleVerticesInitialized )
      {
         InitializeCircleVertices();
      }

      Float4* pCircleVerts = nullptr;

      switch ( upAxis )
      {
      case Axis::X:
      case Axis::NegX:
      pCircleVerts = m_gizmosData->g_circleVerticesXUp;
         break;

      case Axis::Y:
      case Axis::NegY:
      pCircleVerts = m_gizmosData->g_circleVerticesYUp;
         break;

      case Axis::Z:
      case Axis::NegZ:
      pCircleVerts = m_gizmosData->g_circleVerticesZUp;
         break;
      }

      ENGINE_ASSERT( pCircleVerts != nullptr );

      // Create and transform vertices
      auto verts = NewArray<Float3>(g_numCircleVertices );
      for ( auto i = 0; i < g_numCircleVertices; i++ )
      {
         verts[i] = transform. .TransformPoint( pCircleVerts[i] * radius );
      }

      ThreadCommandBuffer& cmd = GetThreadCommandBuffer();

      // Register line commands
      for ( auto i = 1; i < g_numCircleVertices; i++ )
      {
         InternalDrawLine(cmd, verts[i - 1], verts[i], color, lineThickness, depthTestState);
      }

      InternalDrawLine(cmd, verts[g_numCircleVertices - 1], verts[0], color, lineThickness, depthTestState);*/
   }

   void DrawSphere(Float3 const& position, float radius, Color const& color, float lineThickness, DepthTest depthTestState)
   {
      DrawCircle( position, Float3::Left, radius, color, lineThickness, depthTestState);
      DrawCircle( position, Float3::Up, radius, color, lineThickness, depthTestState);
      DrawCircle( position, Float3::Forward, radius, color, lineThickness, depthTestState);
   }

   void DrawCylinder(Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness, DepthTest depthTestState)
   {
      InternalDrawCylinderOrCapsule( false, worldTransform, radius, halfHeight, color, thickness, depthTestState);
   }

   void DrawCapsule(Transform const& worldTransform, float radius, float halfHeight, Float4 const& color, float thickness, DepthTest depthTestState)
   {
      InternalDrawCylinderOrCapsule( true, worldTransform, radius, halfHeight, color, thickness, depthTestState);
   }

   void DrawAxis(Float3 const& worldPosition, Float3 const& up, Float3 const& left,float axisLength, float axisThickness, DepthTest depthTestState)
   {
      Float3 const forward = Float3::Cross(up, left);

      DrawLine( worldPosition, worldPosition + left * axisLength, Colors::Red, axisThickness, depthTestState);
      DrawLine( worldPosition, worldPosition + up * left, Colors::LimeGreen, axisThickness, depthTestState);
      DrawLine( worldPosition, worldPosition + forward * left, Colors::Blue, axisThickness, depthTestState);
   }

   void DrawArrow(Float3 const& startPoint, Float3 const& endPoint, float arrowLength, Float4 const& color, float thickness, DepthTest depthTestState)
   {
      constexpr static float minArrowHeadThickness = 16.0f; // 16 pixels
      constexpr static float maxArrowHeadLength = 0.1f; // 10cm

      Float3 arrowDirection = endPoint - startPoint;
      // float arrowLength = arrowDirection.Length();

      float const arrowHeadLength = Math::Max( arrowLength * 0.8f, arrowLength - maxArrowHeadLength );
      float const arrowHeadThickness = Math::Max( thickness * 2, minArrowHeadThickness );
      Float3 const vArrowHeadStartPoint = startPoint + ( arrowDirection * arrowHeadLength );
      Float3 const arrowHeadStartPoint = vArrowHeadStartPoint;

      ThreadCommandBuffer& cmd = GetThreadCmd();

      InternalDrawLine( cmd, startPoint, arrowHeadStartPoint, color, thickness, depthTestState);
      InternalDrawLine( cmd, arrowHeadStartPoint, endPoint, color, arrowHeadThickness, 2.0f, depthTestState);
   }

   void DrawCone(Transform const& transform, float coneAngle, float length, Float4 const& color, float thickness, DepthTest depthTestState)
   {
      Float3 const capOffset = ( transform.GetForward() * length );
      float const coneCapRadius = Math::Tan( coneAngle ) * length;

      Transform capTransform = transform;
      capTransform.Scale = coneCapRadius;
      capTransform.Translation += capOffset;

      // Draw cone cap
      //-------------------------------------------------------------------------

      auto verts = NewArray<Float3>(g_numCircleVertices );
      for ( auto i = 0; i < g_numCircleVertices; i++ )
      {
         verts[i] = capTransform.LocalToWorld(Float3(m_gizmosData->g_circleVerticesYUp[i]));
      }

      ThreadCommandBuffer& cmd = GetThreadCmd();

      // Register line commands
      for ( auto i = 1; i < g_numCircleVertices; i++ )
      {
         InternalDrawLine( cmd, transform.Translation, verts[i], color, thickness, depthTestState);
         InternalDrawLine( cmd, verts[i - 1], verts[i], color, thickness, depthTestState);
      }

      InternalDrawLine( cmd, transform.Translation, verts[0], color, thickness, depthTestState);
      InternalDrawLine( cmd, verts[0], verts[g_numCircleVertices-1], color, thickness, depthTestState);
   }

   void DrawCone(Float3 const& startPoint, Float3 const& direction, float coneAngle, float length, Float4 const& color, float thickness, DepthTest depthTestState)
   {
      /*Quaternion const orientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, Vector( direction ).GetNormalized3() );
      Transform const coneTransform( orientation, startPoint );
      DrawCone( coneTransform, coneAngle, length, color, thickness, depthTestState);*/
   }

}
