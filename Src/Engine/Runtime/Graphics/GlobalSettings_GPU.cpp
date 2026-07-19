#include "GlobalSettings_GPU.h"
#include "Runtime/Core/IniFile.h"

//-------------------------------------------------------------------------

namespace SE
{
	GPUGlobalSettings::GPUGlobalSettings() : GlobalSettings(), type(), shaderProfile(), validationMode(), gpuPreference()
	{
	}

    bool GPUGlobalSettings::LoadSettings(IniFile const& ini)
    {
		type = (GPURendererType)ini.GetIntOrDefault( "GPU:Type", (int)GPURendererType::Vulkan);
		validationMode = (RHIValidationMode)ini.GetIntOrDefault( "GPU:ValidationMode", (int)RHIValidationMode::Disabled);
		gpuPreference = (RHIGPUPreference)ini.GetIntOrDefault( "GPU:GPUPreference", (int)RHIGPUPreference::Discrete);

        return true;
    }

    bool GPUGlobalSettings::SaveSettings( IniFile& ini ) const
    {
        ini.CreateSection("GPU");
        ini.SetInt( "GPU:Type", (int)type);
        ini.SetInt( "GPU:ValidationMode", (int)validationMode);
        ini.SetInt( "GPU:GPUPreference", (int)gpuPreference);
        return true;
    }
}