#pragma once
#include "Core/Types/Collections/List.h"
#include "Core/Types/Collections/Dictionary.h"

//-------------------------------------------------------------------------
// ID Vector
//-------------------------------------------------------------------------
// Utility container for when we want to iterate over a contiguous set of items but also have fast lookups
// Expects contained types to provide a GetID() function that returns IDType
// e.g 
//      struct Foo
//      {
//          uint32 GetID() const;
//      };
//      
//      TIDVector<uint32, Foo> m_foos;

namespace SE
{
    template<typename IDType, typename ItemType>
    class TIDVector
    {
    public:

        // Flat array access
        //-------------------------------------------------------------------------

        List<ItemType> const& GetList() const { return m_ItemList; }
        ItemType& operator[]( int32 idx ) { return m_ItemList[idx]; }
        int32 size() const { return (int32) m_ItemList.Count(); }
        bool empty() const { return m_ItemList.IsEmpty(); }

        typename List<ItemType>::Iterator begin() { return m_ItemList.begin(); }
        typename List<ItemType>::Iterator end() { return m_ItemList.end(); }

        typename List<ItemType>::Iterator begin() const { return m_ItemList.begin(); }
        typename List<ItemType>::Iterator end() const { return m_ItemList.end(); }

        // ID management Insertion / Deletion / Search
        //-------------------------------------------------------------------------

        // Check if we have an item for a given ID
        bool HasItemForID( IDType const& ID ) const
        {
            return m_indexMap.ContainsKey( ID );
        }

        // Add a new item
        ItemType* Add( ItemType const& item )
        {
            IDType const ID = GetItemID( std::is_pointer<ItemType>(), item );
            ENGINE_ASSERT( !m_indexMap.ContainsKey( ID ));
            int32 const itemIdx = m_ItemList.Count();
            m_ItemList.Add( item );
            m_indexMap.Add(ID, itemIdx);
            return &m_ItemList[itemIdx];
        }

        // Add a new item - Expects the item to not already have an entry and will crash otherwise
        ItemType* Add( ItemType const&& item )
        {
            IDType const ID = GetItemID( std::is_pointer<ItemType>(), item );
            ENGINE_ASSERT( !m_indexMap.ContainsKey( ID ));
            int32 const itemIdx = m_ItemList.Count();
            m_ItemList.Add( ::Forward<ItemType const>( item ) );
            m_indexMap.Add( ID, itemIdx );
            return &m_ItemList[itemIdx];
        }

        // Emplace a new item - Expects the item to not already have an entry and will crash otherwise
        template<class... Args>
        ItemType* Emplace( IDType const& ID, Args&&... args )
        {
            ENGINE_ASSERT( !m_indexMap.ContainsKey( ID ));
            int32 const itemIdx = m_ItemList.Count();
            m_ItemList.Add(::Forward<Args>( args )... );
            ENGINE_ASSERT( GetLastElementID( std::is_pointer<ItemType>() ) == ID );
            m_indexMap.Add( ID, itemIdx);
            return &m_ItemList[itemIdx];
        }

        // Removes an item - Expect the item to exist and will crash otherwise!
        void Remove( IDType const& ID )
        {
            auto foundIter = m_indexMap.Find( ID );
            ENGINE_ASSERT( foundIter != m_indexMap.end() );

            // Update the index for the last element if that exists
            if ( m_ItemList.Count() > 1 )
            {
                IDType lastItemID = GetLastElementID( std::is_pointer<ItemType>() );
                auto foundLastItemIter = m_indexMap.Find( lastItemID );
                foundLastItemIter->Value = foundIter->Value;
            }

            // Erase by swapping with the last element and popping
            m_ItemList.RemoveAt( foundIter->Value );
            m_indexMap.Remove(foundIter);
        }

        // Try to find an item matching the ID, returns nullptr if no item for this ID exists
        ItemType* FindItem( IDType const& ID )
        {
            ItemType* pFound = nullptr;

            auto foundIter = m_indexMap.Find( ID );
            if ( foundIter != m_indexMap.end() )
            {
                pFound = &m_ItemList[foundIter->Value];
            }

            return pFound;
        }

        // Try to find an item matching the ID, if we dont find one create a default item and return a ptr to it
        // This function will emplace a new type if it doesnt exist using the supplied ctor arguments
        template<class... Args>
        ItemType* FindOrAdd( IDType const& ID, Args&&... args )
        {
            auto pFoundItem = FindItem( ID );
            if ( pFoundItem == nullptr )
            {
                pFoundItem = Emplace( ID, ::Forward<Args>( args )... );
            }

            return pFoundItem;
        }

        // Returns the item for a specified ID, expect the item to exists! Will crash if the item does not!
        ItemType* Get( IDType const& ID )
        {
            auto pFoundItem = FindItem( ID );
            ENGINE_ASSERT( pFoundItem != nullptr );
            return pFoundItem;
        }

    private:

        inline IDType GetItemID( std::true_type, ItemType const& pItem ) const
        {
            return pItem->GetID();
        }

        inline IDType GetItemID( std::false_type, ItemType const& item ) const
        {
            return item.GetID();
        }

        inline IDType GetLastElementID( std::true_type ) const
        {
            ENGINE_ASSERT( !m_ItemList.IsEmpty() );
            return m_ItemList.Last()->GetID();
        }

        inline IDType GetLastElementID( std::false_type ) const
        {
            ENGINE_ASSERT( !m_ItemList.IsEmpty() );
            return m_ItemList.Last().GetID();
        }

    private:

        List<ItemType>                 m_ItemList;
        Dictionary<IDType, int32>      m_indexMap; // A mapping between the ID type and the item index in the flat array
    };
}