#include <stdint.h>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/HttpMessage.h"
#include "workflow/WFHttpChunkedClient.h"
#include "workflow/Workflow.h"
#include "llm_client.h"

using namespace wfai;

static constexpr const char *default_url = "https://api.deepseek.com/v1/chat/completions";
static constexpr const char *auth_str = "Bearer ";
static constexpr uint32_t default_streaming_ttft = 100 * 1000; // ms
static constexpr uint32_t default_streaming_tpft = 1 * 1000; // ms
static constexpr uint32_t default_no_streaming_ttft = 500 * 1000; // ms
static constexpr uint32_t default_no_streaming_tpft = 100 * 1000; // ms
static constexpr int default_redirect_max = 3;

// Context for parallel tool calls execution
class ToolCallsContext
{
public:
	ToolCallsContext(ChatCompletionRequest *req,
					 ChatCompletionResponse *resp,
					 LLMClient::llm_extract_t extract,
					 LLMClient::llm_callback_t callback) :
		req(req), resp(resp),
		extract(std::move(extract)),
		callback(std::move(callback))
	{
	}

	~ToolCallsContext()
	{
		for (auto *result : results)
			delete result;
	}

public:
	ChatCompletionRequest *req;
	ChatCompletionResponse *resp;
	std::vector<FunctionResult *> results;
	std::vector<std::string> tool_call_ids;
	LLMClient::llm_extract_t extract;
	LLMClient::llm_callback_t callback;
};

// for streaming
// to collect the argument in tool_calls in each chunk
bool append_tool_call_from_chunk(const ChatCompletionChunk& chunk,
								 ChatCompletionResponse *resp)
{
	if (resp->choices.empty()) // first time to mark
	{
		ChatResponse::Choice choice;
		choice.message.tool_calls.push_back(chunk.choices[0].delta.tool_calls[0]);
		resp->choices.push_back(choice);
		return true;
	}

	// not the first time
	if (resp->choices[0].message.tool_calls.empty())
		return false;

	resp->choices[0].message.tool_calls[0].function.arguments +=
		chunk.choices[0].delta.tool_calls[0].function.arguments;

	return true;
}

LLMClient::LLMClient() :
	LLMClient("", default_url)
{
}

LLMClient::LLMClient(const std::string& api_key) :
	LLMClient(api_key, default_url)
{
}

LLMClient::LLMClient(const std::string& api_key,
					 const std::string& base_url) :
	api_key(api_key),
	base_url(base_url)
{
	this->redirect_max = default_redirect_max;
	this->streaming_ttft = default_streaming_ttft;
	this->streaming_tpft = default_streaming_tpft;
	this->ttft = default_no_streaming_ttft;
	this->tpft = default_no_streaming_tpft;
	this->function_manager = nullptr;
}

WFHttpChunkedTask *LLMClient::create_chat_task(ChatCompletionRequest& request,
											   llm_extract_t extract,
											   llm_callback_t callback)
{
	ChatCompletionRequest *req = new ChatCompletionRequest(request);
	ChatCompletionResponse *resp = new ChatCompletionResponse();

	return this->create(req, resp, std::move(extract), std::move(callback));
}

WFHttpChunkedTask *LLMClient::create(ChatCompletionRequest *req,
									 ChatCompletionResponse *resp,
									 llm_extract_t extract,
									 llm_callback_t callback)
{
	auto extract_handler = std::bind(
		&LLMClient::extract,
		this,
		std::placeholders::_1,
		req,
		resp,
		extract
	);

	callback_t callback_handler;

	if (this->function_manager && req->tool_choice != "none")
	{
		auto tools = this->function_manager->get_functions();
		for (const auto& tool : tools)
			req->tools.emplace_back(tool);

		callback_handler = std::bind(
			&LLMClient::callback_with_tools,
			this,
			std::placeholders::_1,
			req,
			resp,
			std::move(extract), // for multi round session
			std::move(callback)
		);
	}
	else
	{
		callback_handler = std::bind(
			&LLMClient::callback,
			this,
			std::placeholders::_1,
			req,
			resp,
			nullptr,
			std::move(callback)
		);
	}

	auto *task = client.create_chunked_task(
		this->base_url,
		this->redirect_max,
		std::move(extract_handler),
		std::move(callback_handler)
	);

	if (req->stream)
	{
		task->set_watch_timeout(this->streaming_ttft);
		task->set_recv_timeout(this->streaming_tpft);
	}
	else
	{
		task->set_watch_timeout(this->ttft);
		task->set_recv_timeout(this->tpft);
	}

	auto *http_req = task->get_req();
	http_req->add_header_pair("Authorization", auth_str + this->api_key);
	http_req->add_header_pair("Content-Type", "application/json");
	http_req->add_header_pair("Connection", "keep-alive");
	http_req->set_method("POST");

	std::string body = req->to_json();
	http_req->append_output_body(body.data(), body.size());

	return task;
}

