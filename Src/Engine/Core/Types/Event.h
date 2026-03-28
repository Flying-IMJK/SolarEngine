#pragma once

#include "Core/Types/UID.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Collections/ListExtensions.h"

//-------------------------------------------------------------------------

namespace SE
{
    class EventBindingID
    {
        template <typename... Args>
        friend class TEvent;

    public:
        EventBindingID() = default;
        inline bool IsValid() const { return m_ID.IsValid(); }
        inline String ToString() const { return std::move(m_ID.ToString()); }

    private:
        EventBindingID(UID &newID) : m_ID(newID) {}
        EventBindingID(UID &&newID) : m_ID(newID) {}
        inline void Reset() { m_ID = UID::Empty; }

    private:
        UID m_ID;
    };

    //-------------------------------------------------------------------------

    template <typename... Args>
    class TEventHandle;

    template <typename... Args>
    class TEvent
    {
        friend TEventHandle<Args...>;

        struct BoundUser
        {
			BoundUser(): function()
			{
				Platform::CreateUUID(id);
			}

            BoundUser(Function<void(Args...)> &&function)
                : function(::MoveTemp(function))
            {
				Platform::CreateUUID(id);
            }

			BoundUser(Function<void(Args...)> function)
				: function(function)
			{
				Platform::CreateUUID(id);
			}

            void Reset()
            {
				id = UID::Empty;
				function = nullptr;
            }

        public:
            UID id = UID::Empty;
            Function<void(Args...)> function;
        };

    public:
        TEvent() = default;
        TEvent(TEvent const &) = default;
        ~TEvent()
        {
            if (HasBoundUsers())
            {
				LOG_FATAL("Event", "Event still has bound users at destruction");
            }
        }

        TEvent &operator=(TEvent const &rhs) = default;

        inline bool HasBoundUsers() const { return !m_boundUsers.IsEmpty(); }

        inline EventBindingID Bind(Function<void(Args...)> &&function)
        {
			TEvent<Args...>::BoundUser& boundUser = m_boundUsers.AddOne();
			boundUser.function = function;
            return EventBindingID(boundUser.id);
        }

		inline EventBindingID Bind(Function<void(Args...)> &function)
		{
			TEvent<Args...>::BoundUser& boundUser = m_boundUsers.AddOne();
			boundUser.function = function;
			return EventBindingID(boundUser.id);
		}

        inline void Unbind(EventBindingID bindingID)
        {
			Function<bool(const BoundUser&)> predicate = [&bindingID](const BoundUser &boundUser)
			{
			  return boundUser.id == bindingID.m_ID;
			};

            int foundIndex = ListExtensions::IndexOf(m_boundUsers, predicate);

            ENGINE_ASSERT(foundIndex != INVALID_INDEX);
            m_boundUsers.RemoveAt(foundIndex);
            bindingID.Reset();

            // Always free memory when we completely empty the array (this is need for statically created global events since the allocators are released before the events are destroyed)
            if (m_boundUsers.IsEmpty())
            {
                m_boundUsers.Clear();
            }
        }

        //-------------------------------------------------------------------------

        inline void Execute(Args ...args) const
        {
            for (auto &boundUser : m_boundUsers)
            {
                ENGINE_ASSERT(boundUser.function.IsBinded());
                boundUser.function(args...);
            }
        }

    private:
        // TODO: make this threadsafe!
        List<BoundUser> m_boundUsers;
    };

    //-------------------------------------------------------------------------

    // Safety Helper to return events from classes without the risk of the client copying them or firing them
    template <typename... Args>
    class TEventHandle
    {
    public:
        TEventHandle(TEvent<Args...> &event) : m_pEvent(&event) {}
        TEventHandle(TEvent<Args...> *event) : m_pEvent(event) {}

        [[nodiscard]] inline EventBindingID Bind(Function<void(Args...)> &&function)
        {
            return m_pEvent->Bind(MoveTemp(function));
        }

        inline void Unbind(EventBindingID &handle)
        {
            m_pEvent->Unbind(handle);
        }

    private:
        TEvent<Args...> *m_pEvent = nullptr;
    };
}