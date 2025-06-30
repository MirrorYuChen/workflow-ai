cmake_minimum_required(VERSION 3.15)
project(llm_task LANGUAGES CXX)
set(LIBRARY_NAME llm_task)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if(Workflow_DIR)
	find_package(Workflow REQUIRED CONFIG HINTS ${Workflow_DIR})
else()
	find_package(Workflow REQUIRED CONFIG)
endif()

add_library(${LIBRARY_NAME} STATIC
	src/llm_client.cc
	src/chat_request.cc
	src/chat_response.cc
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
	INTERFACE_INCLUDE_DIRECTORIES "${Workflow_DIR}/_include"
)

target_link_libraries(${LIBRARY_NAME} PRIVATE workflow)

install(TARGETS ${LIBRARY_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")

add_executable(deepseek_chatbot examples/deepseek_chatbot.cc)
target_link_libraries(deepseek_chatbot PRIVATE
	pthread
	ssl
	crypto
	rt
	${LIBRARY_NAME}
	workflow
)

