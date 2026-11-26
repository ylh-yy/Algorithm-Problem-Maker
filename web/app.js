"use strict";

const byId = (id) => document.getElementById(id);
const fileTypes = [
    ["题面.md", "tm.md", "题面"],
    ["题解.md", "tj.md", "题解"],
    ["std.cpp", "std.cpp", "标程"],
    ["gen.cpp", "gen.cpp", "生成器"],
    ["valid.cpp", "valid.cpp", "校验器"],
    ["题目评估.md", "题目评估.md", "评估"],
];
const aliases = new Map(fileTypes.flatMap(([key, name]) => [[key, key], [name, key]]));
aliases.set("extra.txt", "交代");
aliases.set("交代.txt", "交代");
aliases.set("交代", "交代");
const encoder = new TextEncoder();
const maxBytes = 8 * 1024 * 1024;
const materials = {
    draft: { text: byId("draft"), list: byId("draft-list"), files: new Map() },
    example: { text: byId("example"), list: byId("example-list"), files: new Map() },
};
let result = null;
let resultNotes = "";
let currentFile = 0;
let showSource = false;
let toastTimer;

const markdown = window.markdownit({ html: false, linkify: false });
function renderMath(text, displayMode) {
    return window.katex.renderToString(text, {
        displayMode, output: "mathml", throwOnError: false, trust: false, maxExpand: 256, maxSize: 16,
    });
}
markdown.inline.ruler.before("escape", "formula", (state, silent) => {
    if (state.src[state.pos] !== "$" || state.src[state.pos - 1] === "$") return false;
    const delimiter = state.src[state.pos + 1] === "$" ? "$$" : "$";
    const start = state.pos + delimiter.length;
    let end = start;
    while (end < state.posMax && state.src[end] !== "\n") {
        if (state.src[end] === "\\") {
            end += 2;
        } else if (state.src.startsWith(delimiter, end)) {
            const text = state.src.slice(start, end);
            if (!text.trim()) return false;
            if (!silent) {
                const token = state.push("formula", "", 0);
                token.content = text;
                token.block = delimiter === "$$";
            }
            state.pos = end + delimiter.length;
            return true;
        } else {
            end += 1;
        }
    }
    return false;
});
markdown.block.ruler.before("fence", "formula_block", (state, start, end, silent) => {
    const line = (index) => state.src.slice(state.bMarks[index] + state.tShift[index], state.eMarks[index]);
    if (line(start).trim() !== "$$") return false;
    let closing = start + 1;
    while (closing < end && line(closing).trim() !== "$$") closing += 1;
    if (closing === end) return false;
    if (!silent) {
        const token = state.push("formula", "", 0);
        token.content = state.getLines(start + 1, closing, state.blkIndent, false);
        token.block = true;
        state.line = closing + 1;
    }
    return true;
});
markdown.renderer.rules.formula = (tokens, index) => renderMath(tokens[index].content, tokens[index].block);

function notice(message = "", error = false) {
    const area = byId("form-notice");
    area.textContent = message;
    area.classList.toggle("error", error);
    area.hidden = !message;
}

function toast(message) {
    const area = byId("toast");
    clearTimeout(toastTimer);
    area.textContent = message;
    area.hidden = false;
    toastTimer = setTimeout(() => { area.hidden = true; }, 2300);
}

function renderUploads(material) {
    material.list.replaceChildren();
    for (const [key, file] of material.files) {
        const row = document.createElement("li");
        const name = document.createElement("span");
        name.textContent = file.name;
        name.title = file.name;
        const remove = document.createElement("button");
        remove.type = "button";
        remove.textContent = "×";
        remove.setAttribute("aria-label", "移除 " + file.name);
        remove.addEventListener("click", () => {
            if (key === "题面.md" && material.text.value === file.text) material.text.value = "";
            material.files.delete(key);
            renderUploads(material);
        });
        row.append(name, remove);
        material.list.append(row);
    }
}

