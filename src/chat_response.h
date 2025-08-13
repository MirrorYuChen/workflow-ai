#ifndef CHAT_RESPONSE_H
#define CHAT_RESPONSE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <cstring>
#include <functional>
#include "workflow/json_parser.h"
#include "llm_util.h"

namespace wfai {

struct Usage
{
	int completion_tokens;			// 模型 completion 产生的 token 数
	int prompt_tokens;				// 用户 prompt 所包含的 token 数
	int prompt_cache_hit_tokens;	// 用户 prompt 中，命中上下文缓存的 token 数
	int prompt_cache_miss_tokens;   // 用户 prompt 中，未命中上下文缓存的 token 数
	int total_tokens;				// 该请求中，所有 token 的数量（prompt + completion）
	int reasoning_tokens;

	struct TokenDetails
	{
		int cached_tokens;
		TokenDetails() : cached_tokens(0) { }
		void clear()
		{
			cached_tokens = 0;
		}
	} prompt_tokens_details;		// prompt tokens 的详细信息

	void clear()
	{
		completion_tokens = 0;
		prompt_tokens = 0;
		prompt_cache_hit_tokens = 0;
		prompt_cache_miss_tokens = 0;
		total_tokens = 0;
		reasoning_tokens = 0;
		prompt_tokens_details.clear();
	}
};

struct TokenLogprob
{
	std::string token;			// 输出的 token
	double logprob;				// 该 token 的对数概率
	std::vector<int> bytes;		// token UTF-8 字节表示的整数列表

	TokenLogprob() = default;
	TokenLogprob(std::string t, double lp) : token(std::move(t)), logprob(lp) {}

	void clear()
	{
		token.clear();
		logprob = 0.0;
		bytes.clear();
	}
};

struct Logprob
{
	TokenLogprob current_info;				// 当前 token 的信息
	std::vector<TokenLogprob> top_logprobs;	// top N 的 token 列表

	void clear()
	{
		current_info.clear();
		top_logprobs.clear();
	}
};

struct Logprobs
{
	std::vector<Logprob> content;	// 输出 token 对数概率信息的列表
	void clear()
	{
		content.clear();
	}
};

struct Buffer
{
	void *ptr;
	size_t size;
	size_t capacity;

	Buffer() : ptr(nullptr), size(0), capacity(0) { }
	~Buffer() { free(ptr); }

	void clear()
	{
		if (ptr)
		{
			free(ptr);
			ptr = nullptr;
		}
		size = 0;
		capacity = 0;
	}
};

class ChatResponse
{
public:
	int state;

public:
	std::string id;					// 该对话的唯一标识符
	std::string object;				// 对象类型
	uint32_t created;				// unix timestamp
	std::string model;				// 使用的模型
	std::string system_fingerprint;	// 系统指纹

	struct Choice
	{
		Choice() : index(0) {}
		
		struct Message
		{
			std::string role;
			std::string content;
			std::string reasoning_content;
			std::vector<ToolCall> tool_calls;

			void clear()
			{
				role.clear();
				content.clear();
				reasoning_content.clear();
				tool_calls.clear();
			}
		};

		// for non-streaming : whole message
		Message message;
		// for streaming : one chunk for delta
		Message delta;

		int index;					// 该completion在模型生成的选择列表中的索引
		Logprobs logprobs;			// 该choice的对数概率信息
		std::string finish_reason;	// 完成原因，可为空

		void clear()
		{
			message.clear();
			delta.clear();
			index = 0;
			logprobs.content.clear();
			finish_reason.clear();
		}
	};

	std::vector<Choice> choices;
	bool is_stream;					// 是否为流式响应
	bool is_done;					// for stream : is last chunk
	Usage usage;					// 该对话补全请求的用量信息

public:
	ChatResponse() : state(RESPONSE_UNDEFINED), created(0), is_done(false) { }

	ChatResponse(ChatResponse&& move); 
	ChatResponse& operator=(ChatResponse&& move);

	virtual ~ChatResponse() { }
	virtual void clear();

public:
	bool parse_json(const char *msg, size_t size);

private:
	bool parse_choice(const json_value_t *choice);
	bool parse_usage(const json_value_t *usage_val);
	bool parse_logprobs(const json_object_t *logprobs_obj,
						Logprobs& logprobs);
	bool parse_token_logprob(const json_value_t *token_val,
							 TokenLogprob& token_info);
	bool parse_top_logprobs(const json_value_t *top_logprobs_val,
							std::vector<TokenLogprob>& top_logprobs);

	virtual bool parse_message(const json_object_t *object, Choice& choice) = 0;
};

class ChatCompletionResponse : public ChatResponse
{
public:
	bool append_buffer(const void *data, size_t size);
	bool parse_json()
	{
		return ChatResponse::parse_json((const char *)this->buffer.ptr,
										this->buffer.size);
	}
	void clear() override;

private:
	Buffer buffer;

public:
	ChatCompletionResponse()
	{
		is_stream = false;
	}

	ChatCompletionResponse(ChatCompletionResponse&& move);
	ChatCompletionResponse& operator=(ChatCompletionResponse&& move);

	virtual ~ChatCompletionResponse() { }

private:
	bool parse_message(const json_object_t *object, Choice& choice) override;
};

class ChatCompletionChunk : public ChatResponse
{
public:
	ChatCompletionChunk()
	{
		this->is_stream = true;
	}

	ChatCompletionChunk(ChatCompletionChunk&& move);
	ChatCompletionChunk& operator=(ChatCompletionChunk&& move);

	virtual ~ChatCompletionChunk() { }

	void clear() override
	{
		ChatResponse::clear();
		this->is_stream = true;
	}

	bool last_chunk() const { return this->is_done; }
	void set_last_chunk(bool flag) { this->is_done = flag; }

private:
	bool parse_message(const json_object_t *object, Choice& choice) override;
};

} // namespace wfai

#endif // CHAT_RESPONSE_H 
