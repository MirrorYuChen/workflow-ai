#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/HttpMessage.h"
#include "workflow/WFHttpChunkedClient.h"
#include "llm_client.h"

using namespace wfai;

static constexpr const char *default_url = "https://api.deepseek.com/v1/chat/completions";
static constexpr const char *auth_str = "Bearer ";
static constexpr uint32_t default_streaming_ttft = 100 * 1000; // ms
static constexpr uint32_t default_streaming_tpft = 1 * 1000; // ms
static constexpr uint32_t default_no_streaming_ttft = 500 * 1000; // ms
static constexpr uint32_t default_no_streaming_tpft = 100 * 1000; // ms
static constexpr int default_redirect_max = 3;

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
}

WFHttpChunkedTask* LLMClient::create_chat_task(ChatCompletionRequest request,
											   llm_extract_t extract,
											   llm_callback_t callback)
{
	ChatCompletionRequest *req = new ChatCompletionRequest(std::move(request));
	ChatCompletionResponse *resp = new ChatCompletionResponse();

	auto extract_handler = std::bind(
		&LLMClient::extract,
		this,
		std::placeholders::_1,
		req,
		resp,
		std::move(extract)
	);
	
	auto callback_handler = std::bind(
		&LLMClient::callback,
		this,
		std::placeholders::_1,
		req,
		resp,
		std::move(callback)
	);

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
						 llm_callback_t callback)
{
	if (!req->stream)
	{
		resp->parse_json();
		// TODO: if (!ret) set error

		if (callback)
			callback(task, req, resp);
	}
	else
	{
		if (callback)
			callback(task, req, nullptr);
	}

	delete req;
	delete resp;
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
		// TODO
		//if (strncmp("[DONE]", msg, size) != 0)
		//	fprintf(stderr, "Error. Invalid chunk data.\n");
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
				wfai::ChatCompletionChunk chunk;
				bool ret = chunk.parse_json(begin, len);
				if (ret)
				{
					if (extract)
						extract(task, req, &chunk);
				}
				// TODO: else
			}
		}
	}
}

