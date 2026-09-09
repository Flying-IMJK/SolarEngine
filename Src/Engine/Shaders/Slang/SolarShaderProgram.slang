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

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderVertexInputBindingAttribute
{
	int slot;
	int stride;
	string inputRate;
	int instanceStepRate;
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderVertexInputElementAttribute
{
	string semantic;
	int semanticIndex;
	string format;
	int slot;
	string offset;
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
#define SHADER_VS_INPUT_BINDING(slot, stride, inputRate, instanceStepRate) \
	SolarShaderVertexInputBinding(slot, stride, #inputRate, instanceStepRate)
#define SHADER_VS_INPUT_ELEMENT(semantic, semanticIndex, format, slot, offset) \
	SolarShaderVertexInputElement(#semantic, semanticIndex, #format, slot, #offset)

#define SHADER_PROGRAM(name, ...) [SolarShaderProgram, __VA_ARGS__] struct name {};
