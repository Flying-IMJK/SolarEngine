#pragma once
#include "Core/Types/Property.h"
#include "Core/Types/Strings/String.h"
#include "Core/TypeSystem/IType.h"

namespace SE::Editor
{
	class ContextMenu;
	class ContentFolder;
	class ContentItem;
	class EditorWindow;
	class EditorApp;

	class ContentOperate : public IType
	{
		SE_CLASS_DEFAULT(ContentOperate, IType)
	public:
		~ContentOperate() override = default;

		/// <summary>
        /// Gets the asset type name (used by GUI, should be localizable).
        /// </summary>
		PRO_GET(Name, ContentOperate, String, __GetName);

        /// <summary>
        /// Gets the default name for the new items created by this proxy.
        /// </summary>
		PRO_GET(NewItemName, ContentOperate, String, __GetNewItemName);

        /// <summary>
        /// Determines whether this proxy is for the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if is proxy for asset item; otherwise, <c>false</c>.</returns>
	    virtual bool IsProxyFor(ContentItem* item) = 0;

        /// <summary>
        /// Determines whether this proxy is for the specified asset.
        /// </summary>
        /// <returns><c>true</c> if is proxy for asset item; otherwise, <c>false</c>.</returns>
	    virtual bool IsProxyFor(TypeID typeID)
        {
            return false;
        }

        /// <summary>
        /// Gets a value indicating whether this proxy if for assets.
        /// </summary>
		PRO_GET(IsAsset, ContentOperate, bool, __GetIsAsset);

        /// <summary>
        /// Gets the file extension used by the items managed by this proxy.
        /// ALL LOWERCASE! WITHOUT A DOT! example: for 'myFile.TxT' proper extension is 'txt'
        /// </summary>
		PRO_GET(FileExtension, ContentOperate, String, __GetFileExtension);


        /// <summary>
        /// Opens the specified item.
        /// </summary>
        /// <param name="editor"></param>
        /// <param name="item">The item.</param>
        /// <returns>Opened window or null if cannot do it.</returns>
		virtual EditorWindow* Open(const EditorApp* editor, ContentItem *item) = 0;

        /// <summary>
        /// Gets a value indicating whether content items used by this proxy can be exported.
        /// </summary>
        PRO_GET(CanExport, ContentOperate, bool, __GetCanExport);

        /// <summary>
        /// Exports the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="outputPath">The output path.</param>
		virtual void Export(ContentItem* item, String outputPath)
        {

        }

        /// <summary>
        /// Determines whether the specified filename is valid for this proxy.
        /// </summary>
        /// <param name="filename">The filename.</param>
        /// <returns><c>true</c> if the filename is valid, otherwise <c>false</c>.</returns>
        virtual bool IsFileNameValid(String filename)
        {
            return true;
        }

        /// <summary>
        /// Determines whether this proxy can create items in the specified target location.
        /// </summary>
        /// <param name="targetLocation">The target location.</param>
        /// <returns><c>true</c> if this proxy can create items in the specified target location; otherwise, <c>false</c>.</returns>
		virtual bool CanCreate(ContentFolder* targetLocation)
        {
            return false;
        }

        /// <summary>
        /// Determines whether this proxy can reimport specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if this proxy can reimport given item; otherwise, <c>false</c>.</returns>
		virtual bool CanReimport(ContentItem* item);

        /// <summary>
        /// Creates new item at the specified output path.
        /// </summary>
        /// <param name="outputPath">The output path.</param>
        /// <param name="arg">The custom argument provided for the item creation. Can be used as a source of data or null.</param>
		virtual void Create(String outputPath, void* arg)
        {

        }

        /// <summary>
        /// Called when content window wants to show the context menu. Allows to add custom functions for the given asset type.
        /// </summary>
        /// <param name="menu">The menu.</param>
        /// <param name="item">The item.</param>
		virtual void OnContentWindowContextMenu(ContextMenu* menu, ContentItem* item)
        {
        }

        /// <summary>
        /// Gets the unique accent color for that asset type.
        /// </summary>
        PRO_GET(AccentColor, ContentOperate, Color, __GetAccentColor);

        /// <summary>
        /// Releases resources and unregisters the proxy utilities. Called during editor closing. For custom proxies from game/plugin modules it should be called before scripting reload.
        /// </summary>
		virtual void Dispose()
        {
        }

	protected:
		virtual Color __GetAccentColor() = 0;
		virtual bool __GetCanExport()
		{
			return false;
		}
		virtual String __GetFileExtension() = 0;
		virtual bool __GetIsAsset()
		{
			return false;
		}
		virtual String __GetNewItemName()
		{
			return __GetName();
		};
		virtual String __GetName() = 0;
	};

	class ContentOperateRegister
	{
		friend class AssetDatabaseModule;
	protected:
		static List<ContentOperateRegister*> registers;
	public:
		virtual ContentOperate* Create() = 0;

		virtual ~ContentOperateRegister() = default;
	};

	template <typename T>
	class TContentOperateRegister final : ContentOperateRegister
	{
		static_assert(TIsBaseOf<ContentOperate, T>::Value, "T is not derived from ContentOperate");
	public:
		TContentOperateRegister()
		{
			registers.Add(this);
		}

		ContentOperate* Create() override
		{
			return New<T>();
		}
	};

	// 注册 ContentOperate
	#define CONTENT_OPERATE_REGISTER(TypeName) ::SE::Editor::TContentOperateRegister<TypeName> TypeName##Register
} // SE

