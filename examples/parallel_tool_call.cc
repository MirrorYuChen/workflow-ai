#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <chrono>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "llm_client.h"

using namespace wfai;

volatile bool stop_flag;
WFFacilities::WaitGroup wait_group(1);
FunctionManager func_mgr;

void get_current_weather(const std::string& arguments, FunctionResult *result)
{
	fprintf(stderr, "function calling...get_current_weather()\n");
	fprintf(stderr, "parameters: %s\n", arguments.c_str());

	std::map<std::string, double> fake_weather_map {
		{"北京", 30},
		{"深圳", 28},
		{"昆明", 26}
	};

	result->name = "get_current_weather";
	result->success = false;

	// parse json
	char *json_buf = (char *)malloc(arguments.length() + 1);
	if (!json_buf)
	{
		result->error_message = "malloc failed";
		return;
	}

	memcpy(json_buf, arguments.data(), arguments.length());
	json_buf[arguments.length()] = '\0';
	json_value_t *root = json_value_parse(json_buf);
	free(json_buf);

	if (!root)
	{
		result->error_message = "parse json in response failed";
		return;
	}

	if (json_value_type(root) != JSON_VALUE_OBJECT)
	{
		result->error_message = "parse json in response failed";
		json_value_destroy(root);
		return;
	}

	const json_object_t *root_obj = json_value_object(root);
	const json_value_t *location_val = json_object_find("location", root_obj);
	if (!location_val || json_value_type(location_val) != JSON_VALUE_STRING)
	{
		result->error_message = "missing required parameter : location";
		json_value_destroy(root);
		return;
	}

	// check weather
	result->success = true;

	std::string location = json_value_string(location_val);
	if (fake_weather_map.find(location) == fake_weather_map.end())
	{
		result->result = "cannot find the weather of " + location;
		json_value_destroy(root);
		return;
	}

	double temperature = fake_weather_map[location];

	const json_value_t *unit_val = json_object_find("unit", root_obj);
	if (unit_val && json_value_type(unit_val) == JSON_VALUE_STRING)
	{
		std::string unit = json_value_string(unit_val);
		if (unit == "fahrenheit")
			temperature = temperature * 1.80 + 32.0;
	}
	result->result = std::to_string(temperature);
	json_value_destroy(root);
	return;
}

// 模拟查询时间的功能
void get_current_time(const std::string& arguments, FunctionResult *result)
{
	fprintf(stderr, "function calling...get_current_time()\n");
	fprintf(stderr, "parameters: %s\n", arguments.c_str());
	
	result->name = "get_current_time";
	result->success = true;

	// 简单返回当前时间戳
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	result->result = std::ctime(&time_t);

	// 移除换行符
	if (!result->result.empty() && result->result.back() == '\n')
		result->result.pop_back();
}

void register_local_functions()
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

	FunctionDefinition time_func = {
		.name = "get_time",
		.description = "获取当前时间",
	};

	func_mgr.register_function(time_func, get_current_time);

	fprintf(stderr, "registered weather and time functions successfully.\n");
}

void callback(WFHttpChunkedTask *task,
			  ChatCompletionRequest *request,
			  ChatCompletionResponse *response)
{
	protocol::HttpResponse *resp = task->get_resp();

	if (task->get_state() != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "Task state: %d error: %d\n",
				task->get_state(), task->get_error());
		wait_group.done();
		return;
	}

	fprintf(stderr, "Response status: %s\n", resp->get_status_code());

	if (!response->choices.empty() && !request->stream == true)
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
	register_local_functions();

	wfai::ChatCompletionRequest request;
	request.model = "deepseek-chat";
	request.messages.push_back({"system", "You are a helpful assistant"});

	// 提出一个需要并行调用多个工具的问题
	request.messages.push_back({"user", "请告诉我北京和深圳的天气情况，还有现在的时间"});

	auto *task = client.create_chat_with_tools(request, extract, callback);

	fprintf(stderr, "Starting parallel tool calls test...\n");
	task->start();
	wait_group.wait();

	return 0;
}
