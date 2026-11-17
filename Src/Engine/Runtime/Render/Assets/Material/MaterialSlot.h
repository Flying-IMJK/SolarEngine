#pragma once

#include <Runtime/Core/Scripting/ScriptingObject.h>
#include <Runtime/Render/RenderEnum.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/MaterialBase.h>

namespace SE
{
    /// <summary>
    /// The material slot descriptor that specifies how to render geometry using it.
    /// </summary>
    SE_CLASS(API(NoSpawn))
    class SE_API_RUNTIME MaterialSlot : public ScriptingObject
    {
        SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(MaterialSlot, ScriptingObject)
    public:
        /// <summary>
        /// The material to use for rendering.
        /// </summary>
        SE_FIELD(API())
        AssetRef<MaterialBase> Material;

        /// <summary>
        /// The shadows casting mode by this visual element.
        /// </summary>
        SE_FIELD(API())
        ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

        /// <summary>
        /// The slot name.
        /// </summary>
        SE_FIELD(API())
        String Name;
    };
}
