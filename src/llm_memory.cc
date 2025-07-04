#include "llm_memory.h"

namespace wfai
{

void Memory::add_message(const std::string &id, const Message &msg)
{
	sessions_[id].push_back(msg);
}

const std::vector<Message> &Memory::get_history(const std::string &id) const
{
	static const std::vector<Message> empty;
	auto it = sessions_.find(id);
	if (it != sessions_.end())
		return it->second;
	return empty;
}

void Memory::clear(const std::string &id)
{
	sessions_.erase(id);
}

void Memory::clear_last_query(const std::string &id)
{
	auto it = sessions_.find(id);
	if (it == sessions_.end())
		return;

	auto& messages = it->second;
	if (messages.empty())
		return;

	if (messages.back().role == "user")
		messages.pop_back();
}

} // namespace wfai
