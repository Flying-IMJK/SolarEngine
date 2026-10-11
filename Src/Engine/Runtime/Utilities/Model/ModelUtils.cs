using System;

namespace SE
{
    partial struct ModelUtilsOptions
    {

        private bool ShowGeometry => Type == ModelUtilsModelType.Model || Type == ModelUtilsModelType.SkinnedModel || Type == ModelUtilsModelType.Prefab;
        private bool ShowModel => Type == ModelUtilsModelType.Model || Type == ModelUtilsModelType.Prefab;
        private bool ShowSkinnedModel => Type == ModelUtilsModelType.SkinnedModel || Type == ModelUtilsModelType.Prefab;
        private bool ShowAnimation => Type == ModelUtilsModelType.Animation || Type == ModelUtilsModelType.Prefab;
        private bool ShowRootMotion => ShowAnimation && RootMotion != ModelUtilsRootMotionMode.None;
        private bool ShowSmoothingNormalsAngle => ShowGeometry && CalculateNormals;
        private bool ShowSmoothingTangentsAngle => ShowGeometry && CalculateTangents;
        private bool ShowGenerateLODs => ShowGeometry && GenerateLODs;
        private bool ShowFramesRange => ShowAnimation && Duration == ModelUtilsAnimationDuration.Custom;
        private bool ShowSplitting => Type != ModelUtilsModelType.Prefab;

        private static bool m_DefaultValid;
        private static ModelUtilsOptions m_Default;

        /// <summary>
        /// The default <see cref="Options"/>.
        /// </summary>
        public static ModelUtilsOptions Default
        {
            get
            {
                if (!m_DefaultValid)
                {
                    m_DefaultValid = true;
                    object obj = m_Default;
                    Utils.InitStructure(obj, typeof(ModelUtilsOptions));
                    m_Default = (ModelUtilsOptions)obj;
                }
                return m_Default;
            }
        }
    }   
}
