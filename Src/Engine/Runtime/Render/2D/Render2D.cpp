
#include "Render2D.h"

#include "Font.h"
#include "FontManager.h"
#include "FontTextureAtlas.h"
#include "RotatedRectangle.h"
#include "SpriteAtlas.h"
#include "TextLayoutOptions.h"

#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Math/Half.h"
#include "Runtime/Core/Math/Matrix3x3.h"
#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Profiler/ProfilerGPU.h"
#include "Runtime/Graphics/DynamicBuffer.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/RenderTargetPool.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Graphics/Textures/GPUTextureDescription.h"
#include "Runtime/Resource/Assets/Materials/MaterialBase.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
    #if SE_EDITOR
    #define RENDER2D_CHECK_RENDERING_STATE \
    if (!Render2D::IsRendering()) \
    { \
        LOG_ERROR("Render", "Calling Render2D is only valid during rendering."); \
        return; \
    }
    #else
    #define RENDER2D_CHECK_RENDERING_STATE
    #endif

    #if SE_EDITOR
    #define RENDER2D_INITIAL_VB_CAPACITY (16 * 1024)
    #else
    #define RENDER2D_INITIAL_VB_CAPACITY (4 * 1024)
    #endif
    #define RENDER2D_INITIAL_IB_CAPACITY (1024)
    #define RENDER2D_INITIAL_DRAW_CALL_CAPACITY (512)


    #define RENDER2D_BLUR_MAX_SAMPLES 64

        // The format for the blur effect temporary buffer
    #define PS_Blur_Format PixelFormat::R8G8B8A8_UNorm

    // True if enable downscaling when rendering blur
    const bool DownsampleForBlur = false;

    GES_PACK_STRUCT(struct Data {
        Matrix ViewProjection;
        });

    GES_PACK_STRUCT(struct BlurData {
        Float2 InvBufferSize;
        uint32 SampleCount;
        float Dummy0;
        Float4 Bounds;
        Float4 WeightAndOffsets[RENDER2D_BLUR_MAX_SAMPLES / 2];
        });


    enum class DrawCallType : byte
    {
        FillRect,
        FillRectNoAlpha,
        FillRT,
        FillTexture,
        FillTexturePoint,
        DrawChar,
        DrawCharMaterial,
        Custom,
        Material,
        Blur,
        ClipScissors,
        LineAA,

        MAX
    };

    struct Render2DDrawCall
    {
        DrawCallType Type;
        uint32 StartIB;
        uint32 CountIB;

        union
        {
            struct
            {
                GPUTextureView* Ptr;
            } AsRT;

            struct
            {
                GPUTexture* Ptr;
            } AsTexture;

            struct
            {
                GPUTexture* Tex;
                MaterialBase* Mat;
            } AsChar;

            struct
            {
                GPUTexture* Tex;
                GPUPipelineState* Pso;
            } AsCustom;

            struct
            {
                MaterialBase* Mat;
                float Width;
                float Height;
            } AsMaterial;

            struct
            {
                float Strength;
                float Width;
                float Height;
                float UpperLeftX;
                float UpperLeftY;
                float BottomRightX;
                float BottomRightY;
            } AsBlur;

            struct
            {
                float X;
                float Y;
                float Width;
                float Height;
            } AsClipScissors;
        };
    };

    struct Render2DVertex
    {
        Float2 Position;
        Half2 TexCoord;
        Color Color;
        Float2 CustomData;
        RotatedRectangle ClipMask;
    };

    struct CachedPSO
    {
        bool Inited = false;
        bool UseDepth;

        GPUPipelineState* PS_Image;

        GPUPipelineState* PS_ImagePoint;

        GPUPipelineState* PS_Color;
        GPUPipelineState* PS_Color_NoAlpha;

        GPUPipelineState* PS_Font;

        GPUPipelineState* PS_BlurH;
        GPUPipelineState* PS_BlurV;
        GPUPipelineState* PS_Downscale;

        GPUPipelineState* PS_LineAA;

        bool Init(GPUShader* shader, bool useDepth);
        void Dispose();
    };

    // Clip
    struct ClipMask
    {
        RotatedRectangle Mask;
        Rectangle Bounds;
    };

    EnumFlags<Render2D::RenderingFeatures> Render2D::Features = { RenderingFeatures::VertexSnapping, RenderingFeatures::FallbackFonts };

    struct Render2DData
    {
        // Private Stuff
        GPUContext* Context = nullptr;
        GPUTextureView* Output = nullptr;
        GPUTextureView* DepthBuffer = nullptr;
        Viewport View;
        Matrix ViewProjection;

        // Drawing
        List<Render2DDrawCall> DrawCalls;
        List<FontLineCache> Lines;
        List<Float2> Lines2;
        bool IsScissorsRectEmpty;
        bool IsScissorsRectEnabled;

        // Transform
        // Note: we use Matrix3x3 instead of Matrix because we use only 2D transformations on CPU side
        // Matrix layout:
        // [ m1, m2, 0 ]
        // [ m3, m4, 0 ]
        // [ t1, t2, 1 ]
        // where 'm' is 2D transformation (scale, shear and rotate), 't' is translation
        List<Matrix3x3, InlinedAllocation<64>> TransformLayersStack;
        Matrix3x3 TransformCached;

        List<ClipMask, InlinedAllocation<64>> ClipLayersStack;
        List<Color, InlinedAllocation<64>> TintLayersStack;

        // Shader
        AssetRef<Shader> GUIShader;
        CachedPSO PsoDepth;
        CachedPSO PsoNoDepth;
        CachedPSO* CurrentPso = nullptr;
        DynamicVertexBuffer VB = DynamicVertexBuffer(RENDER2D_INITIAL_VB_CAPACITY, (uint32)sizeof(Render2DVertex), SE_TEXT("Render2D.VB"));
        DynamicIndexBuffer IB = DynamicIndexBuffer(RENDER2D_INITIAL_IB_CAPACITY, sizeof(uint32), SE_TEXT("Render2D.IB"));
        uint32 VBIndex = 0;
        uint32 IBIndex = 0;
    };

    Render2DData* render2DData;

    #define RENDER2D_WRITE_IB_QUAD(indices) \
    indices[0] = render2DData->VBIndex + 0; \
    indices[1] = render2DData->VBIndex + 1; \
    indices[2] = render2DData->VBIndex + 2; \
    indices[3] = render2DData->VBIndex + 2; \
    indices[4] = render2DData->VBIndex + 3; \
    indices[5] = render2DData->VBIndex + 0; \
    render2DData->IB.Write(indices, sizeof(indices))

    FORCE_INLINE void ApplyTransform(const Float2& value, Float2& result)
    {
        Matrix3x3::Transform2DPoint(value, render2DData->TransformCached, result);
    }

    void ApplyTransform(const Rectangle& value, RotatedRectangle& result)
    {
        const RotatedRectangle rotated(value);
        Matrix3x3::Transform2DPoint(rotated.TopLeft, render2DData->TransformCached, result.TopLeft);
        Matrix3x3::Transform2DVector(rotated.ExtentX, render2DData->TransformCached, result.ExtentX);
        Matrix3x3::Transform2DVector(rotated.ExtentY, render2DData->TransformCached, result.ExtentY);
    }

    FORCE_INLINE Render2DVertex MakeVertex(const Float2& pos, const Float2& uv, const Color& color)
    {
        Float2 point;
        ApplyTransform(pos, point);

        return
        {
            point,
            Half2(uv),
            color * render2DData->TintLayersStack.Peek(),
            { 0.0f, (float)Render2D::Features.Get() },
            render2DData->ClipLayersStack.Peek().Mask
        };
    }

    FORCE_INLINE Render2DVertex MakeVertex(const Float2& point, const Float2& uv, const Color& color, const RotatedRectangle& mask, const Float2& customData)
    {
        return
        {
            point,
            Half2(uv),
            color,
            customData,
            mask,
        };
    }

    FORCE_INLINE Render2DVertex MakeVertex(const Float2& point, const Float2& uv, const Color& color, const RotatedRectangle& mask, const Float2& customData, const Color& tint)
    {
        return
        {
            point,
            Half2(uv),
            color * tint,
            customData,
            mask
        };
    }

    void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Float2& uv0, const Float2& uv1, const Float2& uv2, const Color& color0, const Color& color1, const Color& color2)
    {
        Render2DVertex tris[3];
        tris[0] = MakeVertex(p0, uv0, color0);
        tris[1] = MakeVertex(p1, uv1, color1);
        tris[2] = MakeVertex(p2, uv2, color2);
        render2DData->VB.Write(tris, sizeof(tris));

        uint32 indices[3];
        indices[0] = render2DData->VBIndex + 0;
        indices[1] = render2DData->VBIndex + 1;
        indices[2] = render2DData->VBIndex + 2;
        render2DData->IB.Write(indices, sizeof(indices));

        render2DData->VBIndex += 3;
        render2DData->IBIndex += 3;
    }

    void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Color& color0, const Color& color1, const Color& color2)
    {
        WriteTri(p0, p1, p2, Float2::Zero, Float2::Zero, Float2::Zero, color0, color1, color2);
    }

    void WriteTri(const Float2& p0, const Float2& p1, const Float2& p2, const Float2& uv0, const Float2& uv1, const Float2& uv2)
    {
        WriteTri(p0, p1, p2, uv0, uv1, uv2, Colors::Black, Colors::Black, Colors::Black);
    }

    void WriteRect(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4)
    {
        const Float2 uvUpperLeft = Float2::Zero;
        const Float2 uvBottomRight = Float2::One;

        Render2DVertex quad[4];
        quad[0] = MakeVertex(rect.GetBottomRight(), uvBottomRight, color3);
        quad[1] = MakeVertex(rect.GetBottomLeft(), Float2(uvUpperLeft.x, uvBottomRight.y), color4);
        quad[2] = MakeVertex(rect.GetUpperLeft(), uvUpperLeft, color1);
        quad[3] = MakeVertex(rect.GetUpperRight(), Float2(uvBottomRight.x, uvUpperLeft.y), color2);
        render2DData->VB.Write(quad, sizeof(quad));

        uint32 indices[6];
        RENDER2D_WRITE_IB_QUAD(indices);

        render2DData->VBIndex += 4;
        render2DData->IBIndex += 6;
    }

    void WriteRect(const Rectangle& rect, const Color& color, const Float2& uvUpperLeft, const Float2& uvBottomRight)
    {
        Render2DVertex quad[4];
        quad[0] = MakeVertex(rect.GetBottomRight(), uvBottomRight, color);
        quad[1] = MakeVertex(rect.GetBottomLeft(), Float2(uvUpperLeft.x, uvBottomRight.y), color);
        quad[2] = MakeVertex(rect.GetUpperLeft(), uvUpperLeft, color);
        quad[3] = MakeVertex(rect.GetUpperRight(), Float2(uvBottomRight.x, uvUpperLeft.y), color);
        render2DData->VB.Write(quad, sizeof(quad));

        uint32 indices[6];
        RENDER2D_WRITE_IB_QUAD(indices);

        render2DData->VBIndex += 4;
        render2DData->IBIndex += 6;
    }

    FORCE_INLINE void WriteRect(const Rectangle& rect, const Color& color)
    {
        WriteRect(rect, color, Float2::Zero, Float2::One);
    }

    void Write9SlicingRect(const Rectangle& rect, const Color& color, const Float4& border, const Float4& borderUVs)
    {
        const Rectangle upperLeft(rect.Location.x, rect.Location.y, border.x, border.z);
        const Rectangle upperRight(rect.Location.x + rect.Size.x - border.y, rect.Location.y, border.y, border.z);
        const Rectangle bottomLeft(rect.Location.x, rect.Location.y + rect.Size.y - border.w, border.x, border.w);
        const Rectangle bottomRight(rect.Location.x + rect.Size.x - border.y, rect.Location.y + rect.Size.y - border.w, border.y, border.w);

        const Float2 upperLeftUV(borderUVs.x, borderUVs.z);
        const Float2 upperRightUV(1.0f - borderUVs.y, borderUVs.z);
        const Float2 bottomLeftUV(borderUVs.x, 1.0f - borderUVs.w);
        const Float2 bottomRightUV(1.0f - borderUVs.y, 1.0f - borderUVs.w);

        WriteRect(upperLeft, color, Float2::Zero, upperLeftUV);
        WriteRect(upperRight, color, Float2(upperRightUV.x, 0), Float2(1, upperLeftUV.y));
        WriteRect(bottomLeft, color, Float2(0, bottomLeftUV.y), Float2(bottomLeftUV.x, 1));
        WriteRect(bottomRight, color, bottomRightUV, Float2::One);

        WriteRect(Rectangle(upperLeft.GetUpperRight(), upperRight.GetBottomLeft() - upperLeft.GetUpperRight()), color, Float2(upperLeftUV.x, 0), upperRightUV);
        WriteRect(Rectangle(upperLeft.GetBottomLeft(), bottomLeft.GetUpperRight() - upperLeft.GetBottomLeft()), color, Float2(0, upperLeftUV.y), bottomLeftUV);
        WriteRect(Rectangle(bottomLeft.GetUpperRight(), bottomRight.GetBottomLeft() - bottomLeft.GetUpperRight()), color, bottomLeftUV, Float2(bottomRightUV.x, 1));
        WriteRect(Rectangle(upperRight.GetBottomLeft(), bottomRight.GetUpperRight() - upperRight.GetBottomLeft()), color, upperRightUV, Float2(1, bottomRightUV.y));

        WriteRect(Rectangle(upperLeft.GetBottomRight(), bottomRight.GetUpperLeft() - upperLeft.GetBottomRight()), color, upperRightUV, bottomRightUV);
    }

    void Write9SlicingRect(const Rectangle& rect, const Color& color, const Float4& border, const Float4& borderUVs, const Float2& uvLocation, const Float2& uvSize)
    {
        const Rectangle upperLeft(rect.Location.x, rect.Location.y, border.x, border.z);
        const Rectangle upperRight(rect.Location.x + rect.Size.x - border.y, rect.Location.y, border.y, border.z);
        const Rectangle bottomLeft(rect.Location.x, rect.Location.y + rect.Size.y - border.w, border.x, border.w);
        const Rectangle bottomRight(rect.Location.x + rect.Size.x - border.y, rect.Location.y + rect.Size.y - border.w, border.y, border.w);

        const Float2 upperLeftUV = Float2(borderUVs.x, borderUVs.z) * uvSize + uvLocation;
        const Float2 upperRightUV = Float2(1.0f - borderUVs.y, borderUVs.z) * uvSize + uvLocation;
        const Float2 bottomLeftUV = Float2(borderUVs.x, 1.0f - borderUVs.w) * uvSize + uvLocation;
        const Float2 bottomRightUV = Float2(1.0f - borderUVs.y, 1.0f - borderUVs.w) * uvSize + uvLocation;
        const Float2 uvEnd = uvLocation + uvSize;

        WriteRect(upperLeft, color, uvLocation, upperLeftUV);
        WriteRect(upperRight, color, Float2(upperRightUV.x, uvLocation.y), Float2(uvEnd.x, upperLeftUV.y));
        WriteRect(bottomLeft, color, Float2(uvLocation.x, bottomLeftUV.y), Float2(bottomLeftUV.x, uvEnd.y));
        WriteRect(bottomRight, color, bottomRightUV, uvEnd);

        WriteRect(Rectangle(upperLeft.GetUpperRight(), upperRight.GetBottomLeft() - upperLeft.GetUpperRight()), color, Float2(upperLeftUV.x, uvLocation.y), upperRightUV);
        WriteRect(Rectangle(upperLeft.GetBottomLeft(), bottomLeft.GetUpperRight() - upperLeft.GetBottomLeft()), color, Float2(uvLocation.x, upperLeftUV.y), bottomLeftUV);
        WriteRect(Rectangle(bottomLeft.GetUpperRight(), bottomRight.GetBottomLeft() - bottomLeft.GetUpperRight()), color, bottomLeftUV, Float2(bottomRightUV.x, uvEnd.y));
        WriteRect(Rectangle(upperRight.GetBottomLeft(), bottomRight.GetUpperRight() - upperRight.GetBottomLeft()), color, upperRightUV, Float2(uvEnd.x, bottomRightUV.y));

        WriteRect(Rectangle(upperLeft.GetBottomRight(), bottomRight.GetUpperLeft() - upperLeft.GetBottomRight()), color, upperRightUV, bottomRightUV);
    }

    typedef bool (*CanDrawCallCallback)(const Render2DDrawCall&, const Render2DDrawCall&);

    bool CanDrawCallCallbackTrue(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return true;
    }

    bool CanDrawCallCallbackFalse(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return false;
    }

    bool CanDrawCallCallbackRT(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsRT.Ptr == d2.AsRT.Ptr;
    }

    bool CanDrawCallCallbackTexture(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsTexture.Ptr == d2.AsTexture.Ptr;
    }

    bool CanDrawCallCallbackChar(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsChar.Tex == d2.AsChar.Tex;
    }

    bool CanDrawCallCallbackCharMaterial(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsChar.Tex == d2.AsChar.Tex && d1.AsChar.Mat == d2.AsChar.Mat;
    }

    bool CanDrawCallCallbackCustom(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsCustom.Tex == d2.AsCustom.Tex && d1.AsCustom.Pso == d2.AsCustom.Pso;
    }

    bool CanDrawCallCallbackMaterial(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.AsMaterial.Mat == d2.AsMaterial.Mat;
    }

    // @formatter:off
    CanDrawCallCallback CanDrawCallBatch[] =
    {
        CanDrawCallCallbackTrue, // FillRect,
        CanDrawCallCallbackTrue,  // FillRectNoAlpha,
        CanDrawCallCallbackRT, // FillRT,
        CanDrawCallCallbackTexture, // FillTexture,
        CanDrawCallCallbackTexture, // FillTexturePoint,
        CanDrawCallCallbackChar, // DrawChar,
        CanDrawCallCallbackCharMaterial, // DrawCharMaterial,
        CanDrawCallCallbackFalse, // Custom,
        CanDrawCallCallbackMaterial, // Material,
        CanDrawCallCallbackFalse, // Blur,
        CanDrawCallCallbackFalse, // ClipScissors,
        CanDrawCallCallbackTrue, // LineAA,
    };
    static_assert(ARRAY_SIZE(CanDrawCallBatch) == (int32)DrawCallType::MAX, "Invalid draw calls batching descriptor.");
    // @formatter:on

    bool CanBatchDrawCalls(const Render2DDrawCall& d1, const Render2DDrawCall& d2)
    {
        return d1.Type == d2.Type && CanDrawCallBatch[(int32)d1.Type](d1, d2);
    }

    void DrawBatch(int32 startIndex, int32 count);

    bool CachedPSO::Init(GPUShader* shader, bool useDepth)
    {
        if (Inited)
        {
            Dispose();
        }

        UseDepth = useDepth;

        GPUDevice* device = GPUDevice::instance;

        // Create pipeline states
        GPUPipelineState::Description desc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        desc.DepthEnable = desc.DepthWriteEnable = useDepth;
        desc.DepthWriteEnable = false;
        desc.DepthClipEnable = false;
        desc.VS = shader->GetVS(SE_TEXT("VS"));
        desc.PS = shader->GetPS(SE_TEXT("PS_Image"));
        desc.CullMode = CullMode::TwoSided;
        desc.BlendMode = BlendingMode::AlphaBlend;
        PS_Image = device->CreatePipelineState();
        if (!PS_Image->Init(desc))
            return false;
        //
        desc.BlendMode = BlendingMode::AlphaBlend;
        desc.PS = shader->GetPS(SE_TEXT("PS_ImagePoint"));
        PS_ImagePoint = device->CreatePipelineState();
        if (!PS_ImagePoint->Init(desc))
            return false;
        //
        desc.BlendMode = BlendingMode::AlphaBlend;
        desc.PS = shader->GetPS(SE_TEXT("PS_Color"));
        PS_Color = device->CreatePipelineState();
        if (!PS_Color->Init(desc))
            return false;
        //
        desc.BlendMode = BlendingMode::Opaque;
        PS_Color_NoAlpha = device->CreatePipelineState();
        if (!PS_Color_NoAlpha->Init(desc))
            return false;
        //
        desc.BlendMode = BlendingMode::AlphaBlend;
        desc.PS = shader->GetPS(SE_TEXT("PS_Font"));
        PS_Font = device->CreatePipelineState();
        if (!PS_Font->Init(desc))
            return false;
        //
        desc.PS = shader->GetPS(SE_TEXT("PS_LineAA"));
        PS_LineAA = device->CreatePipelineState();
        if (!PS_LineAA->Init(desc))
            return false;
        //
        desc.VS = GPUPipelineState::Description::DefaultFullscreenTriangle.VS;
        desc.PS = shader->GetPS(SE_TEXT("PS_Blur"));
        desc.BlendMode = BlendingMode::Opaque;
        PS_BlurH = device->CreatePipelineState();
        if (!PS_BlurH->Init(desc))
            return false;
        //
        desc.PS = shader->GetPS(SE_TEXT("PS_Blur"), 1);
        PS_BlurV = device->CreatePipelineState();
        if (!PS_BlurV->Init(desc))
            return false;
        //
        desc.PS = shader->GetPS(SE_TEXT("PS_Downscale"));
        PS_Downscale = device->CreatePipelineState();
        if (!PS_Downscale->Init(desc))
            return false;

        Inited = true;

        return true;
    }

    void CachedPSO::Dispose()
    {
        if (!Inited)
            return;

        DeleteObjectSafe(PS_Image);
        DeleteObjectSafe(PS_ImagePoint);
        DeleteObjectSafe(PS_Color);
        DeleteObjectSafe(PS_Color_NoAlpha);
        DeleteObjectSafe(PS_Font);
        DeleteObjectSafe(PS_BlurH);
        DeleteObjectSafe(PS_BlurV);
        DeleteObjectSafe(PS_Downscale);
        DeleteObjectSafe(PS_LineAA);

        Inited = false;
    }

    class Render2DSystem : public ISystem
    {
        ENGINE_SYSTEM(Render2DSystem)
    public:
        Render2DSystem() : ISystem(SE_TEXT("Render2D"), 10)
        {
        }

        bool OnInit() override;
        void OnDispose() override;
    };

    ENGINE_SYSTEM_REGISTER(Render2DSystem)

    bool Render2D::IsRendering()
    {
        return render2DData->Context != nullptr;
    }

    const Viewport& Render2D::GetViewport()
    {
        return render2DData->View;
    }

