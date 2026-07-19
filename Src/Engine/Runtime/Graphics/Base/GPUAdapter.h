#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/API.h"

namespace SE
{
	// GPU vendors IDs
	#define GPU_VENDOR_ID_AMD 0x1002
	#define GPU_VENDOR_ID_INTEL 0x8086
	#define GPU_VENDOR_ID_NVIDIA 0x10DE
	#define GPU_VENDOR_ID_MICROSOFT 0x1414
	#define GPU_VENDOR_ID_APPLE 0x106B


	class SE_API_RUNTIME GPUAdapter
	{
	public:
		GPUAdapter(const GPUAdapter& other)
		{
			*this = other;
		}

		GPUAdapter()
		{
		}

		GPUAdapter& operator=(const GPUAdapter& other)
		{
			return *this;
		}

	public:
		/// <summary>
		/// Checks if adapter is valid and returns true if it is.
		/// </summary>
		/// <returns>True if valid, otherwise false.</returns>
		virtual bool IsValid() const = 0;

		/// <summary>
		/// Gets the native pointer to the underlying graphics device adapter. It's a low-level platform-specific handle.
		/// </summary>
		virtual void* GetNativePtr() const = 0;

		/// <summary>
		/// Gets the GPU vendor identifier.
		/// </summary>
		virtual uint32 GetVendorId() const = 0;

		/// <summary>
		/// Gets a string that contains the adapter description. Used for presentation to the user.
		/// </summary>
		virtual String GetDescription() const = 0;

	public:
		// Returns true if adapter's vendor is AMD.
		inline bool IsAMD() const
		{
			return GetVendorId() == GPU_VENDOR_ID_AMD;
		}

		// Returns true if adapter's vendor is Intel.
		inline bool IsIntel() const
		{
			return GetVendorId() == GPU_VENDOR_ID_INTEL;
		}

		// Returns true if adapter's vendor is Nvidia.
		inline bool IsNVIDIA() const
		{
			return GetVendorId() == GPU_VENDOR_ID_NVIDIA;
		}

		// Returns true if adapter's vendor is Microsoft.
		inline bool IsMicrosoft() const
		{
			return GetVendorId() == GPU_VENDOR_ID_MICROSOFT;
		}
	};
}
