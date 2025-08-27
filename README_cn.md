<div align="center">

# Workflow AI<br>面向大语言模型的 C++ 任务图

</div>

**Workflow AI** 用于支持 C++ 程序接入大语言模型（LLMs）。大语言模型应用需要的不仅仅是 **API 请求**，还需要复杂的**tool calling 工具调用**和**Agents智能体的编排**，因此这个项目扩展了 **[C++ Workflow](https://github.com/sogou/workflow)** 中的任务串行/并行编排能力，对网络通信和计算的互相调度进行了纯异步的打通。在功能上和 LangChain 和 LangGraph 类似，而在性能上延续Workflow的优异，可以使所有 C++ 应用程序的大语言模型集成变得简单高效。

👉 [ [English README](/README.md) ]

## 1. 主要功能

- 💬 **对话示例**：可以直接和大语言模型比如DeepSeek进行多轮会话的demo
- 🔧 **tool calls**：支持自动tool calls（当前仅限函数调用function call）
- 📒 **记忆模块**：用于多轮会话的上下文记忆模块
- ➡️ **DAG任务图**：基于C++ Workflow构建DAG任务图
- 🌊 **streaming协议**：实时流式响应（支持包括同步、半同步、任务的接口）
- ⚡️ **并行计算**：一次会话内部的多个工具调用可以并行执行
- 🚀 **高性能异步调度**：高效的异步非阻塞网络I/O和计算调度
- 📮 **客户端/代理**：创建客户端或代理任务，服务端功能正在开发

## 2. 发展路线图
<details>
<summary><strong>查看开发路线图</strong>（点击展开）</summary>

这是一个多层大语言模型交互框架的起点。以下是实现状态和未来计划：

### 2.1 核心功能
1. **模型交互层**
   - [x] 对话：任务 → 模型 → 回调
   - [x] 工具：任务 → 模型 → 函数 → 模型 → 回调
   - [x] 对话：任务 + 读取上下文 → 模型 → 回调 + 保持上下文
   - [x] 流式响应：SSE协议
   - [ ] Streamable HTTP协议
   - [ ] 模型 KVCache 加载
   - [ ] Prefill/Decode优化

2. **工具调用层**
   - [x] 单工具执行
   - [x] 并行工具执行
   - [ ] Workflow 原生任务示例（开发中）
   - [ ] MCP 框架（多工具协调）
     - [ ] 本地命令执行（如 ls、grep）
     - [ ] 远程 RPC 集成
        
3. **上下文记忆存储**
   - [x] 上下文内存存储
   - [ ] 本地磁盘存储卸载
   - [ ] 分布式存储卸载

### 2.2 API 模式
- [x] 异步任务 API
- [x] 同步Sync API（已完成，2025年8月1日）
- [x] 半同步Async API（已完成，2025年8月16日）

### 2.3 多模态支持
- [x] 文本到文本
- [ ] 文本到图片
- [ ] 文本到语音
- [ ] 嵌入向量

### 2.4 模型提供商
- [x] DeepSeek API
- [x] OpenAI 兼容 API
- [ ] Claude API（计划中）
- [ ] 本地模型集成（计划中）

### 2.5 网络模式
- [x] 客户端模式
- [x] 代理模式（部分完成）
- [ ] 服务端模式（开发中）
   - [ ] 会话状态管理

### 2.6 任务专业化
- [ ] 预设任务模板（翻译/总结/代码）
- [ ] 提示工程
   - [ ] Few-shot 集成
   - [ ] 动态提示构建

### 2.7 输出结构
- [x] JSON
- [ ] Protobuf
- [ ] 自定义格式
</details>

## 3. 编译

<details>
<summary><strong>使用 Bazel 或 CMake，编译非常简单</strong>（点击展开）</summary>

### 3.1 准备

- 需要 C++11 或更高版本

```bash
git clone https://github.com/holmes1412/workflow-ai.git
cd workflow-ai
```

### 3.2 使用 Bazel 构建（推荐）

```bash
# 构建所有目标
bazel build ...

# 运行基础 DeepSeek 聊天机器人
bazel run :deepseek_chatbot -- <your_api_key>
```

### 3.3 使用 CMake 构建

```bash
# 首次编译需要下载 workflow 源码并先编译它
# git clone https://github.com/sogou/workflow.git /PATH/TO/WORKFLOW
# cd /PATH/TO/WORKFLOW && make

# 编译
mkdir cmake.build && cd cmake.build
cmake .. -D Workflow_DIR=/PATH/TO/WORKFLOW
make

# 运行同步示例
./sync_demo <your_api_key>
```
</details>

## 4. 快速开始

### 4.1 聊天演示

此示例展示了与大语言模型聊天的基本步骤。

这是一轮聊天，我们可以使用三种 API 中的任意一种：`同步`、`异步` 和 `基于任务的接口`。

```
🧑‍💻 用户请求 'hi'
         ↓
    ┌───────────┐
    │ Chat Task │ // 异步网络任务的接口：
    │  to LLMs  │ // 发送请求，获取响应
    └───────────┘
         ↓
  🧑‍💻 extract()    // streaming模式会调起
         ↓
  🧑‍💻 callback()   // 回调，任务结束
```

🤖 **1. 同步 API**

让我们从一个简单的开始：`chat_completion_sync()`。

```cpp
int main()
{
 	LLMClient client("YOUR_API_KEY"); // 通过参数 api_key 构建客户端。支持参数 base_url，默认为 `DeepSeek`

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

🤖 **2. 异步 API**

如果我们使用**流式(streaming)**模式从大语言模型服务器接收每个**数据块(chunk)**，可以使用类似于 Python 中生成器的半同步接口 `chat_completion_async()`。

```cpp
int main()
{
	LLMClient client("YOUR_API_KEY");
	ChatCompletionRequest request;
	request.stream = true; // 设置此项以使用streaming
	request.messages.push_back({"user", "hi"});

	AsyncResult result = client.chat_completion_async(request);

	// ... 这里可以运行任何其他代码直到需要使用数据 ...

	while (true)
	{
		ChatCompletionChunk *chunk = result.get_chunk();
		if (!chunk /*非流式的话是nullptr*/ || chunk->state != RESPONSE_SUCCESS)
			break;

		if (!chunk->choices.empty() && !chunk->choices[0].delta.content.empty())
			printf("%s", chunk->choices[0].delta.content.c_str());

		if (chunk->last_chunk())
			break;
	}

	// 上面的例子是streaming，如果是non streamin，可以使用 get_response() 拿结果
	// ChatCompletionResponse *response = result.get_response();
}
```

🤖 **3. 任务 API**

基于任务的 API 对于构建我们的任务图很有用，

在此示例中，我们使用 `create_chat_task()` 创建一个 **WFHttpChunkedTask**，它可以与任何其他 workflow 任务一起使用，比如把任务push_back 到 SeriesWork、ParalleWork 或 [workflow 中的 DAG](https://github.com/sogou/workflow/blob/master/docs/en/tutorial-11-graph_task.md)。

```cpp

int main()
{
	LLMClient client("YOUR_API_KEY");
	ChatCompletionRequest request;
	request.model = "deepseek-reasoner"; // 设置使用模型 DeepSeek-R1
	request.messages.push_back({"user", "hi"});

	auto *task = client.create_chat_task(request, extract, callback);
	task->start();

	// 异步任务发起后，这里要用pause或使用wait_group.wait()卡住，否则主线程会退出
}

// 在这里我们获取streaming模式的每个数据块
void extract(WFHttpChunkedTask *task, ChatCompletionRequest *req, ChatCompletionChunk *chunk)
{
	if (!chunk->choices.empty())
	{
		if (!chunk->choices[0].delta.reasoning_content.empty())
			printf("%s", chunk->choices[0].delta.reasoning_content.c_str());
		else if (!chunk->choices[0].delta.content.empty())
			printf("%s", chunk->choices[0].delta.content.c_str());
	}
}

// 在这里我们获取最终响应，适用于streaming模式和non streaming模式
void callback(WFHttpChunkedTask *task, ChatCompletionRequest *req, ChatCompletionResponse *resp)
{
	// if (task->get_state() == WFT_STATE_SUCESS)

	if (req->model == "deepseek-reasoner")
		printf("%s\n", resp->choices[0].message.reasoning_content.c_str());
	printf("%s\n", resp->choices[0].message.content.c_str());
}

```

### 4.2 Tool Calls 工具调用

此示例展示如何使用函数调用作为tool calls。

以下任务流程看起来很复杂，因为它包含了内部架构。这里仅介绍我们需要注意的 3 个步骤。

```
👩‍💻 准备工作：注册本地函数
         ↓
👩‍💻 用户请求：若干问题比如天气和时间等
         ↓
    ┌───────────┐
    │ Chat Task │ // 异步网络任务：
    │  to LLMs  │ // 发送请求，获取响应
    └───────────┘
         ↓
大语言模型响应 tool_calls
         ↓
为本地函数计算创建 WFGoTask
         ↓
┌─────────┬─────────┬─────────┐
│ Tool A  │ Tool B  │ Tool C  │ // 并行执行
│ Series1 │ Series2 │ Series3 │ // 通过计算线程
└─────────┴─────────┴─────────┘
         ↓
   收集结果
         ↓
    ┌───────────┐
    │ Chat Task │ // 发送所有上下文和结果
    │  to LLMs  │ // 内存模块支持多轮
    └───────────┘
         ↓ 
  👩‍💻 extract() 
         ↓
  👩‍💻 callback()
```

🤖 代码示例

**步骤 1**：定义我们用作tool calls的`本地函数`。

这个准备工作只需要在所有请求之前做一次。

所有函数的参数都是固定的：
- arguments：来自大语言模型的 Json 格式参数，例如 {"location":"Shenzhen"}
- result：我们函数要填充的返回值

```cpp
void get_current_weather(const std::string& arguments, FunctionResult *result)
{
    result->success = true;
    result->result = "Weather: 25°C, Sunny";
}
```

**步骤 2**：将函数注册到 `function_manager` 并将 function_manager 添加到客户端。

这也是一次性准备工作。

```cpp
int main()
{
    LLMClient client("your_api_key");
    FunctionManager func_mgr;
    client.set_function_manager(&func_mgr);

    // 注册函数
    FunctionDefinition weather_func = {
        .name = "get_weather",
        .description = "Get current weather information"
    };
    func_mgr.register_function(weather_func, get_current_weather);

    ...
}
```

**步骤 3**：发起请求。

只要我们在管理器中有函数并设置了 `request.tool_choice`，大语言模就会告诉我们如何使用相应的工具，这些信息回来之后，这个项目框架会帮助我们自动执行工具（也就是我们曾经注册过的某些函数），然后框架会自动将响应提供给大语言模型，让它根据函数结果进行生成总结，然后大模型给我们返回最终结果。

```cpp
{
    ChatCompletionRequest request;
    request.model = "deepseek-chat";
    request.messages.push_back({"user", "What's the weather like?"});
    request.tool_choice = "auto"; // 设置 `auto` 或 `required` 以启用tool calls

	ChatCompletionResponse response;
	auto result = client.chat_completion_sync(request, response);

	if (result.success)
		printf("%s\n", response.choices[0].message.content.c_str());
	// 回复会是："深圳今天阳光明媚，气温25°C，天气宜人。"
}
```

### 4.3 并行工具执行

该框架会在检测大语言模型返回多个工具调用时，使用 Workflow 的 `ParallelWork` 自动并行执行：

在 [example/parallel_tool_call.cc](./examples/parallel_tool_call.cc) 中，我们这样发起请求：

```cpp
// 当大语言模型返回多个工具调用时，它们会并行执行
request.messages.push_back({"user", "Tell me the weather in Beijing and Shenzhen, and the current time"});
```

这将同时执行两个城市的天气查询和时间查询，显著改善响应时间。

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

## 5. API 参考

### 5.1 核心类

- **`LLMClient`**：大语言模型交互的主要客户端
- **`FunctionManager`**：管理函数注册和执行
- **`ChatCompletionRequest`**：用户发送的请求
- **`ChatCompletionResponse`**：来自大语言模型的响应数据结构

### 5.2 示例

| 示例 | 描述 |
|---------|-------------|
| [sync_demo.cc](./examples/sync_demo.cc) | 同步 API，最简单的演示 |
| [async_demo.cc](./examples/async_demo.cc) | 异步 API，展示获取流式数据块的用法 |
| [task_demo.cc](./examples/task_demo.cc) | 任务 API，能够创建可以接入Workflow图的任务，该接口也是异步的 |
| [deepseek_chatbot.cc](./examples/deepseek_chatbot.cc) | 带上下文记忆的多轮会话 DeepSeek 聊天机器人 |
| [tool_call.cc](./examples/tool_call.cc) | 使用单个工具的基本函数调用 |
| [parallel_tool_call.cc](./examples/parallel_tool_call.cc) | 演示多个工具的并行执行 |

## 5.3 开源许可

本项目采用 Apache License 2.0 许可 - 详见 [LICENSE](./LICENSE) 文件。

## 5.4 依赖的库

- [Sogou Workflow](https://github.com/sogou/workflow) v0.11.9
- OpenSSL
- pthread

更多示例，请查看 [examples/](./examples) 目录。详细文档即将发布。

## 6. 联系方式

如果问题、建议或想合作？欢迎随时联系开发者！

✉️ **邮箱**：[liyingxin1412@gmail.com](mailto:liyingxin1412@gmail.com)  
🧸 **GitHub**：[https://github.com/holmes1412](https://github.com/holmes1412)  
