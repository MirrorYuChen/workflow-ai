#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "workflow/HttpMessage.h"
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
	if (task->get_state() != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "Task state: %d error: %d\n",
				task->get_state(), task->get_error());
		return;
	}

	if (!request->stream)
	{
		if (request->model == "deepseek-reasoner")
		{
			fprintf(stderr, "\n<think>\n%s\n<\\think>\n",
					response->choices[0].message.reasoning_content.c_str());
		}
		fprintf(stderr, "\n%s\n", response->choices[0].message.content.c_str());
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
				"	api_key - API KEY for LLM\n",
				argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	stop_flag = false;

	LLMClient client(argv[1]);

	wfai::ChatCompletionRequest request;
	request.model = "deepseek-reasoner";
	request.stream = true;
	request.messages.push_back({"user", "hi"});

	auto *task = client.create_chat_task(request, extract, callback);

	task->start();
	wait_group.wait();
	return 0;
}