#if COMPILE_WITH_DEV_ENV

    void OnGUIShaderReloading(Asset* obj)
    {
        PsoDepth.Dispose();
        PsoNoDepth.Dispose();
    }

#endif

    bool Render2DSystem::OnInit()
    {
        render2DData = New<Render2DData>();

        // GUI Shader
        render2DData->GUIShader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/GUI"));
        if (render2DData->GUIShader == nullptr)
            return false;
#if COMPILE_WITH_DEV_ENV
        GUIShader.Get()->OnReloading.Bind<OnGUIShaderReloading>();
#endif

        render2DData->DrawCalls.EnsureCapacity(RENDER2D_INITIAL_DRAW_CALL_CAPACITY);

        return true;
    }

    void Render2DSystem::OnDispose()
    {
        render2DData->TintLayersStack.Resize(0);
        render2DData->ClipLayersStack.Resize(0);
        render2DData->DrawCalls.Resize(0);
        render2DData->Lines.Resize(0);
        render2DData->Lines2.Resize(0);

        render2DData->GUIShader = nullptr;

        render2DData->PsoDepth.Dispose();
        render2DData->PsoNoDepth.Dispose();

        render2DData->VB.Dispose();
        render2DData->IB.Dispose();
    }

    void Render2D::BeginFrame()
    {
        ENGINE_ASSERT(!IsRendering());
    }

    void Render2D::Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer)
    {
        ENGINE_ASSERT(output != nullptr);
        Begin(context, output->View(), depthBuffer ? depthBuffer->View() : nullptr, Viewport(output->Size()));
    }

    void Render2D::Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, const Matrix& viewProjection)
    {
        ENGINE_ASSERT(output != nullptr);
        Begin(context, output->View(), depthBuffer ? depthBuffer->View() : nullptr, Viewport(output->Size()), viewProjection);
    }

    void Render2D::Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport)
    {
        Matrix view, projection, viewProjection;
        const float halfWidth = viewport.width * 0.5f;
        const float halfHeight = viewport.height * 0.5f;
        const float zNear = 0.0f;
        const float zFar = 1.0f;
        Matrix::OrthoOffCenter(-halfWidth, halfWidth, halfHeight, -halfHeight, zNear, zFar, projection);
        Matrix::Translation(-halfWidth, -halfHeight, 0, view);
        Matrix::Multiply(view, projection, viewProjection);

        Begin(context, output, depthBuffer, viewport, viewProjection);

        render2DData->IsScissorsRectEnabled = true;
    }

    void Render2D::Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, const Viewport& viewport, const Matrix& viewProjection)
    {
        ENGINE_ASSERT(render2DData->Context == nullptr && render2DData->Output == nullptr);
        ENGINE_ASSERT(context != nullptr && output != nullptr);

        // Setup
        render2DData->Context = context;
        render2DData->Output = output;
        render2DData->DepthBuffer = depthBuffer;
        render2DData->View = viewport;
        render2DData->ViewProjection = viewProjection;
        render2DData->DrawCalls.Clear();

        // Initialize default transform
        const Matrix3x3 defaultTransform = Matrix3x3::Identity;
        render2DData->TransformLayersStack.Clear();
        render2DData->TransformLayersStack.Push(defaultTransform);
        render2DData->TransformCached = defaultTransform;

        // Initialize default clip mask
        const Rectangle defaultBounds(viewport.location, viewport.size);
        const RotatedRectangle defaultMask(defaultBounds);
        render2DData->ClipLayersStack.Clear();
        render2DData->ClipLayersStack.Add({ defaultMask, defaultBounds });

        // Initialize default tint stack
        render2DData->TintLayersStack.Clear();
        render2DData->TintLayersStack.Add({ 1, 1, 1, 1 });

        // Scissors can be enabled only for 2D orthographic projections
        render2DData->IsScissorsRectEnabled = false;

        // Reset geometry buffer
        render2DData->VB.Clear();
        render2DData->IB.Clear();
        render2DData->VBIndex = 0;
        render2DData->IBIndex = 0;
    }

    void Render2D::End()
    {
        RENDER2D_CHECK_RENDERING_STATE;
        ENGINE_ASSERT(render2DData->Context != nullptr && render2DData->Output != nullptr);
        ENGINE_ASSERT(render2DData->GUIShader != nullptr);

        // Skip if has nothing to draw
        if (render2DData->DrawCalls.IsEmpty())
        {
            // End
            render2DData->Context = nullptr;
            render2DData->Output = nullptr;
            return;
        }

        PROFILE_GPU_CPU_NAMED("Render2D");

        // Prepare shader
        GPUShader* shader;
        {
            if (!render2DData->GUIShader->IsLoaded() && render2DData->GUIShader->WaitForLoaded())
            {
                // End
                render2DData->DrawCalls.Clear();
                render2DData->Context = nullptr;
                render2DData->Output = nullptr;
                return;
            }
            shader = render2DData->GUIShader->GetShader();
        }

        // Flush geometry buffers
        render2DData->VB.Flush(render2DData->Context);
        render2DData->IB.Flush(render2DData->Context);

        // Set output
        render2DData->Context->ResetSR();
        if (render2DData->DepthBuffer != nullptr)
        {
            render2DData->Context->SetRenderTarget(render2DData->Output, render2DData->DepthBuffer);
        }
        else
        {
            render2DData->Context->SetRenderTarget(render2DData->Output);
        }

        render2DData->Context->SetViewportAndScissors(render2DData->View);
        render2DData->Context->FlushState();

        // Prepare constant buffer
        GPUConstantBuffer* constantBuffer = shader->GetCB(0);
        Data data;
        Matrix::Transpose(render2DData->ViewProjection, data.ViewProjection);
        render2DData->Context->UpdateCB(constantBuffer, &data);
        render2DData->Context->BindCB(0, constantBuffer);

        // Prepare PSO
        if (!render2DData->PsoDepth.Inited)
        {
            render2DData->PsoDepth.Init(render2DData->GUIShader.Get()->GetShader(), true);
            render2DData->PsoNoDepth.Init(render2DData->GUIShader.Get()->GetShader(), false);
        }
        render2DData->CurrentPso = render2DData->DepthBuffer ? &render2DData->PsoDepth : &render2DData->PsoNoDepth;

        // Flush draw calls
        int32 batchStart = 0, batchSize = 0;
        render2DData->IsScissorsRectEmpty = false;
        for (int32 i = 0; i < render2DData->DrawCalls.Count(); i++)
        {
            // Peek draw call
            const auto& drawCall = render2DData->DrawCalls[i];

            // Check if cannot add element to the batching
            if (batchSize != 0 && !CanBatchDrawCalls(render2DData->DrawCalls[batchStart], drawCall))
            {
                // Flush batched elements
                DrawBatch(batchStart, batchSize);
                batchStart += batchSize;
                batchSize = 0;
            }

            // Add element to batching
            batchSize++;
        }

        // Flush end of batched elements
        if (batchSize != 0)
        {
            DrawBatch(batchStart, batchSize);
        }

        // End
        render2DData->DrawCalls.Clear();
        render2DData->Context = nullptr;
        render2DData->Output = nullptr;
    }

    void Render2D::EndFrame()
    {
        ENGINE_ASSERT(!IsRendering());

        // Synchronize the texture atlases data
        FontManager::Flush();
    }

    void Render2D::PushTransform(const Matrix3x3& transform)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        // Combine transformation
        Matrix3x3 finalTransform;
        Matrix3x3::Multiply(transform, render2DData->TransformCached, finalTransform);

        // Push it
        render2DData->TransformLayersStack.Push(finalTransform);
        render2DData->TransformCached = render2DData->TransformLayersStack.Peek();
    }

    void Render2D::PeekTransform(Matrix3x3& transform)
    {
        transform = render2DData->TransformCached;
    }

    void Render2D::PopTransform()
    {
        RENDER2D_CHECK_RENDERING_STATE;

        ENGINE_ASSERT(render2DData->TransformLayersStack.HasItems());
        render2DData->TransformLayersStack.Pop();
        render2DData->TransformCached = render2DData->TransformLayersStack.Peek();
    }

    void OnClipScissors()
    {
        if (!render2DData->IsScissorsRectEnabled)
            return;

        const auto& mask = render2DData->ClipLayersStack.Peek();

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::ClipScissors;
        drawCall.AsClipScissors.X = mask.Bounds.GetX();
        drawCall.AsClipScissors.Y = mask.Bounds.GetY();
        drawCall.AsClipScissors.Width = mask.Bounds.GetWidth();
        drawCall.AsClipScissors.Height = mask.Bounds.GetHeight();
    }

    void Render2D::PushClip(const Rectangle& clipRect)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        RotatedRectangle clipRectTransformed;
        ApplyTransform(clipRect, clipRectTransformed);
        const Rectangle bounds = Rectangle::Shared(clipRectTransformed.ToBoundingRect(), render2DData->ClipLayersStack.Peek().Bounds);
        render2DData->ClipLayersStack.Push({ clipRectTransformed, bounds });

        OnClipScissors();
    }

    void Render2D::PeekClip(Rectangle& clipRect)
    {
        clipRect = render2DData->ClipLayersStack.Peek().Bounds;
    }

    void Render2D::PopClip()
    {
        RENDER2D_CHECK_RENDERING_STATE;

        render2DData->ClipLayersStack.Pop();

        OnClipScissors();
    }

    void Render2D::PushTint(const Color& tint, bool inherit)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        render2DData->TintLayersStack.Push(inherit ? tint * render2DData->TintLayersStack.Peek() : tint);
    }

    void Render2D::PeekTint(Color& tint)
    {
        tint = render2DData->TintLayersStack.Peek();
    }

    void Render2D::PopTint()
    {
        RENDER2D_CHECK_RENDERING_STATE;

        render2DData->TintLayersStack.Pop();
    }

    void CalculateKernelSize(float strength, int32& kernelSize, int32& downSample)
    {
        kernelSize = Math::RoundToInt(strength * 3.0f);

        if (DownsampleForBlur && kernelSize > 9)
        {
            downSample = kernelSize >= 64 ? 4 : 2;
            kernelSize /= downSample;
        }

        if (kernelSize % 2 == 0)
        {
            kernelSize++;
        }

        kernelSize = Math::Clamp(kernelSize, 3, RENDER2D_BLUR_MAX_SAMPLES / 2);
    }

    static float GetWeight(float dist, float strength)
    {
        float strength2 = strength * strength;
        return (1.0f / Math::Sqrt(2 * Math::PI * strength2)) * Math::Exp(-(dist * dist) / (2 * strength2));
    }

    static Float2 GetWeightAndOffset(float dist, float sigma)
    {
        float offset1 = dist;
        float weight1 = GetWeight(offset1, sigma);

        float offset2 = dist + 1;
        float weight2 = GetWeight(offset2, sigma);

        float totalWeight = weight1 + weight2;

        float offset = 0;
        if (totalWeight > 0)
        {
            offset = (weight1 * offset1 + weight2 * offset2) / totalWeight;
        }

        return Float2(totalWeight, offset);
    }

    static uint32 ComputeBlurWeights(int32 kernelSize, float sigma, Float4* outWeightsAndOffsets)
    {
        const uint32 numSamples = Math::DivideAndRoundUp((uint32)kernelSize, 2u);
        outWeightsAndOffsets[0] = Float4(Float2(GetWeight(0, sigma), 0), GetWeightAndOffset(1, sigma));
        uint32 sampleIndex = 1;
        for (int32 x = 3; x < kernelSize; x += 4)
        {
            outWeightsAndOffsets[sampleIndex] = Float4(GetWeightAndOffset((float)x, sigma), GetWeightAndOffset((float)(x + 2), sigma));
            sampleIndex++;
        }
        return numSamples;
    }

    void DrawBatch(int32 startIndex, int32 count)
    {
        const Render2DDrawCall& d = render2DData->DrawCalls[startIndex];
        GPUBuffer* vb = render2DData->VB.GetBuffer();
        GPUBuffer* ib = render2DData->IB.GetBuffer();
        uint32 countIb = 0;
        for (int32 i = 0; i < count; i++)
            countIb += render2DData->DrawCalls[startIndex + i].CountIB;

        if (d.Type == DrawCallType::ClipScissors)
        {
            Rectangle* scissorsRect = (Rectangle*)&d.AsClipScissors.X;
            render2DData->Context->SetScissor(*scissorsRect);
            render2DData->IsScissorsRectEmpty = scissorsRect->Size.IsAnyZero();
            return;
        }
        if (render2DData->IsScissorsRectEmpty)
            return;

        switch (d.Type)
        {
        case DrawCallType::FillRect:
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Color);
            break;
        case DrawCallType::FillRectNoAlpha:
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Color_NoAlpha);
            break;
        case DrawCallType::FillRT:
            render2DData->Context->BindSR(0, d.AsRT.Ptr);
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Image);
            break;
        case DrawCallType::FillTexture:
            render2DData->Context->BindSR(0, d.AsTexture.Ptr);
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Image);
            break;
        case DrawCallType::FillTexturePoint:
            render2DData->Context->BindSR(0, d.AsTexture.Ptr);
            render2DData->Context->SetState(render2DData->CurrentPso->PS_ImagePoint);
            break;
        case DrawCallType::DrawChar:
            render2DData->Context->BindSR(0, d.AsChar.Tex);
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Font);
            break;
        case DrawCallType::DrawCharMaterial:
        {
            // Apply and bind material
            auto material = d.AsChar.Mat;
            MaterialBase::BindParameters bindParams(render2DData->Context, *(RenderContext*)nullptr);
            Render2D::CustomData customData;
            customData.ViewProjection = render2DData->ViewProjection;
            customData.ViewSize = Float2::One;
            bindParams.customData = &customData;
            material->Bind(bindParams);

            // Bind font atlas as a material parameter
            static StringView FontParamName = SE_TEXT("Font");
            auto param = material->Params.Get(FontParamName);
            if (param && param->GetParameterType() == MaterialParameterType::Texture)
            {
                render2DData->Context->BindSR(param->GetRegister(), d.AsChar.Tex);
            }

            // Bind index and vertex buffers
            render2DData->Context->BindIB(ib);
            render2DData->Context->BindVB(ToSpan(&vb, 1));

            // Draw
            render2DData->Context->DrawIndexed(countIb, 0, d.StartIB);

            // Restore pipeline (material apply overrides it)
            const auto cb = render2DData->GUIShader->GetShader()->GetCB(0);
            render2DData->Context->BindCB(0, cb);

            return;
        }
        case DrawCallType::Custom:
            render2DData->Context->BindSR(0, d.AsCustom.Tex);
            render2DData->Context->SetState(d.AsCustom.Pso);
            break;
        case DrawCallType::Material:
        {
            // Bind material
            auto material = (MaterialBase*)d.AsMaterial.Mat;
            MaterialBase::BindParameters bindParams(render2DData->Context, *(RenderContext*)nullptr);
            Render2D::CustomData customData;
            customData.ViewProjection = render2DData->ViewProjection;
            customData.ViewSize = Float2(d.AsMaterial.Width, d.AsMaterial.Height);
            bindParams.customData = &customData;
            material->Bind(bindParams);

            // Bind index and vertex buffers
            render2DData->Context->BindIB(ib);
            render2DData->Context->BindVB(ToSpan(&vb, 1));

            // Draw
            render2DData->Context->DrawIndexed(countIb, 0, d.StartIB);

            // Restore pipeline (material apply overrides it)
            const auto cb = render2DData->GUIShader->GetShader()->GetCB(0);
            render2DData->Context->BindCB(0, cb);

            return;
        }
        case DrawCallType::Blur:
        {
            PROFILE_GPU("Blur");

            const Float4 bounds(d.AsBlur.UpperLeftX, d.AsBlur.UpperLeftY, d.AsBlur.BottomRightX, d.AsBlur.BottomRightY);
            float blurStrength = Math::Max(d.AsBlur.Strength, 1.0f);
            const auto& limits = GPUDevice::instance->GetGPULimits();
            int32 renderTargetWidth = Math::Min(Math::RoundToInt(d.AsBlur.Width), limits.MaximumTexture2DSize);
            int32 renderTargetHeight = Math::Min(Math::RoundToInt(d.AsBlur.Height), limits.MaximumTexture2DSize);

            int32 kernelSize = 0, downSample = 0;
            CalculateKernelSize(blurStrength, kernelSize, downSample);
            if (downSample > 0)
            {
                renderTargetWidth = Math::DivideAndRoundUp(renderTargetWidth, downSample);
                renderTargetHeight = Math::DivideAndRoundUp(renderTargetHeight, downSample);
                blurStrength /= downSample;
            }

            // Skip if no chance to render anything
            renderTargetWidth = Math::AlignDown(renderTargetWidth, 4);
            renderTargetHeight = Math::AlignDown(renderTargetHeight, 4);
            if (renderTargetWidth <= 0 || renderTargetHeight <= 0)
                return;

            // Get temporary textures
            auto desc = GPUTextureDescription::New2D(renderTargetWidth, renderTargetHeight, PS_Blur_Format);
            auto blurA = RenderTargetPool::Get(desc);
            auto blurB = RenderTargetPool::Get(desc);
            RENDER_TARGET_POOL_SET_NAME(blurA, "Render2D.BlurA");
            RENDER_TARGET_POOL_SET_NAME(blurB, "Render2D.BlurB");

            // Prepare blur data
            BlurData data;
            data.Bounds.x = bounds.x;
            data.Bounds.y = bounds.y;
            data.Bounds.z = bounds.z - bounds.x;
            data.Bounds.w = bounds.w - bounds.y;
            data.InvBufferSize.x = 1.0f / (float)renderTargetWidth;
            data.InvBufferSize.y = 1.0f / (float)renderTargetHeight;
            data.SampleCount = ComputeBlurWeights(kernelSize, blurStrength, data.WeightAndOffsets);
            const auto cb = render2DData->GUIShader->GetShader()->GetCB(1);
            render2DData->Context->UpdateCB(cb, &data);
            render2DData->Context->BindCB(1, cb);

            // Downscale (or not) and extract the background image for the blurring
            render2DData->Context->ResetRenderTarget();
            render2DData->Context->SetRenderTarget(blurA->View());
            render2DData->Context->SetViewportAndScissors((float)renderTargetWidth, (float)renderTargetHeight);
            render2DData->Context->BindSR(0, render2DData->Output);
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Downscale);
            render2DData->Context->DrawFullscreenTriangle();

            // Render the blur (1st pass)
            render2DData->Context->ResetRenderTarget();
            render2DData->Context->SetRenderTarget(blurB->View());
            render2DData->Context->BindSR(0, blurA->View());
            render2DData->Context->SetState(render2DData->CurrentPso->PS_BlurH);
            render2DData->Context->DrawFullscreenTriangle();

            // Render the blur (2nd pass)
            render2DData->Context->ResetRenderTarget();
            render2DData->Context->SetRenderTarget(blurA->View());
            render2DData->Context->BindSR(0, blurB->View());
            render2DData->Context->SetState(render2DData->CurrentPso->PS_BlurV);
            render2DData->Context->DrawFullscreenTriangle();

            // Restore output
            render2DData->Context->ResetRenderTarget();
            render2DData->Context->SetRenderTarget(render2DData->DepthBuffer, render2DData->Output);
            render2DData->Context->SetViewportAndScissors(render2DData->View);
            render2DData->Context->UnBindCB(1);

            // Link for drawing final blur as a texture
            render2DData->Context->BindSR(0, blurA->View());
            render2DData->Context->SetState(render2DData->CurrentPso->PS_Image);

            // Cleanup
            RenderTargetPool::Release(blurA);
            RenderTargetPool::Release(blurB);

            break;
        }
        case DrawCallType::ClipScissors:
            render2DData->Context->SetScissor(*(Rectangle*)&d.AsClipScissors.X);
            return;
        case DrawCallType::LineAA:
            render2DData->Context->SetState(render2DData->CurrentPso->PS_LineAA);
            break;
