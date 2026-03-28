
#pragma once

#include "IStreamingHandler.h"
#include "Core/Utilities/Singleton.hpp"
#include "Core/Types/Collections/List.h"

namespace SE
{

	/// <summary>
	/// Describes streamable resources group object.
	/// </summary>
	class SE_API_RUNTIME StreamingGroup
	{
	public:
		enum class Type : byte
		{
			Custom,
			Textures,
			Models,
			Audio
		};

	protected:

		Type _type;
		IStreamingHandler* _handler;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="StreamingGroup"/> class.
		/// </summary>
		/// <param name="type">The group type.</param>
		/// <param name="handler">Group dedicated handler.</param>
		StreamingGroup(Type type, IStreamingHandler* handler);

	public:

		/// <summary>
		/// Gets the group type.
		/// </summary>
		FORCE_INLINE Type GetType() const
		{
			return _type;
		}

		/// <summary>
		/// Gets the group type name.
		/// </summary>
		FORCE_INLINE const Char* GetTypename() const
		{
			switch (_type)
			{
			case Type::Custom:
				return SE_TEXT("Custom");
			case Type::Textures:
				return SE_TEXT("Textures");
			case Type::Models:
				return SE_TEXT("Models");
			case Type::Audio:
				return SE_TEXT("Audio");
			}

			ENGINE_UNREACHABLE_CODE();
			return SE_TEXT("");
		}

		/// <summary>
		/// Gets the group streaming handler used by this group.
		/// </summary>
		FORCE_INLINE IStreamingHandler* GetHandler() const
		{
			return _handler;
		}
	};

	/// <summary>
	/// Streaming groups manager.
	/// </summary>
	class SE_API_RUNTIME StreamingGroups : public AutoSingleton<StreamingGroups>
	{
	private:

		StreamingGroup* _textures;
		StreamingGroup* _models;
		StreamingGroup* _skinnedModels;
		StreamingGroup* _audio;

		List<StreamingGroup*> _groups;
		List<IStreamingHandler*> _handlers;

	public:

		StreamingGroups();
		~StreamingGroups();

	public:

		/// <summary>
		/// Gets textures group.
		/// </summary>
		FORCE_INLINE StreamingGroup* Textures() const
		{
			return _textures;
		}

		/// <summary>
		/// Gets models group.
		/// </summary>
		FORCE_INLINE StreamingGroup* Models() const
		{
			return _models;
		}

		/// <summary>
		/// Gets skinned models group.
		/// </summary>
		FORCE_INLINE StreamingGroup* SkinnedModels() const
		{
			return _skinnedModels;
		}

		/// <summary>
		/// Gets audio group.
		/// </summary>
		FORCE_INLINE StreamingGroup* Audio() const
		{
			return _audio;
		}

	public:

		/// <summary>
		/// Gets all the groups.
		/// </summary>
		FORCE_INLINE const List<StreamingGroup*>& Groups() const
		{
			return _groups;
		}

		/// <summary>
		/// Gets all the handlers.
		/// </summary>
		FORCE_INLINE const List<IStreamingHandler*>& Handlers() const
		{
			return _handlers;
		}

	public:

		/// <summary>
		/// Adds the specified streaming group.
		/// </summary>
		/// <param name="group">The group.</param>
		void Add(StreamingGroup* group);
	};
}
