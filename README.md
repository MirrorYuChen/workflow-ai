# Workflow AI - C++ Networking and Computing Task for LLMs

As modern LLM applications increasingly require not just API requests but also complex tool calls and agent orchestration, efficiently organizing these mixed network and compute tasks presents a critical challenge for C++ programs. 

**Workflow AI** addresses this challenge by providing a high-performance C++ library for interacting with Large Language Models (LLMs) built on [C++ Workflow](https://github.com/sogou/workflow) framework, which support 💬 `basic chatbot` , 🔧`function calling` and ⚡`parallel execution`.

By abstracting the complexity of mixed I/O and compute workflows, we make LLM integration easy and efficient for all C++ applications.

## 1. Features

- 🚀 **High Performance**: Built on Sogou C++ Workflow for efficient async non-blocking I/O and Computation
- 📒 **Memory**: Simple memory module for multi round sessions
- 🔧 **Function Calling**: Native support for LLM function/tool calling
- ⚡ **Parallel Tool Execution**: Execute multiple tool calls in parallel asynchronously
- 🌊 **Streaming Support**: Real-time streaming responses
- 🛠️ **Client/Proxy**: Create task as Client or Proxy. Server is comming soon


## 2. RoadMap
<details>
<summary><strong>View Development Roadmap</strong> (Click to expand)</summary>

This is the very beginning of a multi-layer LLM interaction framework. Here's the implementation status and future plans:

### 2.1 Core Features
1. **Model Interaction Layer** (Partial)
   - [x] Task → Model → Callback
   - [x] Task → Model → Function → Model → Callback
   - [x] Streaming response (SSE)
   - [ ] Model KVCache loading
   - [ ] Prefill/decode optimization

2. **Tool Calling Layer** (Partial)
   - [x] Single tool execution
   - [x] Parallel tool execution
   - [ ] MCP Framework (Multi-tool Coordination)
     - [ ] Local command execution (e.g., ls, grep)
     - [ ] Remote RPC integration
        
3. **Memory Storage Layer** (Partial)
   - [x] context in-memory storage
   - [ ] offload local disk storage
   - [ ] offload distributed storage

### 2.2 Multi-modal Support
- [x] Text-to-text (Done)
- [ ] Text-to-image (In progress)
- [ ] Text-to-speech (Planned)
- [ ] Embeddings (Planned)

### 2.3 Model Providers
- [x] DeepSeek API (Done)
- [x] OpenAI-compatible APIs (Done)
- [ ] Claude API (In progress)
- [ ] Local model integration (Planned)

### 2.4 Network Modes
- [x] Client mode (Done)
- [x] Proxy mode (Partial)
- [ ] Server mode (In progress)
   - [ ] Session state management

### 2.5 Task Specialization
- [ ] Preset task templates (translation/summary/code)
- [ ] Prompt engineering
   - [ ] Few-shot integration
   - [ ] Dynamic prompt building
- [ ] Tool registry
   - [ ] Weather (get_weather)
   - [ ] Math (calculate)
   - [ ] DB queries (VectorDB/Redis)

### 2.6 Output Structures
- [x] JSON (Done)
- [ ] Protobuf (In progress)
- [ ] Custom formats (Planned)
</details>

## 3. Quick Start

### 3.1 Prerequisites

- Required C++11 or later

```bash
git clone https://github.com/holmes1412/workflow_ai.git
cd workflow_ai
```

### 3.2 Build with Bazel (Recommended)

```bash
# Build all targets
bazel build ...

# Run basic DeepSeek chatbot
bazel run :deepseek_chatbot -- <your_api_key>

# Run parallel tool call example  
bazel run :parallel_tool_call -- <your_api_key>
```

### 3.3 Build with CMake

```bash
mkdir cmake.build && cd cmake.build

# Download workflow source code for the first time
# git clone https://github.com/sogou/workflow.git /PATH/TO/WORKFLOW

cmake .. -D Workflow_DIR=/PATH/TO/WORKFLOW
make
```

## 4. Usage

### 4.1 Basic chat demo

This exmaple shows the basic steps to chat with LLMs.

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

🤖

**Step-1** : Make `client` and `request`. Then create and start a `task` like we usually do in C++ Workflow.

```cpp
#include "llm_client.h"
using namespace wfai;

int main(int argc, char *argv[])
{
    LLMClient client("YOUR_API_KEY"); // build a client by `api_key`. support `base_url` and DeepSeek is default

    wfai::ChatCompletionRequest request; // fill the request for LLMs
    request.model = "deepseek-reasoner";
    request.stream = true;
    request.messages.push_back({"user", "hi"});

    auto *task = client.create_chat_task(request, extract, callback); // create a WFHttpChunkedTask

    task->start();
    wait_group.wait(); // need to pause the main thread since the task is asynchronous
    return 0;
}
```

🤖

**Step-2** : Implement `extract()` for streaming.

 If **request.stream = false**, you may ignore this.  `extract()` will be called every time we get a chunk data from LLMs.  

- task : the task we created just now. We can get user_data or it's SeriesWork.
- request : the request we created. It is copied inside so don't worry about the life cycle.
- chunk : the delta response for each token. 

```cpp
void extract(WFHttpChunkedTask *task, ChatCompletionRequest *request, ChatCompletionChunk *chunk)
{
    // 
    if (!chunk->choices[0].delta.reasoning_content.empty())
        fprintf(stderr, "reason content=%s\n", chunk->choices[0].delta.reasoning_content.c_str());
    else
        fprintf(stderr, "content=%s\n", chunk->choices[0].delta.content.c_str());
}
```

🤖

**Step-3** : Implement `callback()`. 

Every task will get to callback both for steaming or non streaming.

- response : the complete message for this task

```cpp
void callback(WFHttpChunkedTask *task, ChatCompletionRequest *request, ChatCompletionResponse *response)
{
    // ... check state and response
    if (request->model == "deepseek-reasoner")
        fprintf(stderr, "\n<think>\n%s\n<\\think>\n", response->choices[0].message.reasoning_content.c_str());
    fprintf(stderr, "\n%s\n", response->choices[0].message.content.c_str());
}
```

### 4.2 Basic Tool Calling

This example shows how to use function call as tools.

The following task flow seems complecated but it just show the internal architecture. The usage for users consists of 4 steps only.

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
    │  to LLMs  │ // multi round by memory module
    └───────────┘
         ↓ 
  👩‍💻 extract() 
         ↓
  👩‍💻 callback()
```

🤖

**Step-1**  : Define our `function`. 

This preparation only need to do once before all the requests.

The parameters for all the functions are fixed : 
- const std::string& arguments : the arguments from LLMs in Json format, e.g. {"location":"Shenzhen"}
- FunctionResult *result : the result to fill

```cpp
void get_current_weather(const std::string& arguments, FunctionResult *result)
{
    result->success = true;
    result->result = "Weather: 25°C, Sunny";
}
```

🤖

**Step-2** : Register the function into `function_manager` and add function_manager into client. 

This is one time preparation, too.

```cpp

int main() {
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

🤖

**Step-3** : Start a `task` with tools.   

As long as we have function in manager and set `request.tool_choice`, LLMs will tell us how to use corresponding tools and this library will help us execute the tools automatically. Then the library will give the response to LLMs and let it summarise by the function results.

```cpp
{
    ChatCompletionRequest request;
    request.model = "deepseek-chat";
    request.messages.push_back({"user", "What's the weather like?"});
    request.tool_choice = "auto"; // set `auto` or `required` to enable tools using

    auto *task = client.create_chat_task(request, extract_fn, callback_fn);
    task->start();
    ...
}
```

🤖

**Step-4**  : Implement `extract()` and `callback()`.

```cpp
void extract(WFHttpChunkedTask *task, ChatCompletionRequest *request, ChatCompletionChunk *chunk);

void callback(WFHttpChunkedTask *task, ChatCompletionRequest *request, ChatCompletionResponse *response);
```

### 4.3 Parallel Tool Execution

The library automatically detects when multiple tool calls are returned by the LLM and executes them in parallel using Workflow's `ParallelWork`:

```cpp
// When LLM returns multiple tool calls, they execute in parallel
request.messages.push_back({"user", "Tell me the weather in Beijing and Shanghai, and the current time"});
```

This will execute weather queries for both cities and time query simultaneously, significantly improving response time.

## 5. API Reference

### 5.1 Core Classes

- **`LLMClient`**: Main client for LLM interactions
- **`FunctionManager`**: Manages function registration and execution
- **`ChatCompletionRequest`**: Request send by users
- **`ChatCompletionResponse`**: Response data structure from LLMs

### 5.2 Examples

| Example | Description |
|---------|-------------|
| [demo.cc](./examples/demo.cc) | Most simplest demo |
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
🐱 **GitHub**: [https://github.com/holmes1412](https://github.com/holmes1412)  

We welcome all feedback and contributions to make Workflow AI even better!