#if !BUILD_RELEASE
        default:
            ENGINE_UNREACHABLE_CODE();
#endif
        }

        // Draw
        render2DData->Context->BindVB(ToSpan(&vb, 1));
        render2DData->Context->BindIB(ib);
        render2DData->Context->DrawIndexed(countIb, 0, d.StartIB);
    }


    void Render2D::RenderText(Font* font, const StringView& text, const Rectangle& layoutRect, const Color& color, TextAlignment horizontalAlignment,
        TextAlignment verticalAlignment, TextWrapping textWrapping, float baseLinesGapScale, float scale)
    {
        TextLayoutOptions layout;
        layout.Bounds = layoutRect;
        layout.HorizontalAlignment = horizontalAlignment;
        layout.VerticalAlignment = verticalAlignment;
        layout.TextWrapping = textWrapping;
        layout.Scale = scale;
        layout.BaseLinesGapScale = baseLinesGapScale;

        RenderText(font, text, color, layout);
    }

    void Render2D::RenderText(Font* font, MaterialBase* customMaterial, const StringView& text, const Rectangle& layoutRect, const Color& color,
        TextAlignment horizontalAlignment, TextAlignment verticalAlignment, TextWrapping textWrapping, float baseLinesGapScale, float scale)
    {
        TextLayoutOptions layout;
        layout.Bounds = layoutRect;
        layout.HorizontalAlignment = horizontalAlignment;
        layout.VerticalAlignment = verticalAlignment;
        layout.TextWrapping = textWrapping;
        layout.Scale = scale;
        layout.BaseLinesGapScale = baseLinesGapScale;

        RenderText(font, text, color, layout, customMaterial);
    }

    void Render2D::RenderText(Font* font, const StringView& text, const Color& color, const Float2& location, MaterialBase* customMaterial)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        // Check if there is no need to do anything
        if (font == nullptr ||
            text.Length() < 0 ||
            (customMaterial && (!customMaterial->IsReady()/* || !customMaterial->IsGUI()*/)))
            return;

        // Temporary data
        uint32 fontAtlasIndex = 0;
        FontTextureAtlas* fontAtlas = nullptr;
        Float2 invAtlasSize = Float2::One;
        FontCharacterEntry previous;
        int32 kerning;
        float scale = 1.0f / FontManager::FontScale;
        const bool enableFallbackFonts = Features.IsFlag(RenderingFeatures::FallbackFonts);

        // Render all characters
        FontCharacterEntry entry;
        Render2DDrawCall drawCall;
        if (customMaterial)
        {
            drawCall.Type = DrawCallType::DrawCharMaterial;
            drawCall.AsChar.Mat = customMaterial;
        }
        else
        {
            drawCall.Type = DrawCallType::DrawChar;
            drawCall.AsChar.Mat = nullptr;
        }
        Float2 pointer = location;
        for (int32 currentIndex = 0; currentIndex < text.Length(); currentIndex++)
        {
            // Cache current character
            const Char currentChar = text[currentIndex];

            // Check if it isn't a newline character
            if (currentChar != '\n')
            {
                // Get character entry
                font->GetCharacter(currentChar, entry, enableFallbackFonts);

                // Check if need to select/change font atlas (since characters even in the same font may be located in different atlases)
                if (fontAtlas == nullptr || entry.TextureIndex != fontAtlasIndex)
                {
                    // Get texture atlas that contains current character
                    fontAtlasIndex = entry.TextureIndex;
                    fontAtlas = FontManager::GetAtlas(fontAtlasIndex);
                    if (fontAtlas)
                    {
                        fontAtlas->EnsureTextureCreated();
                        drawCall.AsChar.Tex = fontAtlas->GetTexture();
                        invAtlasSize = 1.0f / fontAtlas->GetSize();
                    }
                    else
                    {
                        drawCall.AsChar.Tex = nullptr;
                        invAtlasSize = 1.0f;
                    }
                }

                // Check if character is a whitespace
                const bool isWhitespace = StringUtils::IsWhitespace(currentChar);

                // Get kerning
                if (!isWhitespace && previous.IsValid)
                {
                    kerning = entry.Font->GetKerning(previous.Character, entry.Character);
                }
                else
                {
                    kerning = 0;
                }
                pointer.x += kerning * scale;
                previous = entry;

                // Omit whitespace characters
                if (!isWhitespace)
                {
                    // Calculate character size and atlas coordinates
                    const float x = pointer.x + entry.OffsetX * scale;
                    const float y = pointer.y + (font->GetHeight() + font->GetDescender() - entry.OffsetY) * scale;

                    Rectangle charRect(x, y, entry.UVSize.x * scale, entry.UVSize.y * scale);

                    Float2 upperLeftUV = entry.UV * invAtlasSize;
                    Float2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                    // Add draw call
                    drawCall.StartIB = render2DData->IBIndex;
                    drawCall.CountIB = 6;
                    render2DData->DrawCalls.Add(drawCall);
                    WriteRect(charRect, color, upperLeftUV, rightBottomUV);
                }

                // Move
                pointer.x += entry.AdvanceX * scale;
            }
            else
            {
                // Move
                pointer.x = location.x;
                pointer.y += font->GetHeight() * scale;
            }
        }
    }

    void Render2D::RenderText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const Float2& location, MaterialBase* customMaterial)
    {
        RenderText(font, textRange.Substring(text), color, location, customMaterial);
    }

    void Render2D::RenderText(Font* font, const StringView& text, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        // Check if there is no need to do anything
        if (font == nullptr ||
            text.IsEmpty() ||
            layout.Scale <= Math::ZeroTolerance ||
            (customMaterial && (!customMaterial->IsReady()/* || !customMaterial->IsGUI()*/)))
            return;

        // Temporary data
        uint32 fontAtlasIndex = 0;
        FontTextureAtlas* fontAtlas = nullptr;
        Float2 invAtlasSize = Float2::One;
        FontCharacterEntry previous;
        int32 kerning;
        float scale = layout.Scale / FontManager::FontScale;
        const bool enableFallbackFonts = Features.IsFlag(RenderingFeatures::FallbackFonts);

        // Process text to get lines
        render2DData->Lines.Clear();
        font->ProcessText(text, render2DData->Lines, layout);

        // Render all lines
        FontCharacterEntry entry;
        Render2DDrawCall drawCall;
        if (customMaterial)
        {
            drawCall.Type = DrawCallType::DrawCharMaterial;
            drawCall.AsChar.Mat = customMaterial;
        }
        else
        {
            drawCall.Type = DrawCallType::DrawChar;
            drawCall.AsChar.Mat = nullptr;
        }
        for (int32 lineIndex = 0; lineIndex < render2DData->Lines.Count(); lineIndex++)
        {
            const FontLineCache& line = render2DData->Lines[lineIndex];
            Float2 pointer = line.Location;

            // Render all characters from the line
            for (int32 charIndex = line.FirstCharIndex; charIndex <= line.LastCharIndex; charIndex++)
            {
                // Cache current character
                const Char currentChar = text[charIndex];

                // Check if it isn't a newline character
                if (currentChar != '\n')
                {
                    // Get character entry
                    font->GetCharacter(currentChar, entry, enableFallbackFonts);

                    // Check if need to select/change font atlas (since characters even in the same font may be located in different atlases)
                    if (fontAtlas == nullptr || entry.TextureIndex != fontAtlasIndex)
                    {
                        // Get texture atlas that contains current character
                        fontAtlasIndex = entry.TextureIndex;
                        fontAtlas = FontManager::GetAtlas(fontAtlasIndex);
                        if (fontAtlas)
                        {
                            fontAtlas->EnsureTextureCreated();
                            invAtlasSize = 1.0f / fontAtlas->GetSize();
                            drawCall.AsChar.Tex = fontAtlas->GetTexture();
                        }
                        else
                        {
                            invAtlasSize = 1.0f;
                            drawCall.AsChar.Tex = nullptr;
                        }
                    }

                    // Get kerning
                    const bool isWhitespace = StringUtils::IsWhitespace(currentChar);
                    if (!isWhitespace && previous.IsValid)
                    {
                        kerning = entry.Font->GetKerning(previous.Character, entry.Character);
                    }
                    else
                    {
                        kerning = 0;
                    }
                    pointer.x += (float)kerning * scale;
                    previous = entry;

                    // Omit whitespace characters
                    if (!isWhitespace)
                    {
                        // Calculate character size and atlas coordinates
                        const float x = pointer.x + entry.OffsetX * scale;
                        const float y = pointer.y - entry.OffsetY * scale + Math::Ceil((font->GetHeight() + font->GetDescender()) * scale);

                        Rectangle charRect(x, y, entry.UVSize.x * scale, entry.UVSize.y * scale);
                        charRect.Offset(layout.Bounds.Location);

                        Float2 upperLeftUV = entry.UV * invAtlasSize;
                        Float2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                        // Add draw call
                        drawCall.StartIB = render2DData->IBIndex;
                        drawCall.CountIB = 6;
                        render2DData->DrawCalls.Add(drawCall);
                        WriteRect(charRect, color, upperLeftUV, rightBottomUV);
                    }

                    // Move
                    pointer.x += entry.AdvanceX * scale;
                }
            }
        }
    }

    void Render2D::RenderText(Font* font, const StringView& text, const TextRange& textRange, const Color& color, const TextLayoutOptions& layout, MaterialBase* customMaterial)
    {
        RenderText(font, textRange.Substring(text), color, layout, customMaterial);
    }

    FORCE_INLINE bool NeedAlphaWithTint(const Color& color)
    {
        return (color.a * render2DData->TintLayersStack.Peek().a) < 1.0f;
    }

    FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2)
    {
        return (color1.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color2.a * render2DData->TintLayersStack.Peek().a) < 1.0f;
    }

    FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2, const Color& color3)
    {
        return (color1.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color2.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color3.a * render2DData->TintLayersStack.Peek().a) < 1.0f;
    }

    FORCE_INLINE bool NeedAlphaWithTint(const Color& color1, const Color& color2, const Color& color3, const Color& color4)
    {
        return (color1.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color2.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color3.a * render2DData->TintLayersStack.Peek().a) < 1.0f || (color4.a * render2DData->TintLayersStack.Peek().a) < 1.0f;
    }

    void Render2D::FillRectangle(const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = NeedAlphaWithTint(color) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        WriteRect(rect, color);
    }

    void Render2D::FillRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = NeedAlphaWithTint(color1, color2, color3, color4) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        WriteRect(rect, color1, color2, color3, color4);
    }

    void Render2D::DrawRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4, float thickness)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        const auto& mask = render2DData->ClipLayersStack.Peek().Mask;
        thickness *= (render2DData->TransformCached.M11 + render2DData->TransformCached.M22 + render2DData->TransformCached.M33) * 0.3333333f;

        Float2 points[5];
        ApplyTransform(rect.GetUpperLeft(), points[0]);
        ApplyTransform(rect.GetUpperRight(), points[1]);
        ApplyTransform(rect.GetBottomRight(), points[2]);
        ApplyTransform(rect.GetBottomLeft(), points[3]);
        points[4] = points[0];

        Color colors[5];
        colors[0] = color1;
        colors[1] = color2;
        colors[2] = color3;
        colors[3] = color4;
        colors[4] = colors[0];

        Render2DVertex v[4];
        uint32 indices[6];
        Float2 p1t, p2t;
        Color c1t, c2t;

        p1t = points[0];
        c1t = colors[0];

