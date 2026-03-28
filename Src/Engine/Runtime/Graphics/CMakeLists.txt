
add_subdirectory(Base)
#add_subdirectory(Compiler)
add_subdirectory(Textures)
add_subdirectory(Shaders)
add_subdirectory(Async)

RUNTIME_ADD(
        GlobalSettings_GPU.h
        GlobalSettings_GPU.cpp

        Viewport.h
        Viewport.cpp

        RenderTargetPool.h
        RenderTargetPool.cpp
        GPUDevice.h
        GPUDevice.cpp
        GPUContext.h
        GPUContext.cpp
        GPUResourceProperty.h

        GPUSwapChain.h
        GPUSwapChain.cpp

        DynamicBuffer.h
        DynamicBuffer.cpp

        GPUTimerQuery.h

        GraphicWindow.h
        GraphicWindow.cpp
)