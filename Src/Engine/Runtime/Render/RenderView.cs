namespace SE
{
    /// <summary>
    /// Managed render-view operations matching the native implementation.
    /// </summary>
    public unsafe partial struct RenderView
    {
        /// <summary>Updates the cached view matrices and frustums.</summary>
        public void UpdateCachedData()
        {
            Matrix.Invert(ref View, out IV);
            Matrix.Invert(ref Projection, out IP);
            Matrix.Multiply(ref View, ref Projection, out Matrix viewProjection);
            frustum = new BoundingFrustum(ref viewProjection);
            Matrix.Invert(ref viewProjection, out IVP);
            cullingFrustum = frustum;
            NonJitteredProjection = Projection;
        }

        /// <summary>Copies camera parameters into this render view.</summary>
        public void CopyFrom(Camera camera, ref Viewport viewport)
        {
            Float3 cameraPos = camera.Position;
            // LargeWorlds.UpdateOrigin(ref Origin, cameraPos);
            Position = cameraPos - Origin;
            Direction = camera.Forward;
            Near = camera.NearPlane;
            Far = camera.FarPlane;
            camera.GetMatrices(ref View, ref Projection, viewport, Origin);
            NonJitteredProjection = Projection;
            TemporalAAJitter = Float4.Zero;
            RenderLayersMask = camera.RenderLayersMask;
            // Flags = camera.RenderFlags;
            Mode = camera.RenderMode;
            UpdateCachedData();
        }
    }
}