#if RENDER2D_USE_LINE_AA
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::LineAA;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 4 * (6 + 3);

        // This must be the same as in HLSL code
        const float filterScale = 1.0f;
        const float thicknessHalf = (2.82842712f + thickness) * 0.5f + filterScale;

        for (int32 i = 1; i < 5; i++)
        {
            p2t = points[i];
            c2t = colors[i];

            Float2 line = p2t - p1t;
            Float2 up = thicknessHalf * Float2::Normalize(Float2(-line.y, line.x));
            Float2 right = thicknessHalf * Float2::Normalize(line);

            // Line

            v[0] = MakeVertex(p2t + up, Float2::UnitX, c2t, mask, { thickness, (float)Features });
            v[1] = MakeVertex(p1t + up, Float2::UnitX, c1t, mask, { thickness, (float)Features });
            v[2] = MakeVertex(p1t - up, Float2::Zero, c1t, mask, { thickness, (float)Features });
            v[3] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { thickness, (float)Features });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = VBIndex + 0;
            indices[1] = VBIndex + 1;
            indices[2] = VBIndex + 2;
            indices[3] = VBIndex + 2;
            indices[4] = VBIndex + 3;
            indices[5] = VBIndex + 0;
            IB.Write(indices, sizeof(uint32) * 6);

            VBIndex += 4;
            render2DData->IBIndex += 6;

            // Corner cap

            const float tmp = thickness * 0.69f;
            v[0] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { tmp, (float)Features });
            v[1] = MakeVertex(p2t + right, Float2::Zero, c2t, mask, { tmp, (float)Features });
            v[2] = MakeVertex(p2t, Float2(0.5f, 0.0f), c2t, mask, { tmp, (float)Features });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = VBIndex + 1;
            indices[1] = VBIndex + 2;
            indices[2] = VBIndex + 0;

            IB.Write(indices, sizeof(uint32) * 3);

            VBIndex += 4;
            render2DData->IBIndex += 3;

            p1t = p2t;
            c1t = c2t;
        }
