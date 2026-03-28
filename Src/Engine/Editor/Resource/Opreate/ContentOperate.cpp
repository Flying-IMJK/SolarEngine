
#include "ContentOperate.h"

#include "Editor/Resource/Items/ContentItem.h"

namespace SE::Editor
{
	bool ContentOperate::CanReimport(ContentItem* item)
	{
		return CanCreate(item->ParentFolder);
	}


	List<ContentOperateRegister*> ContentOperateRegister::registers;
} // SE