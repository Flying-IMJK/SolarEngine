#pragma once

#include "Variable.h"
#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/Core/Formatting.h"
#include "Runtime/Core/Templates.h"

namespace SE
{
	/// <summary>
	/// Globally Unique Identifier
	/// </summary>
	struct SE_API_RUNTIME UID
	{
	public:
		/// <summary>
		/// Accepted format specifiers for the format parameter
		/// </summary>
		enum class FormatType
		{
			// 32 digits:
			// 00000000000000000000000000000000
			N,

			// 32 digits separated by hyphens:
			// 00000000-0000-0000-0000-000000000000
			D,

			// 32 digits separated by hyphens, enclosed in braces:
			// {00000000-0000-0000-0000-000000000000}
			B,

			// 32 digits separated by hyphens, enclosed in parentheses:
			// (00000000-0000-0000-0000-000000000000)
			P,
		};

	public:
		union
		{
			struct
			{
				// The first component
				uint32 A;

				// The second component
				uint32 B;

				// The third component
				uint32 C;

				// The fourth component
				uint32 D;
			};

			// Raw bytes with the GUID
			byte Raw[16];

			// Raw values with the GUID
			uint32 Values[4];
		};

	public:
		// Empty Guid (considered as invalid ID)
		static UID Empty;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		UID()
		{
		}

		// Creates and initializes a new Guid from the specified components
		// @param a The first component
		// @param b The second component
		// @param c The third component
		// @param d The fourth component
		UID(uint32 a, uint32 b, uint32 c, uint32 d) : A(a), B(b), C(c), D(d)
		{
		}

	public:
		bool operator==(const UID& other) const
		{
			return ((A ^ other.A) | (B ^ other.B) | (C ^ other.C) | (D ^ other.D)) == 0;
		}

		bool operator!=(const UID& other) const
		{
			return ((A ^ other.A) | (B ^ other.B) | (C ^ other.C) | (D ^ other.D)) != 0;
		}

		// Provides access to the GUIDs components
		// @param index The index of the component to return (0...3)
		// @returns The component value
		uint32& operator[](int32 index);

		// Provides read-only access to the GUIDs components.
		// @param index The index of the component to return (0...3).
		// @return The component
		const uint32& operator[](int index) const;

		// Invalidates the Guid
		FORCE_INLINE void Invalidate()
		{
			A = B = C = D = 0;
		}

		// Checks whether this Guid is valid or not. A Guid that has all its components set to zero is considered invalid.
		// @return true if valid, otherwise false
		FORCE_INLINE bool IsValid() const
		{
			return (A | B | C | D) != 0;
		}

		/// <summary>
		/// Checks if Guid is valid
		/// </summary>
		/// <returns>True if Guid isn't empty</returns>
		explicit operator bool() const
		{
			return (A | B | C | D) != 0;
		}

	public:
		String ToString() const;
		String ToString(FormatType format) const;
		void ToString(char* buffer, FormatType format) const;
		void ToString(Char* buffer, FormatType format) const;

	public:
		/// <summary>
		/// Try to parse Guid from string
		/// </summary>
		/// <param name="text">Input text</param>
		/// <param name="value">Result value</param>
		/// <returns>True if cannot parse text, otherwise false</returns>
		static bool Parse(const StringView& text, UID& value);

		/// <summary>
		/// Try to parse Guid from string
		/// </summary>
		/// <param name="text">Input text</param>
		/// <param name="value">Result value</param>
		/// <returns>True if cannot parse text, otherwise false</returns>
		static bool Parse(const StringAnsiView& text, UID& value);

		/// <summary>
		/// Creates the unique identifier.
		/// </summary>
		/// <returns>New Guid</returns>
		FORCE_INLINE static UID New()
		{
			UID result;
			Platform::CreateUUID(result);
			return result;
		}
	};


	inline uint32 GetHash(const UID& key)
	{
		return static_cast<int32>(key.A ^ key.B ^ key.C ^ key.D);
	}
}


template<>
struct TIsPODType<SE::UID>
{
	enum
	{
		Value = true
	};
};

DEFINE_DEFAULT_FORMATTING(SE::UID, "{:0>8x}{:0>8x}{:0>8x}{:0>8x}", v.A, v.B, v.C, v.D);