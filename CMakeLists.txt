cmake_minimum_required(VERSION 3.15)
project(workflow-ai LANGUAGES CXX)
set(LIBRARY_NAME workflow-ai)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if(Workflow_DIR)
	find_package(Workflow REQUIRED CONFIG HINTS ${Workflow_DIR})
	if(EXISTS "${Workflow_DIR}/_include" AND EXISTS "${Workflow_DIR}/_lib")
		message(STATUS "Using Workflow source directory: ${Workflow_DIR}")
		set(WORKFLOW_INCLUDE_DIR "${Workflow_DIR}/_include")
		set(WORKFLOW_LIB_DIR "${Workflow_DIR}/_lib")
	else()
		if(NOT DEFINED WORKFLOW_INCLUDE_DIR)
			set(WORKFLOW_INCLUDE_DIR "${Workflow_DIR}/include")
		endif()
		if (NOT DEFINED WORKFLOW_LIB_DIR)
			set(WORKFLOW_LIB_DIR "${Workflow_DIR}/lib64")
		endif()
		message(STATUS "Workflow include dir: ${WORKFLOW_INCLUDE_DIR}")
		message(STATUS "Workflow library dir: ${WORKFLOW_LIB_DIR}")
	endif()
else()
	find_package(Workflow REQUIRED CONFIG)
endif()

add_library(${LIBRARY_NAME} STATIC
	src/chat_request.cc
	src/chat_response.cc
	src/llm_session.cc
	src/llm_client.cc
	src/llm_memory.cc
	src/llm_function.cc
)

target_include_directories(${LIBRARY_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_library(WORKFLOW_LIB
	NAMES libworkflow.a workflow
	HINTS ${WORKFLOW_LIB_DIR}
	REQUIRED
)

add_library(workflow STATIC IMPORTED)
set_target_properties(workflow PROPERTIES
	IMPORTED_LOCATION "${WORKFLOW_LIB}"
	INTERFACE_INCLUDE_DIRECTORIES "${WORKFLOW_INCLUDE_DIR}"
)

target_link_libraries(${LIBRARY_NAME} PRIVATE workflow)

install(TARGETS ${LIBRARY_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")


set(LINK_LIBS)
if(APPLE)
	set(LINK_LIBS pthread ssl crypto ${LIBRARY_NAME} workflow)
else()
	set(LINK_LIBS pthread ssl crypto rt ${LIBRARY_NAME} workflow)
endif()

file(GLOB EXAMPLES_SRC "examples/*.cc")

foreach(EXAMPLE_FILE ${EXAMPLES_SRC})
	get_filename_component(EXE_NAME ${EXAMPLE_FILE} NAME_WE)
	add_executable(${EXE_NAME} ${EXAMPLE_FILE})
	target_link_libraries(${EXE_NAME} PRIVATE ${LINK_LIBS} ${LIBRARY_NAME} workflow)
endforeach()

