# arm_docs 文档说明

本目录用于存放高空机械臂相关的架构与协议文档，并支持将 Markdown 文档导出为 PDF（支持 Mermaid 图渲染）。

## 目录结构

- `架构设计_V1.0.md`
- `通讯协议规范_V1.0.md`
- `业务消息API_V1.0.md`
- `PLC协议_指令语义化封装清单_V1.0.md`
- `export_md_pdf_mermaid.js`（PDF 导出脚本）
- `js_sdk/`（JS SDK 示例与协议实现）

## PDF 导出能力

已提供导出脚本：`export_md_pdf_mermaid.js`

特性：

1. 支持 Markdown 中的 Mermaid 代码块渲染后再导出 PDF。
2. 默认导出当前目录下文件名匹配 `_V1.0.md` 的文档。
3. 允许手动指定要导出的 `.md` 文件列表。
4. 导出过程中使用临时 HTML，中间文件在导出完成后自动删除。

## 使用方式

在根目录执行以下命令。

### 1. 安装依赖（首次执行）

```bash
npm install --no-save marked@4.3.0 puppeteer-core@19.11.1
```

### 2. 导出默认文档（推荐）

```bash
node export_md_pdf_mermaid.js
```

默认会导出：

- `架构设计_V1.0.md`
- `通讯协议规范_V1.0.md`
- `业务消息API_V1.0.md`
- `PLC协议_指令语义化封装清单_V1.0.md`

### 3. 指定文档导出

```bash
node export_md_pdf_mermaid.js "架构设计_V1.0.md" "通讯协议规范_V1.0.md" "业务消息API_V1.0.md"
```

## 导出结果

导出后会在同目录生成同名 `.pdf` 文件，例如：

- `架构设计_V1.0.pdf`
- `通讯协议规范_V1.0.pdf`
- `业务消息API_V1.0.pdf`
- `PLC协议_指令语义化封装清单_V1.0.pdf`

## 环境要求

1. Node.js 16+
2. Windows 已安装 Microsoft Edge（脚本会调用本机 Edge 进行 PDF 打印）

## 常见问题

1. Mermaid 图未显示：
- 确认已按说明安装依赖。
- 重新执行导出命令并重新打开 PDF。

2. 导出失败：
- 确认 Edge 可执行文件存在（系统默认安装路径）。
- 确认执行目录是项目根目录。
