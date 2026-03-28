
#include "SceneTicking.h"

namespace SE
{
	SceneTicking::TickData::TickData(int32 capacity)
	: /*Scripts(capacity)
	, */Ticks(capacity)
	{
	}


	void SceneTicking::TickData::RemoveTick(void* callee)
	{
		for (int32 i = 0; i < Ticks.Count(); i++)
		{
			if (Ticks.Get()[i].Callee == callee)
			{
				Ticks.RemoveAt(i);
				break;
			}
		}
	}

	void SceneTicking::TickData::Tick()
	{
		// TickScripts(Scripts);

		for (int32 i = 0; i < Ticks.Count(); i++)
		{
			Ticks.Get()[i].Call();
		}
	}

#if SE_EDITOR
	void SceneTicking::TickData::RemoveTickExecuteInEditor(void* callee)
	{
		for (int32 i = 0; i < TicksExecuteInEditor.Count(); i++)
		{
			if (TicksExecuteInEditor.Get()[i].Callee == callee)
			{
				TicksExecuteInEditor.RemoveAt(i);
				break;
			}
		}
	}

	void SceneTicking::TickData::TickExecuteInEditor()
	{
		// TickScripts(ScriptsExecuteInEditor);

		for (int32 i = 0; i < TicksExecuteInEditor.Count(); i++)
			TicksExecuteInEditor.Get()[i].Call();
	}

#endif

	void SceneTicking::TickData::Clear()
	{
		// Scripts.Clear();
		Ticks.Clear();
#if SE_EDITOR
		// ScriptsExecuteInEditor.Clear();
		TicksExecuteInEditor.Clear();
#endif
	}


	SceneTicking::FixedUpdateTickData::FixedUpdateTickData() : TickData(512)
	{
	}


	SceneTicking::UpdateTickData::UpdateTickData() : TickData(1024)
	{
	}


	SceneTicking::LateUpdateTickData::LateUpdateTickData() : TickData(64)
	{
	}


	SceneTicking::LateFixedUpdateTickData::LateFixedUpdateTickData() : TickData(64)
	{
	}

} // SE