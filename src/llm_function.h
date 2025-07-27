#ifndef LLM_FUNCTION_H 
#define LLM_FUNCTION_H 

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "workflow/json_parser.h"
#include "llm_util.h"

namespace wfai {

struct FunctionCall
{
	std::string name;
	std::string arguments; // json

	FunctionCall() = default;
	FunctionCall(const std::string& n, const std::string& args)
		: name(n), arguments(args) {}
};

class FunctionManager
{
public:
	bool register_function(const FunctionDefinition& definition,
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

