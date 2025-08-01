#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "llm_client.h"

using namespace wfai;

volatile bool stop_flag = false;
FunctionManager func_mgr;

void get_current_weather(const std::string& arguments, FunctionResult *result)
{
	printf("function calling... get_current_weather(%s)\n", arguments.c_str());
	result->name = "get_current_weather";
	result->success = true;
	result->result = "Temperature: 25°C, Sunny"; // whaterver the location is
}

void register_functions()
{
	FunctionDefinition weather_func = {
		.name = "get_weather",
		.description = "Get ",
	};
	
	ParameterProperty location_prop = {
		.type = "string",
		.description = "Name of the City, e.g. Beijing, Shenzhen.",
	};
	
	weather_func.add_parameter("location", location_prop, true);
	func_mgr.register_function(weather_func, get_current_weather);
	
	printf("registered weather function successfully.\n");
}

void sig_handler(int signo)
{
	stop_flag = true;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <api_key>\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	LLMClient client(argv[1]);
	client.set_function_manager(&func_mgr);
	register_functions();

	printf("=== Synchronous LLMClient DEMO ===\n\n");

	printf("1. Basic Chat in NO Streaming Mode:\n");
	{
		ChatCompletionRequest request;
		request.model = "deepseek-chat";
		request.messages.push_back({"user", "hi"});
		ChatCompletionResponse response;

		auto result = client.chat_completion_sync(request, response);
		
		if (result.success) {
			printf("✓ Request Success (Status Code : %d)\n", result.status_code);
			printf("Response : %s\n\n", response.choices[0].message.content.c_str());
		} else {
			printf("✗ Request Failed : %s\n\n", result.error_message.c_str());
		}
	}

	printf("2. Basic Chat in Streaming Mode:\n");
	{
		ChatCompletionRequest request;
		request.model = "deepseek-chat";
		request.stream = true;
		request.messages.push_back({"user", "What is your tokenizer?"});
		ChatCompletionResponse response;

		auto stream_callback = [](const ChatCompletionChunk& chunk) {
			if (!chunk.choices.empty() && !chunk.choices[0].delta.content.empty())
			{
				fprintf(stderr, "%s", chunk.choices[0].delta.content.c_str());
			}
		};

		auto result = client.chat_completion_sync(request, response, stream_callback);

		printf("\n");
		if (result.success)
		{
			printf("✓ Request Success (Status Code : %d)\n\n", result.status_code);
		} else {
			printf("✗ Request Failed : %s\n\n", result.error_message.c_str());
		}
	}

	printf("3. Chat With Tool Calls:\n");
	{
		ChatCompletionRequest request;
		request.model = "deepseek-chat";
		request.stream = false;
		request.tool_choice = "auto";  // enable tool calls
		request.messages.push_back({"user", "What's the weather in Shenzhen?"});
		ChatCompletionResponse response;

		auto result = client.chat_completion_sync(request, response);

		if (result.success)
		{
			printf("✓ Request Success (Status Code : %d)\n", result.status_code);
			printf("Response : %s\n\n", response.choices[0].message.content.c_str());
		} else {
			printf("✗ Request Failed : %s\n\n", result.error_message.c_str());
		}
	}

	return 0;
}