#else
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = NeedAlphaWithTint(color1, color2) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 4 * (6 + 3);

        const float thicknessHalf = thickness * 0.5f;

        for (int32 i = 1; i < 5; i++)
        {
            p2t = points[i];
            c2t = colors[i];

            Float2 line = p2t - p1t;
            Float2 up = thicknessHalf * Float2::Normalize(Float2(-line.y, line.x));
            Float2 right = thicknessHalf * Float2::Normalize(line);

            // Line

            v[0] = MakeVertex(p2t + up, Float2::UnitX, c2t, mask, { 0.0f, (float)Features.Get() });
            v[1] = MakeVertex(p1t + up, Float2::UnitX, c1t, mask, { 0.0f, (float)Features.Get() });
            v[2] = MakeVertex(p1t - up, Float2::Zero, c1t, mask, { 0.0f, (float)Features.Get() });
            v[3] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { 0.0f, (float)Features.Get() });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = render2DData->VBIndex + 0;
            indices[1] = render2DData->VBIndex + 1;
            indices[2] = render2DData->VBIndex + 2;
            indices[3] = render2DData->VBIndex + 2;
            indices[4] = render2DData->VBIndex + 3;
            indices[5] = render2DData->VBIndex + 0;
            render2DData->IB.Write(indices, sizeof(uint32) * 6);

            render2DData->VBIndex += 4;
            render2DData->IBIndex += 6;

            // Corner cap

            v[0] = MakeVertex(p2t - up, Float2::Zero, c2t, mask, { 0.0f, (float)Features.Get() });
            v[1] = MakeVertex(p2t + right, Float2::Zero, c2t, mask, { 0.0f, (float)Features.Get() });
            v[2] = MakeVertex(p2t, Float2(0.5f, 0.0f), c2t, mask, { 0.0f, (float)Features.Get() });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = render2DData->VBIndex + 1;
            indices[1] = render2DData->VBIndex + 2;
            indices[2] = render2DData->VBIndex + 0;

            render2DData->IB.Write(indices, sizeof(uint32) * 3);

            render2DData->VBIndex += 4;
            render2DData->IBIndex += 3;

            p1t = p2t;
            c1t = c2t;
        }
