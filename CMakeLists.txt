cmake_minimum_required(VERSION 3.18)

project("VIS-OSG-Qt")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# <dep: Qt>
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package("Qt5" COMPONENTS "Widgets" REQUIRED)
# </dep: Qt>

# <dep: OSG>
set(OSG_ROOT "<NOT-FOUND>" CACHE PATH "Root of OpenSceneGraph library")
set(OSG_ROOT_DBG "<NOT-FOUND>" CACHE PATH "Root of OpenSceneGraph library (Debug)")
if (${OSG_ROOT} STREQUAL "<NOT-FOUND>")
	message(FATAL_ERROR "OSG_ROOT NOT set!")
endif()
if (${OSG_ROOT_DBG} STREQUAL "<NOT-FOUND>")
	message(FATAL_ERROR "OSG_ROOT_DBG NOT set!")
endif()

list(
	APPEND
	OSG_LIBS
	"osg"
	"osgViewer"
	"osgDB"
	"osgGA"
)

if ((${CMAKE_BUILD_TYPE} AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug") OR
	${CMAKE_CONFIGURATION_TYPES} STREQUAL "Debug"
	)
	set(OSG_INC_DIR "${OSG_ROOT_DBG}/include")
	set(OSG_LIB_DIR "${OSG_ROOT_DBG}/lib")

	foreach(OSG_LIB ${OSG_LIBS})
		list(APPEND NEW_LIBS "${OSG_LIB}d")
	endforeach()
	set(OSG_LIBS ${NEW_LIBS})
else()
	set(OSG_INC_DIR "${OSG_ROOT}/include")
	set(OSG_LIB_DIR "${OSG_ROOT}/lib")
endif()
# </dep: OSG>

# <lib: CommonGUI>
file(GLOB COMM_GUI_HEADER_SRC "${CMAKE_CURRENT_LIST_DIR}/include/common_gui/*.h")
file(GLOB COMM_GUI_UI_SRC "${CMAKE_CURRENT_LIST_DIR}/include/common_gui/*.ui")
# </lib: CommonGUI>

# <lib: SciVis>
add_compile_definitions(
	SCIVIS_SHADER_PREFIX="${CMAKE_CURRENT_LIST_DIR}/shader/"
)
# </lib: SciVis>

# <app>
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")
file(GLOB APP_PATHS "${CMAKE_CURRENT_LIST_DIR}/app/*" LIST_DIR)
foreach(APP_PATH ${APP_PATHS})
	get_filename_component(APP ${APP_PATH} NAME)
	message(STATUS "Found app: ${APP}")

	set(TARGET_NAME ${APP})
	file(GLOB HEDAER_SRCS "${APP_PATH}/*.h")
	file(GLOB SRCS "${APP_PATH}/*.cpp")
	file(GLOB UI_SRCS "${APP_PATH}/*.ui")
	add_executable(
		${APP}
		${UI_SRCS} ${COMM_GUI_UI_SRC} # Necessary! For UIC
		${HEDAER_SRCS} ${COMM_GUI_HEADER_SRC} # Necessary! For header only target, Qt needs to MOC its headers
		${SRCS}
	)
	target_include_directories(
		${TARGET_NAME}
		PRIVATE
		"${CMAKE_CURRENT_LIST_DIR}/include"
		${OSG_INC_DIR}
	)
	target_link_directories(
		${TARGET_NAME}
		PRIVATE
		${OSG_LIB_DIR}
	)
	target_link_libraries(
		${APP}
		PRIVATE
		"Qt5::Widgets"
		${OSG_LIBS}
	)
endforeach()
# </app>
