
#  imgui docking 1.89.9

add_subdirectory(misc)

CORE_ADD(
        imconfig.h
        imgui.h

        imgui_internal.h
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h

        imgui.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp

        imgui_impl_win32.h
        imgui_impl_win32.cpp

        imgui_demo.cpp
)