#endif
    }

    void Render2D::DrawTexture(GPUTextureView* rt, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillRT;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsRT.Ptr = rt;
        WriteRect(rect, color);
    }

    void Render2D::DrawTexture(GPUTexture* t, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall drawCall;
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsTexture.Ptr = t;
        render2DData->DrawCalls.Add(drawCall);
        WriteRect(rect, color);
    }

    void Render2D::DrawTexture(TextureBase* t, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall drawCall;
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
        render2DData->DrawCalls.Add(drawCall);
        WriteRect(rect, color);
    }

    void Render2D::DrawTexture(Texture* t, const Rectangle& rect, const Color& color)
    {
        DrawTexture(static_cast<TextureBase*>(t), rect, color);
    }

    void Render2D::DrawSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
            return;

        Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
        WriteRect(rect, color, sprite->Area.GetUpperLeft(), sprite->Area.GetBottomRight());
    }

    void Render2D::DrawTexturePoint(GPUTexture* t, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexturePoint;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsTexture.Ptr = t;
        WriteRect(rect, color);
    }

    void Render2D::DrawSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
            return;

        Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexturePoint;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
        WriteRect(rect, color, sprite->Area.GetUpperLeft(), sprite->Area.GetBottomRight());
    }

    void Render2D::Draw9SlicingTexture(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall drawCall;
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6 * 9;
        drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
        render2DData->DrawCalls.Add(drawCall);
        Write9SlicingRect(rect, color, border, borderUVs);
    }

    void Render2D::Draw9SlicingTexturePoint(TextureBase* t, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall drawCall;
        drawCall.Type = DrawCallType::FillTexturePoint;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6 * 9;
        drawCall.AsTexture.Ptr = t ? t->GetTexture() : nullptr;
        render2DData->DrawCalls.Add(drawCall);
        Write9SlicingRect(rect, color, border, borderUVs);
    }

    /*void Render2D::Draw9SlicingSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
            return;

        Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6 * 9;
        drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
        Write9SlicingRect(rect, color, border, borderUVs, sprite->Area.Location, sprite->Area.Size);
    }*/

    /*void Render2D::Draw9SlicingSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Float4& border, const Float4& borderUVs, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (spriteHandle.Index == INVALID_INDEX || !spriteHandle.Atlas || !spriteHandle.Atlas->GetTexture()->HasResidentMip())
            return;

        Sprite* sprite = &spriteHandle.Atlas->Sprites.At(spriteHandle.Index);
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexturePoint;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6 * 9;
        drawCall.AsTexture.Ptr = spriteHandle.Atlas->GetTexture();
        Write9SlicingRect(rect, color, border, borderUVs, sprite->Area.Location, sprite->Area.Size);
    }*/

    void Render2D::DrawCustom(GPUTexture* t, const Rectangle& rect, GPUPipelineState* ps, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (ps == nullptr || !ps->IsValid())
            return;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::Custom;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsCustom.Tex = t;
        drawCall.AsCustom.Pso = ps;
        WriteRect(rect, color);
    }

