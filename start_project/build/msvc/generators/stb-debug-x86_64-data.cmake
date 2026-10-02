########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(stb_COMPONENT_NAMES "")
if(DEFINED stb_FIND_DEPENDENCY_NAMES)
  list(APPEND stb_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES stb_FIND_DEPENDENCY_NAMES)
else()
  set(stb_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(stb_PACKAGE_FOLDER_DEBUG "C:/Users/louis/.conan2/p/stb19b77fb56ffd7/p")
set(stb_BUILD_MODULES_PATHS_DEBUG )


set(stb_INCLUDE_DIRS_DEBUG "${stb_PACKAGE_FOLDER_DEBUG}/include")
set(stb_RES_DIRS_DEBUG )
set(stb_DEFINITIONS_DEBUG "-DSTB_TEXTEDIT_KEYTYPE=unsigned")
set(stb_SHARED_LINK_FLAGS_DEBUG )
set(stb_EXE_LINK_FLAGS_DEBUG )
set(stb_OBJECTS_DEBUG )
set(stb_COMPILE_DEFINITIONS_DEBUG "STB_TEXTEDIT_KEYTYPE=unsigned")
set(stb_COMPILE_OPTIONS_C_DEBUG )
set(stb_COMPILE_OPTIONS_CXX_DEBUG )
set(stb_LIB_DIRS_DEBUG )
set(stb_BIN_DIRS_DEBUG )
set(stb_LIBRARY_TYPE_DEBUG UNKNOWN)
set(stb_IS_HOST_WINDOWS_DEBUG 1)
set(stb_LIBS_DEBUG )
set(stb_SYSTEM_LIBS_DEBUG )
set(stb_FRAMEWORK_DIRS_DEBUG )
set(stb_FRAMEWORKS_DEBUG )
set(stb_BUILD_DIRS_DEBUG )
set(stb_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(stb_COMPILE_OPTIONS_DEBUG)
if (NOT "${stb_COMPILE_OPTIONS_CXX_DEBUG}" STREQUAL "")
    list(APPEND stb_COMPILE_OPTIONS_DEBUG
        "$<$<COMPILE_LANGUAGE:CXX>:${stb_COMPILE_OPTIONS_CXX_DEBUG}>")
endif ()
if (NOT "${stb_COMPILE_OPTIONS_C_DEBUG}" STREQUAL "")
    list(APPEND stb_COMPILE_OPTIONS_DEBUG
        "$<$<COMPILE_LANGUAGE:C>:${stb_COMPILE_OPTIONS_C_DEBUG}>")
endif ()
set(stb_LINKER_FLAGS_DEBUG)
if (NOT "${stb_SHARED_LINK_FLAGS_DEBUG}" STREQUAL "")
    list(APPEND stb_LINKER_FLAGS_DEBUG
        "$<$<OR:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>>:${stb_SHARED_LINK_FLAGS_DEBUG}>")
endif ()
if (NOT "${stb_EXE_LINK_FLAGS_DEBUG}" STREQUAL "")
    list(APPEND stb_LINKER_FLAGS_DEBUG
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${stb_EXE_LINK_FLAGS_DEBUG}>")
endif ()


set(stb_COMPONENTS_DEBUG )