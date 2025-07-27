#ifndef CHAT_REQUEST_H
#define CHAT_REQUEST_H

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include "workflow/json_parser.h"
#include "llm_util.h"

namespace wfai {

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
	int top_logprobs;

//friend:
//	class LLMClient;
};

} // namespace wfai

#endif // CHAT_REQUEST_H 

