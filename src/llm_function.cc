#include <sstream>
#include <iostream>
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
/*
std::vector<FunctionDefinition> FunctionManager::get_function_definitions() const
{
	std::vector<FunctionDefinition> definitions;
	for (const auto& pair : this->functions)
		definitions.push_back(pair.second);

	return definitions;
}
*/

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

/*
FunctionBuilder::FunctionBuilder(const std::string& name,
								 const std::string& desc)
{
	this->definition.name = name;
	this->definition.description = desc;
}

FunctionBuilder& FunctionBuilder::add_string_parameter(const std::string& name,
													   const std::string& desc,
													   bool required)
{
	ParameterProperty property = {
		.type = "string",
		.description = desc,
	};

	this->definition.add_parameter(name, std::move(property), required);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_number_parameter(const std::string& name,
													   const std::string& desc,
													   bool required)
{
	ParameterProperty property = {
		.type = "number",
		.description = desc,
	};

	this->definition.add_parameter(name, std::move(property), required);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_boolean_parameter(const std::string& name,
														const std::string& desc,
														bool required)
{
	ParameterProperty property = {
		.type = "boolean",
		.description = desc,
	};

	this->definition.add_parameter(name, std::move(property), required);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_enum_parameter(const std::string& name,
													 const std::string& desc, 
													 const std::vector<std::string>& values,
													 bool required)
{
	ParameterProperty property;
	property.type = "string";  // 枚举在JSON Schema中通常定义为string类型
	property.description = desc;
	property.enum_values = values;  // 存储有效枚举值列表

	this->definition.add_parameter(name, std::move(property), required);

	return *this;
}

FunctionDefinition FunctionBuilder::build() const
{
	return this->definition;
}
*/

} // namespace wfai
