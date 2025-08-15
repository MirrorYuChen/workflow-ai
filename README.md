<div align="center">

# Workflow AI<br>C++ Task Chain and Graph for LLMs

</div>

**Workflow AI** provides a high-performance C++ library functionally similar to LangChain and LangGraph by interacting with Large Language Models (LLMs) and extends the task serial/parallel orchestration capabilities in  **[C++ Workflow](https://github.com/sogou/workflow)**.

Modern LLMs applications increasingly require not just **API requests** but also complex **tool calls** and **agent orchestration**. By abstracting the complexity of mixed I/O and compute workflows, this library makes LLMs integration easy and efficient for all C++ applications.

## 1. Features

- 💬 **Chatbot**: Basic demo which can be use directly as a chatbot
- 🚀 **High Performance**: Built on C++ Workflow for efficient async non-blocking I/O and Computation
- 📒 **Memory**: Simple memory module for multi round sessions
- 🔧 **Function Calling**: Native support for LLM function/tool calling
- ⚡ **Parallel Graph Execution**: Execute multiple tool calls in parallel asynchronously
- 🌊 **Streaming Support**: Real-time streaming responses
- 🛠️ **Client / Proxy**: Create task as Client or Proxy. Server is comming soon


## 2. RoadMap
<details>
<summary><strong>View Development Roadmap</strong> (Click to expand)</summary>

This is the very beginning of a multi-layer LLM interaction framework. Here's the implementation status and future plans:

### 2.1 Core Features
1. **Model Interaction Layer** (Partial)
   - [x] Task → Model → Callback
   - [x] Task → Model → Function → Model → Callback
   - [x] Streaming response (SSE)
   - [ ] Streamable protocol
   - [ ] Model KVCache loading
   - [ ] Prefill/decode optimization

2. **Tool Calling Layer** (Partial)
   - [x] Single tool execution
   - [x] Parallel tool execution
   - [ ] Workflow native task integration
   - [ ] MCP Framework (Multi-tool Coordination)
     - [ ] Local command execution (e.g., ls, grep)
     - [ ] Remote RPC integration
        
3. **Memory Storage Layer** (Partial)
   - [x] Context in-memory storage
   - [ ] Offload local disk storage
   - [ ] Offload distributed storage

### 2.2 API Modes
- [x] Asynchronous Task API (Done)
- [x] Synchronous API (2025.August.01)
- [ ] Semi-Sync API (In Progress)

### 2.3 Multi-modal Support
- [x] Text-to-text (Done)
- [ ] Text-to-image (In progress)
- [ ] Text-to-speech (Planned)
- [ ] Embeddings (Planned)

### 2.4 Model Providers
- [x] DeepSeek API (Done)
- [x] OpenAI-compatible APIs (Done)
- [ ] Claude API (In progress)
- [ ] Local model integration (Planned)

### 2.5 Network Modes
- [x] Client mode (Done)
- [x] Proxy mode (Partial)
- [ ] Server mode (In progress)
   - [ ] Session state management

### 2.6 Task Specialization
- [ ] Preset task templates (translation/summary/code)
- [ ] Prompt engineering
   - [ ] Few-shot integration
   - [ ] Dynamic prompt building

### 2.7 Output Structures
- [x] JSON (Done)
- [ ] Protobuf (In progress)
- [ ] Custom formats (Planned)
</details>

## 3. Compile

<details>
<summary><strong>Very Easy to Compile with Bazel or CMake</strong> (Click to expand)</summary>

### 3.1 Prerequisites

- Required C++11 or later

```bash
git clone https://github.com/holmes1412/workflow-ai.git
cd workflow-ai
```

### 3.2 Build with Bazel (Recommended)

```bash
# Build all targets
bazel build ...

# Run basic DeepSeek chatbot
bazel run :deepseek_chatbot -- <your_api_key>
```

### 3.3 Build with CMake

```bash
mkdir cmake.build && cd cmake.build

# Download workflow source code and make for the first time
# git clone https://github.com/sogou/workflow.git /PATH/TO/WORKFLOW
# cd /PATH/TO/WORKFLOW && make

cmake .. -D Workflow_DIR=/PATH/TO/WORKFLOW
make
./demo <your_api_key>
```
</details>

## 4. Quick Start

### 4.1 Chat Demo

This exmaple shows the basic steps to chat with LLMs.

Here is a one round chat , we can use any of the three kinds of APIs : `synchronous`, `aynchronous` and `task-based`.

```
🧑‍💻 user request 'hi'
         ↓
    ┌───────────┐
    │ Chat Task │ // asynchronous network task
    │  to LLMs  │ // send request get response
    └───────────┘
         ↓
  🧑‍💻 extract() // for streaming
         ↓
  🧑‍💻 callback()
```

🤖 **1. Synchronous API**

Let's begin with some simple one :`chat_completion_sync()` .

```cpp
int main()
{
 	LLMClient client("YOUR_API_KEY"); // build a client by `api_key`. support `base_url` and DeepSeek is default

	ChatCompletionRequest request;
	request.messages.push_back({"user", "hi"});

	ChatCompletionResponse response;
	SyncResult result = client.chat_completion_sync(request, response);

	if (result.success)
		printf("%s\n", response.choices[0].message.content.c_str());
	else
		printf("Request Failed : %s\n", result.error_message.c_str());

	return 0;
}
```

🤖 **2. Asynchronous API**

If we use **streaming** mode to receive every chunk from LLM servers, we can use `chat_completion_async()` which is similar to the generator in Python.

```cpp
int main()
{
	LLMClient client("YOUR_API_KEY");
	ChatCompletionRequest request;
	request.stream = true; // set this to use streaming
	request.messages.push_back({"user", "hi"});

	AsyncResult result = client.chat_completion_async(request);

	// ... you may do anything else until you need the data ...

	while (true)
	{
		ChatCompletionChunk *chunk = result.get_chunk();
		if (!chunk /*non-streaming*/ || chunk->state != RESPONSE_SUCCESS)
			break;

		if (!chunk->choices.empty() && !chunk->choices[0].delta.content.empty())
			printf("%s", chunk->choices[0].delta.content.c_str());

		if (chunk->last_chunk())
			break;
	}

	// if non streaming mode, use get_response()
	//ChatCompletionResponse *response = result.get_response();
}
```

🤖 **3. Task API**

Task-based APIs is useful for organizing our task graph.

In this example we use `create_chat_task()` so we can create a **WFHttpChunkedTask**, which can be used with any other workflow task, push_back into a SeriesWork, ParalleWork, or a [DAG in workflow](https://github.com/sogou/workflow/blob/master/docs/en/tutorial-11-graph_task.md) .

```cpp

int main()
{
	LLMClient client("YOUR_API_KEY");
	ChatCompletionRequest request;
	request.model = "deepseek-reasoner"; // set to use model DeepSeek-R1
	request.messages.push_back({"user", "hi"});

	auto *task = client.create_chat_task(request, extract, callback);
	task->start();

	// pause or use wait_group.wait()
}

// here we get each chunk
void extract(WFHttpChunkedTask *task, ChatCompletionRequest *req, ChatCompletionChunk *chunk)
{
	if (!chunk->choices.empty())
	{
		if (!chunk->choices[0].delta.reasoning_content.empty())
			printf("%s", chunk->choices[0].delta.reasoning_content.c_str());
		else if (chunk->choices[0].delta.content.empty())
			printf("%s", chunk->choices[0].delta.content.c_str());
	}
}

// here we get the final response, both streaming or non streaming
void callback(WFHttpChunkedTask *task, ChatCompletionRequest *req, ChatCompletionResponse *resp)
{
	// if (task->get_state() == WFT_STATE_SUCESS)

	if (req->model == "deepseek-reasoner")
		printf("%s\n", resp->choices[0].message.reasoning_content.c_str());
	printf("%s\n", resp->choices[0].message.content.c_str());
}

```

### 4.2 Tool Calling

This example shows how to use function call as tools.

The following task flow seems complecated but it just show the internal architecture.

```
👩‍💻 preparation: register functions
         ↓
👩‍💻 user request for multiple info
         ↓
    ┌───────────┐
    │ Chat Task │ // asynchronous network task
    │  to LLMs  │ // send request get response
    └───────────┘
         ↓
LLMs response for tool_calls
         ↓
create WFGoTask for local function computing
         ↓
┌─────────┬─────────┬─────────┐
│ Tool A  │ Tool B  │ Tool C  │ // execute in parallel
│ Series1 │ Series2 │ Series3 │ // by compute threads
└─────────┴─────────┴─────────┘
         ↓
   collect results
         ↓
    ┌───────────┐
    │ Chat Task │ // send all the context and results
    │  to LLMs  │ // multi round supported by memory module
    └───────────┘
         ↓ 
  👩‍💻 extract() 
         ↓
  👩‍💻 callback()
```

🤖 Code Example. Here are 3 steps.

**Step-1**  : Define our `function`. 

This preparation only need to do once before all the requests.

The parameters for all the functions are fixed : 
- arguments : the arguments from LLMs in Json format, e.g. {"location":"Shenzhen"}
- result : the return value for our function to fill

```cpp
void get_current_weather(const std::string& arguments, FunctionResult *result)
{
    result->success = true;
    result->result = "Weather: 25°C, Sunny";
}
```

**Step-2** : Register the function into `function_manager` and add function_manager into client. 

This is one time preparation, too.

```cpp
int main()
{
    LLMClient client("your_api_key");
    FunctionManager func_mgr;
    client.set_function_manager(&func_mgr);

    // Register function
    FunctionDefinition weather_func = {
        .name = "get_weather",
        .description = "Get current weather information"
    };
    func_mgr.register_function(weather_func, get_current_weather);

    ...
}
```

**Step-3** : Start a request with tools.  

As long as we have function in manager and set `request.tool_choice`, LLMs will tell us how to use corresponding tools and this library will help us execute the tools automatically. Then the library will give the response to LLMs and let it summarise by the function results.

```cpp
{
    ChatCompletionRequest request;
    request.model = "deepseek-chat";
    request.messages.push_back({"user", "What's the weather like?"});
    request.tool_choice = "auto"; // set `auto` or `required` to enable tools using

	ChatCompletionResponse response;
	auto result = client.chat_completion_sync(request, response);

	if (result.success)
		printf("%s\n", response.choices[0].message.content.c_str());
	// "Shenzhen has sunny weather today, with a temperature of 25°C and pleasant weather."
}
```

### 4.3 Parallel Tool Execution

The library automatically detects when multiple tool calls are returned by the LLM and executes them in parallel using Workflow's `ParallelWork`:

In [example/parallel_tool_call.cc](./examples/parallel_tool_call.cc), we can make request like this:

```cpp
// When LLM returns multiple tool calls, they execute in parallel
request.messages.push_back({"user", "Tell me the weather in Beijing and Shenzhen, and the current time"});
```

This will execute weather queries for both cities and time query simultaneously, significantly improving response time.

```
./bazel-bin/parallel_tool_call <API_KEY>
registered weather and time functions successfully.
Starting parallel tool calls test...
function calling...get_current_weather()
function calling...get_current_time()
parameters: {"location": "Beijing"}
function calling...get_current_weather()
parameters: {"location": "Shenzhen"}
parameters: {}
Response status: 200

Response Content:
The current temperature in Beijing is 30°C and in Shenzhen it is 28°C. The time now is 10:05 PM on Friday, August 8, 2025.
```

## 5. API Reference

### 5.1 Core Classes

- **`LLMClient`**: Main client for LLM interactions
- **`FunctionManager`**: Manages function registration and execution
- **`ChatCompletionRequest`**: Request send by users
- **`ChatCompletionResponse`**: Response data structure from LLMs

### 5.2 Examples

| Example | Description |
|---------|-------------|
| [sync_demo.cc](./examples/sync_demo.cc) | Most simplest demo in synchronous API |
| [async_demo.cc](./examples/async_demo.cc) | Asynchronous API to show the usage of getting streaming chunk |
| [task_demo.cc](./examples/task_demo.cc) | Task API which is able to create tasks for Graph, also asynchronous |
| [deepseek_chatbot.cc](./examples/deepseek_chatbot.cc) | DeepSeek chatbot implementation for multi round session with memory |
| [tool_call.cc](./examples/tool_call.cc) | Basic function calling with single tool |
| [parallel_tool_call.cc](./examples/parallel_tool_call.cc) | Demonstrates parallel execution of multiple tools |

## 5.3 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](./LICENSE) file for details.

## 5.4 Dependencies

- [Sogou Workflow](https://github.com/sogou/workflow) v0.11.9
- OpenSSL
- pthread

For more examples, check the [examples/](./examples) directory.  Detailed documentations are coming soon.

## 6. Contact

Have questions, suggestions, or want to contribute? Feel free to reach out!

✉️ **Email**: [liyingxin1412@gmail.com](mailto:liyingxin1412@gmail.com)  
🧸  **GitHub**: [https://github.com/holmes1412](https://github.com/holmes1412)  

We welcome all feedback and contributions to make Workflow AI even better!

