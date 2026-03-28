#pragma once
#include "Core/Types/Variable.h"

namespace SE
{
	#define INPUT_LAYOUT_ELEMENT_ALIGN 0xffffffff
	#define INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA 0
	#define INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA 1

	/// <summary>
	/// 顶点着色程序的最大输入元素量
	/// </summary>
	#define VERTEX_SHADER_MAX_INPUT_ELEMENTS 16

	/// <summary>
	/// Maximum allowed amount of shader program permutations
	/// </summary>
	#define SHADER_PERMUTATIONS_MAX_COUNT 16

	/// <summary>
	/// Maximum allowed amount of parameters for permutation
	/// </summary>
	#define SHADER_PERMUTATIONS_MAX_PARAMS_COUNT 4

	/// <summary>
	/// 绑定到管线的常量缓冲区的最大允许数量(最大槽索引为MAX_CONSTANT_BUFFER_SLOTS-1)
	/// </summary>
	#define MAX_CONSTANT_BUFFER_SLOTS 4

	/**
	 * GPU 程序 阶段
	 */
	enum class ShaderStage : int32
	{
		Vertex,
		Hull,
		Domain,
		Geometry,
		Mesh,
		Pixel,
		Compute,
		Max
	};

	/// <summary>
	/// Shader macro definition structure
	/// </summary>
	struct ShaderMacro
	{
		/// <summary>
		/// Macro name
		/// </summary>
		const char* Name;

		/// <summary>
		/// Macro value
		/// </summary>
		const char* Definition;
	};
}
