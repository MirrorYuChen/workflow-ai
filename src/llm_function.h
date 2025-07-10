#ifndef LLM_FUNCTION_H 
#define LLM_FUNCTION_H 

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "workflow/json_parser.h"
#include "chat_request.h"

namespace wfai {

struct FunctionParameter
{
	std::string type;
	std::string description;
	std::vector<std::string> enum_values; // 对于enum类型
	std::map<std::string, FunctionParameter> properties; // 对于object类型
	std::vector<std::string> required; // 对于object类型的必需字段
	std::string default_value;

	FunctionParameter() : type("string") {}
	FunctionParameter(const std::string& t, const std::string& desc = "") 
		: type(t), description(desc) {}
};

struct FunctionDefinition
{
	std::string name;
	std::string description;
	std::map<std::string, FunctionParameter> parameters;
	std::vector<std::string> required_parameters;

	FunctionDefinition() = default;
	FunctionDefinition(const std::string& n, const std::string& desc)
		: name(n), description(desc) {}
};

struct FunctionCall
{
	std::string name;
	std::string arguments; // json

	FunctionCall() = default;
	FunctionCall(const std::string& n, const std::string& args)
		: name(n), arguments(args) {}
};

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

class FunctionManager
{
public:
	void register_function(const FunctionDefinition& definition,
						   FunctionHandler handler);
	std::vector<Tool> get_functions() const;
	std::vector<FunctionDefinition> get_function_definitions() const;
	FunctionResult execute_function(const FunctionCall& call) const;
	bool has_function(const std::string& name) const;
	void clear_functions();

private:
	std::map<std::string, FunctionDefinition> functions;
	std::map<std::string, FunctionHandler> handlers;
};

class FunctionBuilder
{
public:
	FunctionBuilder(const std::string& name, const std::string& description);

	FunctionBuilder& add_string_parameter(const std::string& name,
										  const std::string& description,
										  bool required = false);
	FunctionBuilder& add_number_parameter(const std::string& name,
										  const std::string& description,
										  bool required = false);
	FunctionBuilder& add_boolean_parameter(const std::string& name,
										   const std::string& description,
										   bool required = false);
	FunctionBuilder& add_enum_parameter(const std::string& name,
										const std::string& description, 
										const std::vector<std::string>& values,
										bool required = false);

	FunctionDefinition build() const;

private:
	FunctionDefinition definition;
};

} // namespace wfai

#endif

