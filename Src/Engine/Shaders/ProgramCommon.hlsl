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


enum ShadeTarget
{

	/// <summary>
	/// The features set defined by the core capabilities of OpenGL ES2.
	/// </summary>
	ES2 = 0,

	/// <summary>
	/// The features set defined by the core capabilities of OpenGL ES3.
	/// </summary>
	ES3 = 1,

	/// <summary>
	/// The features set defined by the core capabilities of OpenGL ES3.1.
	/// </summary>
	ES3_1 = 2,

	/// <summary>
	/// The features set defined by the core capabilities of DirectX 10 Shader Model 4.
	/// </summary>
	SM4 = 3,

	/// <summary>
	/// The features set defined by the core capabilities of DirectX 11 Shader Model 5.
	/// </summary>
	SM5 = 4,

	/// <summary>
	/// The features set defined by the core capabilities of DirectX 12 Shader Model 6.
	/// </summary>
	SM6 = 5,
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShadeTargetAttribute
{
	ShadeTarget target;
};


#define SHADER_VS(entry) SolarShaderStage("vertex", #entry)
#define SHADER_HS(entry) SolarShaderStage("hull", #entry)
#define SHADER_DS(entry) SolarShaderStage("domain", #entry)
#define SHADER_GS(entry) SolarShaderStage("geometry", #entry)
#define SHADER_PS(entry) SolarShaderStage("fragment", #entry)
#define SHADER_CS(entry) SolarShaderStage("compute", #entry)
#define SHADER_TARGET(target) SolarShadeTarget(target)
#define SHADER_MACRO(...) SolarShaderMacroGroup(#__VA_ARGS__)
#define SHADER_PROGRAM(name, ...) [SolarShaderProgram, __VA_ARGS__] struct name {};
