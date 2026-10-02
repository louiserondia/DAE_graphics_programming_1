########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(pugixml_COMPONENT_NAMES "")
if(DEFINED pugixml_FIND_DEPENDENCY_NAMES)
  list(APPEND pugixml_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES pugixml_FIND_DEPENDENCY_NAMES)
else()
  set(pugixml_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(pugixml_PACKAGE_FOLDER_DEBUG "C:/Users/louis/.conan2/p/pugixdf108d6a0c1a2/p")
set(pugixml_BUILD_MODULES_PATHS_DEBUG )


set(pugixml_INCLUDE_DIRS_DEBUG )
set(pugixml_RES_DIRS_DEBUG )
set(pugixml_DEFINITIONS_DEBUG )
set(pugixml_SHARED_LINK_FLAGS_DEBUG )
set(pugixml_EXE_LINK_FLAGS_DEBUG )
set(pugixml_OBJECTS_DEBUG )
set(pugixml_COMPILE_DEFINITIONS_DEBUG )
set(pugixml_COMPILE_OPTIONS_C_DEBUG )
set(pugixml_COMPILE_OPTIONS_CXX_DEBUG )
set(pugixml_LIB_DIRS_DEBUG "${pugixml_PACKAGE_FOLDER_DEBUG}/lib")
set(pugixml_BIN_DIRS_DEBUG )
set(pugixml_LIBRARY_TYPE_DEBUG STATIC)
set(pugixml_IS_HOST_WINDOWS_DEBUG 1)
set(pugixml_LIBS_DEBUG pugixml)
set(pugixml_SYSTEM_LIBS_DEBUG )
set(pugixml_FRAMEWORK_DIRS_DEBUG )
set(pugixml_FRAMEWORKS_DEBUG )
set(pugixml_BUILD_DIRS_DEBUG )
set(pugixml_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(pugixml_COMPILE_OPTIONS_DEBUG)
if (NOT "${pugixml_COMPILE_OPTIONS_CXX_DEBUG}" STREQUAL "")
    list(APPEND pugixml_COMPILE_OPTIONS_DEBUG
        "$<$<COMPILE_LANGUAGE:CXX>:${pugixml_COMPILE_OPTIONS_CXX_DEBUG}>")
endif ()
if (NOT "${pugixml_COMPILE_OPTIONS_C_DEBUG}" STREQUAL "")
    list(APPEND pugixml_COMPILE_OPTIONS_DEBUG
        "$<$<COMPILE_LANGUAGE:C>:${pugixml_COMPILE_OPTIONS_C_DEBUG}>")
endif ()
set(pugixml_LINKER_FLAGS_DEBUG)
if (NOT "${pugixml_SHARED_LINK_FLAGS_DEBUG}" STREQUAL "")
    list(APPEND pugixml_LINKER_FLAGS_DEBUG
        "$<$<OR:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>>:${pugixml_SHARED_LINK_FLAGS_DEBUG}>")
endif ()
if (NOT "${pugixml_EXE_LINK_FLAGS_DEBUG}" STREQUAL "")
    list(APPEND pugixml_LINKER_FLAGS_DEBUG
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${pugixml_EXE_LINK_FLAGS_DEBUG}>")
endif ()


set(pugixml_COMPONENTS_DEBUG )