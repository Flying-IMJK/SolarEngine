
#include "SceneObject.h"

#include "Actor.h"
#include "Core/Serialization/Serialization.h"
#include "Prefabs/Prefab.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE
{
	void SceneBeginData::OnDone()
	{

	}

	SceneObject::SceneObject() :
		m_Parent(nullptr)
	    , m_PrefabID(UID::Empty)
	    , m_PrefabObjectID(UID::Empty), m_IsDuringPlay(false)
	{
	}

	SceneObject::~SceneObject()
	{
	}

	void SceneObject::LinkPrefab(const UID& prefabId, const UID& prefabObjectId)
	{
		ASSERT(prefabId.IsValid());

		// Link
		m_PrefabID = prefabId;
		m_PrefabObjectID = prefabObjectId;

		if (m_PrefabID.IsValid() && m_PrefabObjectID.IsValid())
		{
			/*auto prefab = AssetContent::LoadAsync<Prefab>(_prefabID);
			if (prefab == nullptr || prefab->WaitForLoaded())
			{
				_prefabID = SGUID::Empty;
				_prefabObjectID = SGUID::Empty;
				LOG_WARNING("Level", "Failed to load prefab linked to the actor.");
			}*/
		}
	}

	void SceneObject::BreakPrefabLink()
	{
		// Invalidate link
		m_PrefabID = UID::Empty;
		m_PrefabObjectID = UID::Empty;
	}

	String SceneObject::GetNamePath(Char separatorChar) const
	{
		return String::Empty;
	}

	void SceneObject::Serialize(SerializeContext& context)
	{
		SERIALIZE_GET_OTHER_OBJ(SceneObject, context.otherObj);

		context.stream.JKEY("ID");
		context.stream.UUID(GetInstanceID());

		if (other && HasPrefabLink())
		{
			context.stream.JKEY("PrefabID");
			context.stream.UUID(m_PrefabID);

			context.stream.JKEY("PrefabObjectID");
			context.stream.UUID(m_PrefabObjectID);
		}
		else
		{
			context.stream.JKEY("TypeName");
			context.stream.String(GetTypeInfo()->friendlyName);
		}

		if (m_Parent)
		{
			context.stream.JKEY("ParentID");
			context.stream.UUID(m_Parent->GetInstanceID());
		}

/*#if !COMPILE_WITHOUT_CSHARP
		// Handle C# objects data serialization
		if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
		{
			stream.JKEY("V");
			if (other)
			{
				ManagedSerialization::SerializeDiff(stream, GetOrCreateManagedInstance(), other->GetOrCreateManagedInstance());
			}
			else
			{
				ManagedSerialization::Serialize(stream, GetOrCreateManagedInstance());
			}
		}
#endif

		// Handle custom scripting objects data serialization
		if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
		{
			stream.JKEY("D");
			_type.Module->SerializeObject(stream, this, other);
		}*/
	}

	void SceneObject::Deserialize(DeserializeContext& context)
	{
		// _id is deserialized by Actor/Script impl
		// _parent is deserialized by Actor/Script impl
		// _prefabID is deserialized by Actor/Script impl
		DESERIALIZE_MEMBER(PrefabObjectID, m_PrefabObjectID);

/*#if !COMPILE_WITHOUT_CSHARP
		// Handle C# objects data serialization
		if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
		{
			auto* const v = SERIALIZE_FIND_MEMBER(stream, "V");
			if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
			{
				ManagedSerialization::Deserialize(v->value, GetOrCreateManagedInstance());
			}
		}
#endif

		// Handle custom scripting objects data serialization
		if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
		{
			auto* const v = SERIALIZE_FIND_MEMBER(stream, "D");
			if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
			{
				_type.Module->DeserializeObject(v->value, this, modifier);
			}
		}*/
	}

	bool Serialization::ShouldSerialize(const SceneObject* v, const void* otherObj)
	{
		return !otherObj || v != (SceneObject*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, SceneObject* obj)
	{
		context.stream.StartObject();

		// Serialize prefab instances with diff detection
		if (obj->HasPrefabLink())
		{
			// Load prefab
			auto prefab = AssetContent::Load<Prefab>(obj->GetPrefabID());
			if (prefab)
			{
				// Request the prefab to be deserialized to the default instance (used for comparison to generate a diff)
				prefab->GetDefaultInstance();

				// Get prefab object instance from the prefab
				SceneObject* prefabObject;
				if (prefab->ObjectsCache.TryGet(obj->GetPrefabObjectID(), prefabObject))
				{
					// Serialize modified properties compared with the default object from prefab
					SerializeContext::ObjectScope scope(context, prefabObject);
					context.stream.EndObject();
					return;
				}
				else
				{
					LOG_WARNING("Level", "Missing object {1} in prefab {0}.", prefab->ToString(), obj->GetPrefabObjectID());
				}
			}
			else
			{
				LOG_WARNING("Level", "Missing prefab {0}.", obj->GetPrefabID());
			}
		}

		// Serialize modified properties compared with the default class object
		{
			const void* defaultInstance =  obj->GetTypeInfo()->GetDefaultInstance();
			SerializeContext::ObjectScope scope(context, defaultInstance);
			obj->Serialize(context);
		}

		context.stream.EndObject();
	}

	void Serialization::Deserialize(DeserializeContext& context, SceneObject* v)
	{

	}
} // SE