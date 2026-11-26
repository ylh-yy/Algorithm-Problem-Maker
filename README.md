# Algorithm Problem Maker

一个在 Windows 上运行的算法题工作台。输入半成品题目和可选参考题，使用 OpenAI 或 DeepSeek 补全题面、题解、标程、数据生成器、校验器和难度评估。

## 构建与启动

准备 Windows 和已加入 `PATH` 的 MinGW-w64。打开 PowerShell，进入项目根目录并运行：

```powershell
New-Item -ItemType Directory -Force -Path .\bin | Out-Null
g++ -std=c++17 .\src\web.cpp .\src\read.cpp .\src\work.cpp .\src\write.cpp -o .\bin\web.exe -lwinhttp -lws2_32
```

编译完成后启动服务，并打开 [本地出题工作台](http://127.0.0.1:8787/)。

```powershell
.\bin\web.exe
```

使用 VS Code 时，也可以运行「构建网页版出题机」任务。保持服务窗口打开，使用 `Ctrl+C` 停止服务。若端口被占用，可用 `.\bin\web.exe 8790` 启动并访问 `http://127.0.0.1:8790/`。

## 网页用法

1. **选择 API 服务。** 选择 OpenAI 或 DeepSeek，粘贴自己的 API Key，或读取只含一行 Key 的文本文件。Key 用于本次请求。
2. **填写题目草稿。** 粘贴题意、约束或已有的输入输出格式。也可上传已有的 Markdown、C++、测试数据文件，或直接选择整个题目文件夹。
3. **按需添加参考题。** 粘贴参考题面或上传文件夹。仓库中的 `problems/example/` 可以直接用作参考。生成要求来自根目录的 `task.txt`。
4. **生成和导出。** 点击「生成出题包」。右侧可预览 Markdown 和公式、查看源码、复制单个文件或整份 JSON，也可以下载当前文件或导出 ZIP。

## 导出的题目包

```text
题目名_已完成/
├── tm.md           题面
├── tj.md           题解
├── std.cpp         标程
├── gen.cpp         数据生成器
├── valid.cpp       输入校验器
├── 题目评估.md
├── extra.txt
└── data/
```

解压后，在题目目录编译 `std.cpp`、`valid.cpp` 和 `gen.cpp`。运行 `.\gen.exe 1` 会生成第一组 `data/1.in` 和 `data/1.out`，其余测试点按编号生成。
