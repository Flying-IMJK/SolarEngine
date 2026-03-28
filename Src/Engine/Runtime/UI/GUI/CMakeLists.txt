
add_subdirectory(Brushes)
add_subdirectory(Common)
add_subdirectory(Panels)

RUNTIME_ADD(
    Enums.h
    Control.h
    Control.cpp
    Control.Bounds.cpp
    ContainerControl.h
    ContainerControl.cpp
    ScrollableControl.h
    ScrollableControl.cpp
    RootControl.h
    RootControl.cpp
    WindowRootControl.h
    WindowRootControl.cpp

    Margin.h
    Margin.cpp
    DragData.h
    Tooltip.h
    Tooltip.cpp
    Style.h
    Style.cpp
)
