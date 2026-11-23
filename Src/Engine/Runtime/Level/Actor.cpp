
#include <Runtime/Level/Actor.h>

#include <Runtime/Core/Profiler/ProfilerCPU.h>
#include <Runtime/Core/Scripting/Scripting.h>
#include <Runtime/Core/Serialization/JsonTools.h>
#include <Runtime/Core/Serialization/JsonWriters.hpp>
#include <Runtime/Core/Serialization/Serialization.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Level/Level.h>
#include <Runtime/Level/SceneObjectsFactory.h>
#include <Runtime/Resource/AssetContent.h>

namespace SE
{
#define ACTOR_ORIENTATION_EPSILON 0.000000001f

    Actor::Actor(const SpawnParams& params)
        : SceneObject(params)
        , m_IsActive(true)
        , m_IsActiveInHierarchy(true)
        , m_IsPrefabRoot(false)
        , m_IsEnabled(false)
        , m_Layer(0)
        , m_StaticFlags(StaticMask::FullyStatic)
        , m_LocalTransform(Transform::Identity)
        , m_Transform(Transform::Identity)
        , m_Sphere(BoundingSphere::Empty)
        , m_Box(BoundingBox::Zero)
        , m_Scene(nullptr)
    {
        m_DrawNoCulling = 0;
        m_DrawCategory = 0;
    }

    RenderActor::RenderActor(const SpawnParams& params)
        : Actor(params)
    {
    }

    Actor::~Actor() = default;