async function upload(material, selection) {
    const supported = selection.filter((file) => aliases.has(file.name) || /\.(md|txt|cpp|in|out)$/i.test(file.name));
    if (!supported.length) throw new Error("请选择 Markdown、文本、C++ 源码或测试数据文件。");
    if (supported.reduce((total, file) => total + file.size, 0) > maxBytes)
        throw new Error("所选文件超过 8 MiB，请减少上传材料。");
    const entries = await Promise.all(supported.map(async (file) => {
        let key = aliases.get(file.name);
        if (!key && /\.(in|out)$/i.test(file.name)) key = "data/" + file.name;
        if (!key && supported.length === 1 && /\.(md|txt)$/i.test(file.name)) key = "题面.md";
        if (!key) key = file.name;
        return [key, { name: file.name, text: await file.text() }];
    }));
    for (const [key, file] of entries) {
        material.files.set(key, file);
        if (key === "题面.md") material.text.value = file.text;
    }
    renderUploads(material);
}

function collect(material) {
    const files = new Map();
    for (const [key, file] of material.files) {
        if (key !== "题面.md" && file.text.trim()) files.set(key, file.text);
    }
    if (material.text.value.trim()) files.set("题面.md", material.text.value);
    return Object.fromEntries(files);
}

for (const button of document.querySelectorAll("[data-upload]")) {
    const input = byId(button.dataset.upload);
    button.addEventListener("click", () => input.click());
    input.addEventListener("change", async () => {
        const selection = Array.from(input.files);
        input.value = "";
        if (!selection.length) return;
        const key = input.id.split("-")[0];
        notice();
        try {
            await upload(materials[key], selection);
        } catch (error) {
            notice(error.message, true);
        }
    });
}

byId("upload-key").addEventListener("click", () => byId("key-file").click());
byId("key-file").addEventListener("change", async (event) => {
    const picker = event.currentTarget;
    const file = picker.files[0];
    picker.value = "";
    if (!file) return;
    try {
        const key = file.size <= 512 ? (await file.text()).trim() : "";
        if (!/^[A-Za-z0-9_.~+/=-]+$/.test(key))
            throw new Error("请选择只包含一行 API Key 的文本文件。");
        byId("api-key").value = key;
        notice();
        toast("已读取 API Key");
    } catch (error) {
        notice(error.message, true);
    }
});

byId("toggle-key").addEventListener("click", () => {
    const input = byId("api-key");
    const visible = input.type === "password";
    input.type = visible ? "text" : "password";
    const button = byId("toggle-key");
    button.textContent = visible ? "隐藏" : "显示";
    button.setAttribute("aria-pressed", String(visible));
});

const tabs = fileTypes.map(([key, , label], index) => {
    const tab = document.createElement("button");
    tab.type = "button";
    tab.className = "tab";
    tab.id = "file-tab-" + index;
    tab.textContent = label;
    tab.title = key;
    tab.setAttribute("role", "tab");
    tab.setAttribute("aria-controls", "file-panel");
    tab.addEventListener("click", () => {
        currentFile = index;
        showSource = false;
        renderResult();
    });
    tab.addEventListener("keydown", (event) => {
        const positions = { ArrowLeft: -1, ArrowRight: 1 };
        let target;
        if (event.key in positions) target = (index + positions[event.key] + fileTypes.length) % fileTypes.length;
        else if (event.key === "Home") target = 0;
        else if (event.key === "End") target = fileTypes.length - 1;
        else return;
        event.preventDefault();
        tabs[target].click();
        tabs[target].focus();
    });
    byId("file-tabs").append(tab);
    return tab;
});

function renderResult() {
    byId("empty").hidden = Boolean(result);
    byId("file-panel").hidden = !result;
    byId("copy-all").disabled = !result;
    byId("export-all").disabled = !result;
    tabs.forEach((tab, index) => {
        tab.disabled = !result;
        tab.tabIndex = index === currentFile ? 0 : -1;
        tab.setAttribute("aria-selected", String(index === currentFile));
    });
    if (!result) return;

    const [key, filename] = fileTypes[currentFile];
    const isMarkdown = key.endsWith(".md");
    const showMarkdown = isMarkdown && !showSource;
    const preview = byId("rendered");
    const source = byId("source");
    const toggle = byId("view-source");
    byId("file-panel").setAttribute("aria-labelledby", tabs[currentFile].id);
    byId("filename").textContent = filename;
    toggle.hidden = !isMarkdown;
    toggle.textContent = showSource ? "Markdown 预览" : "查看源码";
    source.hidden = showMarkdown;
    preview.hidden = !showMarkdown;
    source.textContent = result[key];
    if (showMarkdown) preview.innerHTML = markdown.render(result[key]);
    else preview.replaceChildren();
}

