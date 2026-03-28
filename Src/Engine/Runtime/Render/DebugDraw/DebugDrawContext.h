#pragma once

#include "Core/Types/Variable.h"
#include "Core/Containers/List.h"

#include "DebugDrawGroup.h"

namespace SE
{
    class DebugDrawContext
    {
    public:
        List<DebugDrawGroup*> m_DebugDrawGroups;
        DebugDrawGroup* TryGetOrCreateDebugDrawGroup(const String& name);
        void Clear();
        void Tick(float delta_time);
    
    private:
        std::mutex m_Mutex;
        void RemoveDeadPrimitives(float delta_time);
    };

}