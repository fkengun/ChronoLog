set(ARGOBOTS_FOUND TRUE)

    # Include directories
    find_path(Argobots_INCLUDE_DIRS abt.h PATH_SUFFIXES include/ NO_CACHE)
    if (NOT IS_DIRECTORY "${Argobots_INCLUDE_DIRS}")
	    set(ARGOBOTS_FOUND FALSE)
    else ()
	find_path(Argobots_LIBRARY_PATH libabt.so PATH_SUFFIXES lib/)
	set(Argobots_LIBRARIES ${Argobots_LIBRARY_PATH}/libabt.so)
	message(${Argobots_INCLUDE_DIRS})
	message(${Argobots_LIBRARY_PATH})
	#add_library(Argobots STATIC IMPORTED)
	link_directories(${Argobots_LIBRARY_PATH})
	#set_target_properties(Argobots PROPERTIES IMPORTED_LOCATION ${Argobots_LIBRARY_PATH})
	#add_library(Argobots::Argobots ALIAS Argobots)
	#target_include_directories(Argobots INTERFACE ${Argobots_INCLUDE_DIRS})
	#target_link_libraries(Argobots INTERFACE -L${Argobots_LIBRARY_PATH})
	#target_compile_options(Argobots INTERFACE ${argobots_DEFINITIONS})
    endif ()
    include(FindPackageHandleStandardArgs)
    # handle the QUIETLY and REQUIRED arguments and set ortools to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(Argobots
	    REQUIRED_VARS ARGOBOTS_FOUND Argobots_LIBRARIES Argobots_INCLUDE_DIRS)
