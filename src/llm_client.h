#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include "workflow/WFHttpChunkedClient.h"
#include "workflow/WFFuture.h"
#include "workflow/Workflow.h"
#include "workflow/msgqueue.h"
#include "llm_util.h"
#include "chat_response.h"
#include "chat_request.h"
#include "llm_function.h"

namespace wfai {

class SessionContext;

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

	///// Synchronous APIs /////
	struct SyncResult
	{
		bool success;
		int status_code;
		std::string error_message;
		ChatCompletionResponse response;

		SyncResult() : success(false), status_code(0) {}
	};

	SyncResult chat_completion_sync(ChatCompletionRequest& request,
									ChatCompletionResponse& response);
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

class SessionContext
{
public:
	ChatCompletionRequest *req;
	ChatCompletionResponse *resp;
	LLMClient::llm_extract_t extract;
	LLMClient::llm_callback_t callback;
	msgqueue_t *msgqueue;

public:
	SessionContext(ChatCompletionRequest *req,
				   ChatCompletionResponse *resp,
				   LLMClient::llm_extract_t extract,
				   LLMClient::llm_callback_t callback,
				   bool flag);

	~SessionContext();

	ChatCompletionChunk *get_chunk(); // get chunk for blocking call

private:
	bool flag; // whether ctx responsible for req and resp
};

} // namespace llm_client

#endif

