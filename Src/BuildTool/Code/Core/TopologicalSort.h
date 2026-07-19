#pragma once
#include "String.h"

namespace SE::BuildTool
{
	class TopologicalSorter
	{
		public:

		// This is a utility class to enable sorting of types in a topological manner
		struct Node
		{
			enum class Mark
			{
				None,
				Permanent,
				Temporary,
			};

			Node() : m_ID( -1 ), m_mark( Mark::None ) {}
			Node( int32 ID ) : m_ID( ID ), m_mark( Mark::None ) {}

			int32             m_ID;           // Some way to identify the actual data this refers to
			std::vector<Node*>      m_children;     // The children of this node
			Mark                m_mark;         // Marking mechanism for algorithm
		};

		//-------------------------------------------------------------------------

		static bool Sort( std::vector<Node>& list );
	};

} // SE