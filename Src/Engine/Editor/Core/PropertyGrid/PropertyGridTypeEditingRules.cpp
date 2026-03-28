#include "PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    ENGINE_GLOBAL_REGISTRY(TypeEditingRulesFactory);

    //-------------------------------------------------------------------------

    TypeEditingRules* TypeEditingRulesFactory::TryCreateRules( EditorContext const* pEditorContext, IType* pTypeInstance )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pTypeInstance->GetType() == pCurrentFactory->GetSupportedTypeID() )
            {
                return pCurrentFactory->TryCreateRulesInternal( pEditorContext, pTypeInstance );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }
}