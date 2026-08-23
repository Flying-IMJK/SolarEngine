#pragma once

#include "Runtime/API.h"
#include "Runtime/Entity/EntitySpatialComponent.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
namespace SE
{
    class SE_API_RUNTIME LocalEnvironmentMapComponent : public SpatialEntityComponent
    {
		SE_CLASS(LocalEnvironmentMapComponent, SpatialEntityComponent);
//        ENGINE_ENTITY_COMPONENT( LocalEnvironmentMapComponent );

    public:

        inline GPUTexture const* GetEnvironmentMapTexture() const { return m_environmentMapTexture/*.GetPtr()*/; }

    private:

//		SE_PROPERTY()
//		TResPtr<GPUTexture>
		GPUTexture* m_environmentMapTexture;
    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME GlobalEnvironmentMapComponent : public EntityComponent
    {
		SE_CLASS(GlobalEnvironmentMapComponent, EntityComponent);
//		ENGINE_SINGLETON_ENTITY_COMPONENT( GlobalEnvironmentMapComponent );

    public:

        inline bool HasSkyboxTexture() const { return false;/* m_skyboxTexture.IsLoaded();*/ }
        inline GPUTexture const* GetSkyboxTexture() const { return nullptr;/* m_skyboxTexture.GetPtr();*/ }

        inline bool HasSkyboxRadianceTexture() const { return false; /*m_skyboxRadianceTexture.IsLoaded();*/ }
        inline GPUTexture const* GetSkyboxRadianceTexture() const { return nullptr; /*m_skyboxRadianceTexture.GetPtr();*/ }

        inline float GetSkyboxIntensity() const { return m_skyboxIntensity; }

        //TODO: lighting hack
        inline float GetExposure() const { return m_exposure; }

    private:

//		SE_PROPERTY()
//		TResPtr<CubemapTexture>
		GPUTexture* m_skyboxTexture;
//		SE_PROPERTY()
//		TResPtr<CubemapTexturere>
		GPUTexture* m_skyboxRadianceTexture;
		SE_PROPERTY()
		float m_skyboxIntensity = 1.0;
		SE_PROPERTY()
		float m_exposure = -1.0;
    };
}