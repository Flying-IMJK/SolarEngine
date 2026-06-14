
#include "TopologicalSort.h"

namespace SE::ReflectTool
{
	static bool VisitNode( List<TopologicalSorter::Node>& sortedList, TopologicalSorter::Node& node )
	{
		if ( node.m_mark == TopologicalSorter::Node::Mark::Temporary )
		{
			return false;
		}

		if ( node.m_mark == TopologicalSorter::Node::Mark::None )
		{
			node.m_mark = TopologicalSorter::Node::Mark::Temporary;
			for ( auto pChild : node.m_children )
			{
				VisitNode( sortedList, *pChild );
			}
			node.m_mark = TopologicalSorter::Node::Mark::Permanent;
			sortedList.Add( node );
		}

		return true;
	}

	bool TopologicalSorter::Sort( List<Node>& list )
	{
		List<Node> sortedList;
		auto const numNodes = list.Count();

		for ( auto i = 0; i < numNodes; i++ )
		{
			if ( list[i].m_mark == Node::Mark::None )
			{
				if ( !VisitNode( sortedList, list[i] ) )
				{
					return false; // list is not a DAG
				}

				i = INVALID_INDEX;
			}
		}

		list.Swap( sortedList );
		return true;
	}
} // SE