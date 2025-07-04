#include <stdlib.h>
#include <string.h>
#include <string>
#include "workflow/WFHttpChunkedClient.h"
#include "chat_response.h"
#include "chat_request.h"

namespace wfai {

class LLMClient
{
public:
	using llm_extract_t = std::function<void(WFHttpChunkedTask*,
											 ChatCompletionRequest*,
											 ChatCompletionChunk*)>;

	using llm_callback_t = std::function<void(WFHttpChunkedTask*,
											  ChatCompletionRequest*,
											  ChatCompletionResponse*)>;

	WFHttpChunkedTask* create_chat_task(ChatCompletionRequest request,
										llm_extract_t extract,
										llm_callback_t callback);

public:
	LLMClient();
	LLMClient(const std::string& api_key);
	LLMClient(const std::string& api_key, const std::string& base_url);

private:
	void extract(WFHttpChunkedTask *task,
				 ChatCompletionRequest *req,
				 ChatCompletionResponse *resp,
				 llm_extract_t extract);

	void callback(WFHttpChunkedTask *task,
				  ChatCompletionRequest *req,
				  ChatCompletionResponse *resp,
				  llm_callback_t callback);
private:
	WFHttpChunkedClient client;
	std::string api_key;
	std::string base_url;
	int ttft; // time to first token (seconds)
	int tpft; // time per output token (seconds)
	int streaming_ttft;
	int streaming_tpft;
	int redirect_max;
};

} // namespace llm_client
