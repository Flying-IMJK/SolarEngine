
add_subdirectory(Collections)
add_subdirectory(Strings)

RUNTIME_ADD(
		Variable.h
		Variable.cpp
		BitFlags.h
		BitFlagsUtility.h
		Property.h

		Delegate.h
		LoadingStatus.h
		Event.h
		Pair.h
		Hash.h
		Hash.cpp

		DateTime.h
		DateTime.cpp
		TimeSpan.h
		TimeSpan.cpp
		Stopwatch.h

        UID.h
        UID.cpp
		Object.h
		Object.cpp

#		CS
		Object.cs
)