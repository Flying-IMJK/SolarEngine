#pragma once
#include "ViewportCamera.h"
#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE::Editor
{
    /// <summary>
    /// Implementation of <see cref="ViewportCamera"/> that simulated the first-person camera which can fly though the scene.
    /// </summary>
    class FPSCamera : public ViewportCamera
    {
    private:
        Transform _startMove;
        Transform _endMove;
        float _moveStartTime = -1;

    public:
        /// <summary>
        /// Gets a value indicating whether this viewport is animating movement.
        /// </summary>
        bool GetIsAnimatingMove();

        /// <summary>
        /// The target point location. It's used to orbit around it when user clicks Alt+LMB.
        /// </summary>
        Float3 TargetPoint = Float3(-200);

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="direction">The view direction.</param>
        void SetView(Float3 position, Float3 direction);

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="position">The view position.</param>
        /// <param name="orientation">The view rotation.</param>
        void SetView(Float3 position, Quaternion orientation);

        /// <summary>
        /// Start animating viewport movement to the target transformation.
        /// </summary>
        /// <param name="position">The target position.</param>
        /// <param name="orientation">The target orientation.</param>
        void MoveViewport(Float3 position, Quaternion orientation);

        /// <summary>
        /// Start animating viewport movement to the target transformation.
        /// </summary>
        /// <param name="target">The target transform.</param>
        void MoveViewport(Transform target);

        /// <inheritdoc />
        void ShowSphere(BoundingSphere& sphere, Quaternion& orientation) override;

        /// <inheritdoc />
        void SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius) override;

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        void UpdateView(float dt, const Float3& moveDelta, const Float2& mouseDelta, bool& centerMouse) override;

    };
} // SE
