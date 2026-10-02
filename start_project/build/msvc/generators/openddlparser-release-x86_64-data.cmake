########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(openddl-parser_COMPONENT_NAMES "")
if(DEFINED openddl-parser_FIND_DEPENDENCY_NAMES)
  list(APPEND openddl-parser_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES openddl-parser_FIND_DEPENDENCY_NAMES)
else()
  set(openddl-parser_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(openddl-parser_PACKAGE_FOLDER_RELEASE "C:/Users/louis/.conan2/p/opend71bc4a454f145/p")
set(openddl-parser_BUILD_MODULES_PATHS_RELEASE )


set(openddl-parser_INCLUDE_DIRS_RELEASE )
set(openddl-parser_RES_DIRS_RELEASE )
set(openddl-parser_DEFINITIONS_RELEASE "-DOPENDDL_STATIC_LIBARY")
set(openddl-parser_SHARED_LINK_FLAGS_RELEASE )
set(openddl-parser_EXE_LINK_FLAGS_RELEASE )
set(openddl-parser_OBJECTS_RELEASE )
set(openddl-parser_COMPILE_DEFINITIONS_RELEASE "OPENDDL_STATIC_LIBARY")
set(openddl-parser_COMPILE_OPTIONS_C_RELEASE )
set(openddl-parser_COMPILE_OPTIONS_CXX_RELEASE )
set(openddl-parser_LIB_DIRS_RELEASE "${openddl-parser_PACKAGE_FOLDER_RELEASE}/lib")
set(openddl-parser_BIN_DIRS_RELEASE )
set(openddl-parser_LIBRARY_TYPE_RELEASE STATIC)
set(openddl-parser_IS_HOST_WINDOWS_RELEASE 1)
set(openddl-parser_LIBS_RELEASE openddlparser)
set(openddl-parser_SYSTEM_LIBS_RELEASE )
set(openddl-parser_FRAMEWORK_DIRS_RELEASE )
set(openddl-parser_FRAMEWORKS_RELEASE )
set(openddl-parser_BUILD_DIRS_RELEASE )
set(openddl-parser_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(openddl-parser_COMPILE_OPTIONS_RELEASE)
if (NOT "${openddl-parser_COMPILE_OPTIONS_CXX_RELEASE}" STREQUAL "")
    list(APPEND openddl-parser_COMPILE_OPTIONS_RELEASE
        "$<$<COMPILE_LANGUAGE:CXX>:${openddl-parser_COMPILE_OPTIONS_CXX_RELEASE}>")
endif ()
if (NOT "${openddl-parser_COMPILE_OPTIONS_C_RELEASE}" STREQUAL "")
    list(APPEND openddl-parser_COMPILE_OPTIONS_RELEASE
        "$<$<COMPILE_LANGUAGE:C>:${openddl-parser_COMPILE_OPTIONS_C_RELEASE}>")
endif ()
set(openddl-parser_LINKER_FLAGS_RELEASE)
if (NOT "${openddl-parser_SHARED_LINK_FLAGS_RELEASE}" STREQUAL "")
    list(APPEND openddl-parser_LINKER_FLAGS_RELEASE
        "$<$<OR:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>>:${openddl-parser_SHARED_LINK_FLAGS_RELEASE}>")
endif ()
if (NOT "${openddl-parser_EXE_LINK_FLAGS_RELEASE}" STREQUAL "")
    list(APPEND openddl-parser_LINKER_FLAGS_RELEASE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${openddl-parser_EXE_LINK_FLAGS_RELEASE}>")
endif ()


set(openddl-parser_COMPONENTS_RELEASE )