#ifndef LLM_UTIL_H
#define LLM_UTIL_H

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include "workflow/json_parser.h"

namespace wfai {

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

	// for tool
	int tool_call_id;
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

using FunctionHandler = std::function<FunctionResult(const std::string& arguments)>;

} // namespace wfai

#endif // LLM_UTIL_H

