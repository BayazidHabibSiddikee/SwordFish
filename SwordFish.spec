# -*- mode: python ; coding: utf-8 -*-

import os
import sys

block_cipher = None

# Calculate absolute path to the project root
project_root = os.path.abspath('.')

a = Analysis(
    ['src/main.py'],
    pathex=[project_root],
    binaries=[],
    datas=[
        ('src/tools.html', 'src'),
        ('src/qrcode.png', 'src'),
        ('tools', 'tools'),
        ('utils', 'utils'),
        ('icon.png', '.'),
        ('tools/alarm.wav', '.'),
    ],
    hiddenimports=[
        'PySide6.QtWebEngineWidgets',
        'PySide6.QtWebEngineCore',
        'PySide6.QtWebChannel',
        'yt_dlp',
        'pypdf',
        'arrow',
        'deep_translator',
        'youtube_transcript_api',
        'requests',
        'duckduckgo_search',
        'geopy',
        'folium',
        'bs4',
        'httpx',
        'pyttsx3',
        'docx',
        'pikepdf',
        'img2pdf',
        'qrcode',
        'fpdf',
        'adblockparser',
        'mammoth',
        'fitz',
        'pdf2docx',
        'pandas',
        'openpyxl',
        'pdfplumber',
        'pptx',
        'src.styles',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)
pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='SwordFish',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=['icon.png'],
)
