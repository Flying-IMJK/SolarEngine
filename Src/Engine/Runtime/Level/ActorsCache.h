#pragma once

#include "Actor.h"
#include "Core/Types/Collections/CollectionPoolCache.h"

namespace SE
{
	/// <summary>
	/// The actors collection lookup type (id -> actor).
	/// </summary>
	typedef Dictionary<UID, Actor*, HeapAllocation> ActorsLookup;

	/// <summary>
	/// Acceleration structure used to improve operations performed on a set of actors. Cache the data and allows to reuse memory container for less allocations. It's a thread-safe.
	/// </summary>
	class ActorsCache
	{
	public:
		typedef ActorsLookup ActorsLookupType;
		typedef List<Actor*> ActorsListType;
		typedef List<SceneObject*> SceneObjectsListType;

	public:
		/// <summary>
		/// Gets the actors lookup cached. Safe allocation, per thread, uses caching.
		/// </summary>
		static CollectionPoolCache<ActorsLookupType> ActorsLookupCache;

		/// <summary>
		/// Gets the actors lookup cached. Safe allocation, per thread, uses caching.
		/// </summary>
		static CollectionPoolCache<ActorsListType> ActorsListCache;

		/// <summary>
		/// Gets the scene objects lookup cached. Safe allocation, per thread, uses caching.
		/// </summary>
		static CollectionPoolCache<SceneObjectsListType> SceneObjectsListCache;
	};

} // SE
