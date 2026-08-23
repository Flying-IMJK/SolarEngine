
add_subdirectory(Import)
add_subdirectory(Introducer)
add_subdirectory(Provider)
add_subdirectory(EntryItems)

# PARENT_SCOPE only one level
# this command is to pass the variable to parent
EDITOR_ADD(
	ResourceCompiler.h
	ResourceCompiler.cpp

	ResourceDatabase.h
	ResourceDatabase.cpp
	ResourcePicker.h
	ResourcePicker.cpp
	ResourceToolDefines.h
	ResourceImportSettings.h
	ResourceImportSettings.cpp
)