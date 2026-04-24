const fs = require('fs');
const os = require('os');
const path = require('path');
const { marked } = require('marked');
const puppeteer = require('puppeteer-core');

function findEdgePath() {
  const candidates = [
    'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  ];
  for (const p of candidates) {
    if (fs.existsSync(p)) {
      return p;
    }
  }
  throw new Error('Microsoft Edge executable not found.');
}

function defaultMarkdownFiles(cwd) {
  return fs
    .readdirSync(cwd, { withFileTypes: true })
    .filter((d) => d.isFile())
    .map((d) => d.name)
    .filter((name) => /_V1\.0\.md$/i.test(name));
}

function buildHtml(title, mdText) {
  const renderer = new marked.Renderer();
  const rawCode = renderer.code.bind(renderer);
  renderer.code = (code, infostring) => {
    const lang = (infostring || '').trim().toLowerCase();
    if (lang === 'mermaid') {
      return `<div class="mermaid">\n${code}\n</div>\n`;
    }
    return rawCode(code, infostring);
  };

  marked.setOptions({
    renderer,
    gfm: true,
    breaks: false,
    mangle: false,
    headerIds: true,
  });

  const bodyHtml = marked.parse(mdText);
  return `<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>${title}</title>
  <style>
    @page { size: A4; margin: 18mm 14mm; }
    body {
      max-width: 980px;
      margin: 24px auto;
      padding: 0 14px;
      font-family: "Microsoft YaHei", "PingFang SC", "Noto Sans CJK SC", sans-serif;
      color: #111827;
      line-height: 1.7;
      font-size: 15px;
    }
    h1, h2, h3, h4 { color: #0f172a; }
    h1 { border-bottom: 1px solid #e5e7eb; padding-bottom: 8px; }
    pre {
      background: #f8fafc;
      border: 1px solid #e2e8f0;
      border-radius: 8px;
      padding: 10px;
      overflow-x: auto;
    }
    code {
      background: #f3f4f6;
      padding: 2px 4px;
      border-radius: 4px;
      font-family: Consolas, "Courier New", monospace;
    }
    pre code { background: transparent; padding: 0; }
    table { border-collapse: collapse; width: 100%; }
    th, td {
      border: 1px solid #d1d5db;
      padding: 6px 8px;
      text-align: left;
    }
    th { background: #f3f4f6; }
    .mermaid {
      border: 1px solid #e5e7eb;
      border-radius: 8px;
      padding: 10px;
      margin: 12px 0;
      background: #fff;
      page-break-inside: avoid;
    }
    .mermaid svg { max-width: 100%; height: auto; }
  </style>
</head>
<body>
${bodyHtml}
<script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
<script>
  window.__MERMAID_DONE__ = false;
  (async () => {
    try {
      mermaid.initialize({ startOnLoad: false, theme: 'default', securityLevel: 'loose' });
      await mermaid.run({ querySelector: '.mermaid' });
      window.__MERMAID_DONE__ = true;
    } catch (e) {
      console.error(e);
      window.__MERMAID_DONE__ = true;
    }
  })();
</script>
</body>
</html>`;
}

async function exportOne(browser, mdPath) {
  const absMd = path.resolve(mdPath);
  const base = path.basename(absMd, path.extname(absMd));
  const outPdf = path.join(path.dirname(absMd), `${base}.pdf`);

  const mdText = fs.readFileSync(absMd, 'utf8');
  const html = buildHtml(base, mdText);
  const tmpHtml = path.join(os.tmpdir(), `md2pdf-${Date.now()}-${Math.random().toString(36).slice(2)}.html`);

  fs.writeFileSync(tmpHtml, html, 'utf8');

  try {
    const page = await browser.newPage();
    await page.goto(`file:///${tmpHtml.replace(/\\/g, '/')}`, { waitUntil: 'networkidle0', timeout: 60000 });
    await page.waitForFunction('window.__MERMAID_DONE__ === true', { timeout: 60000 });
    if (fs.existsSync(outPdf)) {
      fs.unlinkSync(outPdf);
    }
    await page.pdf({ path: outPdf, format: 'A4', printBackground: true, preferCSSPageSize: true });
    await page.close();
    return outPdf;
  } finally {
    if (fs.existsSync(tmpHtml)) {
      fs.unlinkSync(tmpHtml);
    }
  }
}

async function main() {
  const cwd = process.cwd();
  const cliFiles = process.argv.slice(2);
  const files = cliFiles.length ? cliFiles : defaultMarkdownFiles(cwd);

  if (!files.length) {
    throw new Error('No markdown files to export.');
  }

  const edgePath = findEdgePath();
  const browser = await puppeteer.launch({
    executablePath: edgePath,
    headless: true,
    args: ['--disable-gpu', '--no-sandbox'],
  });

  const ok = [];
  const fail = [];
  try {
    for (const file of files) {
      if (!fs.existsSync(file) || path.extname(file).toLowerCase() !== '.md') {
        fail.push({ file, error: 'File missing or not markdown.' });
        continue;
      }
      try {
        const pdf = await exportOne(browser, file);
        const st = fs.statSync(pdf);
        ok.push({ name: path.basename(pdf), time: new Date(st.mtimeMs).toLocaleString(), length: st.size });
      } catch (e) {
        fail.push({ file, error: e.message });
      }
    }
  } finally {
    await browser.close();
  }

  if (!ok.length) {
    throw new Error(`No PDF exported. Failures: ${JSON.stringify(fail, null, 2)}`);
  }

  console.table(ok);
  if (fail.length) {
    console.log('Failed files:');
    console.table(fail);
  }
}

main().catch((e) => {
  console.error(e.message || e);
  process.exit(1);
});
