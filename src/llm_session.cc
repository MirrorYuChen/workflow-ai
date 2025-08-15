#include "llm_session.h"

using namespace wfai;

SessionContext::SessionContext(ChatCompletionRequest *req,
							   ChatCompletionResponse *resp,
							   llm_extract_t extract,
							   llm_callback_t callback,
							   bool flag) :
	req(req), resp(resp),
	extract(std::move(extract)), callback(std::move(callback)),
	flag(flag), result(nullptr)
{
}

SessionContext::~SessionContext()
{
	if (this->flag)
	{
		delete this->req;
		delete this->resp;
	}
}

void SessionContext::set_callback(llm_callback_t cb)
{
	this->callback = std::move(cb);
}

void SessionContext::set_async_result(AsyncResult *result)
{
	// AsyncResult is available so ptr will not be delete inside here
	this->result = result->get_ptr();
	this->result->incref();
}

AsyncResultPtr *SessionContext::get_async_result() const
{
	return this->result;
}

bool SessionContext::is_async_streaming() const
{
	return this->result && this->result->is_streaming();
}

void SessionContext::async_msgqueue_put(ChatCompletionChunk *chunk)
{
	this->result->msg_queue_put(chunk);
}

void SessionContext::async_msgqueue_set_nonblock()
{
	this->result->msg_queue_set_nonblock();
}

////////// AsyncResult //////////////

AsyncResult::AsyncResult()
{
	this->ptr = new AsyncResultPtr();
}

AsyncResult::~AsyncResult()
{
	this->ptr->decref();
}

ChatCompletionChunk *AsyncResult::get_chunk()
{
	return this->ptr->get_chunk();
}

ChatCompletionResponse *AsyncResult::get_response()
{
	return this->ptr->get_response();
}

AsyncResultPtr *AsyncResult::get_ptr()
{
	return this->ptr;
}

AsyncResult::AsyncResult(AsyncResult&& move) :
	ptr(move.ptr)
{
	move.ptr = nullptr;
}

AsyncResult& AsyncResult::operator=(AsyncResult&& move)
{
	if (this != &move)
	{
		this->ptr = move.ptr;
		move.ptr = nullptr;
	}

	return *this;
}

void AsyncResult::msg_queue_create(size_t len)
{
	this->ptr->msgqueue = msgqueue_create(len,
							sizeof(struct ChatCompletionChunk));
}

bool AsyncResult::success() const
{
	return this->ptr->success;
}

int AsyncResult::status_code() const
{
	return this->ptr->status_code;
}

const std::string& AsyncResult::error_message() const
{
	return this->ptr->error_message;
}

AsyncResultPtr::AsyncResultPtr() :
	ref(1),
	success(false),
	status_code(0),
	current_chunk(nullptr),
	response(nullptr)
{
	this->promise = new WFPromise<ChatCompletionResponse *>();
	this->future = this->promise->get_future();
}

AsyncResultPtr::~AsyncResultPtr()
{
	this->clear();
}

void AsyncResultPtr::clear()
{
	if (this->current_chunk)
		delete this->current_chunk;

	if (this->response)
		delete this->response;

	if (this->msgqueue)
	{
		msgqueue_set_nonblock(this->msgqueue);
		while (true)
		{
			ChatCompletionChunk *chunk;
			chunk = static_cast<ChatCompletionChunk *>(msgqueue_get(this->msgqueue));
			if (chunk == nullptr)
				break;
			delete chunk;
		}

		msgqueue_destroy(this->msgqueue);
	}
}

ChatCompletionChunk *AsyncResultPtr::get_chunk()
{
	if (!this->msgqueue) // non streaming
		return nullptr;

	if (this->current_chunk)
		delete this->current_chunk;

	this->current_chunk =
		static_cast<ChatCompletionChunk *>(msgqueue_get(this->msgqueue));

	return this->current_chunk;
}

ChatCompletionResponse *AsyncResultPtr::get_response()
{
	ChatCompletionResponse *resp = this->future.get();
	return resp;
}

void AsyncResultPtr::incref()
{
	this->ref++;
}

void AsyncResultPtr::decref()
{
	if (--this->ref == 0)
		delete this;
}

bool AsyncResultPtr::is_streaming() const
{
	return this->msgqueue == nullptr ? false : true;
}

void AsyncResultPtr::msg_queue_put(ChatCompletionChunk *chunk)
{
	msgqueue_put(chunk, this->msgqueue);
}

ChatCompletionChunk *AsyncResultPtr::msg_queue_get()
{
	return static_cast<ChatCompletionChunk *>(msgqueue_get(this->msgqueue));
}

void AsyncResultPtr::msg_queue_set_nonblock()
{
	msgqueue_set_nonblock(this->msgqueue);
}

void AsyncResultPtr::set_status_code(int code)
{
	this->status_code = code;
}

void AsyncResultPtr::set_success(bool success)
{
	this->success = success;
}
	
void AsyncResultPtr::set_error_message(const std::string& msg)
{
	this->error_message = msg;
}

WFPromise<ChatCompletionResponse *> *AsyncResultPtr::get_promise() const
{
	return this->promise;
}

