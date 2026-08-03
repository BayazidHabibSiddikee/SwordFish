// src/reading_mode.cpp — Clean article reading view
// Uses a minimal built-in Readability-inspired extraction (no external deps).
#include "reading_mode.h"

#include <QWebEnginePage>

ReadingMode::ReadingMode(QObject *parent) : QObject(parent) {}

void ReadingMode::toggle(QWebEnginePage *page) {
    if (!page) return;
    if (m_active) restore(page);
    else          inject(page);
}

void ReadingMode::inject(QWebEnginePage *page) {
    m_active = true;
    emit activated();

    // Minimal article extractor + clean renderer injected as JS
    // Extracts main article content, renders in a clean readable layout
    page->runJavaScript(R"JS(
(function() {
    if (window.__sfReaderActive) return;
    window.__sfReaderActive = true;

    // ── Save original HTML ──
    window.__sfOriginalHTML = document.documentElement.outerHTML;
    window.__sfOriginalTitle = document.title;

    // ── Extract article content ──
    function extractArticle() {
        // Try semantic tags first
        const candidates = ['article', 'main', '[role="main"]',
            '.post-content', '.article-body', '.entry-content',
            '.content-body', '.story-body', '#article', '#content', '#main'];
        for (const sel of candidates) {
            const el = document.querySelector(sel);
            if (el && el.innerText.trim().length > 200) return el;
        }
        // Fallback: find the div with most text
        let best = null, bestLen = 0;
        document.querySelectorAll('div, section').forEach(el => {
            const len = el.innerText.trim().length;
            if (len > bestLen) { bestLen = len; best = el; }
        });
        return best || document.body;
    }

    const article = extractArticle();
    const title   = document.title || '';
    const content = article ? article.innerHTML : '<p>Could not extract article.</p>';

    // ── Build clean reader HTML ──
    const readerHTML = `<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>${title}</title>
<style>
  :root { --bg:#0d1117; --fg:#c9d1d9; --accent:#58a6ff; --border:#30363d; }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { background: var(--bg); color: var(--fg);
    font-family: 'Georgia', 'Times New Roman', serif; }
  body { max-width: 720px; margin: 0 auto; padding: 40px 24px; line-height: 1.8; font-size: 18px; }
  h1 { font-size: 2em; color: #e6edf3; margin-bottom: 12px; line-height: 1.3; }
  h2 { font-size: 1.5em; color: #c9d1d9; margin: 24px 0 8px; }
  h3 { font-size: 1.2em; color: #adbac7; margin: 20px 0 6px; }
  p  { margin: 14px 0; }
  a  { color: var(--accent); text-decoration: underline; }
  img { max-width: 100%; border-radius: 6px; margin: 16px 0; }
  blockquote { border-left: 3px solid var(--accent); padding-left: 16px;
    color: #8b949e; margin: 16px 0; font-style: italic; }
  code, pre { background: #161b22; color: #79c0ff; padding: 2px 6px;
    border-radius: 4px; font-family: monospace; font-size: 0.9em; }
  pre { padding: 12px; overflow-x: auto; }
  hr  { border: none; border-top: 1px solid var(--border); margin: 24px 0; }
  figure, figcaption { color: #8b949e; font-size: 0.85em; text-align: center; }
  /* Exit button */
  #__sf_reader_exit {
    position: fixed; top: 16px; right: 16px;
    background: #1c2128; color: #00d2ff;
    border: 1px solid #00b4d8; border-radius: 6px;
    padding: 8px 16px; cursor: pointer; font-size: 13px;
    font-family: sans-serif; z-index: 9999;
    transition: background 0.2s;
  }
  #__sf_reader_exit:hover { background: #0a3040; }
  #__sf_reader_header { margin-bottom: 32px; border-bottom: 1px solid var(--border); padding-bottom: 20px; }
</style>
</head>
<body>
<button id="__sf_reader_exit" onclick="window.__sfExitReader()">✕ Exit Reading Mode</button>
<div id="__sf_reader_header"><h1>${title}</h1></div>
<div id="__sf_reader_content">${content}</div>
<script>
window.__sfExitReader = function() {
    // Signal exit via title
    document.title = '__sf_exit_reader__';
    setTimeout(() => { document.title = '${title.replace("'", "\\'")}'; }, 200);
};
// Remove scripts, ads, navs from content
document.querySelectorAll('#__sf_reader_content script, #__sf_reader_content nav, #__sf_reader_content aside, #__sf_reader_content [class*="ad"], #__sf_reader_content [id*="ad"]').forEach(e => e.remove());
</script>
</body>
</html>`;

    // Replace document content
    document.open();
    document.write(readerHTML);
    document.close();

    // Watch for exit signal via title change
    const obs = new MutationObserver(() => {
        if (document.title === '__sf_exit_reader__') {
            obs.disconnect();
        }
    });
    if (document.querySelector('title'))
        obs.observe(document.querySelector('title'), { childList: true });
})();
)JS");
}

void ReadingMode::restore(QWebEnginePage *page) {
    m_active = false;
    emit deactivated();

    page->runJavaScript(R"JS(
(function() {
    window.__sfReaderActive = false;
    if (window.__sfOriginalHTML) {
        document.open();
        document.write(window.__sfOriginalHTML);
        document.close();
        window.__sfOriginalHTML = null;
    }
})();
)JS");
}
