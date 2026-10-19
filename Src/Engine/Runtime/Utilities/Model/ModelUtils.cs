using System;

namespace SE
{
    partial struct ModelUtilsOptions
    {

        private bool ShowGeometry
        {
            get
            {
                return Type == ModelUtilsModelType.Model || Type == ModelUtilsModelType.SkinnedModel
                            || Type == ModelUtilsModelType.Prefab;
            }
        }
        private bool ShowModel => Type == ModelUtilsModelType.Model || Type == ModelUtilsModelType.Prefab;
        private bool ShowSkinnedModel => Type == ModelUtilsModelType.SkinnedModel || Type == ModelUtilsModelType.Prefab;
        private bool ShowAnimation => Type == ModelUtilsModelType.Animation || Type == ModelUtilsModelType.Prefab;
        private bool ShowRootMotion => ShowAnimation && RootMotion != ModelUtilsRootMotionMode.None;
        private bool ShowSmoothingNormalsAngle => ShowGeometry && CalculateNormals;
        private bool ShowSmoothingTangentsAngle => ShowGeometry && CalculateTangents;
        private bool ShowGenerateLODs => ShowGeometry && GenerateLODs;
        private bool ShowFramesRange => ShowAnimation && Duration == ModelUtilsAnimationDuration.Custom;
        private bool ShowSplitting => Type != ModelUtilsModelType.Prefab;

        private static bool s_DefaultValid;
        private static ModelUtilsOptions s_Default;

        /// <summary>
        /// The default <see cref="Options"/>.
        /// </summary>
        public static ModelUtilsOptions Default
        {
            get
            {
                if (!s_DefaultValid)
                {
                    s_DefaultValid = true;
                    object obj = s_Default;
                    Utils.InitStructure(obj, typeof(ModelUtilsOptions));
                    s_Default = (ModelUtilsOptions)obj;
                }
                return s_Default;
            }
        }
    }
}