    List<byte> Actor::ToBytes(const List<Actor*>& actors)
    {
        List<byte> result;
        if (actors.IsEmpty())
        {
            return result;
        }
        List<UID> ids;
        for (Actor* actor : actors)
        {
            if (!actor || TypeIs<Scene>(actor) || actor->HasPrefabLink() || ids.Contains(actor->GetID()))
            {
                LOG_ERROR("Level", "Cannot serialize actor clipboard data: invalid, duplicate, or unsupported actor.");
                return result;
            }
            ids.Add(actor->GetID());
        }

        Json::StringBuffer buffer;
        CompactJsonWriter  writer(buffer);
        writer.StartObject();
        writer.JKEY("Actors");
        writer.StartArray();
        SerializeContext context(writer);
        for (Actor* actor : actors)
        {
            writer.StartObject();
            actor->Serialize(context);
            writer.JKEY("OrderInParent");
            writer.Int(actor->GetOrderInParent());
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();
        result.Set((const byte*)buffer.GetString(), (int32)buffer.GetSize());
        return result;
    }

    bool Actor::ParseActorsData(const Span<byte>& data, SerializeDocument& document, List<UID>& ids)
    {
        if (data.Length() <= 0)
        {
            return false;
        }
        document.Parse((const char*)data.Get(), data.Length());
        if (document.HasParseError() || !document.IsObject() || !document.HasMember("Actors") || !document["Actors"].IsArray() || document["Actors"].Empty())
        {
            return false;
        }

        // Use each type's existing serialization to check field shapes before spawning.
        const auto validate = [](const auto& self, const Json::Value& value, const Json::Value& expected) -> bool {
            if (expected.IsObject())
            {
                if (!value.IsObject())
                {
                    return false;
                }
                for (auto member = expected.MemberBegin(); member != expected.MemberEnd(); ++member)
                {
                    const auto actual = value.FindMember(member->name);
                    if (actual == value.MemberEnd() || !self(self, actual->value, member->value))
                    {
                        return false;
                    }
                }
                return true;
            }
            if (expected.IsArray())
            {
                if (!value.IsArray())
                {
                    return false;
                }
                for (auto& item : value.GetArray())
                {
                    if (!expected.Empty() && !self(self, item, expected[0]))
                    {
                        return false;
                    }
                }
                return true;
            }
            if (expected.IsBool())
            {
                return value.IsBool();
            }
            if (expected.IsDouble())
            {
                return value.IsFloat();
            }
            // Prefer signed ranges when JSON integer flags overlap; Layer is checked separately.
            if (expected.IsInt())
            {
                return value.IsInt();
            }
            if (expected.IsUint())
            {
                return value.IsUint();
            }
            if (expected.IsInt64())
            {
                return value.IsInt64();
            }
            if (expected.IsUint64())
            {
                return value.IsUint64();
            }
            return value.GetType() == expected.GetType();
        };
        Dictionary<UID, UID> parents;
        for (auto& value : document["Actors"].GetArray())
        {
            if (!value.IsObject() || !value.HasMember("ID") || !value["ID"].IsString() || value["ID"].GetStringLength() != 32 || !value.HasMember("TypeName") || !value["TypeName"].IsString() ||
                !value.HasMember("TypeID") || !value["TypeID"].IsUint() || !value.HasMember("OrderInParent") ||
                !value["OrderInParent"].IsInt() || value.HasMember("PrefabID") || value.HasMember("PrefabObjectID"))
            {
                return false;
            }
            // Layer is the unsigned byte read directly by the base Actor deserializer.
            if (!value.HasMember("Layer") || !value["Layer"].IsUint() || value["Layer"].GetUint() > 31)
            {
                return false;
            }
            UID id;
            if (UID::Parse(value["ID"].GetStringAnsiView(), id) || !id.IsValid() || parents.ContainsKey(id))
            {
                return false;
            }
            const ScriptingTypeHandle type = Scripting::FindScriptingType(value["TypeName"].GetStringAnsiView());
            if (!type || !Actor::TypeInitializer.IsAssignableFrom(type) || Scene::TypeInitializer.IsAssignableFrom(type))
            {
                return false;
            }
            Actor* prototype = static_cast<Actor*>(type.GetType().GetDefaultInstance());
            if (!prototype)
            {
                return false;
            }
            Json::StringBuffer buffer;
            CompactJsonWriter  writer(buffer);
            SerializeContext   serializeContext(writer);
            writer.StartObject();
            prototype->Serialize(serializeContext);
            writer.EndObject();
            Json::Document expected;
            expected.Parse(buffer.GetString(), buffer.GetSize());
            if (expected.HasParseError())
            {
                return false;
            }
            // TypeID is checked as uint above, independently of the scripting type.
            expected.RemoveMember("TypeID");
            if (!validate(validate, value, expected))
            {
                return false;
            }
            UID parent = UID::Empty;
            if (value.HasMember("ParentID") && (!value["ParentID"].IsString() || value["ParentID"].GetStringLength() != 32 || UID::Parse(value["ParentID"].GetStringAnsiView(), parent)))
            {
                return false;
            }
            parents.Add(id, parent);
            ids.Add(id);
        }

        // Reject cycles before creating or linking any objects.
        for (const UID& id : ids)
        {
            UID   parent = id;
            int32 depth  = 0;
            while (parents.TryGet(parent, parent))
            {
                if (++depth > ids.Count())
                {
                    return false;
                }
            }
        }
        return true;
    }

    List<UID> Actor::TryGetSerializedObjectsIds(const Span<byte>& data)
    {
        Json::Document document;
        List<UID>      ids;
        if (!ParseActorsData(data, document, ids))
        {
            ids.Clear();
        }
        return ids;
    }

    List<Actor*> Actor::FromBytes(const Span<byte>& data, const List<UID>& newIds, Actor* pasteParent)
    {
        List<Actor*>   actors;
        Json::Document document;
        List<UID>      ids;
        if (!Level::IsAnySceneLoaded() || (pasteParent && !pasteParent->GetScene()) || !ParseActorsData(data, document, ids))
        {
            LOG_ERROR("Level", "Cannot deserialize actor clipboard data.");
            return actors;
        }
        if (newIds.Count() != ids.Count())
        {
            LOG_ERROR("Level", "Actor clipboard data requires one new ID per serialized actor.");
            return actors;
        }
        Dictionary<UID, UID> idsMapping;
        List<UID>            usedIds;
        for (int32 index = 0; index < ids.Count(); index++)
        {
            const UID& newId = newIds[index];
            if (!newId.IsValid() || usedIds.Contains(newId) || Scripting::TryFindObject(newId))
            {
                LOG_ERROR("Level", "Actor clipboard data requires unique, unused mapped IDs.");
                return actors;
            }
            usedIds.Add(newId);
            idsMapping.Add(ids[index], newId);
        }

        auto modifier        = SerializeModifierCache::Modifier.Get();
        modifier->IdsMapping = idsMapping;
        SceneObjectsFactory::Context context(modifier.Value);
        auto&                        values = document["Actors"];
        // Register every object first, so references can resolve in either direction.
        for (int32 index = 0; index < ids.Count(); index++)
        {
            Actor* actor = TypeTryCast<Actor>(SceneObjectsFactory::Spawn(context, values[index]));
            if (!actor)
            {
                LOG_ERROR("Level", "Failed to create actor from clipboard data.");
                for (Actor* created : actors)
                {
                    created->DeleteObjectNow();
                }
                actors.Clear();
                return actors;
            }
            if (!actor->IsRegistered())
            {
                actor->RegisterObject();
            }
            actors.Add(actor);
        }
        for (int32 index = 0; index < actors.Count(); index++)
        {
            SceneObjectsFactory::Deserialize(context, actors[index], values[index]);
        }

        List<Actor*> roots;
        for (int32 index = 0; index < actors.Count(); index++)
        {
            Actor* actor = actors[index];
            if (actors.Contains(actor->m_Parent))
            {
                actor->SetOrderInParent(values[index]["OrderInParent"].GetInt());
                continue;
            }
            roots.Add(actor);
            Actor* parent = pasteParent ? pasteParent : actor->m_Parent;
            if (!parent || !parent->GetScene())
            {
                parent = Level::GetScene(0);
            }
            // Link without SetParent: initialization and editor notifications must wait
            // until all roots and their references have been restored.
            if (actor->m_Parent != parent)
            {
                if (actor->m_Parent)
                {
                    actor->m_Parent->Children.RemoveKeepOrder(actor);
                }
                actor->m_Parent = parent;
                parent->Children.Add(actor);
            }
        }
        for (Actor* root : roots)
        {
            root->InitializeHierarchy();
            root->OnTransformChanged();
        }
        SceneBeginData beginData;
        for (Actor* root : roots)
        {
            root->BeginPlay(&beginData);
        }
        beginData.OnDone();
        for (Actor* root : roots)
        {
            Level::CallActorEvent(Level::ActorEventType::OnActorSpawned, root, nullptr);
        }
        return actors;
    }

    void Actor::SetLayer(int32 layerIndex)
    {
        layerIndex = Math::Clamp(layerIndex, 0, 31);
        if (layerIndex == m_Layer)
            return;
        m_Layer = layerIndex;
        OnLayerChanged();
    }

    void Actor::SetLayerRecursive(int32 layerIndex)
    {
        layerIndex = Math::Clamp(layerIndex, 0, 31);
        for (const auto& child : Children)
            child->SetLayerRecursive(layerIndex);
        if (layerIndex == m_Layer)
            return;
        m_Layer = layerIndex;
        OnLayerChanged();
    }

    const String& Actor::GetLayerName() const
    {
        return String::Empty;// Level::Layers[_layer];
    }

    void Actor::SetLayerName(const StringView& value)
    {
        if (m_Name == value)
            return;
        m_Name = value;
        if (GetScene())
            Level::CallActorEvent(Level::ActorEventType::OnActorNameChanged, this, nullptr);
    }

    void Actor::SetLayerNameRecursive(const StringView& value)
    {
        /*for (int32 i = 0; i < 32; i++)
        {
            if (Level::Layers[i] == value)
            {
                SetLayerRecursive(i);
                return;
            }
        }*/
        LOG_WARNING("Level", "Unknown layer name '{0}'", value);
    }

    void Actor::SetName(const StringView& value)
    {
        if (m_Name == value)
        {
            return;
        }

        m_Name = value;
        if (GetScene())
        {
            Level::CallActorEvent(Level::ActorEventType::OnActorNameChanged, this, nullptr);
        }
    }

    Actor* Actor::GetChild(int32 index) const
    {
        if (index < 0 || index >= Children.Count())
        {
            LOG_ERROR("Level", "GetChild index {0} is out of range {1}", index, Children.Count());
            return nullptr;
        }
        return Children[index];
    }

    Actor* Actor::GetChild(const StringView& name) const
    {
        for (int32 i = 0; i < Children.Count(); i++)
        {
            auto e = Children.Get()[i];
            if (e->GetName() == name)
                return e;
        }
        return nullptr;
    }

    void Actor::DestroyChildren(float timeLeft)
    {
        PROFILE_CPU();
        List<Actor*> children = Children;
        const bool useGameTime = timeLeft > Math::ZeroTolerance;
        for (Actor* child : children)
        {
            child->SetParent(nullptr, false, false);
            child->DeleteObject(timeLeft, useGameTime);
        }
    }

    void Actor::SetIsActive(bool value)
    {
        if (value != GetIsActive())
        {
            m_IsActive = value;
            OnActiveChanged();
        }
    }

    void Actor::SetStaticFlags(StaticMask flags) 
    { 
        if (m_StaticFlags == flags)
        {
            return;
        }
        m_StaticFlags = flags; 
        OnStaticFlagsChanged();
    }

    void Actor::SetStaticFlag(StaticMask flag, bool value)
    {
        if (EnumHasAnyFlags(m_StaticFlags, flag) != value)
        {
            m_StaticFlags = value ? EnumAddFlags(m_StaticFlags, flag) : EnumRemoveFlags(m_StaticFlags, flag);
            OnStaticFlagsChanged();
        }
    }

    void Actor::SetTransform(const Transform& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set transform value is nan");
            return;
        }

        if (!(Float3::NearEqual(m_Transform.Translation, value.Translation) &&
            Quaternion::NearEqual(m_Transform.Orientation, value.Orientation, ACTOR_ORIENTATION_EPSILON) &&
            Float3::NearEqual(m_Transform.Scale, value.Scale)))
        {
            if (m_Parent)
                m_Parent->m_Transform.WorldToLocal(value, m_LocalTransform);
            else
                m_LocalTransform = value;
            OnTransformChanged();
        }
    }