void LLMClient::callback(WFHttpChunkedTask *task,
						 ChatCompletionRequest *req,
						 ChatCompletionResponse *resp,
						 llm_extract_t extract,
						 llm_callback_t callback)
{
	if (!req->stream)
		resp->parse_json();
	// TODO: if (!ret) set error

	if (callback)
		callback(task, req, resp);

	delete req;
	delete resp;
}

void LLMClient::callback_with_tools(WFHttpChunkedTask *task,
									ChatCompletionRequest *req,
									ChatCompletionResponse *resp,
									llm_extract_t extract,
									llm_callback_t callback)
{
	if (!req->stream)
	{
		if(!resp->parse_json())
		{
			if (callback)
				callback(task, req, resp);

			delete req;
			delete resp;
			return;
		}
	}

	// parse resp
	if (task->get_state() != WFT_STATE_SUCCESS ||
		resp->choices.empty() ||
		resp->choices[0].message.tool_calls.empty())
		return;

	// append the llm response
	Message resp_msg;
	resp_msg.role = "assistant";

	for (const auto &tc : resp->choices[0].message.tool_calls)
		resp_msg.tool_calls.push_back(tc);

	req->messages.push_back(resp_msg);

	// remove previous tools infomation
	req->tool_choice = "none";
	req->tools.clear();

	ToolCallsContext *ctx = new ToolCallsContext(
		req, resp, std::move(extract), std::move(callback));

	// calculate
	if (resp->choices[0].message.tool_calls.size() == 1)
	{
		const auto& tc = resp->choices[0].message.tool_calls[0];
		// if (tc.type == "function")
		FunctionResult *res = new FunctionResult();
		ctx->results.push_back(res);
		ctx->tool_call_ids.push_back(tc.id);

		WFGoTask *next = this->function_manager->async_execute(
			tc.function.name,
			tc.function.arguments,
			res);

		next->user_data = ctx;

		auto callback_handler = std::bind(
			&LLMClient::tool_calls_callback,
			this,
			std::placeholders::_1
		);

		if (next) // should return WFEmptyTask instead of nullptr
		{
			next->set_callback(std::move(callback_handler));
			series_of(task)->push_back(next);
		}
	}
	else
	{
		auto p_cb = std::bind(
			&LLMClient::parallel_tool_calls_callback,
			this,
			std::placeholders::_1
		);

		ParallelWork *pwork = Workflow::create_parallel_work(std::move(p_cb));
		pwork->set_context(ctx);

		// Create series for each tool call
		for (const auto& tc : resp->choices[0].message.tool_calls)
		{
			FunctionResult *res = new FunctionResult();
			ctx->results.push_back(res);
			ctx->tool_call_ids.push_back(tc.id);

			WFGoTask *go_task = this->function_manager->async_execute(
				tc.function.name,
				tc.function.arguments,
				res);

			if (go_task)
			{
				SeriesWork *series = Workflow::create_series_work(go_task, nullptr);
				pwork->add_series(series);
			}
		}

		series_of(task)->push_back(pwork);
	}
}

