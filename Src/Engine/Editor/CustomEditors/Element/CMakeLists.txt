
add_subdirectory(Container)


# PARENT_SCOPE only one level
# this command is to pass the variable to parent
EDITOR_ADD(
    LabelElement.h
    LabelElement.cpp
    LayoutElements.h
    LayoutElements.cpp

    IFloatValueElement.h
    FloatValueElement.h
    FloatValueElement.cpp
)