load("@rules_cc//cc:defs.bzl", "cc_library", "cc_binary")

cc_library(
	name = "llm_task",
	srcs = [
		"src/llm_client.cc",
		"src/chat_request.cc",
		"src/chat_response.cc",
	],
	hdrs = [
		"src/llm_client.h",
		"src/chat_request.h",
		"src/chat_response.h",
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
