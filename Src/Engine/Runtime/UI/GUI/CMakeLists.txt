
add_subdirectory(Brushes)
add_subdirectory(Common)
add_subdirectory(Panels)

RUNTIME_ADD(
    Enums.h
    Enums.cs
    Control.h
    Control.cpp
    Control.Bounds.cpp
    Control.cs
    ContainerControl.h
    ContainerControl.cpp
    ContainerControl.cs
    ScrollableControl.h
    ScrollableControl.cpp
    ScrollableControl.cs
    RootControl.h
    RootControl.cpp
    RootControl.cs
    WindowRootControl.h
    WindowRootControl.cpp
    WindowRootControl.cs
    RenderOutputControl.cs

    Margin.h
    Margin.cpp
    DragData.h
    DragData.cs
    Tooltip.h
    Tooltip.cpp
    Tooltip.cs
    Style.h
    Style.cpp
    Style.cs
)
