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
void get_current_weather(const std::string& arguments, FunctionResult *result)
{
	fprintf(stderr, "function calling...get_current_weather()\n");
	fprintf(stderr, "parameters: %s\n", arguments.c_str());

//  Fill function result here
//	result->success = true;
//	result->result = "31.415926";
	return;
}

void register_local_function()
{
	FunctionDefinition weather_func = {
		.name = "weather_function",
		.description = "获取指定地点的当前天气信息",
	};

	ParameterProperty location_prop = {
		.type = "string",
		.description = "城市或地区名称，例如：'北京市'、'New York'",
	};

	weather_func.add_parameter("location", location_prop, true);
	func_mgr.register_function(weather_func, get_current_weather);
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
	if (strcmp(resp->get_status_code(), "200") != 0 || response->state)
	{
		fprintf(stderr, "LLM response state: %d\n", response->state);
		if (!response->error.empty())
			fprintf(stderr, "LLM Error Message: %s\n", response->error.c_str());
		wait_group.done();
		return;
	}

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
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	stop_flag = false;

	LLMClient client("", "http://localhost:11434/v1/chat/completions");
	client.set_function_manager(&func_mgr);
	register_local_function(); // make sure we have manager and functions

	wfai::ChatCompletionRequest request;
	request.model = "llama3.1:8b";
	request.stream = false;
	request.messages.push_back({"system", "You are a helpful assistant"});
	request.messages.push_back({"user", "深圳现在的天气怎么样？"});
	request.tool_choice = "auto"; // enable to use tools

	auto *task = client.create_chat_task(request, extract, callback);

	task->start();
	wait_group.wait();
	return 0;
}
