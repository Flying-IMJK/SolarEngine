[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderProgramAttribute {};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderStageAttribute
{
	string stage;
	string entry;
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderMacroGroupAttribute
{
	string members;
};

/*
#define SHADER_VS(entry) \
[SolarShaderStage("vertex", #entry)]
#define SHADER_HS(entry) \
[SolarShaderStage("hull", #entry)]
#define SHADER_DS(entry) \
[SolarShaderStage("domain", #entry)]
#define SHADER_GS(entry) \
[SolarShaderStage("geometry", #entry)]
#define SHADER_PS(entry) \
[SolarShaderStage("fragment", #entry)]
#define SHADER_CS(entry) \
[SolarShaderStage("compute", #entry)]
#define SHADER_MACRO(...) \
[SolarShaderMacroGroup(#__VA_ARGS__)]

#define SHADER_PROGRAM(name, ...)   \
    [SolarShaderProgram]            \
    __VA_ARGS__						\
	struct name {};
*/

#define SHADER_VS(entry) SolarShaderStage("vertex", #entry)
#define SHADER_HS(entry) SolarShaderStage("hull", #entry)
#define SHADER_DS(entry) SolarShaderStage("domain", #entry)
#define SHADER_GS(entry) SolarShaderStage("geometry", #entry)
#define SHADER_PS(entry) SolarShaderStage("fragment", #entry)
#define SHADER_CS(entry) SolarShaderStage("compute", #entry)
#define SHADER_MACRO(...) SolarShaderMacroGroup(#__VA_ARGS__)

#define SHADER_PROGRAM(name, ...) [SolarShaderProgram, __VA_ARGS__] struct name {};