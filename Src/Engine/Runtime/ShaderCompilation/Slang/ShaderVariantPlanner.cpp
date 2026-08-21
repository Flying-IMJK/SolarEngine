#include "ShaderVariantPlanner.h"

namespace SE
{
	bool ShaderVariantPlanner::ContainsDefine(const List<String>& defines, const String& value)
	{
		for (int32 i = 0; i < defines.Count(); i++)
		{
			if (defines[i] == value)
			{
				return true;
			}
		}
		return false;
	}

	void ShaderVariantPlanner::SortDefines(List<String>& defines)
	{
		for (int32 i = 0; i < defines.Count(); i++)
		{
			for (int32 j = i + 1; j < defines.Count(); j++)
			{
				if (defines[j] < defines[i])
				{
					String tmp = defines[i];
					defines[i] = defines[j];
					defines[j] = tmp;
				}
			}
		}
	}

	String ShaderVariantPlanner::SerializeDefines(const List<String>& defines)
	{
		String result;
		for (int32 i = 0; i < defines.Count(); i++)
		{
			if (i != 0)
			{
				result += SE_TEXT(";");
			}
			result += defines[i];
		}
		return result;
	}

	void ShaderVariantPlanner::BuildCartesian(const List<ShaderVariantGroup>& groups, const int32 groupIndex, List<String>& currentDefines, List<ShaderVariantPlan>& variants)
	{
		if (groupIndex == groups.Count())
		{
			ShaderVariantPlan plan;
			plan.Defines = currentDefines;
			SortDefines(plan.Defines);
			plan.Variant = SerializeDefines(plan.Defines);
			variants.Add(plan);
			return;
		}

		const ShaderVariantGroup& group = groups[groupIndex];
		for (int32 i = 0; i < group.Members.Count(); i++)
		{
			const String& member = group.Members[i];
			const bool emptyChoice = member == SE_TEXT("_");
			if (!emptyChoice)
			{
				currentDefines.Add(member);
			}
			BuildCartesian(groups, groupIndex + 1, currentDefines, variants);
			if (!emptyChoice)
			{
				currentDefines.Resize(currentDefines.Count() - 1);
			}
		}
	}

	bool ShaderVariantPlanner::BuildAll(const List<ShaderVariantGroup>& groups, List<ShaderVariantPlan>& variants, String& error)
	{
		variants.Clear();
		for (int32 i = 0; i < groups.Count(); i++)
		{
			if (groups[i].Members.Count() == 0)
			{
				error = SE_TEXT("Shader variant macro group is empty.");
				return false;
			}
		}

		List<String> currentDefines;
		BuildCartesian(groups, 0, currentDefines, variants);
		if (variants.Count() == 0)
		{
			ShaderVariantPlan empty;
			empty.Variant = String::Empty;
			variants.Add(empty);
		}
		return true;
	}

	bool ShaderVariantPlanner::Normalize(const List<ShaderVariantGroup>& groups, const List<String>& requestedDefines, ShaderVariantPlan& variant, String& error)
	{
		List<String> normalized;

		for (int32 defineIndex = 0; defineIndex < requestedDefines.Count(); defineIndex++)
		{
			bool known = false;
			for (int32 groupIndex = 0; groupIndex < groups.Count(); groupIndex++)
			{
				if (ContainsDefine(groups[groupIndex].Members, requestedDefines[defineIndex]))
				{
					known = true;
					break;
				}
			}
			if (!known)
			{
				error = SE_TEXT("Unknown shader variant macro: ") + requestedDefines[defineIndex];
				return false;
			}
		}

		for (int32 groupIndex = 0; groupIndex < groups.Count(); groupIndex++)
		{
			const ShaderVariantGroup& group = groups[groupIndex];
			int32 selectedIndex = -1;
			for (int32 memberIndex = 0; memberIndex < group.Members.Count(); memberIndex++)
			{
				const String& member = group.Members[memberIndex];
				if (member != SE_TEXT("_") && ContainsDefine(requestedDefines, member))
				{
					if (selectedIndex != -1)
					{
						error = SE_TEXT("Multiple macros selected from the same shader variant group.");
						return false;
					}
					selectedIndex = memberIndex;
				}
			}

			if (selectedIndex == -1)
			{
				selectedIndex = 0;
			}

			const String& selected = group.Members[selectedIndex];
			if (selected != SE_TEXT("_"))
			{
				normalized.Add(selected);
			}
		}

		SortDefines(normalized);
		variant.Defines = normalized;
		variant.Variant = SerializeDefines(normalized);
		return true;
	}
}
