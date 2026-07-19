
add_subdirectory(Assets)
add_subdirectory(Utils)
add_subdirectory(GUI)
add_subdirectory(RenderPass)
add_subdirectory(2D)


RUNTIME_ADD(
        RenderContext.h
        RenderContext.cpp

        RenderEnum.h
        RendererPass.h
        RenderDrawCall.h

        Renderer.h
        Renderer.cpp
        RenderTask.h
        RenderTask.cpp
        RenderView.h
        RenderView.cpp
        RenderBuffers.h
        RenderBuffers.cpp

        RenderList.h
        RenderList.cpp

        SceneRendering.h
        SceneRendering.cpp

        PostProcessSettings.h
        PostProcessSettings.cpp
)
