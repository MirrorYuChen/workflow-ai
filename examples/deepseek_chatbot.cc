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

#define MAX_CONTENT_LENGTH 1024

volatile bool stop_flag;
WFFacilities::WaitGroup wait_group(1);

struct example_context
{
	// for request
	LLMClient *client;
	std::string model;
	bool stream;

	// for printing response
	int state;
	enum
	{
		CHAT_STATE_BEGIN = 0,
		CHAT_STATE_REASONING,
		CHAT_STATE_CONTEXT,
		CHAT_STATE_FINISH,
	};

	void reset_state()
	{
		state = CHAT_STATE_BEGIN;
	}

	bool set_reasoning()
	{
		bool ret = false;

		if (state == CHAT_STATE_BEGIN)
			ret = true;

		state = CHAT_STATE_REASONING;
		return ret;
	}

	bool set_context()
	{
		bool ret = false;

		if (state == CHAT_STATE_BEGIN || state == CHAT_STATE_REASONING)
			ret = true;

		state = CHAT_STATE_CONTEXT;
		return ret;
	}

/*
	bool set_finish()
	{
		state = CHAT_STATE_FINISH;
	}
*/
};

void next_query(SeriesWork *series);

void sig_handler(int signo)
{
	stop_flag = true;
	wait_group.done();
}

void print_http_info(const protocol::HttpRequest *req,
					 const protocol::HttpResponse *resp)
{
	fprintf(stderr, "\n[HTTP REQUEST]\n");
	fprintf(stderr, "%s %s %s\r\n", req->get_method(),
									req->get_http_version(),
									req->get_request_uri());
	std::string name;
	std::string value;
	protocol::HttpHeaderCursor req_cursor(req);

	while (req_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());

	fprintf(stderr, "\n[HTTP RESPONSE]\n");

	fprintf(stderr, "%s %s %s\r\n", resp->get_http_version(),
									resp->get_status_code(),
									resp->get_reason_phrase());

	protocol::HttpHeaderCursor resp_cursor(resp);

	while (resp_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");
}

void print_llm_info(const ChatCompletionRequest *req,
					const ChatCompletionResponse *resp)
{
	if (!resp)
		return;

	fprintf(stderr, "\n[LLM INFO]\n");
	if (!resp->choices.empty() && resp->choices[0].finish_reason.empty())
	{
		fprintf(stderr, "finish_reason: %s\n",
				resp->choices[0].finish_reason.c_str());
	}

	const auto &usage = resp->usage;
	fprintf(stderr, "prompt_tokens: %d\n", usage.prompt_tokens);
	fprintf(stderr, "completion_tokens: %d\n", usage.completion_tokens);
	fprintf(stderr, "total_tokens: %d\n", usage.total_tokens);
	fprintf(stderr, "prompt_cache_hit_tokens: %d\n", usage.prompt_cache_hit_tokens);
	fprintf(stderr, "prompt_cache_miss_tokens: %d\n", usage.prompt_cache_miss_tokens);
	fprintf(stderr, "reasoning_tokens: %d\n", usage.reasoning_tokens);
}

void callback(WFHttpChunkedTask *task,
			  ChatCompletionRequest *request,
			  ChatCompletionResponse *response)
{
	if (task->get_state() == WFT_STATE_SUCCESS)
	{
		if (!request->stream && !response->choices.empty())
		{
			const auto& choice = response->choices[0];
			if (request->model == "deepseek-reasoner")
			{
				fprintf(stderr, "\n<think>\n%s\n<\\think>\n",
						choice.message.reasoning_content.c_str());
			}
			fprintf(stderr, "\n%s\n", choice.message.content.c_str());
		}

		print_llm_info(request, response);
	}
	else
	{
		fprintf(stderr, "Task state: %d error: %d\n",
				task->get_state(), task->get_error());
	}

	print_http_info(task->get_req(), task->get_resp());

	next_query(series_of(task));
}

void extract(WFHttpChunkedTask *task,
			 ChatCompletionRequest *request,
			 ChatCompletionChunk *chunk)
{
	if (task->get_state () != WFT_STATE_SUCCESS ||
		request->model != "deepseek-reasoner" ||
		chunk->choices.empty())
	{
		return;
	}

	example_context *ctx = (example_context *)series_of(task)->get_context();
	const auto& choice = chunk->choices[0];

	if (choice.delta.reasoning_content.length() &&
		choice.delta.content.length() == 0)
	{
		if (ctx->set_reasoning())
			fprintf(stderr, "\n<think>\n");
		fprintf(stderr, "%s", choice.delta.reasoning_content.c_str());
	}
	else if (choice.delta.reasoning_content.length() == 0 &&
			 choice.delta.content.length())
	{
		if (ctx->set_context())
			fprintf(stderr, "\n<\\think>\n");
		fprintf(stderr, "%s", choice.delta.content.c_str());
	}
	else if (choice.finish_reason.c_str())
	{
//		if (ctx->set_finish())
		fprintf(stderr, "\n");
	}
}

void next_query(SeriesWork *series)
{
	int len;
	char query[MAX_CONTENT_LENGTH];
	std::string body;
	example_context *ctx = (example_context *)series->get_context();

	fprintf(stderr, "Query> ");
	while (stop_flag == false && (fgets(query, MAX_CONTENT_LENGTH, stdin)))
	{
		len = strlen(query);
		if (len == 0 || strncmp(query, "\0", len) == 0)
		{
			fprintf(stderr, "Query> ");
			continue;
		}

		if (stop_flag == true)
			break;

		query[len - 1] = '\0';

		ChatCompletionRequest request;
		request.model = ctx->model;
		request.stream = ctx->stream;
		request.messages.push_back({"user", query});

		auto *next = ctx->client->create_chat_task(request, extract, callback);

		series->push_back(next);
		ctx->reset_state();
		break;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 4)
	{
		fprintf(stderr, "USAGE: %s <api_key> [model] [stream]\n"
				"	api_key - API KEY for DeepSeek\n"
				"	model - set 'deepseek-chat' or 'deepseek-reasoning'\n"
				"	stream - set 'true' for streaming. default: 'false'\n",
				argv[0]);
		exit(1);
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	stop_flag = false;

	LLMClient client(argv[1]);

	struct example_context ctx;
	ctx.client = &client;
	ctx.model = "deepseek-reasoning";
	ctx.stream = true;

	if (argc >= 3)
		ctx.model = argv[2];

	if (argc == 4)
	{
		if (strcasecmp(argv[3], "false") == 0)
		{
			ctx.stream = false;
		}
		else if (strcasecmp(argv[3], "true") != 0)
		{
			fprintf(stderr, "Error! Please set 'true' or 'false'.\n");
			exit(1);
		}
	}

	WFCounterTask *counter = WFTaskFactory::create_counter_task(
		0,
		[](WFCounterTask *task) { return next_query(series_of(task)); }
	);

	SeriesWork *series = Workflow::create_series_work(
		counter,
		[](const SeriesWork *series)
		{
			wait_group.done(); // wake up main thread when series finished
		}
	);

	series->set_context(&ctx);
	series->start();
	wait_group.wait(); // pause main thread

	return 0;
}

