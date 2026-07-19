#pragma once

#include "DragEventArgs.h"
#include "Runtime/Core/Platform/Window.h"

namespace SE
{
    class DragData;
}

namespace SE::Editor
{
    class DragHelperBase
    {
    public:
        /// <summary>
        /// Gets a value indicating whether this instance has valid drag.
        /// </summary>
        virtual bool GetHasValidDrag() = 0;

        /// <summary>
        /// Gets the drag effect.
        /// </summary>
        virtual DragDropEffect GetEffect() = 0;

        /// <summary>
        /// Called when drag enters.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>True if handled.</returns>
        virtual bool OnDragEnter(DragData* data) = 0;

        /// <summary>
        /// Called when drag leaves.
        /// </summary>
        virtual void OnDragLeave() = 0;

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        virtual void OnDragDrop() = 0;

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        /// <param name="dragEventArgs">The <see cref="DragEventArgs"/> instance containing the event data.</param>
        virtual void OnDragDrop(Ref<DragEventArgs> dragEventArgs) = 0;
    };

    /// <summary>
    /// Base class for drag and drop operation helpers.
    /// </summary>
    /// <typeparam name="T">Type of the objects to collect from drag data.</typeparam>
    /// <typeparam name="U">Type of the drag-drop event.</typeparam>
    template<typename T, typename U, typename = typename TEnableIf<TIsBaseOf<DragEventArgs, U>::Value>::Type>
    class DragHelper : public DragHelperBase
    {
    public:
        /// <summary>
        /// The objects gathered.
        /// </summary>
        List<T*> Objects = List<T*>();

        /// <summary>
        /// Gets a value indicating whether this instance has valid drag data.
        /// </summary>
        bool GetHasValidDrag() override { return Objects.Count() > 0; }

        /// <summary>
        /// Gets the current drag effect.
        /// </summary>
        DragDropEffect GetEffect() override { return GetHasValidDrag() ? DragDropEffect::Move : DragDropEffect::None; }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">An item.</param>
        /// <returns>The data.</returns>
        virtual DragData* ToDragData(T* item) = 0;

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items array</param>
        /// <returns>The data.</returns>
        virtual DragData* ToDragData(List<T*>& items) = 0;

        /// <summary>
        /// Tries to parse the drag data.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty IEnumerable if cannot get any valid.</returns>
        virtual List<T*> FromDragData(DragData* data) = 0;

        /// <summary>
        /// Handler drag drop event.
        /// </summary>
        /// <param name="dragEventArgs">The drag event arguments.</param>
        /// <param name="item">The item.</param>
        virtual void DragDrop(U* dragEventArgs, List<T*>& item)
        {
        }

        /// <summary>
        /// Invalids the drag data.
        /// </summary>
        void InvalidDrag()
        {
            Objects.Clear();
        }

        /// <summary>
        /// Called when drag enters.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>True if drag event is valid and can be performed, otherwise false.</returns>
        bool OnDragEnter(DragData* data) override
        {
            ENGINE_ASSERT(data != nullptr && ValidateFunction.IsBinded());
            Objects.Clear();

            auto items = FromDragData(data);
            for (auto item :items)
            {
                if (ValidateFunction(item))
                    Objects.Add(item);
            }

            return GetHasValidDrag();
        }

        /// <summary>
        /// Called when drag leaves.
        /// </summary>
        void OnDragLeave() override
        {
            Objects.Clear();
        }

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        void OnDragDrop() override
        {
            if (GetHasValidDrag())
            {
                DragDrop(nullptr, Objects);
            }
            Objects.Clear();
        }

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        /// <param name="dragEventArgs">Arguments</param>
        void OnDragDrop(Ref<DragEventArgs> dragEventArgs) override
        {
            if (GetHasValidDrag())
            {
                U* dragEvent = ::SE::TypeTryCast<U>(dragEventArgs.get());
                DragDrop(dragEvent, Objects);
            }
            Objects.Clear();
        }

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragHelper(const Function<bool(T*)> &validateFunction) : ValidateFunction(validateFunction)
        {

        }

    protected:
        /// <summary>
        /// The validation function
        /// </summary>
        Function<bool(T*)> ValidateFunction;
    };

} // SE

