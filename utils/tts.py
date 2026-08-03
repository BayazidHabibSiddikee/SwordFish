import sys

try:
    import pyttsx3

    _engine = pyttsx3.init()
    _voices = _engine.getProperty("voices")
    for v in _voices:
        if "female" in v.name.lower():
            _engine.setProperty("voice", v.id)
            break
    _engine.setProperty("rate", 160)
except Exception:
    _engine = None


def speak_female(text: str):
    if _engine:
        _engine.say(text)
        _engine.runAndWait()
    else:
        print(f"[TTS] (no engine) {text}")