#if RENDER2D_USE_LINE_AA

    void DrawLineCap(const Float2& capOrigin, const Float2& capDirection, const Float2& up, const Color& color, float thickness)
    {
        const auto& mask = ClipLayersStack.Peek().Mask;

        Render2DVertex v[5];
        v[0] = MakeVertex(capOrigin, Float2(0.5f, 0.0f), color, mask, { thickness, (float)Render2D::Features });
        v[1] = MakeVertex(capOrigin + capDirection + up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
        v[2] = MakeVertex(capOrigin + capDirection - up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
        v[3] = MakeVertex(capOrigin + up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
        v[4] = MakeVertex(capOrigin - up, Float2::Zero, color, mask, { thickness, (float)Render2D::Features });
        render2DData->VB.Write(v, sizeof(v));

        uint32 indices[9];
        indices[0] = VBIndex + 0;
        indices[1] = VBIndex + 3;
        indices[2] = VBIndex + 1;
        indices[3] = VBIndex + 0;
        indices[4] = VBIndex + 1;
        indices[5] = VBIndex + 2;
        indices[6] = VBIndex + 0;
        indices[7] = VBIndex + 2;
        indices[8] = VBIndex + 4;
        IB.Write(indices, sizeof(indices));

        VBIndex += 5;
        render2DData->IBIndex += 9;
    }

#endif

    void DrawLines(const Float2* points, int32 pointsCount, const Color& color1, const Color& color2, float thickness)
    {
        ENGINE_ASSERT(points && pointsCount >= 2);
        const auto& mask = render2DData->ClipLayersStack.Peek().Mask;

        thickness *= (render2DData->TransformCached.M11 + render2DData->TransformCached.M22 + render2DData->TransformCached.M33) * 0.3333333f;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.StartIB = render2DData->IBIndex;

        Render2DVertex v[4];
        uint32 indices[6];
        Float2 p1t, p2t;

#if RENDER2D_USE_LINE_AA
        // This must be the same as in HLSL code
        const float filterScale = 1.0f;
        const float thicknessHalf = (2.82842712f + thickness) * 0.5f + filterScale;

        drawCall.Type = DrawCallType::LineAA;
        drawCall.CountIB = 9 + 9;

        Float2 line;
        Float2 normal;
        Float2 up;
        ApplyTransform(points[0], p1t);

        // Starting cap
        {
            ApplyTransform(points[1], p2t);

            line = p2t - p1t;
            normal = Float2::Normalize(Float2(-line.y, line.x));
            up = normal * thicknessHalf;
            const Float2 capDirection = thicknessHalf * Float2::Normalize(p1t - p2t);

            DrawLineCap(p1t, capDirection, up, color1, thickness);
        }

        // Lines
        for (int32 i = 1; i < pointsCount; i++)
        {
            ApplyTransform(points[i], p2t);

            line = p2t - p1t;
            normal = Float2::Normalize(Float2(-line.y, line.x));
            up = normal * thicknessHalf;

            v[0] = MakeVertex(p2t + up, Float2::UnitX, color2, mask, { thickness, (float)Render2D::Features });
            v[1] = MakeVertex(p1t + up, Float2::UnitX, color1, mask, { thickness, (float)Render2D::Features });
            v[2] = MakeVertex(p1t - up, Float2::Zero, color1, mask, { thickness, (float)Render2D::Features });
            v[3] = MakeVertex(p2t - up, Float2::Zero, color2, mask, { thickness, (float)Render2D::Features });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = VBIndex + 0;
            indices[1] = VBIndex + 1;
            indices[2] = VBIndex + 2;
            indices[3] = VBIndex + 2;
            indices[4] = VBIndex + 3;
            indices[5] = VBIndex + 0;
            IB.Write(indices, sizeof(uint32) * 6);

            VBIndex += 4;
            render2DData->IBIndex += 6;
            drawCall.CountIB += 6;

            p1t = p2t;
        }

        // Ending cap
        {
            ApplyTransform(points[0], p1t);
            ApplyTransform(points[1], p2t);

            const Float2 capDirection = thicknessHalf * Float2::Normalize(p2t - p1t);

            DrawLineCap(p2t, capDirection, up, color2, thickness);
        }
#else
        const float thicknessHalf = thickness * 0.5f;

        drawCall.Type = NeedAlphaWithTint(color1, color2) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.CountIB = 0;

        ApplyTransform(points[0], p1t);
        for (int32 i = 1; i < pointsCount; i++)
        {
            ApplyTransform(points[i], p2t);

            const Float2 line = p2t - p1t;
            const Float2 direction = thicknessHalf * Float2::Normalize(p2t - p1t);
            const Float2 normal = Float2::Normalize(Float2(-line.y, line.x));

            v[0] = MakeVertex(p2t + thicknessHalf * normal + direction, Float2::Zero, color2, mask, { 0.0f, (float)Render2D::Features.Get() });
            v[1] = MakeVertex(p1t + thicknessHalf * normal - direction, Float2::Zero, color1, mask, { 0.0f, (float)Render2D::Features.Get() });
            v[2] = MakeVertex(p1t - thicknessHalf * normal - direction, Float2::Zero, color1, mask, { 0.0f, (float)Render2D::Features.Get() });
            v[3] = MakeVertex(p2t - thicknessHalf * normal + direction, Float2::Zero, color2, mask, { 0.0f, (float)Render2D::Features.Get() });
            render2DData->VB.Write(v, sizeof(Render2DVertex) * 4);

            indices[0] = render2DData->VBIndex + 0;
            indices[1] = render2DData->VBIndex + 1;
            indices[2] = render2DData->VBIndex + 2;
            indices[3] = render2DData->VBIndex + 2;
            indices[4] = render2DData->VBIndex + 3;
            indices[5] = render2DData->VBIndex + 0;
            render2DData->IB.Write(indices, sizeof(uint32) * 6);

            render2DData->VBIndex += 4;
            render2DData->IBIndex += 6;
            drawCall.CountIB += 6;

            p1t = p2t;
        }
#endif
    }

    void Render2D::DrawLine(const Float2& p1, const Float2& p2, const Color& color1, const Color& color2, float thickness)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Float2 points[2];
        points[0] = p1;
        points[1] = p2;

        DrawLines(points, 2, color1, color2, thickness);
    }

    void Render2D::DrawBezier(const Float2& p1, const Float2& p2, const Float2& p3, const Float2& p4, const Color& color, float thickness)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        // Find amount of segments to use
        const Float2 d1 = p2 - p1;
        const Float2 d2 = p3 - p2;
        const Float2 d3 = p4 - p3;
        const float len = d1.Length() + d2.Length() + d3.Length();
        const int32 segmentCount = Math::Clamp(Math::CeilToInt(len * 0.05f), 1, 100);
        const float segmentCountInv = 1.0f / segmentCount;

        // Draw segmented curve
        /*Float2 p;
        AnimationUtils::Bezier(p1, p2, p3, p4, 0, p);
        Lines2.Clear();
        Lines2.Add(p);
        for (int32 i = 1; i <= segmentCount; i++)
        {
            const float t = i * segmentCountInv;
            AnimationUtils::Bezier(p1, p2, p3, p4, t, p);
            Lines2.Add(p);
        }*/
        DrawLines(render2DData->Lines2.Get(), render2DData->Lines2.Count(), color, color, thickness);
    }

    void Render2D::DrawMaterial(MaterialBase* material, const Rectangle& rect, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        if (material == nullptr || !material->IsReady()/* || !material->IsGUI()*/)
            return;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::Material;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsMaterial.Mat = material;
        drawCall.AsMaterial.Width = rect.GetWidth();
        drawCall.AsMaterial.Height = rect.GetHeight();
        WriteRect(rect, color);
    }

    void Render2D::DrawBlur(const Rectangle& rect, float blurStrength)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Float2 p;
        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::Blur;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 6;
        drawCall.AsBlur.Strength = blurStrength;
        drawCall.AsBlur.Width = rect.GetWidth();
        drawCall.AsBlur.Height = rect.GetHeight();
        ApplyTransform(rect.GetUpperLeft(), p);
        drawCall.AsBlur.UpperLeftX = p.x;
        drawCall.AsBlur.UpperLeftY = p.y;
        ApplyTransform(rect.GetBottomRight(), p);
        drawCall.AsBlur.BottomRightX = p.x;
        drawCall.AsBlur.BottomRightY = p.y;
        WriteRect(rect, Colors::White);
    }

    void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        ENGINE_ASSERT(vertices.Length() == uvs.Length());

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = vertices.Length();
        drawCall.AsTexture.Ptr = t;

        for (int32 i = 0; i < vertices.Length(); i += 3)
        {
            WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], uvs[i], uvs[i + 1], uvs[i + 2]);
        }
    }

    void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Color& color)
    {
        Color colors[3] = { (Color)color, (Color)color, (Color)color };
        Span<Color> spancolor(colors, 3);
        DrawTexturedTriangles(t, vertices, uvs, spancolor);
    }

    void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        ENGINE_ASSERT(vertices.Length() == uvs.Length());
        ENGINE_ASSERT(vertices.Length() == colors.Length());

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = vertices.Length();
        drawCall.AsTexture.Ptr = t;

        for (int32 i = 0; i < vertices.Length(); i += 3)
            WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], uvs[i], uvs[i + 1], uvs[i + 2], colors[i], colors[i + 1], colors[i + 2]);
    }

    void Render2D::DrawTexturedTriangles(GPUTexture* t, const Span<uint16>& indices, const Span<Float2>& vertices, const Span<Float2>& uvs, const Span<Color>& colors)
    {
        RENDER2D_CHECK_RENDERING_STATE;
        ENGINE_ASSERT(vertices.Length() == uvs.Length());
        ENGINE_ASSERT(vertices.Length() == colors.Length());

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = DrawCallType::FillTexture;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = indices.Length();
        drawCall.AsTexture.Ptr = t;

        for (int32 i = 0; i < indices.Length();)
        {
            const uint16 i0 = indices.Get()[i++];
            const uint16 i1 = indices.Get()[i++];
            const uint16 i2 = indices.Get()[i++];
            WriteTri(vertices[i0], vertices[i1], vertices[i2], uvs[i0], uvs[i1], uvs[i2], colors[i0], colors[i1], colors[i2]);
        }
    }

    void Render2D::FillTriangles(const Span<Float2>& vertices, const Span<Color>& colors, bool useAlpha)
    {
        ENGINE_ASSERT(vertices.Length() == colors.Length());
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = useAlpha ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = vertices.Length();

        for (int32 i = 0; i < vertices.Length(); i += 3)
            WriteTri(vertices[i], vertices[i + 1], vertices[i + 2], colors[i], colors[i + 1], colors[i + 2]);
    }

    void Render2D::FillTriangle(const Float2& p0, const Float2& p1, const Float2& p2, const Color& color)
    {
        RENDER2D_CHECK_RENDERING_STATE;

        Render2DDrawCall& drawCall = render2DData->DrawCalls.AddOne();
        drawCall.Type = NeedAlphaWithTint(color) ? DrawCallType::FillRect : DrawCallType::FillRectNoAlpha;
        drawCall.StartIB = render2DData->IBIndex;
        drawCall.CountIB = 3;
        WriteTri(p0, p1, p2, color, color, color);
    }

    void Render2D::CallDrawing(Function<void()> drawableElement, GPUContext* context, GPUTexture* output)
    {
        ENGINE_ASSERT(context != nullptr && output != nullptr && drawableElement.IsBinded())

        Begin(context, output);
        drawableElement();
        End();
    }

    void Render2D::CallDrawing(Function<void()> drawableElement, GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, Matrix& viewProjection)
    {
        ENGINE_ASSERT(context != nullptr && output != nullptr && drawableElement.IsBinded())

        if (depthBuffer != nullptr)
        {
            if (!depthBuffer->IsAllocated())
            {
                LOG_ERROR("Render", "Depth buffer is not allocated. Use GPUTexture.Init before rendering.");
                return;
            }
            if (output->Size() != depthBuffer->Size())
            {
                LOG_ERROR("Render", "Output buffer and depth buffer dimensions must be equal.");
                return;
            }
        }

        Begin(context, output, depthBuffer, viewProjection);
        drawableElement();
        End();
    }
} // SE