byId("view-source").addEventListener("click", () => {
    showSource = !showSource;
    renderResult();
});

byId("generator").addEventListener("submit", async (event) => {
    event.preventDefault();
    const key = byId("api-key").value.trim();
    const provider = new FormData(event.currentTarget).get("provider");
    const files = collect(materials.draft);
    if (!key) {
        notice("请填写 API Key。", true);
        return;
    }
    if (!Object.keys(files).length) {
        notice("请填写题目草稿或上传已有题目文件。", true);
        byId("draft").focus();
        return;
    }

    const request = JSON.stringify({ provider, files, example: collect(materials.example) });
    if (encoder.encode(request).length > maxBytes) {
        notice("题目和参考材料合计超过 8 MiB，请减少上传文件。", true);
        return;
    }
    const generate = byId("generate");
    generate.disabled = true;
    generate.textContent = "正在生成…";
    byId("result-status").textContent = "正在请求 " + (provider === "deepseek" ? "DeepSeek" : "OpenAI");
    byId("empty-title").textContent = "正在生成出题包";
    byId("empty-text").textContent = "题面和配套源码生成后将显示在这里。";
    notice();
    try {
        const response = await fetch("/api/generate", {
            method: "POST",
            headers: { "Content-Type": "application/json", Authorization: "Bearer " + key },
            body: request,
            cache: "no-store",
        });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || "生成请求失败");
        if (!data.files || fileTypes.some(([name]) => typeof data.files[name] !== "string")) {
            throw new Error("返回结果缺少题目文件。");
        }
        result = data.files;
        resultNotes = files["交代"] || "";
        currentFile = 0;
        showSource = false;
        renderResult();
        byId("result-status").textContent = "已生成 " + fileTypes.length + " 个文件";
    } catch (error) {
        byId("result-status").textContent = result ? "本次生成失败，仍可查看上次结果" : "生成失败";
        byId("empty-title").textContent = "还没有生成结果";
        byId("empty-text").textContent = "检查提示后可再次生成。";
        notice(error.message || "生成请求失败，请检查网络。", true);
    } finally {
        generate.disabled = false;
        generate.textContent = "生成出题包";
    }
});

async function copy(text) {
    try {
        await navigator.clipboard.writeText(text);
        toast("已复制");
    } catch {
        toast("浏览器未允许访问剪贴板，可使用下载按钮。");
    }
}

function download(data, name, type) {
    const url = URL.createObjectURL(new Blob([data], { type }));
    const link = document.createElement("a");
    link.href = url;
    link.download = name;
    link.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}

byId("copy-all").addEventListener("click", () => copy(JSON.stringify(result, null, 2)));
byId("copy-file").addEventListener("click", () => copy(result[fileTypes[currentFile][0]]));
byId("export-file").addEventListener("click", () => {
    const [key, name] = fileTypes[currentFile];
    download(result[key], name, "text/plain;charset=utf-8");
});
byId("export-all").addEventListener("click", () => {
    const title = result["题面.md"].match(/^#[ \t]+(.+)$/m)?.[1] ?? "题目";
    const name = (title.replace(/[<>:"/\\|?*\u0000-\u001f]/g, "_").trim().slice(0, 60) || "题目") + "_已完成";
    const files = Object.fromEntries(fileTypes.map(([key, filename]) => [filename, encoder.encode(result[key])]));
    files["extra.txt"] = encoder.encode(resultNotes);
    files["data/"] = new Uint8Array();
    download(window.fflate.zipSync({ [name]: files }), name + ".zip", "application/zip");
});

renderResult();
