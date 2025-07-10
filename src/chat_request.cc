#include <string>
#include <cctype>
#include "chat_request.h"

namespace wfai {

static constexpr const char *default_model = "deepseek-chat";

// default : for normal task
ChatCompletionRequest::ChatCompletionRequest() :
	model(default_model),
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

std::string escape_string(const std::string &s)
{
	std::string result;
	result.reserve(s.length() * 2);

	for (char c : s)
	{
		switch (c)
		{
			case '"':  result += "\\\""; break;
			case '\\': result += "\\\\"; break;
			case '\b': result += "\\b";  break;
			case '\f': result += "\\f";  break;
			case '\n': result += "\\n";  break;
			case '\r': result += "\\r";  break;
			case '\t': result += "\\t";  break;
			default:
				if (c >= '\x00' && c <= '\x1f')
				{
					// 快速十六进制格式化
					char hex[7];
					snprintf(hex, sizeof(hex), "\\u%04x",
							 static_cast<unsigned char>(c));
					result += hex;
				}
				else
				{
					result += c;
				}
		}
	}
	return result;
}

std::string ChatCompletionRequest::to_json() const 
{
	std::string json = "{";

	json += "\"messages\":[";
	for (size_t i = 0; i < messages.size(); i++) 
	{
		std::string escaped_content = escape_string(messages[i].content);
		json += "{";
		json += "\"content\":\"" + escaped_content + "\",";
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

	json += this->tools_to_json();

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
			json += "\"" + escape_string(stop[i]) + "\"";
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

std::string ChatCompletionRequest::tools_to_json() const
{
	std::string json;

	if (!tool_choice.empty())
	{
		json += ",\"tool_choice\":";
		if (tool_choice == "auto" || tool_choice == "none")
			json += "\"" + tool_choice + "\"";
		else
			json += tool_choice;
	}

	if (tool_choice == "none" || tools.empty())
		return json;

	json += ",\"tools\":[";
	for (size_t i = 0; i < tools.size(); i++)
	{
		const Tool& tool = tools[i];
		json += "{";
		json += "\"type\":\"" + tool.type + "\",";
		json += "\"function\":{";
		json += "\"name\":\"" + escape_string(tool.function.name) + "\"";

		if (!tool.function.description.empty())
		{
			json += ",\"description\":\"" +
				escape_string(tool.function.description) + "\"";
		}

		const auto& params = tool.function.parameters;
		json += ",\"parameters\":{";
		json += "\"type\":\"" + params.type + "\"";

		if (!params.properties.empty())
		{
			json += ",\"properties\":{";
			size_t prop_count = 0;
			for (const auto& pair : params.properties)
			{
				const auto& prop = pair.second;
				json += "\"" + escape_string(pair.first) + "\":{";
				json += "\"type\":\"" + escape_string(prop.type) + "\"";

				if (!prop.description.empty())
				{
					json += ",\"description\":\"" +
							escape_string(prop.description) + "\"";
				}

				if (!prop.enum_values.empty())
				{
					json += ",\"enum\":[";
					for (size_t j = 0; j < prop.enum_values.size(); j++)
					{
						json += "\"" + escape_string(prop.enum_values[j]) + "\"";
						if (j < prop.enum_values.size() - 1)
							json += ",";
					}
					json += "]";
				}

				if (!prop.default_value.empty())
				{
					json += ",\"default\":\"" +
							escape_string(prop.default_value) + "\"";
				}

				json += "}";
				if (++prop_count < params.properties.size())
					json += ",";
			}
			json += "}";
		}

		if (!params.required.empty())
		{
			json += ",\"required\":[";
			for (size_t j = 0; j < params.required.size(); j++)
			{
				json += "\"" + escape_string(params.required[j]) + "\"";
				if (j < params.required.size() - 1)
					json += ",";
			}
			json += "]";
		}

		json += "}}}"; // end parameters, function, tool

		if (i < tools.size() - 1)
			json += ",";
	}
	json += "]";

	return json;
}

} // namespace wfai
