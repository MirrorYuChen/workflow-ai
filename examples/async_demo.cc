#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "llm_client.h"

using namespace wfai;

volatile bool stop_flag = false;

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

	printf("=== Asynchronous LLMClient DEMO ===\n\n");

	printf("Async Streaming Mode (using AsyncResult):\n");
	{
		ChatCompletionRequest request;
		request.stream = true;
		request.max_tokens = 10;
		request.messages.push_back({"user", "What is your tokenizer?"});

		auto result = client.chat_completion_async(request);

		// ... you may do anything else until you need the data ...

		while (true)
		{
			ChatCompletionChunk *chunk = result.get_chunk();
			if (!chunk /*non-streaming*/ || chunk->state != RESPONSE_SUCCESS)
				break;

			if (!chunk->choices.empty() && !chunk->choices[0].delta.content.empty())
			{
				printf("%s", chunk->choices[0].delta.content.c_str());
				fflush(stdout);
			}

			if (chunk->last_chunk())
			{
				// print some other info
				break;
			}
		}

		// if non streaming, use this to get response
		// ChatCompletionResponse *response = result.get_response();

		printf("\n\n✓ Async streaming completed\n\n");
	}

	return 0;
}
