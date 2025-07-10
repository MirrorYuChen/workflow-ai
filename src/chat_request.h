#ifndef CHAT_REQUEST_H
#define CHAT_REQUEST_H

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include "workflow/json_parser.h"

namespace wfai {

struct Property
{
	std::string type;
	std::string description;
	std::vector<std::string> enum_values;	// 可选，如果为空则表示没有
	std::string default_value;				// 可选，空表示没有，可能不太好
};

struct Tool
{
	std::string type = "function";

	struct Function
	{
		std::string name;
		std::string description;

		struct ParametersSchema
		{
			std::string type = "object";
			std::map<std::string, Property> properties;
			std::vector<std::string> required;
		};

		ParametersSchema parameters;
	};

	Function function;

	Tool() = default;
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

private:
	std::string tools_to_json() const;

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

