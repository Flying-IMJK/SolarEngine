#pragma once

#include "Core/Logging/Logging.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/String.h"
#include "TypeSystem/IType.h"

//-------------------------------------------------------------------------
// Defines an interface for an engine system
//-------------------------------------------------------------------------
//
// Engine systems are global (often singleton) systems that are not tied to a game world
// i.e. InputSystem, ResourceSystem, TypeRegistry...
//
// This interface exists primarily for type safety and ease of use
// Each system needs a publicly accessible uint32_t ID called 'SystemID'
// Use the macro provided below to declare new systems
//
//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_CORE ISystem
    {
		friend class Systems;

	protected:
		explicit ISystem(const Char* name, int32 order = 0);

	protected:
		virtual bool OnInit() { return true; }
		virtual void OnUpdate() {}
		virtual void OnLateUpdate() {}
		virtual void OnRender() {}
		virtual void OnDispose() {}
		virtual void OnBeforeExit() {}

    public:
        ISystem()          = delete;
		virtual ~ISystem() = default;
        ISystem(ISystem const &) = default;
		ISystem &operator=(ISystem const &) = default;

        virtual uint32 GetSystemID() const = 0;

		const Char* name;
		int32 order;
	private:
		bool IsInitialized = false;
    };

    //-------------------------------------------------------------------------
	class SE_API_CORE SystemRegister
	{
	public:

        virtual ISystem* Create() = 0;
		virtual uint32   GetID() const = 0;

		virtual ~SystemRegister() = default;
	};

	template <typename T, const Char* name>
	class TSystemRegister final : SystemRegister
	{
		friend class ISystem;
		static_assert(TIsBaseOf<ISystem, T>::Value, "T is not derived from ISystem");
	private:
		Function<ISystem*()> m_CreateCall;
	public:
	    TSystemRegister() : m_CreateCall()
		{
			m_CreateCall.Bind<TSystemRegister, &TSystemRegister::Create>(this);
			Systems::Register(name, this);
		}

		ISystem* Create() override
		{
			return New<T>();
		}

		uint32 GetID() const override
		{
			return T::s_systemID;
		}
	};

	class SE_API_CORE Systems
    {
    public:
		static void Initialize();
		static void ShutDown();
		static void Register(StringView name, SystemRegister* pRegister);
		static void Unregister(SystemRegister* pRegister);
		static void Sort();

		static void Update();
		static void LateUpdate();
		static void Render();

        template <typename T>
        static T* GetSystem()
        {
            static_assert(TIsBaseOf<ISystem, T>::Value, "T is not derived from ISystem");

			SystemInfo item;
			if (m_SystemCache.TryGet(T::s_systemID, item))
			{
				if (item.systemInstance)
				{
					return reinterpret_cast<T *>(item.systemInstance);
				}
			}

            ENGINE_UNREACHABLE_CODE();
            return nullptr;
        }

    private:
		struct SystemInfo
		{
			StringView name;
			SystemRegister* systemRegister;
			ISystem* systemInstance;
		};

        static List<ISystem*> m_Systems;
		static Dictionary<uint32, SystemInfo> m_SystemCache;
    };
}

//-------------------------------------------------------------------------
// 添加System
#define ENGINE_SYSTEM(TypeName)                                                   \
    public:                                                                       \
    friend Systems;                                                               \
    constexpr static uint32 const s_systemID = Hash::FNV1a::GetHash32(#TypeName); \
    virtual uint32 GetSystemID() const override final { return TypeName::s_systemID; }

// 注册系统
#define ENGINE_SYSTEM_REGISTER(TypeName) 									\
	const Char typeSystemName[] = SE_TEXT(#TypeName);						\
	::SE::TSystemRegister<TypeName, typeSystemName> TypeName##Register;

