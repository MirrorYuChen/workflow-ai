#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include "workflow/WFHttpChunkedClient.h"
#include "workflow/Workflow.h"
#include "llm_util.h"
#include "chat_response.h"
#include "chat_request.h"
#include "llm_function.h"

namespace wfai {

class LLMClient
{
public:
	///// Asynchronous APIs /////
	using llm_extract_t = std::function<void(WFHttpChunkedTask *,
											 ChatCompletionRequest *,
											 ChatCompletionChunk *)>;

	using llm_callback_t = std::function<void(WFHttpChunkedTask *,
											  ChatCompletionRequest *,
											  ChatCompletionResponse *)>;

	using extract_t = std::function<void (WFHttpChunkedTask *)>;
	using callback_t = std::function<void (WFHttpChunkedTask *)>;

	WFHttpChunkedTask *create_chat_task(ChatCompletionRequest& request,
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
							  ChatCompletionResponse *resp,
							  llm_extract_t extract,
							  llm_callback_t callback);

	// TODO: optimize the parameter by context
	void extract(WFHttpChunkedTask *task,
				 ChatCompletionRequest *req,
				 ChatCompletionResponse *resp,
				 llm_extract_t extract);

	void callback(WFHttpChunkedTask *task,
				  ChatCompletionRequest *req,
				  ChatCompletionResponse *resp,
				  llm_extract_t extract, // useless here
				  llm_callback_t callback);

	void callback_with_tools(WFHttpChunkedTask *task,
							 ChatCompletionRequest *req,
							 ChatCompletionResponse *resp,
							 llm_extract_t extract,
							 llm_callback_t callback);

	void tool_calls_callback(WFGoTask *task);
	void parallel_tool_calls_callback(const ParallelWork *pwork);

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

