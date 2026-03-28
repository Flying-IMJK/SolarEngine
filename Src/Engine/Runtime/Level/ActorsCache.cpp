
#include "ActorsCache.h"

namespace SE
{
	CollectionPoolCache<ActorsCache::ActorsLookupType> ActorsCache::ActorsLookupCache;
	CollectionPoolCache<ActorsCache::ActorsListType> ActorsCache::ActorsListCache;
	CollectionPoolCache<ActorsCache::SceneObjectsListType> ActorsCache::SceneObjectsListCache;
} // SE