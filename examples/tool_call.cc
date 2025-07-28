#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "llm_client.h"

using namespace wfai;

volatile bool stop_flag;
WFFacilities::WaitGroup wait_group(1);
FunctionManager func_mgr;

// 模拟本地查询天气的功能，比如：{"location":"深圳","unit":"celsius"}
FunctionResult get_current_weather(const std::string& arguments)
{
	fprintf(stderr, "function calling...get_current_weather()\n");
	fprintf(stderr, "parameters: %s\n", arguments.c_str());

	std::map<std::string, double> fake_weather_map {
		{"北京", 30},
		{"深圳", 28},
		{"上海", 25}
	};

	FunctionResult result;
	result.name = "get_current_time";
	result.success = false;

	// parse json
	char *json_buf = (char *)malloc(arguments.length() + 1);
	if (!json_buf)
	{
		result.error_message = "malloc failed";
		return result;
	}

	memcpy(json_buf, arguments.data(), arguments.length());
	json_buf[arguments.length()] = '\0';
	json_value_t *root = json_value_parse(json_buf);
	free(json_buf);

	if (!root || json_value_type(root) != JSON_VALUE_OBJECT)
	{
		result.error_message = "parse json in response failed";
		return result;
	}

	const json_object_t *root_obj = json_value_object(root);
	const json_value_t *location_val = json_object_find("location", root_obj);
	if (!location_val || json_value_type(location_val) != JSON_VALUE_STRING)
	{
		result.error_message = "missing required parameter : location";
		return result;
	}

	// check weather
	result.success = true;

	std::string location = json_value_string(location_val);
	if (fake_weather_map.find(location) == fake_weather_map.end())
	{
		result.result = "cannot find the weather of " + location;
		return result;
	}

	double temperature = fake_weather_map[location];

	const json_value_t *unit_val = json_object_find("unit", root_obj);
	if (unit_val && json_value_type(unit_val) == JSON_VALUE_STRING)
	{
		std::string unit = json_value_string(unit_val);
		if (unit == "fahrenheit")
			temperature = temperature * 1.80 + 32.0;
	}

	result.result = std::to_string(temperature);
	return result;
}

void register_local_function()
{
	FunctionDefinition weather_func = {
		.name = "get_weather",
		.description = "获取指定地点的当前天气信息",
	};

	ParameterProperty location_prop = {
		.type = "string",
		.description = "城市或地区名称，例如：'北京市'、'New York'",
	};

	ParameterProperty unit_prop = {
		.type = "string",
		.description = "温度单位，默认使用摄氏度",
		.enum_values = {"celsius", "fahrenheit"},
		.default_value = "celsius",
	};

	weather_func.add_parameter("location", location_prop, true);
	weather_func.add_parameter("unit", unit_prop, false);

	func_mgr.register_function(weather_func, get_current_weather);
	fprintf(stderr, "register weather function successfully.\n");
}

void callback(WFHttpChunkedTask *task,
			  ChatCompletionRequest *request,
			  ChatCompletionResponse *response)
{
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	fprintf(stderr, "Task state: %d\n", state);

	if (state != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "Task error: %d\n", error);
		wait_group.done();
		return;
	}

	fprintf(stderr, "Response status: %s\n", resp->get_status_code());

	if (!response->choices.empty())
	{
		if (!response->choices[0].message.tool_calls.empty())
		{
			fprintf(stderr, "\n=== Tool Calls Detected ===\n");
			for (const auto& tc : response->choices[0].message.tool_calls)
			{
				fprintf(stderr, "Tool ID: %s\n", tc.id.c_str());
				fprintf(stderr, "Type: %s\n", tc.type.c_str());
				fprintf(stderr, "Function: %s\n", tc.function.name.c_str());
				fprintf(stderr, "Arguments: %s\n", tc.function.arguments.c_str());
				fprintf(stderr, "----------------------------\n");
			}
		}

		if (!request->stream == true)
		{
			if (request->model == "deepseek-reasoner" &&
				!response->choices[0].message.reasoning_content.empty())
			{
				fprintf(stderr, "\n<think>\n%s\n<\\think>\n",
						response->choices[0].message.reasoning_content.c_str());
			}

			fprintf(stderr, "\nResponse Content:\n%s\n",
				response->choices[0].message.content.c_str());
		}
	}

	wait_group.done();
}

void extract(WFHttpChunkedTask *task,
			 ChatCompletionRequest *request,
			 ChatCompletionChunk *chunk)
{
	if (chunk && !chunk->choices.empty())
	{
		if (!chunk->choices[0].delta.reasoning_content.empty())
		{
			fprintf(stderr, "Reasoning: %s\n",
					chunk->choices[0].delta.reasoning_content.c_str());
		}
	
		if (!chunk->choices[0].delta.content.empty())
		{
			fprintf(stderr, "Content: %s\n",
					chunk->choices[0].delta.content.c_str());
		}
	}
}

void sig_handler(int signo)
{
	stop_flag = true;
	wait_group.done();
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s <api_key>\n"
				"	 api_key - API KEY for LLM\n",
				argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	stop_flag = false;

	LLMClient client(argv[1]);
	client.set_function_manager(&func_mgr);
	register_local_function();

	wfai::ChatCompletionRequest request;
	request.model = "deepseek-chat";
//	request.stream = true;
	request.messages.push_back({"system", "You are a helpful assistant"});
	request.messages.push_back({"user", "深圳现在的天气怎么样？"});

	auto *task = client.create_chat_with_tools(request, extract, callback);

	task->start();
	wait_group.wait();

	return 0;
}
