#ifndef CHAT_RESPONSE_H
#define CHAT_RESPONSE_H

#include <string>
#include <vector>
#include <cstring>
#include <functional>
#include "workflow/json_parser.h"

namespace wfai
{

/*
struct TokenDetails
{
	int cached_tokens = 0;
};
*/

struct Usage
{
	int completion_tokens;			// 模型 completion 产生的 token 数
	int prompt_tokens;				// 用户 prompt 所包含的 token 数
	int prompt_cache_hit_tokens;	// 用户 prompt 中，命中上下文缓存的 token 数
	int prompt_cache_miss_tokens;   // 用户 prompt 中，未命中上下文缓存的 token 数
	int total_tokens;				// 该请求中，所有 token 的数量（prompt + completion）
	int reasoning_tokens;			// completion_tokens_details
//	TokenDetails prompt_tokens_details;  // prompt tokens 的详细信息
//	TokenDetails completion_tokens_details; // completion tokens 的详细信息

	Usage() :
		completion_tokens(0),
		prompt_tokens(0),
		prompt_cache_hit_tokens(0),
		prompt_cache_miss_tokens(0),
		total_tokens(0),
		reasoning_tokens(0) { }

	Usage(Usage&& move)
	{
		prompt_tokens = move.prompt_tokens;
		completion_tokens = move.completion_tokens;
		total_tokens = move.total_tokens;
//		prompt_tokens_details = std::move(move.prompt_tokens_details);
//		completion_tokens_details = std::move(move.completion_tokens_details);
		prompt_cache_hit_tokens = move.prompt_cache_hit_tokens;
		prompt_cache_miss_tokens = move.prompt_cache_miss_tokens;
	}

	Usage& operator=(Usage&& move)
	{
		if (this != &move) 
		{
			prompt_tokens = move.prompt_tokens;
			completion_tokens = move.completion_tokens;
			total_tokens = move.total_tokens;
//			prompt_tokens_details = std::move(move.prompt_tokens_details);
//			completion_tokens_details = std::move(move.completion_tokens_details);
			prompt_cache_hit_tokens = move.prompt_cache_hit_tokens;
			prompt_cache_miss_tokens = move.prompt_cache_miss_tokens;
		}
		return *this;
	}	
};

struct ToolCall
{
	std::string id;				// tool 调用的 ID
	std::string type;			// tool 的类型，目前仅支持 function
	struct
	{
		std::string name;		// 模型调用的 function 名
		std::string arguments;	// 要调用的 function 的参数，格式为 JSON
	} function;
};

struct TokenLogprob
{
	std::string token;			// 输出的 token
	double logprob;				// 该 token 的对数概率
	std::vector<int> bytes;		// token UTF-8 字节表示的整数列表
};

struct TopLogprob
{
	std::string token;			// 输出的 token
	double logprob;				// 该 token 的对数概率
	std::vector<int> bytes;		// token UTF-8 字节表示的整数列表
};

struct Logprob
{
	TokenLogprob current_info;				// 当前 token 的信息
	std::vector<TopLogprob> top_logprobs;	// top N 的 token 列表
};

struct Logprobs
{
	std::vector<Logprob> content;	// 输出 token 对数概率信息的列表
};

struct Buffer
{
	void *ptr;
	size_t size;
	size_t capacity;

	Buffer() : size(0), capacity(0) { }
	~Buffer() { if (capacity) free(ptr); }
};

class ChatResponse
{
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
		};

		// for non-streaming : whole message
		Message message;
		// for streaming : one chunk for delta
		Message delta;

		int index;					// 该completion在模型生成的选择列表中的索引
		Logprobs logprobs;			// 该choice的对数概率信息
		std::string finish_reason;	// 完成原因，可为空
	};

	std::vector<Choice> choices;

	bool is_stream;					// 是否为流式响应
	bool is_done;					// 是否完成
	Usage usage;					// 该对话补全请求的用量信息

public:
	ChatResponse() : created(0), is_done(false) { }

	ChatResponse(ChatResponse&& move); 
	ChatResponse& operator=(ChatResponse&& move);

	virtual ~ChatResponse() { }

public:
	bool parse_json(const char *msg, size_t size);

private:
	bool parse_choice(const json_value_t *choice);
	bool parse_usage(const json_value_t *usage_val);
	bool parse_logprobs(const json_object_t* logprobs_obj, Logprobs& logprobs);
	bool parse_token_logprob(const json_value_t* token_val, TokenLogprob& token_info);
	bool parse_top_logprobs(const json_value_t* top_logprobs_val, std::vector<TopLogprob>& top_logprobs);

	virtual bool parse_content(const json_object_t *object, Choice& choice) = 0;
};

class ChatCompletionResponse : public ChatResponse
{
public:
	bool append_buffer(const void *data, size_t size);
	bool parse_json()
	{
		return ChatResponse::parse_json((const char *)this->buffer.ptr, this->buffer.size);
	}

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
	bool parse_content(const json_object_t *object, Choice& choice) override;
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

private:
	bool parse_content(const json_object_t *object, Choice& choice) override;
};

} // namespace wfai

#endif // CHAT_RESPONSE_H 
