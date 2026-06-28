# 🗡️ SwordFish Web Browser

![SwordFish Logo](icon.png)

SwordFish is a **privacy-first, power-user browser** built with Python 3 and PySide6. It's designed for researchers, developers, and students who need a high-performance browsing experience integrated with a suite of 20+ productivity tools.

---

## 🚀 Key Features

### 🌐 High-End Browsing
*   **Modern Aesthetic**: Clean, responsive UI with multi-tab support.
*   **Privacy-First**: No external tracking, local profile storage, and a dedicated **Stealth Mode**.
*   **Advanced Adblocker**: 4-level protection system (None, Low, Medium, Ultimate) with built-in adult content filtering.
*   **Media Downloader**: Integrated `yt-dlp` support for downloading video (up to 4K) and audio (MP3/M4A/OGG) directly from the UI.
*   **English-Only Mode**: Force English locale for all web content.

### 🛠️ Integrated Tools Hub
Access professional-grade tools directly from the browser's "🔧 Tools" menu:
*   **PDF & Document Suite**: 
    *   Convert between Word, Excel, PPTX, Images, and PDF.
    *   Merge/Split PDFs, extract text, and more.
*   **Utility Suite**:
    *   **Calculator & Converter**: Unit conversions (Temperature, Physics, Math) and Programmer's calculator (Bin/Hex/Dec).
    *   **Archive Tools**: Manage Zip, 7z, and Tar archives.
    *   **Media Tools**: Extract YouTube transcripts, real-time Translator, and Dictionary.
*   **Smart Features**: Web Search, Weather updates, QR Code Generator, and secure Note Taker.

### 🛡️ Security & Integrity
*   **Integrity Lock**: A unique self-verification system that protects core files (`src/`, `icon.png`, `qrcode.png`) using a secret cipher. Unauthorized modifications trigger an automatic lockdown.
*   **Network Security**: Built-in Public IP display, Connectivity tester, and Proxy management.
*   **Privacy Tools**: MAC Address spoofing (Linux) and cookie management.

---

## 📦 Installation

### 🐧 Linux (Recommended)
The unified installer handles system dependencies (ffmpeg), python libraries, and desktop integration:
```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFish.git
cd SwordFish
chmod +x install.sh
./install.sh
```
*Launch via the application menu or run `./swordfish.sh`.*

### 🪟 Windows
1. Open Command Prompt as **Administrator**.
2. Run the dependency installer:
   ```cmd
   requirements.bat
   ```
3. Run the application:
   ```cmd
   python src/main.py
   ```

---

## 📁 Project Structure
*   `src/`: Core browser engine and UI logic.
*   `tools/`: 20+ specialized productivity modules.
*   `utils/`: Security (Adblock, Integrity), Network, and TTS utilities.
*   `assets/`: Icons and static resources.

---



---
*Built for power users who demand privacy and productivity.*
