#pragma once

#include "DirectionalLight.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/Assets/Texture/CubeTexture.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
    class GPUPipelineState;

    SE_ENUM(Reflect)
    enum class SkyType
    {
        SkyBox = 0,
        Physical
    };

    /// <summary>
    /// Sky actor renders atmosphere around the scene with fog and sky.
    /// </summary>
    class SE_API_RUNTIME Sky : public RenderActor, public ISkyRenderer/*, public IAtmosphericFogRenderer,*/
    {
        SE_DEFINE_CLASS(Sky, RenderActor);
    private:
        AssetRef<Shader> _shader;
        GPUPipelineState* _psSky;
        GPUPipelineState* _psFog;
        GPUPipelineState* _psSkyBox;
        int32 _sceneRenderingKey = -1;

    public:
        Sky();
        ~Sky() override;

        SkyType RenderSkyType;

        /// <summary>
        /// The cube texture to draw.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(10), DefaultValue(null), EditorDisplay(\"Skybox\")")
        AssetRef<CubeTexture> CubeTexture;

        /// <summary>
        /// The skybox texture tint color.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(40), DefaultValue(typeof(Color), \"1,1,1,1\"), EditorDisplay(\"Skybox\")")
        Color Color = Colors::White;

        /// <summary>
        /// The skybox texture exposure value. Can be used to make skybox brighter or dimmer.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(50), DefaultValue(0.0f), Limit(-100, 100, 0.01f), EditorDisplay(\"Skybox\")")
        float Exposure = 0.0f;

    public:
        /// <summary>
        /// Directional light that is used to simulate the sun.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(10), DefaultValue(null), EditorDisplay(\"Sky\")")
        // ScriptingObjectReference<DirectionalLight> SunLight;

        /// <summary>
        /// The sun disc scale.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(20), DefaultValue(2.0f), EditorDisplay(\"Sky\"), Limit(0, 100, 0.01f)")
        float SunDiscScale = 2.0f;

        /// <summary>
        /// The sun power.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(30), DefaultValue(8.0f), EditorDisplay(\"Sky\"), Limit(0, 1000, 0.01f)")
        float SunPower = 8.0f;

    private:
#if COMPILE_WITH_DEV_ENV
        void OnShaderReloading(Asset* obj)
        {
            _psSky = nullptr;
            _psFog = nullptr;
        }
#endif
        // void InitConfig(AtmosphericFogData& config) const;

    public:
        // [Actor]
#if SE_EDITOR
        /*BoundingBox GetEditorBox() const override
        {
            const Float3 size(50);
            return BoundingBox(_transform.Translation - size, _transform.Translation + size);
        }*/
#endif

        void RenderDraw(RenderContext& renderContext) override;
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;

        bool HasContentLoaded() const;
        // bool IntersectsItself(const Ray& ray, float& distance, Float3& normal) override;

        // [IAtmosphericFogRenderer]
        // void DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output) override;

        // [ISkyRenderer]
        bool IsDynamicSky() const override;
        void ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world) override;

    protected:
        // [Actor]
        void EndPlay() override;
        void OnEnable() override;
        void OnDisable() override;
        void OnTransformChanged() override;
    };
} // SE
