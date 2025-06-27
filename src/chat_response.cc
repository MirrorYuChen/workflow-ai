#include "chat_response.h"

namespace llm_task {

ChatResponse::ChatResponse(ChatResponse&& move)
{
	id = std::move(move.id);
	object = std::move(move.object);
	created = move.created;
	model = std::move(move.model);
	system_fingerprint = std::move(move.system_fingerprint);
	choices = std::move(move.choices);
	usage = std::move(move.usage);
	is_stream = move.is_stream;
	is_done = move.is_done;
/*
	is_first_chunk = move.is_first_chunk;
	is_last_chunk = move.is_last_chunk;
	stream_content = std::move(move.stream_content);
*/
}

ChatResponse& ChatResponse::operator=(ChatResponse&& move)
{
	if (this != &move)
	{
		id = std::move(move.id);
		object = std::move(move.object);
		created = move.created;
		model = std::move(move.model);
		system_fingerprint = std::move(move.system_fingerprint);
		choices = std::move(move.choices);
		usage = std::move(move.usage);
		is_stream = move.is_stream;
		is_done = move.is_done;
/*
		is_first_chunk = move.is_first_chunk;
		is_last_chunk = move.is_last_chunk;
		stream_content = std::move(move.stream_content);
*/
	}
	return *this;
}

bool ChatResponse::parse_choice(const json_value_t *choice)
{
	const json_object_t *choice_obj = json_value_object(choice);
	if (!choice_obj)
		return false;

	Choice new_choice;

	const json_value_t *index_val = json_object_find("index", choice_obj);
	if (index_val && json_value_type(index_val) == JSON_VALUE_NUMBER)
		new_choice.index = json_value_number(index_val);

	const json_value_t *finish_val = json_object_find("finish_reason", choice_obj);
	if (finish_val && json_value_type(finish_val) == JSON_VALUE_STRING)
		new_choice.finish_reason = json_value_string(finish_val);

	if (!this->parse_content(choice_obj, new_choice))
		return false;

	const json_value_t *logprobs_val = json_object_find("logprobs", choice_obj);
	if (logprobs_val && json_value_type(logprobs_val) == JSON_VALUE_OBJECT)
	{
		const json_object_t *logprobs_obj = json_value_object(logprobs_val);
		if (!parse_logprobs(logprobs_obj, new_choice.logprobs))
			return false;
	}

	choices.push_back(std::move(new_choice));
	return true;
}

bool ChatResponse::parse_usage(const json_value_t *usage_val)
{
	if (!usage_val || json_value_type(usage_val) != JSON_VALUE_OBJECT)
		return false;

	const json_object_t *usage_obj = json_value_object(usage_val);
	if (!usage_obj)
		return false;

	const json_value_t *prompt_tokens_val = json_object_find("prompt_tokens", usage_obj);
	if (prompt_tokens_val && json_value_type(prompt_tokens_val) == JSON_VALUE_NUMBER)
		usage.prompt_tokens = json_value_number(prompt_tokens_val);

	const json_value_t *completion_tokens_val = json_object_find("completion_tokens", usage_obj);
	if (completion_tokens_val && json_value_type(completion_tokens_val) == JSON_VALUE_NUMBER)
		usage.completion_tokens = json_value_number(completion_tokens_val);

	const json_value_t *total_tokens_val = json_object_find("total_tokens", usage_obj);
	if (total_tokens_val && json_value_type(total_tokens_val) == JSON_VALUE_NUMBER)
		usage.total_tokens = json_value_number(total_tokens_val);

	return true;
}

bool ChatResponse::parse_logprobs(const json_object_t* logprobs_obj, Logprobs& logprobs)
{
	if (!logprobs_obj)
		return false;

	const json_value_t* content_val = json_object_find("content", logprobs_obj);
	if (!content_val || json_value_type(content_val) != JSON_VALUE_ARRAY)
		return false;

	const json_array_t* content_arr = json_value_array(content_val);
	const json_value_t* content_item;
	json_array_for_each(content_item, content_arr)
	{
		if (json_value_type(content_item) != JSON_VALUE_OBJECT)
			continue;

		Logprob logprob;
		const json_object_t* content_obj = json_value_object(content_item);

		const json_value_t* token_val = json_object_find("token", content_obj);
		if (!token_val || !parse_token_logprob(token_val, logprob.current_info))
			continue;

		const json_value_t* top_logprobs_val = json_object_find("top_logprobs", content_obj);
		if (top_logprobs_val)
		{
			if (!parse_top_logprobs(top_logprobs_val, logprob.top_logprobs))
				continue;
		}

		logprobs.content.push_back(std::move(logprob));
	}

	return true;
}

bool ChatResponse::parse_token_logprob(const json_value_t* token_val, TokenLogprob& token_info)
{
	if (!token_val || json_value_type(token_val) != JSON_VALUE_OBJECT)
		return false;

	const json_object_t* token_obj = json_value_object(token_val);

	const json_value_t* token_str = json_object_find("token", token_obj);
	if (!token_str || json_value_type(token_str) != JSON_VALUE_STRING)
		return false;
	token_info.token = json_value_string(token_str);

	const json_value_t* logprob_val = json_object_find("logprob", token_obj);
	if (!logprob_val || json_value_type(logprob_val) != JSON_VALUE_NUMBER)
		return false;
	token_info.logprob = json_value_number(logprob_val);

	const json_value_t* bytes_val = json_object_find("bytes", token_obj);
	if (bytes_val)
	{
		if (json_value_type(bytes_val) == JSON_VALUE_ARRAY)
		{
			const json_array_t* bytes_arr = json_value_array(bytes_val);
			const json_value_t* byte_val;
			json_array_for_each(byte_val, bytes_arr)
			{
				if (json_value_type(byte_val) == JSON_VALUE_NUMBER)
					token_info.bytes.push_back(json_value_number(byte_val));
			}
		}
	}

	return true;
}

bool ChatResponse::parse_top_logprobs(const json_value_t* top_logprobs_val, std::vector<TopLogprob>& top_logprobs)
{
	if (!top_logprobs_val || json_value_type(top_logprobs_val) != JSON_VALUE_ARRAY)
		return false;

	const json_array_t* top_arr = json_value_array(top_logprobs_val);
	const json_value_t* top_val;
	json_array_for_each(top_val, top_arr)
	{
		if (json_value_type(top_val) != JSON_VALUE_OBJECT)
			continue;

		TopLogprob top_info;
		if (parse_token_logprob(top_val, *reinterpret_cast<TokenLogprob*>(&top_info)))
		{
			top_logprobs.push_back(std::move(top_info));
		}
	}

	return true;
}

bool ChatResponse::parse_json(const char *msg, size_t size)
{
	char *json_buf = (char*)malloc(size + 1);
	if (!json_buf)
	{
		perror("malloc");
		return false;
	}

	memcpy(json_buf, msg, size);
	json_buf[size] = '\0';

	json_value_t *root = json_value_parse(json_buf);
	free(json_buf);

	if (!root || json_value_type(root) != JSON_VALUE_OBJECT)
	{
		// TODO: set as error
		return false;
	}

	const json_object_t *obj = json_value_object(root);

	const json_value_t *id_val = json_object_find("id", obj);
	if (id_val && json_value_type(id_val) == JSON_VALUE_STRING)
		id = json_value_string(id_val);

	const json_value_t *object_val = json_object_find("object", obj);
	if (object_val && json_value_type(object_val) == JSON_VALUE_STRING)
		object = json_value_string(object_val);

	const json_value_t *created_val = json_object_find("created", obj);
	if (created_val && json_value_type(created_val) == JSON_VALUE_NUMBER)
		created = json_value_number(created_val);

	const json_value_t *model_val = json_object_find("model", obj);
	if (model_val && json_value_type(model_val) == JSON_VALUE_STRING)
		model = json_value_string(model_val);

	const json_value_t *fingerprint_val = json_object_find("system_fingerprint", obj);
	if (fingerprint_val && json_value_type(fingerprint_val) == JSON_VALUE_STRING)
		system_fingerprint = json_value_string(fingerprint_val);

	const json_value_t *choices_val = json_object_find("choices", obj);
	if (!choices_val || json_value_type(choices_val) != JSON_VALUE_ARRAY)
	{
		fprintf(stderr, "Error: No necessary object 'choices'\n");
		json_value_destroy(root);
		return false;
	}

	const json_array_t *arr = json_value_array(choices_val);
	const json_value_t *choice;
	bool ret = true;

	json_array_for_each(choice, arr)
	{
		if (json_value_type(choice) != JSON_VALUE_OBJECT)
			continue;

		if (!parse_choice(choice))
		{
			fprintf(stderr, "Error: Invalid choice data\n");
			ret = false;
			break;
		}
	}

	const json_value_t *usage_val = json_object_find("usage", obj);
	if (usage_val)
	{
		this->parse_usage(usage_val);
		// TODO:
		// check ret as error
		// is_last_chunk = true;
		// stream_content.last_chunk = std::string(msg, size);
	}

	json_value_destroy(root);
	return ret;
}

ChatCompletionResponse::ChatCompletionResponse(ChatCompletionResponse&& move) : 
	ChatResponse(std::move(move))
{
}

ChatCompletionResponse& ChatCompletionResponse::operator=(ChatCompletionResponse&& move)
{
	if (this != &move) {
		ChatResponse::operator=(std::move(move));
		usage = std::move(move.usage);
	}
	return *this;
}

bool ChatCompletionChunk::parse_content(const json_object_t *object, Choice& choice)
{
	const json_value_t *delta_val = json_object_find("delta", object);
	if (!delta_val || json_value_type(delta_val) != JSON_VALUE_OBJECT)
		return false;

	const json_object_t *delta_obj = json_value_object(delta_val);
	if (!delta_obj)
		return false;

	const json_value_t *role_val = json_object_find("role", delta_obj);
	if (role_val && json_value_type(role_val) == JSON_VALUE_STRING)
		choice.delta.role = json_value_string(role_val);

	const json_value_t *content_val = json_object_find("content", delta_obj);
	if (content_val && json_value_type(content_val) == JSON_VALUE_STRING)
		choice.delta.content = json_value_string(content_val);

	const json_value_t *reasoning_val = json_object_find("reasoning_content", delta_obj);
	if (reasoning_val && json_value_type(reasoning_val) == JSON_VALUE_STRING)
		choice.delta.reasoning_content = json_value_string(reasoning_val);
	
/*
	if (stream_content.first_chunk.empty()) {
		is_first_chunk = true;
		stream_content.first_chunk = std::string(msg, size);
	}
*/
	return true;
}

ChatCompletionChunk::ChatCompletionChunk(ChatCompletionChunk&& move) :
	ChatResponse(std::move(move))
{
}

ChatCompletionChunk& ChatCompletionChunk::operator=(ChatCompletionChunk&& move)
{
	if (this != &move)
		ChatResponse::operator=(std::move(move));

	return *this;
}

bool ChatCompletionResponse::parse_content(const json_object_t *object, Choice& choice)
{
	const json_value_t *message_val = json_object_find("message", object);
	if (!message_val || json_value_type(message_val) != JSON_VALUE_OBJECT)
		return false;

	const json_object_t *message_obj = json_value_object(message_val);
	if (!message_obj)
		return false;

	const json_value_t *role_val = json_object_find("role", message_obj);
	if (role_val && json_value_type(role_val) == JSON_VALUE_STRING)
		choice.message.role = json_value_string(role_val);

	const json_value_t *content_val = json_object_find("content", message_obj);
	if (content_val && json_value_type(content_val) == JSON_VALUE_STRING)
		choice.message.content = json_value_string(content_val);

	const json_value_t *reasoning_val = json_object_find("reasoning_content", message_obj);
	if (reasoning_val && json_value_type(reasoning_val) == JSON_VALUE_STRING)
		choice.message.reasoning_content = json_value_string(reasoning_val);

	return true;
}

bool ChatCompletionResponse::append_buffer(const void* data, size_t size)
{
	if (this->buffer.size + size > this->buffer.capacity)
	{
		size_t new_cap = (this->buffer.size + size) * 3 / 2;
		void *new_buf = realloc(this->buffer.ptr, new_cap);
		
		if (!new_buf)
			return false;

		this->buffer.ptr = new_buf;
		this->buffer.capacity = new_cap;
	}
	
	memcpy((char *)this->buffer.ptr + this->buffer.size, data, size);
	this->buffer.size += size;
	return true;
}

} // namespace llm_task
