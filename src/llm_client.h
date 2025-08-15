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
#include "llm_session.h"
#include "llm_function.h"

namespace wfai {

class LLMClient
{
public:
	///// Asynchronous APIs /////

	using extract_t = std::function<void (WFHttpChunkedTask *)>;
	using callback_t = std::function<void (WFHttpChunkedTask *)>;

	WFHttpChunkedTask *create_chat_task(ChatCompletionRequest& request,
										llm_extract_t extract,
										llm_callback_t callback);

	///// Synchronous APIs /////
	SyncResult chat_completion_sync(ChatCompletionRequest& request,
									ChatCompletionResponse& response);

	///// Asynchronous but blocking APIs /////
	AsyncResult chat_completion_async(ChatCompletionRequest& request);

public:
	LLMClient();
	LLMClient(const std::string& api_key);
	LLMClient(const std::string& api_key, const std::string& base_url);

	// Will add functions for all tasks from this client when tool_choice != 'none'
	void set_function_manager(FunctionManager *manager);

	bool register_function(const FunctionDefinition& function,
						   FunctionHandler handler);

public:
	WFHttpChunkedTask *create(SessionContext *ctx);

	void extract(WFHttpChunkedTask *task, SessionContext *ctx);

	void callback(WFHttpChunkedTask *task, SessionContext *ctx);

	void callback_with_tools(WFHttpChunkedTask *task, SessionContext *ctx);

	void tool_calls_callback(WFGoTask *task, SessionContext *ctx);
	void p_tool_calls_callback(const ParallelWork *pwork, SessionContext *ctx);

	void sync_callback(WFHttpChunkedTask *task,
					   ChatCompletionRequest *req,
					   ChatCompletionResponse *resp,
					   WFPromise<SyncResult> *promise);

	void async_callback(WFHttpChunkedTask *task,
						ChatCompletionRequest *req,
						ChatCompletionResponse *resp,
						SessionContext *ctx);

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

