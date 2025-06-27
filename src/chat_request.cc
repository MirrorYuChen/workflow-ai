#include "chat_request.h"

namespace llm_task {

// default : for normal task
ChatCompletionRequest::ChatCompletionRequest() :
	model("deepseek-chat"),
	frequency_penalty(0),
	max_tokens(4096),
	presence_penalty(0),
	response_format("text"),
	stream(false),
	temperature(1.0),
	top_p(1.0),
	tool_choice("none"),
	logprobs(false)
{
}

/*
// for generating task
static ChatCompletionRequest ChatCompletionRequest::CreativeRequest() 
{
	ChatCompletionRequest req;
	req.temperature = 1.0;
	req.frequency_penalty = 0.5;
	req.presence_penalty = 0.5;
	return req;
}

// for translate task
static ChatCompletionRequest TranslationRequest() 
{
	ChatCompletionRequest req;
	req.temperature = 0.3;
	req.frequency_penalty = 0;
	req.presence_penalty = 0;
	return req;
}
*/

std::string ChatCompletionRequest::to_json() const 
{
	std::string json = "{";

	json += "\"messages\":[";
	for (size_t i = 0; i < messages.size(); i++) 
	{
		json += "{";
		json += "\"content\":\"" + messages[i].content + "\",";
		json += "\"role\":\"" + messages[i].role + "\"";
		if (!messages[i].name.empty())
			json += ",\"name\":\"" + messages[i].name + "\"";
		json += "}";
		if (i < messages.size() - 1)
			json += ",";
	}
	json += "],";

	json += "\"model\":\"" + model + "\"";

	if (stream)
		json += ",\"stream\":true";
	if (stream_options)
		json += ",\"stream_options\":\"" + *stream_options + "\"";
	if (tools)
		json += ",\"tools\":\"" + *tools + "\"";
	if (tool_choice != "none")
		json += ",\"tool_choice\":\"" + tool_choice + "\"";

	if (frequency_penalty != 0)
		json += ",\"frequency_penalty\":" + std::to_string(frequency_penalty);
	if (max_tokens != 4096)
		json += ",\"max_tokens\":" + std::to_string(max_tokens);
	if (presence_penalty != 0)
		json += ",\"presence_penalty\":" + std::to_string(presence_penalty);
	if (response_format != "text")
		json += ",\"response_format\":{\"type\":\"" + response_format + "\"}";
	
	if (!stop.empty())
	{
		json += ",\"stop\":[";
		for (size_t i = 0; i < stop.size(); ++i)
		{
			json += "\"";
			if (stop[i] == "\\")
				json += "\\\\";
			else if (stop[i] == "\"")
				json += "\\\"";
			else if (stop[i] == "\n")
				json += "\\n";
			else if (stop[i] == "\r")
				json += "\\r";
			else if (stop[i] == "\t")
				json += "\\t";
			else
				json += stop[i];
			json += "\"";
			if (i < stop.size() - 1)
				json += ",";
		}
		json += "]";
	}
	if (temperature != 1.0)
		json += ",\"temperature\":" + std::to_string(temperature);
	if (top_p != 1.0)
		json += ",\"top_p\":" + std::to_string(top_p);
	if (logprobs)
		json += ",\"logprobs\":true";
	if (top_logprobs)
		json += ",\"top_logprobs\":" + std::to_string(*top_logprobs);

	json += "}";
	return json;
}

} // namespace llm_task