    void Actor::SetPosition(const Float3& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set position value is nan");
            return;
        }

        if (!Float3::NearEqual(m_Transform.Translation, value))
        {
            if (m_Parent)
                m_LocalTransform.Translation = m_Parent->m_Transform.WorldToLocal(value);
            else
                m_LocalTransform.Translation = value;
            OnTransformChanged();
        }
    }

    void Actor::SetOrientation(const Quaternion& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set quaternion value is nan");
            return;
        }

        if (!Quaternion::NearEqual(m_Transform.Orientation, value, ACTOR_ORIENTATION_EPSILON))
        {
            if (m_Parent)
                m_Parent->m_Transform.WorldToLocal(value, m_LocalTransform.Orientation);
            else
                m_LocalTransform.Orientation = value;
            OnTransformChanged();
        }
    }

    void Actor::SetScale(const Float3& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set scale value is nan");
            return;
        }

        if (!Float3::NearEqual(m_Transform.Scale, value))
        {
            if (m_Parent)
                Float3::Divide(value, m_Parent->m_Transform.Scale, m_LocalTransform.Scale);
            else
                m_LocalTransform.Scale = value;
            OnTransformChanged();
        }
    }

    Matrix Actor::GetRotation() const
    {
        Matrix result;
        Matrix::RotationQuaternion(m_Transform.Orientation, result);
        return result;
    }

    void Actor::SetRotation(const Matrix& value)
    {
        Quaternion orientation;
        Quaternion::RotationMatrix(value, orientation);
        SetOrientation(orientation);
    }

    void Actor::SetForward(const Float3& value)
    {
        ENGINE_ASSERT(!value.IsNanOrInfinity());

        Quaternion orientation;
        if (Float3::Dot(value, Float3::Up) >= 0.999f)
        {
            Quaternion::RotationAxis(Float3::Left, Math::HALF_PI, orientation);
        }
        else
        {
            const Float3 right = Float3::Cross(value, Float3::Up);
            const Float3 up = Float3::Cross(right, value);
            Quaternion::LookRotation(value, up, orientation);
        }
        SetOrientation(orientation);
    }

    bool Actor::IntersectsItself(const Ray& ray, float& distance, Float3& normal)
    {
        return m_Box.Intersects(ray, distance, normal);
    }

    void Actor::ResetLocalTransform()
    {
        SetLocalTransform(Transform::Identity);
    }

    void Actor::SetLocalTransform(const Transform& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set transform value is nan");
            return;
        }
        if (!(Float3::NearEqual(m_LocalTransform.Translation, value.Translation) &&
            Quaternion::NearEqual(m_LocalTransform.Orientation, value.Orientation, ACTOR_ORIENTATION_EPSILON) &&
            Float3::NearEqual(m_LocalTransform.Scale, value.Scale)))
        {
            m_LocalTransform = value;
            OnTransformChanged();
        }
    }

    void Actor::SetLocalPosition(const Float3& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set position value is nan");
            return;
        }
        if (!Float3::NearEqual(m_LocalTransform.Translation, value))
        {
            m_LocalTransform.Translation = value;
            OnTransformChanged();
        }
    }

    void Actor::SetLocalQuaternion(const Quaternion& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set quaternion value is nan");
            return;
        }

        Quaternion v = value;
        v.Normalize();
        if (!Quaternion::NearEqual(m_LocalTransform.Orientation, v, ACTOR_ORIENTATION_EPSILON))
        {
            m_LocalTransform.Orientation = v;
            OnTransformChanged();
        }
    }

    void Actor::SetLocalScale(const Float3& value)
    {
        if (value.IsNanOrInfinity())
        {
            LOG_ERROR("Level", "set scale value is nan");
            return;
        }

        if (!Float3::NearEqual(m_LocalTransform.Scale, value))
        {
            m_LocalTransform.Scale = value;
            OnTransformChanged();
        }
    }

    void Actor::GetWorldToLocalMatrix(Matrix& worldToLocal) const
    {
        GetLocalToWorldMatrix(worldToLocal);
        worldToLocal.Invert();
    }

    void Actor::GetLocalToWorldMatrix(Matrix& localToWorld) const
    {
#if 0
        _transform.GetWorld(localToWorld);
#else
        m_LocalTransform.GetWorld(localToWorld);
        if (m_Parent)
        {
            Matrix parentToWorld;
            m_Parent->GetLocalToWorldMatrix(parentToWorld);
            localToWorld = localToWorld * parentToWorld;
        }
#endif
    }

    void Actor::InitializeHierarchy()
    {
        Initialize();

        /*for (int32 i = 0; i < Scripts.Count(); i++)
            Scripts[i]->Initialize();*/

        for (int32 i = 0; i < Children.Count(); i++)
        {
            Children[i]->InitializeHierarchy();
        }
    }

    void Actor::OnEnable()
    {
        m_IsEnabled = true;
    }

    void Actor::OnDisable()
    {
        m_IsEnabled = false;
    }

    void Actor::SetSceneInHierarchy(Scene* scene)
    {
        m_Scene = scene;

        for (int32 i = 0; i < Children.Count(); i++)
        {
            Children[i]->SetSceneInHierarchy(scene);
        }
    }

    void Actor::OnEnableInHierarchy()
    {
        if (IsActiveInHierarchy() && GetScene() && !m_IsEnabled)
        {
            OnEnable();

            for (int32 i = 0; i < Children.Count(); i++)
            {
                Children[i]->OnEnableInHierarchy();
            }
        }
    }

    void Actor::OnDisableInHierarchy()
    {
        if (IsActiveInHierarchy() && GetScene() && m_IsEnabled)
        {
            for (int32 i = 0; i < Children.Count(); i++)
            {
                Children[i]->OnDisableInHierarchy();
            }

            OnDisable();
        }
    }

    void Actor::OnTransformChanged()
    {
        ENGINE_ASSERT(!m_LocalTransform.IsNanOrInfinity());

        if (m_Parent)
        {
            m_Parent->m_Transform.LocalToWorld(m_LocalTransform, m_Transform);
        }
        else
        {
            m_Transform = m_LocalTransform;
        }

        for (auto child : Children)
        {
            child->OnTransformChanged();
        }
    }

    void Actor::OnActiveChanged()
    {
        const bool wasActiveInTree = IsActiveInHierarchy();
        m_IsActiveInHierarchy = m_IsActive && (m_Parent == nullptr || m_Parent->IsActiveInHierarchy());
        if (wasActiveInTree != IsActiveInHierarchy())
            OnActiveInTreeChanged();

        //if (GetScene())
        Level::CallActorEvent(Level::ActorEventType::OnActorActiveChanged, this, nullptr);
    }

    void Actor::OnActiveInTreeChanged()
    {
        if (IsDuringPlay() && GetScene())
        {
            if (IsActiveInHierarchy())
            {
                if (!m_IsEnabled)
                {
                    OnEnable();
                }
            }
            else if (m_IsEnabled)
            {
                OnDisable();
            }
        }

        for (auto child : Children)
        {
            if (child->GetIsActive() && child->m_IsActiveInHierarchy != m_IsActiveInHierarchy)
            {
                child->m_IsActiveInHierarchy = m_IsActiveInHierarchy;
                child->OnActiveInTreeChanged();
            }
        }
    }

    void Actor::OnStaticFlagsChanged()
    {
    }

    void Actor::OnLayerChanged()
    {
    }

    void Actor::Initialize()
    {
#if ENABLE_ASSERTION
        CHECK(!IsDuringPlay());
#endif

        // Cache
        if (m_Parent)
        {
            m_Scene = m_Parent->GetScene();
        }
        m_IsActiveInHierarchy = m_IsActive && (m_Parent == nullptr || m_Parent->IsActiveInHierarchy());

        /*// Use lazy creation for the managed instance, just register the object
        if (!IsRegistered())
            RegisterObject();*/
    }

    void Actor::BeginPlay(SceneBeginData* data)
    {
        m_IsDuringPlay = true;

        OnBeginPlay();

        // Update children
        for (int32 i = 0; i < Children.Count(); i++)
        {
            Actor* actor = Children.Get()[i];
            if (!actor->IsDuringPlay())
            {
                actor->BeginPlay(data);
            }
        }

        // Fire events for scripting
        if (IsActiveInHierarchy() && GetScene() && !m_IsEnabled)
        {
            OnEnable();
        }
    }

    void Actor::OnDeleteObject()
    {
        if (IsDuringPlay())
        {
            EndPlay();
        }
        else if (m_IsEnabled)
        {
            OnDisable();
        }

        // Notify the editor while the actor, its children, and scene are still valid.
        if (m_Scene)
        {
            Level::CallActorEvent(Level::ActorEventType::OnActorDeleted, this, nullptr);
        }
        while (Children.HasItems())
        {
            Children.Last()->DeleteObjectNow();
        }

        {
            Threading::ScopeLock lock(Level::GetScenesLock());
            if (m_Parent)
            {
                m_Parent->Children.RemoveKeepOrder(this);
                m_Parent = nullptr;
            }
            m_Scene = nullptr;
        }
        SceneObject::OnDeleteObject();
    }

    void Actor::EndPlay()
    {
        // Fire event for scripting
        if (IsActiveInHierarchy() && GetScene())
        {
            ENGINE_ASSERT(GetScene());
            OnDisable();
        }

        OnEndPlay();

        m_IsDuringPlay = false;

        // Call event deeper
        for (int32 i = 0; i < Children.Count(); i++)
        {
            Actor* actor = Children.Get()[i];
            if (actor->IsDuringPlay())
            {
                actor->EndPlay();
            }
        }
    }

    UID Actor::GetSceneObjectId() const
    {
        return GetID();
    }

    void Actor::SetParent(Actor* value, bool worldPositionsStays, bool canBreakPrefabLink)
    {
        if (m_Parent == value)
            return;
#if USE_EDITOR || !BUILD_RELEASE
        if (TypeIs<Scene>(this))
        {
            LOG_ERROR("Level", "Cannot change parent of the Scene. Use Level to manage scenes.");
            return;
        }
#endif

        // Peek the previous state
        const Transform prevTransform = m_Transform;
        const bool wasActiveInTree = IsActiveInHierarchy();
        const auto prevParent = m_Parent;
        const auto prevScene = m_Scene;
        const auto newScene = value ? value->m_Scene : nullptr;

        // Detect it actor is not in a game but new parent is already in a game (we should spawn it)
        const bool isBeingSpawned = !IsDuringPlay() && newScene && value->IsDuringPlay();

        // Actors system doesn't support editing scene hierarchy from multiple threads
        // if (!Threading::IsMainThread() && (IsDuringPlay() || isBeingSpawned))
        // {
        //     LOG_ERROR("Level", "Editing scene hierarchy is only allowed on a main thread.");
        //     return;
        // }

        // Handle changing scene (unregister from it)
        const bool isSceneChanging = prevScene != newScene;
        if (prevScene && isSceneChanging && wasActiveInTree)
        {
            OnDisableInHierarchy();
        }

        Level::GetScenesLock().Lock();

        // Unlink from the old one
        if (m_Parent)
        {
            m_Parent->Children.RemoveKeepOrder(this);
        }

        // Set value
        m_Parent = value;

        // Link to the new one
        if (m_Parent)
        {
            m_Parent->Children.Add(this);
        }

        // Sync scene change if need to
        if (isSceneChanging)
        {
            SetSceneInHierarchy(newScene);
        }

        Level::GetScenesLock().Unlock();

        // Cache flag
        m_IsActiveInHierarchy = m_IsActive && (m_Parent == nullptr || m_Parent->IsActiveInHierarchy());

        // Break prefab link for non-root prefab instance objects
        /*if (HasPrefabLink() && !_isPrefabRoot && IsDuringPlay() && canBreakPrefabLink)
        {
            BreakPrefabLink();
        }*/

        // Update the transform
        if (worldPositionsStays)
        {
            if (m_Parent)
            {
                m_Parent->m_Transform.WorldToLocal(prevTransform, m_LocalTransform);
            }
            else
            {
                m_LocalTransform = prevTransform;
            }
        }

        // Fire events
        // OnParentChanged();
        if (wasActiveInTree != IsActiveInHierarchy())
        {
            OnActiveInTreeChanged();
        }

        OnTransformChanged();

        if (newScene && isSceneChanging && !isBeingSpawned && IsActiveInHierarchy())
        {
            // Handle scene changing c.d. (register to the new one)
            OnEnableInHierarchy();
        }

        //if (_isDuringPlay)
        if (!isBeingSpawned)
        {
            Level::CallActorEvent(Level::ActorEventType::OnActorParentChanged, this, prevParent);
        }

        // Spawn
        if (isBeingSpawned)
        {
            ENGINE_ASSERT(m_Parent != nullptr && GetScene() != nullptr);

            // Fire events
            InitializeHierarchy();
            {
                SceneBeginData beginData;
                BeginPlay(&beginData);
                beginData.OnDone();
            }
            Level::CallActorEvent(Level::ActorEventType::OnActorSpawned, this, nullptr);
        }
    }

    void Actor::SetParent(Actor* value, bool canBreakPrefabLink)
    {
        SetParent(value, false, canBreakPrefabLink);
    }

    int32 Actor::GetOrderInParent() const
    {
        return m_Parent ? m_Parent->Children.Find((Actor*)this) : INVALID_INDEX;
    }

    void Actor::SetOrderInParent(int32 index)
    {
        if (!m_Parent)
        {
            return;
        }

        auto& parentChildren = m_Parent->Children;
        const int32 currentIndex = parentChildren.Find(this);
        ENGINE_ASSERT(currentIndex != INVALID_INDEX);

        if (currentIndex != index)
        {
            parentChildren.RemoveAtKeepOrder(currentIndex);
            if (index < 0 || index >= parentChildren.Count())
            {
                parentChildren.Add(this);
            }
            else
            {
                parentChildren.Insert(index, this);
            }

            // Fire event
            Level::CallActorEvent(Level::ActorEventType::OnActorOrderInParentChanged, this, nullptr);
        }
    }

    void Actor::Serialize(SerializeContext& context)
    {
        SceneObject::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(Actor, context.otherObj);
        const bool isPrefabDiff = other && HasPrefabLink();

        SERIALIZE_BIT_MEMBER(IsActive, m_IsActive);
        SERIALIZE_MEMBER(Name, m_Name);
        SERIALIZE_MEMBER(Transform, m_LocalTransform);
        SERIALIZE_MEMBER(StaticFlags, m_StaticFlags);
        // SERIALIZE(HideFlags);
        SERIALIZE_MEMBER(Layer, m_Layer);
        /*if (!other || Tags != other->Tags)
        {
            if (Tags.Count() == 1)
            {
                stream.JKEY("Tag");
                stream.String(Tags.Get()->ToString());
            }
            else
            {
                stream.JKEY("Tags");
                stream.StartArray();
                for (auto& tag : Tags)
                    stream.String(tag.ToString());
                stream.EndArray();
            }
        }*/

        /*if (isPrefabDiff)
        {
            // Prefab object instance may have removed child objects (actors/scripts)
            // Scene deserialization by default adds missing objects to synchronize changes applied to prefab but not applied to scene
            // In order to handle removed objects per instance we need to save the ids of the prefab object ids that are not used by this object

            bool hasRemovedObjects = false;
            for (int32 i = 0; i < other->Children.Count(); i++)
            {
                const UID prefabObjectId = other->Children[i]->GetPrefabObjectID();
                if (GetChildByPrefabObjectId(this, prefabObjectId) == nullptr)
                {
                    if (!hasRemovedObjects)
                    {
                        hasRemovedObjects = true;
                        stream.JKEY("RemovedObjects");
                        stream.StartArray();
                    }

                    stream.UUID(prefabObjectId);
                }
            }
            for (int32 i = 0; i < other->Scripts.Count(); i++)
            {
                const UID prefabObjectId = other->Scripts[i]->GetPrefabObjectID();
                if (GetScriptByPrefabObjectId(this, prefabObjectId) == nullptr)
                {
                    if (!hasRemovedObjects)
                    {
                        hasRemovedObjects = true;
                        stream.JKEY("RemovedObjects");
                        stream.StartArray();
                    }

                    stream.Guid(prefabObjectId);
                }
            }
            if (hasRemovedObjects)
            {
                stream.EndArray();
            }
        }*/

    }

    void Actor::Deserialize(DeserializeContext& context)
    {
        SceneObject::Deserialize(context);

        DESERIALIZE_BIT_MEMBER(IsActive, m_IsActive);
        DESERIALIZE_MEMBER(Name, m_Name);
        DESERIALIZE_MEMBER(Transform, m_LocalTransform);
        DESERIALIZE_MEMBER(StaticFlags, m_StaticFlags);
        // DESERIALIZE(HideFlags);
        DESERIALIZE_MEMBER(Layer, m_Layer);

        const auto member = SERIALIZE_FIND_MEMBER(context.stream, "ParentID");
        if (member != context.stream->MemberEnd())
        {
            UID parentId;
            {
                DeserializeContext::StreamScope scope(context, member->value);
                Serialization::Deserialize(context, parentId);
            }
            context.modifier->IdsMapping.TryGet(parentId, parentId);
            Actor* parent = TypeTryCast<Actor>(Scripting::TryFindObject(parentId));
            if (IsDuringPlay())
            {
                SetParent(parent, false, false);
            }
            else if (m_Parent != parent)
            {
                if (m_Parent)
                {
                    m_Parent->Children.RemoveKeepOrder(this);
                }
                m_Parent = parent;
                if (m_Parent)
                {
                    m_Parent->Children.Add(this);
                }
            }
        }

        // StaticFlags update - added StaticFlags::Navigation
        // [Deprecated on 17.05.2020, expires on 17.05.2021]
        /*if (modifier->EngineBuild < 6178 && (int32)_staticFlags == (1 + 2 + 4))
        {
            _staticFlags |= StaticFlags::Navigation;
        }

        const auto tag = stream.FindMember("Tag");
        if (tag != stream.MemberEnd())
        {
            if (tag->value.IsString() && tag->value.GetStringLength())
            {
                Tags.Clear();
                Tags.Add(Tags::Get(tag->value.GetText()));
            }
        }
        else
        {
            const auto tags = stream.FindMember("Tags");
            if (tags != stream.MemberEnd() && tags->value.IsArray())
            {
                Tags.Clear();
                for (rapidjson::SizeType i = 0; i < tags->value.Size(); i++)
                {
                    auto& e = tags->value[i];
                    if (e.IsString() && e.GetStringLength())
                        Tags.Add(Tags::Get(e.GetText()));
                }
            }
        }*/

        /*{
            const auto member = stream.FindMember("PrefabID");
            if (member != stream.MemberEnd())
            {
    #if USE_EDITOR
                if (_isPrefabRoot)
                {
                    ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
                    PrefabManager::PrefabsReferences[_prefabID].Remove(this);
                }
    #endif

                Serialization::Deserialize(member->value, m_PrefabID);
                m_IsPrefabRoot = 0;

                auto prefab = AssetContent::LoadAsync<Prefab>(m_PrefabID);
                if (prefab == nullptr || prefab->WaitForLoaded())
                {
                    m_PrefabID = UID::Empty;
                    m_PrefabObjectID = UID::Empty;
                    LOG_WARNING("Level", "Failed to load prefab linked to the actor on load.");
                }
                else if (prefab->GetRootObjectId() == m_PrefabObjectID)
                {
                    m_IsPrefabRoot = 1;
    #if SE_EDITOR
                    ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
                    PrefabManager::PrefabsReferences[_prefabID].Add(this);
    #endif
                }
            }
        }*/
    }
} // namespace SE
