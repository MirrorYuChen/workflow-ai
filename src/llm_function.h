#ifndef LLM_FUNCTION_H 
#define LLM_FUNCTION_H 

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "workflow/WFTask.h"
#include "workflow/json_parser.h"
#include "llm_util.h"

namespace wfai {

class FunctionManager
{
public:
	bool register_function(const FunctionDefinition& definition,
						   FunctionHandler handler);
	std::vector<Tool> get_functions() const;
	bool has_function(const std::string& name) const;
	void clear_functions();

	void execute(const std::string& name,
				 const std::string& arguments,
				 FunctionResult *res) const;
	WFGoTask *async_execute(const std::string& name,
							const std::string& arguments,
							FunctionResult *res) const;

private:
	std::map<std::string, FunctionDefinition> functions;
	std::map<std::string, FunctionHandler> handlers;
};

} // namespace wfai

#endif

