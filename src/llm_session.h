#ifndef LLM_SESSION_H
#define LLM_SESSION_H

#include <string>
#include <atomic>
#include "workflow/WFFuture.h"
#include "workflow/msgqueue.h"
#include "llm_util.h"
#include "chat_response.h"
#include "chat_request.h"
#include "llm_function.h"

namespace wfai {

class AsyncResultPtr;

struct SyncResult
{
	bool success;
	int status_code;
	std::string error_message;
	ChatCompletionResponse response;

	SyncResult() : success(false), status_code(0) {}
};

class AsyncResult
{
public:
	// for users
	ChatCompletionChunk *get_chunk();
	ChatCompletionResponse *get_response();
	bool success() const;
	int status_code() const;
	const std::string& error_message() const;

	// for LLMClient
	void msg_queue_create(size_t len);

public:
	AsyncResult();
	~AsyncResult();

	AsyncResult(AsyncResult&& move);
	AsyncResult& operator=(AsyncResult&& move);
	AsyncResultPtr *get_ptr();

private:
	AsyncResultPtr *ptr;
};

class SessionContext
{
public:
	ChatCompletionRequest *req;
	ChatCompletionResponse *resp;
	llm_extract_t extract;
	llm_callback_t callback;

public:
	SessionContext(ChatCompletionRequest *req,
				   ChatCompletionResponse *resp,
				   llm_extract_t extract,
				   llm_callback_t callback,
				   bool flag);

	~SessionContext();

	void set_callback(llm_callback_t cb);

	void set_async_result(AsyncResult *result);
	AsyncResultPtr *get_async_result() const;
	bool is_async_streaming() const;
	void async_msgqueue_put(ChatCompletionChunk *chunk);
	void async_msgqueue_set_nonblock();

private:
	bool flag; // whether ctx responsible for req and resp
	AsyncResultPtr *result; // for async
};

class AsyncResultPtr
{
public:
	AsyncResultPtr();
	~AsyncResultPtr();

	AsyncResultPtr(const AsyncResultPtr&) = delete;
	AsyncResultPtr& operator=(const AsyncResultPtr&) = delete;

	void incref();
	void decref();

	ChatCompletionChunk *get_chunk();
	ChatCompletionResponse *get_response();

	void set_success(bool success);
	void set_status_code(int code);
	void set_error_message(const std::string& msg);
	WFPromise<ChatCompletionResponse *> *get_promise() const;

	bool is_streaming() const;
	void msg_queue_put(ChatCompletionChunk *chunk);
	ChatCompletionChunk *msg_queue_get();
	void msg_queue_set_nonblock();

private:
	void clear();

private:
	std::atomic<int> ref;
	bool success;
	int status_code;
	std::string error_message;
	ChatCompletionChunk *current_chunk;
	ChatCompletionResponse *response;
	WFPromise<ChatCompletionResponse *> *promise;
	WFFuture<ChatCompletionResponse *> future;
	msgqueue_t *msgqueue;

	friend class AsyncResult;
};

} // namespace wfai

#endif // LLM_SESSION_H

