#include <sstream>
#include <iostream>
#include "llm_function.h"

namespace wfai {

void FunctionManager::register_function(const FunctionDefinition& definition,
										FunctionHandler handler)
{
	this->functions[definition.name] = definition;
	this->handlers[definition.name] = handler;
}

std::vector<Tool> FunctionManager::get_functions() const
{
	std::vector<Tool> tools;
	
	for (const auto& func : functions)
	{
		const FunctionDefinition& func_def = func.second;
		Tool tool;
		tool.type = "function";
		tool.function.name = func_def.name;
		tool.function.description = func_def.description;
		tool.function.parameters.type = "object";

		for (const auto& pair : func_def.parameters)
		{
			const std::string& param_name = pair.first;
			const FunctionParameter& param = pair.second;

			Property prop;
			prop.type = param.type;
			prop.description = param.description;

			if (!param.enum_values.empty())
				prop.enum_values = param.enum_values;

			if (!param.default_value.empty())
				prop.default_value = param.default_value;

			tool.function.parameters.properties[param_name] = prop;
		}

		tool.function.parameters.required = func_def.required_parameters;
		tools.push_back(tool);
	}

	return tools;
}

std::vector<FunctionDefinition> FunctionManager::get_function_definitions() const
{
	std::vector<FunctionDefinition> definitions;
	for (const auto& pair : this->functions)
		definitions.push_back(pair.second);

	return definitions;
}

FunctionResult FunctionManager::execute_function(const FunctionCall& call) const
{
	FunctionResult result;
	result.name = call.name;

	auto it = this->handlers.find(call.name);
	if (it == this->handlers.end())
	{
		result.success = false;
		result.error_message = "Function not found: " + call.name;
		return result;
	}

	result = it->second(call.arguments); // TODO: change into WFGoTask
	if (!result.success)
	{
		result.error_message = "Function " + call.name + " execution error.";
		// + std::string(result.err_msg)
	}
	return result;
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
	this->definition.parameters[name] = FunctionParameter("string", desc);
	if (required)
		this->definition.required_parameters.push_back(name);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_number_parameter(const std::string& name,
													   const std::string& desc,
													   bool required)
{
	this->definition.parameters[name] = FunctionParameter("number", desc);
	if (required)
		this->definition.required_parameters.push_back(name);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_boolean_parameter(const std::string& name,
														const std::string& desc,
														bool required)
{
	this->definition.parameters[name] = FunctionParameter("boolean", desc);
	if (required)
		this->definition.required_parameters.push_back(name);
	return *this;
}

FunctionBuilder& FunctionBuilder::add_enum_parameter(const std::string& name,
													 const std::string& desc, 
													 const std::vector<std::string>& values,
													 bool required)
{
	FunctionParameter param("string", desc);
	param.enum_values = values;
	this->definition.parameters[name] = param;
	if (required)
		this->definition.required_parameters.push_back(name);
	return *this;
}

FunctionDefinition FunctionBuilder::build() const
{
	return this->definition;
}

} // namespace wfai
