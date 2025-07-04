#ifndef CHAT_REQUEST_H
#define CHAT_REQUEST_H

#include <string>
#include <vector>
#include <cstring>
#include "workflow/json_parser.h"

namespace wfai {

struct Tool
{
	std::string type; // DeepSeek only support 'function'
	std::string name;
	std::string description;
	std::string parameters; // JSON格式的参数定义
};

struct Message
{
	// for system/user/assistant/tool
	std::string role;
	std::string content;
	std::string name; // optional

	// for assistant
	bool prefix;
	std::string reasoning_context;

	// for tool
	int tool_call_id;
};

class ChatCompletionRequest
{
public:
	std::string to_json() const;
	ChatCompletionRequest();

public:
	std::vector<Message> messages;
	std::string model;
	double frequency_penalty;
	int max_tokens;
	double presence_penalty;
	std::string response_format; // type : text or json_object
	std::vector<std::string> stop;
	bool stream;
	std::string *stream_options = nullptr; // TODO
	double temperature;
	double top_p;
	std::vector<Tool> tools;
	std::string tool_choice; // none, auto, required
	bool logprobs;
	int *top_logprobs = nullptr;

//friend:
//	class LLMClient;
};

} // namespace wfai

#endif // CHAT_REQUEST_H 

