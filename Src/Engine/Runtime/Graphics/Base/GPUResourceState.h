#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/Collections/List.h"

namespace SE
{
	/// <summary>
	/// Tracking of per-resource or per-subresource state for GPU resources that require to issue resource access barriers during rendering.
	/// </summary>
	template<typename StateType, StateType InvalidState>
	class GPUResourceState
	{
	private:
		/// <summary>
		/// The whole resource state (used only if _allSubresourcesSame is true).
		/// </summary>
		StateType _resourceState;

		/// <summary>
		/// Set to 1 if _resourceState is valid. In this case, all subresources have the same state.
		/// Set to 0 if _subresourceState is valid. In this case, each subresources may have a different state (or may be unknown).
		/// </summary>
		bool _allSubresourcesSame;

		/// <summary>
		/// The per subresource state (used only if _allSubresourcesSame is 0).
		/// </summary>
		List<StateType> _subresourceState;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUResourceState"/> class.
		/// </summary>
		GPUResourceState()
			: _resourceState(InvalidState)
			, _allSubresourcesSame(true),
			_subresourceState(0)
		{
		}

	public:
		void Initialize(uint32 subresourceCount, StateType initialState, bool usePerSubresourceTracking)
		{
			ENGINE_ASSERT(_subresourceState.IsEmpty() && subresourceCount > 0);
			_allSubresourcesSame = true;
			_resourceState = initialState;
			if (usePerSubresourceTracking && subresourceCount > 1)
			{
				_subresourceState.Resize(subresourceCount, false);
			}
#ifdef SE_DEVELOPMENT
			_subresourceState.SetAll(InvalidState);
#endif
		}

		bool IsInitializated() const
		{
			return _resourceState != InvalidState || _subresourceState.HasItems();
		}

		void Release()
		{
			_resourceState = InvalidState;
			_subresourceState.Resize(0);
		}

		bool AreAllSubresourcesSame() const
		{
			return _allSubresourcesSame;
		}

		int32 GetSubresourcesCount() const
		{
			return _subresourceState.Count();
		}

		bool CheckResourceState(StateType state) const
		{
			if (_allSubresourcesSame)
				return state == _resourceState;
			for (int32 i = 0; i < _subresourceState.Count(); i++)
			{
				if (_subresourceState[i] != state)
					return false;
			}
			return true;
		}

		StateType GetSubresourceState(uint32 subresourceIndex) const
		{
			if (_allSubresourcesSame)
				return _resourceState;
			ENGINE_ASSERT(subresourceIndex >= 0 && subresourceIndex < static_cast<uint32>(_subresourceState.Count()));
			return _subresourceState[subresourceIndex];
		}

		void SetResourceState(StateType state)
		{
			_allSubresourcesSame = true;
			_resourceState = state;
#ifdef SE_DEVELOPMENT
			for (int32 i = 0; i < _subresourceState.Count(); i++)
            	_subresourceState[i] = InvalidState;
#endif
		}

		void SetSubresourceState(int32 subresourceIndex, StateType state)
		{
			// Check if use single state for the whole resource
			if (subresourceIndex == -1 || _subresourceState.Count() <= 1)
			{
				SetResourceState(state);
			}
			else
			{
				ENGINE_ASSERT(subresourceIndex < static_cast<int32>(_subresourceState.Count()));

				// Transition for all sub-resources
				if (_allSubresourcesSame)
				{
					for (int32 i = 0; i < _subresourceState.Count(); i++)
						_subresourceState[i] = _resourceState;
					_allSubresourcesSame = false;
#ifdef SE_DEVELOPMENT
					_resourceState = InvalidState;
#endif
				}

				_subresourceState[subresourceIndex] = state;
			}
		}
	};
}
