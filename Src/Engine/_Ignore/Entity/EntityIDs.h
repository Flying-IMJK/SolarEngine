#pragma once
#include "Core/Tools/Hash.h"
#include "Core/Logging/Logging.h"
#include "Core/Types/SGUID.h"

//-------------------------------------------------------------------------

namespace SE
{
    // Serializable Entity Map ID
    //-------------------------------------------------------------------------

    using EntityMapID = SGUID;

	EntityMapID GenerateEntityMapID();

    //-------------------------------------------------------------------------

    struct EntityWorldID
    {
        static EntityWorldID Generate();

    public:

        EntityWorldID() = default;
        explicit EntityWorldID( uint64 v ) : m_value( v ) { ENGINE_ASSERT( v != 0 ); }

        inline bool IsValid() const { return m_value != 0; }
        inline void Clear() { m_value = 0; }
        inline bool operator==( EntityWorldID const& rhs ) const { return m_value == rhs.m_value; }
        inline bool operator!=( EntityWorldID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64 m_value = 0;
    };

    //-------------------------------------------------------------------------

    struct EntityID
    {
        static EntityID Generate();

    public:

        EntityID() = default;
        explicit EntityID( uint64 v ) : m_value( v ) { ENGINE_ASSERT( v != 0 ); }

        inline bool IsValid() const { return m_value != 0; }
        inline void Clear() { m_value = 0; }
        inline bool operator==( EntityID const& rhs ) const { return m_value == rhs.m_value; }
        inline bool operator!=( EntityID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64 m_value = 0;
    };

	inline uint32 GetHash(const EntityID & entityId)
	{
		return entityId.m_value;
	}

    //-------------------------------------------------------------------------

    struct ComponentID
    {
        static ComponentID Generate();

    public:

        ComponentID() = default;
        explicit ComponentID( uint64 v ) : m_value( v ) { ENGINE_ASSERT( v != 0 ); }

        inline bool IsValid() const { return m_value != 0; }
        inline void Clear() { m_value = 0; }
        inline bool operator==( ComponentID const& rhs ) const { return m_value == rhs.m_value; }
        inline bool operator!=( ComponentID const& rhs ) const { return m_value != rhs.m_value; }

    public:

        uint64 m_value = 0;
    };

	inline uint32 GetHash(const ComponentID & componentId)
	{
		return GetHash(componentId.m_value);
	}
}

//-------------------------------------------------------------------------

namespace std
{
    template <>
    struct hash<SE::EntityWorldID>
    {
        inline size_t operator()( SE::EntityWorldID const& t ) const { return t.m_value; }
    };

    template <>
    struct hash<SE::EntityID>
    {
        inline size_t operator()( SE::EntityID const& t ) const { return t.m_value; }
    };

    template <>
    struct hash<SE::ComponentID>
    {
        inline size_t operator()( SE::ComponentID const& t ) const { return t.m_value; }
    };
}