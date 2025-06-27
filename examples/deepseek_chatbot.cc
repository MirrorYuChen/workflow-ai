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

using namespace llm_task;

volatile bool stop_flag;
WFFacilities::WaitGroup wait_group(1);

void callback(WFHttpChunkedTask *task,
			  ChatCompletionRequest *request,
			  ChatCompletionResponse *response)
{
	protocol::HttpRequest *req = task->get_req();
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	fprintf(stderr, "Task state: %d\n", state);

	if (state != WFT_STATE_SUCCESS)
	{
		fprintf(stderr, "Task error: %d\n", error);
		return;
	}

	std::string name;
	std::string value;

	fprintf(stderr, "%s %s %s\r\n", req->get_method(),
									req->get_http_version(),
									req->get_request_uri());

	protocol::HttpHeaderCursor req_cursor(req);

	while (req_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

	fprintf(stderr, "%s %s %s\r\n", resp->get_http_version(),
									resp->get_status_code(),
									resp->get_reason_phrase());

	protocol::HttpHeaderCursor resp_cursor(resp);

	while (resp_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

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
	fprintf(stderr, "reason content=%s content=%s\n",
			chunk->choices[0].delta.reasoning_content.c_str(),
			chunk->choices[0].delta.content.c_str());
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
	// TODO: init with conf
	// client.set_redirect_max();
	// client.set_ttft();

	llm_task::ChatCompletionRequest request;
	request.model = "deepseek-reasoner";
	request.stream = true;
	request.messages.push_back({"user", "hi"});

	auto *task = client.create_chat_task(request,
										 extract,
										 callback);

	task->start();
	wait_group.wait();

	return 0;
}
