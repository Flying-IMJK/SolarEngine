#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	struct SE_API_RUNTIME ShaderVariantGroup
	{
		List<String> Members;
		String DefaultMember;
	};

	struct SE_API_RUNTIME ShaderVariantPlan
	{
		List<String> Defines;
		String Variant;
	};

	class SE_API_RUNTIME ShaderVariantPlanner
	{
	public:
        static bool BuildAll(const List<ShaderVariantGroup>& groups, List<ShaderVariantPlan>& variants, String& error);
		static bool Normalize(const List<ShaderVariantGroup>& groups, const List<String>& requestedDefines, ShaderVariantPlan& variant, String& error);

	private:
		static bool ContainsDefine(const List<String>& defines, const String& value);
		static void SortDefines(List<String>& defines);
		static String SerializeDefines(const List<String>& defines);
		static void BuildCartesian(const List<ShaderVariantGroup>& groups, int32 groupIndex, List<String>& currentDefines, List<ShaderVariantPlan>& variants);
	};
}
