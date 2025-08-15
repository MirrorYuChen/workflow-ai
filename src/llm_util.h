#ifndef LLM_UTIL_H
#define LLM_UTIL_H

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <functional>
#include "workflow/json_parser.h"
#include "workflow/WFHttpChunkedClient.h"

namespace wfai {

class ChatCompletionRequest;
class ChatCompletionChunk;
class ChatCompletionResponse;

using llm_extract_t = std::function<void(WFHttpChunkedTask *,
										 ChatCompletionRequest *,
										 ChatCompletionChunk *)>;

using llm_callback_t = std::function<void(WFHttpChunkedTask *,
										  ChatCompletionRequest *,
										  ChatCompletionResponse *)>;

///// for response and sync result /////

enum
{
	RESPONSE_UNDEFINED			= -1,
	RESPONSE_SUCCESS			=  0,

	RESPONSE_FRAMEWORK_ERROR	=  1,
	RESPONSE_NETWORK_ERROR		=  2, // http 4xx/5xx

	RESPONSE_PARSE_ERROR		=  11, // parse json error
	RESPONSE_CONTENT_ERROR		=  12, // lack of some content
};

///// for request and response /////

struct ToolCall
{
	std::string id;				// tool 调用的 ID
	std::string type;			// tool 的类型，目前仅支持 function
	int index;
	struct
	{
		std::string name;		// 模型调用的 function 名
		std::string arguments;	// 要调用的 function 的参数，格式为 JSON
	} function;

	ToolCall() : index(0) {}

	ToolCall(const std::string& tc_id,
			 const std::string& func_name,
			 const std::string& args) :
		id(tc_id), type("function"), index(0)
	{
		function.name = func_name;
		function.arguments = args;
	}
};

///// for request /////

struct Message
{
	// for system/user/assistant/tool
	std::string role;
	std::string content;
	std::string name; // optional

	// for assistant
	bool prefix;
	std::string reasoning_context;
	std::vector<ToolCall> tool_calls; // for assistant messages with tool calls

	// for tool, corresponde to struct ToolCall.id
	std::string tool_call_id;

	Message() : prefix(false) {}

	// Constructor for simple messages
	Message(const std::string& r, const std::string& c) :
		role(r), content(c), prefix(false) {}

	// Constructor for tool result messages
	Message(const std::string& r, const std::string& c, const std::string& tcid) :
		role(r), content(c),  prefix(false), tool_call_id(tcid) {}
};

struct ParameterProperty
{
	std::string type;
	std::string description;
	std::vector<std::string> enum_values;	// 可选，如果为空则表示没有
	std::string default_value;				// 可选，空表示没有，可能不太好
};

struct FunctionDefinition
{
	std::string name;
	std::string description;

	struct ParametersSchema
	{
		std::string type = "object";
		std::map<std::string, ParameterProperty> properties;
		std::vector<std::string> required;
	};

	ParametersSchema parameters;

	bool add_parameter(std::string name,
					   ParameterProperty property,
					   bool required)
	{
		if (parameters.properties.find(name) != parameters.properties.end())
			return false;

		parameters.properties.emplace(name, std::move(property));

		if (required)
			parameters.required.push_back(std::move(name));

		return true;
	}
};

struct Tool
{
	std::string type = "function";
	FunctionDefinition function;

	Tool() = default;
};

///// for local function call /////

struct FunctionResult
{
	std::string name;
	std::string result; // json or text
	bool success;
	std::string error_message;

	FunctionResult() : success(true) {}

	FunctionResult(const std::string& n,
				   const std::string& res) :
		name(n),
		result(res),
		success(true)
	{
	}

	FunctionResult(const std::string& n,
				   const std::string& res,
				   bool succ,
				   const std::string& err) :
		name(n),
		result(res),
		success(succ),
		error_message(err)
	{
	}
};

using FunctionHandler =
	std::function<void(const std::string& arguments, FunctionResult *result)>;

} // namespace wfai

#endif // LLM_UTIL_H

