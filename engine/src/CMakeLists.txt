include_directories(${PROJECT_SOURCE_DIR}/include)

include_directories(${STB_INCLUDE})

include_directories(${RAPIDJSON_INCLUDE})

include_directories(${SFML_INCLUDE})
link_directories(${SFML_LIB})

include_directories(${FBXSDK_INCLUDE})

if(OS_MACOSX)
	find_library(COREFOUNDATION_LIB CoreFoundation)
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIR})
endif()

file(GLOB_RECURSE FURY_SRC ${PROJECT_SOURCE_DIR} "*.cpp")

if(BUILD_SHARED_LIBS)
	add_library(fury SHARED ${FURY_SRC})
	if(OS_WINDOWS)
		target_link_libraries(fury sfml-window sfml-system opengl32 ${FBXSDK_LIB})
	elseif(OS_MACOSX)
		target_link_libraries(fury sfml-window sfml-system ${OPENGL_LIBRARIES} ${COREFOUNDATION_LIB} ${FBXSDK_LIB})
		set_target_properties(fury PROPERTIES BUILD_WITH_INSTALL_RPATH 1 INSTALL_NAME_DIR "@executable_path")
	endif()
else()
	add_library(fury STATIC ${FURY_SRC})
endif()

install(TARGETS fury DESTINATION lib)