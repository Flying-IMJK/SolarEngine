#pragma once
#include "DragHelper.hpp"

namespace SE::Editor
{
    /// <summary>
    /// Handles a list of <see cref="DragHelper{T, U}"/>s
    /// </summary>
    class DragHandlers : public List<DragHelperBase*>
    {
    public:
        /// <summary>
        /// Called when drag enter.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>The result.</returns>
        DragDropEffect OnDragEnter(DragData* data);

        /// <summary>
        /// Called when drag leaves.
        /// </summary>
        void OnDragLeave();

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        /// <param name="dragEventArgs">The <see cref="DragEventArgs"/> instance containing the event data.</param>
        void OnDragDrop(DragEventArgs* dragEventArgs);

        /// <summary>
        /// Determines whether has valid drag handler to handle the drag request.
        /// </summary>
        bool GetHasValidDrag();

        /// <summary>
        /// Gets the first valid drag helper.
        /// </summary>
        /// <returns>The drag helper.</returns>
        DragHelperBase* WithValidDrag();

        /// <summary>
        /// Gets the valid drag effect to use.
        /// </summary>
        DragDropEffect GetEffect();
    };
}
