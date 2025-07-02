load("@rules_cc//cc:defs.bzl", "cc_library", "cc_binary")

cc_library(
	name = "llm_task",
	srcs = [
		"src/chat_request.cc",
		"src/chat_response.cc",
		"src/llm_client.cc",
		"src/llm_memory.cc",
	],
	hdrs = [
		"src/chat_request.h",
		"src/chat_response.h",
		"src/llm_client.h",
		"src/llm_memory.h",
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

cc_binary(
	name = "demo",
	srcs = ["examples/demo.cc"],
	deps = [":llm_task",
			"@workflow//:http",
			"@workflow//:workflow_hdrs"],
	linkopts = [
		'-lpthread',
		'-lssl',
		'-lcrypto',
	],
	visibility = ["//visibility:public"],
)

cc_binary(
	name = "deepseek_chatbot",
	srcs = ["examples/deepseek_chatbot.cc"],
	deps = [":llm_task",
			"@workflow//:http",
			"@workflow//:workflow_hdrs"],
	linkopts = [
		'-lpthread',
		'-lssl',
		'-lcrypto',
	],
	visibility = ["//visibility:public"],
)
