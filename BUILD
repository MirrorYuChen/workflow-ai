load("@rules_cc//cc:defs.bzl", "cc_library", "cc_binary")

cc_library(
	name = "llm_task",
	srcs = [
		"src/chat_request.cc",
		"src/chat_response.cc",
		"src/llm_session.cc",
		"src/llm_client.cc",
		"src/llm_memory.cc",
		"src/llm_function.cc",
	],
	hdrs = [
		"src/llm_util.h",
		"src/chat_request.h",
		"src/chat_response.h",
		"src/llm_session.h",
		"src/llm_client.h",
		"src/llm_memory.h",
		"src/llm_function.h",
	],
	includes = ["src"],
	deps = [
		"@workflow//:http",
	],
	linkopts = [
		'-lpthread',
		'-lssl',
		'-lcrypto',
	],
	visibility = ["//visibility:public"],
)

EXAMPLES = [
	"sync_demo",
	"async_demo",
	"task_demo",
	"ollama_client",
	"deepseek_chatbot",
	"tool_call",
	"parallel_tool_call",
]

[cc_binary(
	name = example,
	srcs = ["examples/{}.cc".format(example)],
	deps = [
			":llm_task",
			"@workflow//:http",
			"@workflow//:workflow_hdrs"],
	linkopts = [
		'-lpthread',
		'-lssl',
		'-lcrypto',
	],
	visibility = ["//visibility:public"],
) for example in EXAMPLES]

