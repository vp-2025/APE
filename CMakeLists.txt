cmake_minimum_required(VERSION 3.24)
project(ape++)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../binClion)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
	# /std:c++20 implies conformance mode, which drops the MSVC extension the
	# "string() << ..." idiom needs (binding a temporary to std::string&)
	add_compile_options(/Zc:referenceBinding-)
	# "using namespace std" + the SDK's byte typedef conflict with std::byte in c++17+
	add_definitions(-D_HAS_STD_BYTE=0)
endif()

add_definitions(-DWIN32 -DSCI_LEXER) # -D_WINDOWS -DUNICODE -D_UNICODE
add_definitions(-DCLION)
include_directories(. lib flex lexBase lexers dialogs c:/boost scintilla/include scintilla/src scintilla/lexlib)

file(GLOB LOCAL_FILES "*.cpp" )
file(GLOB DLG_FILES "dialogs/*.cpp" )
file(GLOB LEXBASE_FILES "lexBase/*.cpp" )
file(GLOB LEXERS_FILES "lexers/*.cpp" )
file(GLOB LIB_FILES "lib/*.cpp" )
file(GLOB SHARED_FILES "shared/*.cpp" )
file(GLOB SCI_LEXLIB_FILES "scintilla/lexlib/*.cxx" )
file(GLOB SCI_SRC_FILES "scintilla/src/*.cxx" )
file(GLOB SCI_WIN32_FILES "scintilla/win32/*.cxx" )

add_executable(ape++ WIN32 ${LOCAL_FILES} ${DLG_FILES} ${LEXBASE_FILES} ${LEXERS_FILES} ${LIB_FILES} ${SHARED_FILES}
	${SCI_LEXLIB_FILES} ${SCI_SRC_FILES} ${SCI_WIN32_FILES} ape++.rc shared/cfile.h)

set_property(TARGET ape++ PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

SET_TARGET_PROPERTIES(ape++ PROPERTIES LINK_FLAGS " /MANIFEST:NO") # manifest is in resources
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(ape++ PROPERTIES OUTPUT_NAME ape++D)
endif()

set(APE_DEPLOY_DIR "C:/tools/ape++" CACHE PATH "Release exe is copied here after the build")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_command(TARGET ape++ POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${APE_DEPLOY_DIR}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ape++>" "${APE_DEPLOY_DIR}/ape++.exe"
		COMMENT "Copying ape++ to ${APE_DEPLOY_DIR}"
		VERBATIM)
endif()
