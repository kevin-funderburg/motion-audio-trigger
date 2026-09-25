import logging
from pathlib import Path
import subprocess

from flask import Flask, request


logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)

app = Flask(__name__)
SOUND_FILE = Path(__file__).parent / "media/audio/The-creature-2026-09-21-14-47-I-see-you-over-there.mp3"


@app.post("/motion")
def motion():
    app.logger.info("Motion request received from %s", request.remote_addr)

    try:
        subprocess.run(["pw-play", str(SOUND_FILE)], check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        app.logger.exception("Audio playback failed")
        return {"status": "playback failed"}, 500

    return {"status": "received"}, 202


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
