#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include "workflow/WFHttpChunkedClient.h"
#include "llm_util.h"
#include "chat_response.h"
#include "chat_request.h"
#include "llm_function.h"

namespace wfai {

class LLMClient
{
public:
	using llm_extract_t = std::function<void(WFHttpChunkedTask *,
											 ChatCompletionRequest *,
											 ChatCompletionChunk *)>;

	using llm_callback_t = std::function<void(WFHttpChunkedTask *,
											  ChatCompletionRequest *,
											  ChatCompletionResponse *)>;

	using extract_t = std::function<void (WFHttpChunkedTask *)>;
	using callback_t = std::function<void (WFHttpChunkedTask *)>;

	WFHttpChunkedTask *create_chat_task(ChatCompletionRequest request,
										llm_extract_t extract,
										llm_callback_t callback);

	WFHttpChunkedTask *create_chat_with_tools(ChatCompletionRequest request,
											  llm_extract_t extract,
											  llm_callback_t callback);

public:
	LLMClient();
	LLMClient(const std::string& api_key);
	LLMClient(const std::string& api_key, const std::string& base_url);

	// Will add functions for all tasks from this client when tool_choice != 'none'
	void set_function_manager(FunctionManager *manager);

	bool register_function(const FunctionDefinition& function,
						   FunctionHandler handler);

private:
	WFHttpChunkedTask *create(ChatCompletionRequest *req,
							  extract_t extract,
							  callback_t callback);

	void extract(WFHttpChunkedTask *task,
				 ChatCompletionRequest *req,
				 ChatCompletionResponse *resp,
				 llm_extract_t extract);

	void callback(WFHttpChunkedTask *task,
				  ChatCompletionRequest *req,
				  ChatCompletionResponse *resp,
				  llm_callback_t callback);

private:
	WFHttpChunkedClient client;
	std::string api_key;
	std::string base_url;
	int ttft; // time to first token (seconds)
	int tpft; // time per output token (seconds)
	int streaming_ttft;
	int streaming_tpft;
	int redirect_max;
	FunctionManager *function_manager;
};

} // namespace llm_client

#endif

