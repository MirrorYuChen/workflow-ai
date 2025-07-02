#ifndef LLM_TASK_MEMORY_H
#define LLM_TASK_MEMORY_H

#include <string>
#include <vector>
#include <unordered_map>
#include "chat_request.h"

namespace llm_task
{

class Memory
{
public:
	void add_message(const std::string &session_id, const Message &msg);
	const std::vector<Message> &get_history(const std::string &session_id) const;
	void clear(const std::string &session_id);
	void clear_last_query(const std::string &id);

private:
	std::unordered_map<std::string, std::vector<Message>> sessions_;
};

} // namespace llm_task

#endif // LLM_TASK_MEMORY_H
