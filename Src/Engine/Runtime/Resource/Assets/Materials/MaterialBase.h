#pragma once

#include "Runtime/Render/Assets/Material/IMaterial.h"
#include "Runtime/Render/Assets/Material/MaterialParams.h"

#include "Runtime//Resource/BinaryAsset.h"

namespace SE
{
    class MaterialInstance;

    SE_CLASS(Reflect, API, NoSpawn, Abstract)
    class SE_API_RUNTIME MaterialBase : public BinaryAsset, public IMaterial
    {
        SE_DEFINE_CLASS_DEFAULT(MaterialBase, BinaryAsset);
        ASSET_HEADER(MaterialBase);
    public:
        /// <summary>
        /// The material parameters collection.
        /// </summary>
        MaterialParams Params;

        /// <summary>
        /// Event called when parameters collections gets modified.
        /// </summary>
        Action ParamsChanged;

        /// <summary>
        /// Returns true if material is an material instance.
        /// </summary>
        virtual bool IsMaterialInstance() const = 0;

    public:
        /// <summary>
        /// Gets the material parameters collection.
        /// </summary>
        const List<MaterialParameter>& GetParameters() const
        {
            return Params;
        }

        /// <summary>
        /// Gets the material info, structure which describes material surface.
        /// </summary>
        FORCE_INLINE const MaterialInfo& Info() const
        {
            return GetInfo();
        }

        /// <summary>
        /// Gets the material parameter.
        /// </summary>
        FORCE_INLINE MaterialParameter* GetParameter(const StringView& name)
        {
            return Params.Get(name);
        }

        /// <summary>
        /// Gets the material parameter value.
        /// </summary>
        /// <returns>The parameter value.</returns>
        Variant GetParameterValue(const StringView& name);

        /// <summary>
        /// Sets the material parameter value (and sets IsOverride to true).
        /// </summary>
        /// <param name="name">The parameter name.</param>
        /// <param name="value">The value to set.</param>
        /// <param name="warnIfMissing">True if warn if parameter is missing, otherwise will do nothing.</param>
        void SetParameterValue(const StringView& name, const Variant& value, bool warnIfMissing = true);

        /// <summary>
        /// Creates the virtual material instance of this material which allows to override any material parameters.
        /// </summary>
        /// <returns>The created virtual material instance asset.</returns>
        MaterialInstance* CreateVirtualInstance();

        uint32 GetSerializedVersion() const override;

    public:
        // [BinaryAsset]
#if SE_EDITOR
        void GetReferences(List<UID>& output) const override
        {
            // Base
            BinaryAsset::GetReferences(output);

            Params.GetReferences(output);
        }
#endif
    };
} // SE
