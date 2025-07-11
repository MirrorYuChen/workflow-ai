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

	if (!request->stream && response && !response->choices.empty())
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
		if (!chunk->choices[0].delta.tool_calls.empty())
		{
			fprintf(stderr, "Tool call detected in stream:\n");
			for (const auto& tc : chunk->choices[0].delta.tool_calls)
			{
				fprintf(stderr, "Tool ID: %s\n", tc.id.c_str());
				fprintf(stderr, "Function: %s\n", tc.function.name.c_str());
				fprintf(stderr, "Arguments: %s\n", tc.function.arguments.c_str());
				fprintf(stderr, "----------------------------\n");
			}
		}
	
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

	wfai::ChatCompletionRequest request;
	request.model = "deepseek-chat";
	request.stream = false;
	request.messages.push_back({"system", "You are a helpful assistant"});
	request.messages.push_back({"user", "深圳现在的天气怎么样？"});

	// 创建天气工具
	wfai::Tool weather_tool;
	weather_tool.type = "function";
	weather_tool.function.name = "get_current_weather";
	weather_tool.function.description = "获取指定地点的当前天气信息";
	
	// 设置参数模式
	weather_tool.function.parameters.type = "object";
	
	// 添加位置参数属性
	wfai::Property location_prop;
	location_prop.type = "string";
	location_prop.description = "城市或地区名称，例如：'北京市'、'New York'";
	weather_tool.function.parameters.properties["location"] = location_prop;

	// 添加单位参数属性
	wfai::Property unit_prop;
	unit_prop.type = "string";
	unit_prop.description = "温度单位，默认使用摄氏度";
	unit_prop.enum_values = {"celsius", "fahrenheit"};
	unit_prop.default_value = "celsius";
	weather_tool.function.parameters.properties["unit"] = unit_prop;

	// 设置必需参数
	weather_tool.function.parameters.required = {"location"};

	request.tools.push_back(weather_tool);
	request.tool_choice = "auto";

	fprintf(stderr, "Sending request with tool definition...\n");

	auto *task = client.create_chat_task(request, extract, callback);

	task->start();
	wait_group.wait();

	return 0;
}
