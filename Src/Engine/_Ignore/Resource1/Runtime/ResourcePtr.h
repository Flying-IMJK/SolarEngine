#pragma once

#include "ResourceRecord.h"
#include "Core/TypeSystem/Types.h"

//-------------------------------------------------------------------------

namespace SGE
{
    //-------------------------------------------------------------------------
    // Generic Resource Ptr
    //-------------------------------------------------------------------------
    // There is no direct access to runtime resources through generic resource ptr
    // You should generally try to avoid using generic resource ptrs
	class SE_API_RUNTIME ResPtr : public IReflectedType
    {
		SE_CLASS(ResPtr, IReflectedType)

        friend ResRecord;
        friend class ResourceSystem;
    public:
        ResPtr() = default;
        ResPtr(nullptr_t){};
        ResPtr(ResID ID) : m_resourceID(ID) {}
        ResPtr(ResPtr const &rhs) { operator=(rhs); }
        ResPtr(ResPtr &&rhs) { operator=(MoveTemp(rhs)); }

        // Is the resource ID set for this ptr - doesnt signify if the resource is loaded
        inline bool IsSet() const { return m_resourceID.IsValid(); }

        inline ResID const &GetResourceID() const { return m_resourceID; }

        inline void Clear()
        {
			// Only allowed to clear unloaded resource ptrs
            ENGINE_ASSERT(m_pResourceRecord == nullptr);
            m_resourceID.Clear();
        }

        template <typename T>
        inline T *GetPtr() { return (T *)m_pResourceRecord->GetResourceData(); }

        inline bool operator==(nullptr_t) { return m_pResourceRecord == nullptr; }
        inline bool operator!=(nullptr_t) { return m_pResourceRecord != nullptr; }

        inline bool operator==(ResPtr const &rhs) const { return m_resourceID == rhs.m_resourceID; }

        inline bool operator!=(ResPtr const &rhs) const { return m_resourceID != rhs.m_resourceID; }

        virtual inline ResPtr &operator=(ResPtr const &rhs)
        {
            // Can't change a loaded resource, unload it first
            ENGINE_ASSERT(m_pResourceRecord == nullptr || m_pResourceRecord->IsUnloaded());
            m_resourceID = rhs.m_resourceID;
            m_pResourceRecord = rhs.m_pResourceRecord;
            return *this;
        }

        inline ResPtr &operator=(ResPtr &&rhs)
        {
            m_resourceID = rhs.m_resourceID;
            m_pResourceRecord = rhs.m_pResourceRecord;
            rhs.m_resourceID = ResID();
            rhs.m_pResourceRecord = nullptr;
            return *this;
        }

        inline List<ResID> const &GetInstallDependencies() const
        {
            ENGINE_ASSERT(m_pResourceRecord != nullptr && (m_pResourceRecord->IsLoaded() || m_pResourceRecord->IsLoading() || m_pResourceRecord->HasLoadingFailed()));
            return m_pResourceRecord->GetDependencies();
        }

        // Load status
        //-------------------------------------------------------------------------
        // Note: loading status is a frame delayed, so if you want to know if a resource has been requested (and still to be processed) used the "WasRequested" function

        inline bool WasRequested() const { return m_pResourceRecord != nullptr; }

        inline LoadingStatus GetLoadingStatus() const { return (m_pResourceRecord != nullptr) ? m_pResourceRecord->GetLoadingStatus() : LoadingStatus::Unloaded; }
        inline bool IsLoading() const { return GetLoadingStatus() == LoadingStatus::Loading; }
        inline bool IsLoaded() const { return GetLoadingStatus() == LoadingStatus::Loaded; }
        inline bool IsUnloading() const { return GetLoadingStatus() == LoadingStatus::Unloading; }
        inline bool IsUnloaded() const { return GetLoadingStatus() == LoadingStatus::Unloaded; }
        inline bool HasLoadingFailed() const { return GetLoadingStatus() == LoadingStatus::Failed; }

	protected:
        ResRecord const *m_pResourceRecord = nullptr;
		ResID m_resourceID;
	};

    template <typename T>
    class TResPtr : public ResPtr
    {
        static_assert(std::is_base_of<IResource, T>::value, "Invalid specialization for TResourcePtr, only classes derived from IResource are allowed.");
    public:
        TResPtr() : ResPtr() {}
        TResPtr(nullptr_t) : ResPtr(nullptr) {}
        TResPtr(ResID ID) : ResPtr(ID) { ENGINE_ASSERT(!ID.IsValid() || ID.GetTypeID() == Typeof<T>()); }
        TResPtr(ResPtr const &otherResourcePtr) { operator=(otherResourcePtr); }

        // Move ctor
        TResPtr(ResPtr &&otherResourcePtr)
        {
            operator=(otherResourcePtr);
            otherResourcePtr = ResPtr();
        }

        inline bool operator==(nullptr_t) const { return m_pResourceRecord == nullptr; }
        inline bool operator!=(nullptr_t) const { return m_pResourceRecord != nullptr; }
//        inline bool operator==(ResPtr const &rhs) const { return m_resourceID == rhs.m_resourceID; }
//        inline bool operator!=(ResPtr const &rhs) const { return m_resourceID != rhs.m_resourceID; }
        inline bool operator==(TResPtr const &rhs) const { return m_resourceID == rhs.m_resourceID; }
        inline bool operator!=(TResPtr const &rhs) const { return m_resourceID != rhs.m_resourceID; }

        inline T const *operator->() const
        {
            ENGINE_ASSERT(m_pResourceRecord != nullptr);
            return reinterpret_cast<T const *>(m_pResourceRecord->GetResourceData());
        }
        inline T const *GetPtr() const
        {
            ENGINE_ASSERT(m_pResourceRecord != nullptr);
            return reinterpret_cast<T const *>(m_pResourceRecord->GetResourceData());
        }

        inline TypeID GetSpecializedResourceTypeID() const { return Typeof<T>(); }

        inline TResPtr<T> &operator=(ResPtr const &rhs)
        {
            // Can't change a loaded resource, unload it first
            ENGINE_ASSERT(m_pResourceRecord == nullptr || m_pResourceRecord->IsUnloaded());

            if (rhs.IsSet())
            {
                if (rhs.GetResourceID().GetTypeID() == Typeof<T>())
                {
                    ResPtr::operator=(rhs);
                }
                else // Invalid Assignment
                {
                    ENGINE_HALT();
                }
            }
            else
            {
                m_resourceID.Clear();
            }

            return *this;
        }

        template <typename U>
        inline bool operator==(const TResPtr<U> &rhs) { return m_resourceID == rhs.m_resourceID; }

        template <typename U>
        inline bool operator!=(const TResPtr<U> &rhs) { return m_resourceID != rhs.m_resourceID; }

        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        inline String const &GetResourceCompilationLog() const
        {
            ENGINE_ASSERT(IsSet() && WasRequested() && m_pResourceRecord != nullptr);
            return m_pResourceRecord->GetCompilationLog();
        }
        #endif
    };
}