#include "workflow/WFTaskFactory.h"
#include "llm_function.h"

namespace wfai {

bool FunctionManager::register_function(const FunctionDefinition& def,
										FunctionHandler handler)
{
	if (this->functions.find(def.name) != this->functions.end())
		return false;

	this->functions.emplace(def.name, def);
	this->handlers.emplace(def.name, std::move(handler));
	return true;
}

std::vector<Tool> FunctionManager::get_functions() const
{
	std::vector<Tool> tools;

	for (const auto& func : this->functions)
	{
		Tool tool;
		tool.type = "function";
		tool.function = func.second;
		tools.push_back(std::move(tool));
	}

	return tools;
}

void FunctionManager::execute(const std::string& name,
							  const std::string& arguments,
							  FunctionResult *result) const
{
	result->name = name;

	auto it = this->handlers.find(name);
	if (it == this->handlers.end())
	{
		result->success = false;
		result->error_message = "Function not found: " + name;
		return;
	}

	it->second(arguments, result);
	if (!result->success)
	{
		result->error_message = "Function " + name + " execution error.";
		// + std::string(result->err_msg)
	}
	return;
}

WFGoTask *FunctionManager::async_execute(const std::string& name,
										 const std::string& arguments,
										 FunctionResult *result) const
{
	auto it = this->handlers.find(name);
	if (it == this->handlers.end())
	{
		if (result)
		{
			result->name = name;
			result->success = false;
			result->error_message = "Function not found: " + name;
		}
		return nullptr; // TODO: should use WFEmptyTask instead
	}

	WFGoTask *task = WFTaskFactory::create_go_task(name, it->second,
												   arguments, result);
	return task;
}

bool FunctionManager::has_function(const std::string& name) const
{
	return this->functions.find(name) != this->functions.end();
}

void FunctionManager::clear_functions()
{
	this->functions.clear();
	this->handlers.clear();
}

} // namespace wfai
