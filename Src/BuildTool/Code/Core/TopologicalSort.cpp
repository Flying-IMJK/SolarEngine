
#include "TopologicalSort.h"

namespace SE::BuildTool
{
	static bool VisitNode( std::vector<TopologicalSorter::Node>& sortedList, TopologicalSorter::Node& node )
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
			sortedList.push_back( node );
		}

		return true;
	}

	bool TopologicalSorter::Sort( std::vector<Node>& list )
	{
		std::vector<Node> sortedList;
		auto const numNodes = list.size();

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

		list.swap( sortedList );
		return true;
	}
} // SE