void LLMClient::parallel_tool_calls_callback(const ParallelWork *pwork)
{
	ToolCallsContext *ctx = static_cast<ToolCallsContext *>(pwork->get_context());
//	if (!ctx)
//		return;

	// Add all tool call results to the request messages
	for (size_t i = 0; i < ctx->results.size(); ++i)
	{
		Message msg;
		msg.role = "tool";
		msg.tool_call_id = ctx->tool_call_ids[i];
		if (ctx->results[i]->success)
			msg.content = ctx->results[i]->result;
		else
			msg.content = ctx->results[i]->error_message;
		ctx->req->messages.push_back(std::move(msg));
	}

	delete ctx->resp;
	ctx->resp = new ChatCompletionResponse();

	auto extract_handler = std::bind(
		&LLMClient::extract,
		this,
		std::placeholders::_1,
		ctx->req,
		ctx->resp,
		ctx->extract
	);

	auto callback_handler = std::bind(
		&LLMClient::callback,
		this,
		std::placeholders::_1,
		ctx->req,
		ctx->resp,
		nullptr,
		ctx->callback
	);

	auto *next = this->create(ctx->req, ctx->resp, ctx->extract, ctx->callback);
	series_of(pwork)->push_back(next);
	delete ctx;
}

void LLMClient::tool_calls_callback(WFGoTask *task)
{
	ToolCallsContext *ctx = static_cast<ToolCallsContext*>(task->user_data);
//	if (!ctx || ctx->results.empty() || ctx->tool_call_ids.empty())
//		return;

	Message msg;
	msg.role = "tool";
	msg.tool_call_id = ctx->tool_call_ids[0];
	if (ctx->results[0]->success)
		msg.content = ctx->results[0]->result;
	else
		msg.content = ctx->results[0]->error_message;
	ctx->req->messages.push_back(std::move(msg));

	delete ctx->resp;
	ctx->resp = new ChatCompletionResponse();

	auto extract_handler = std::bind(
		&LLMClient::extract,
		this,
		std::placeholders::_1,
		ctx->req,
		ctx->resp,
		ctx->extract
	);

	auto callback_handler = std::bind(
		&LLMClient::callback,
		this,
		std::placeholders::_1,
		ctx->req,
		ctx->resp,
		nullptr,
		ctx->callback
	);

	auto *next = this->create(ctx->req, ctx->resp, ctx->extract, ctx->callback);
	series_of(task)->push_back(next);
	delete ctx;
}

void LLMClient::extract(WFHttpChunkedTask *task,
						ChatCompletionRequest *req,
						ChatCompletionResponse *resp,
						llm_extract_t extract)
{
	protocol::HttpMessageChunk *msg_chunk = task->get_chunk();
	const void *msg;
	size_t size;
	
	if (!msg_chunk->get_chunk_data(&msg, &size))
	{
		// TODO : mark error : invalid chunk data
		return;
	}

	if (!req->stream)
	{
		resp->append_buffer(static_cast<const char*>(msg), size);

		if (extract)
			extract(task, req, nullptr); // not a chunk for no streaming
	}
	else
	{
		const char *p = static_cast<const char *>(msg);
		const char *msg_end = p + size;
		const char *begin;
		const char *end;
		size_t len;

		while (p < msg_end)
		{
			begin = strstr(p, "data: ");
			if (!begin || begin >= msg_end)
				break;

			begin += 6;

			end = strstr(begin, "data: "); // \r\n
			end = end ? end : msg_end;
			p = end;

			while (end > begin && (*(end - 1) == '\n' || *(end - 1) == '\r'))
				--end;

			len = end - begin;
			if (len > 0)
			{
				ChatCompletionChunk chunk;
				bool ret = chunk.parse_json(begin, len);
				if (ret)
				{
					if (!chunk.choices.empty() &&
						!chunk.choices[0].delta.tool_calls.empty())
					{
						ret = append_tool_call_from_chunk(chunk, resp);
						//TODO:
					}

					if (extract)
						extract(task, req, &chunk);
				}
				// TODO: else
			}
		}
	}
}

void LLMClient::set_function_manager(FunctionManager *manager)
{
	this->function_manager = manager;
}

bool LLMClient::register_function(const FunctionDefinition& def,
								  FunctionHandler handler)
{
	return this->function_manager->register_function(def, std::move(handler));
}

