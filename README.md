# 🗡️ SwordFish Web Browser

![SwordFish Logo](icon.png)

SwordFish is a **modern, high-performance, and secure** web browser built with Python 3 and PySide6. It combines a sleek "Tools Hub" with advanced security features like built-in adblocking, proxy management, and a unique **Integrity Lock** system.

---

## 🚀 Key Features

### 🌐 High-End Browsing
*   **Modern Aesthetic**: A clean, light-themed UI inspired by the Tools Hub.
*   **Tabbed Management**: Smooth tab switching with a dedicated "New Tab" button.
*   **Advanced Adblocker**: Four levels of protection (None, Low, Medium, Ultimate) to keep your browsing clean.
*   **Stealth Mode**: Private browsing that leaves no trace of history or cookies.
*   **Media Downloader**: Download high-quality video or audio directly from supported sites like YouTube.

### 🛠️ Integrated Tools Hub
Access 20+ productivity tools without leaving your browser:
*   **Document Suite**: Merge/Split PDFs, Convert Word/Excel/PPTX to PDF and vice-versa.
*   **Media Suite**: Extract YouTube transcripts, Convert images to PDF.
*   **Smart Tools**: Real-time Translator, Weather updates, and Web Search.
*   **Utilities**: QR Code Generator, Unit Converter, Calculator, and a secure Note Taker.

### 🛡️ Security & Integrity
*   **Network Security**: Built-in Public IP display, Connectivity tester, and Proxy switcher.
*   **MAC Spoofing**: Randomize your hardware address for maximum anonymity (Linux).
*   **Integrity Lock**: The browser self-verifies its core files (`src/`), `icon.png`, and `qrcode.png` using a secret cipher. If any file is tampered with, the browser locks down to protect your data.

---

## 📦 Quick Installation (Linux)

To install everything and add the browser to your system menu:

```bash
chmod +x install.sh
./install.sh
```
*After installation, you can find **SwordFish** in your Application Menu/Search.*

---

## 🪟 Windows Installation

1.  Open **Command Prompt** as Administrator.
2.  Run the setup:
    ```cmd
    requirements.bat
    ```
3.  (Optional) Build the executable:
    ```cmd
    build_exe.bat
    ```

---

## 📁 Architecture & Data
*   **Codebase**: Core logic resides in `src/`, extensions in `tools/`, and security in `utils/`.
*   **Data Storage**:
    *   **Linux/Mac**: `~/.swordfish_webbrowser`
    *   **Windows**: `%APPDATA%\SwordFish`

---

## 🔒 Security Notice
SwordFish is protected by a **File Integrity System**. Unauthorized modifications to the `src/` folder or core images will cause the application to fail its security check. To intentionally update the browser after code changes, run:
`python3 utils/integrity.py`

---
*Developed for power users who demand privacy and productivity.*
