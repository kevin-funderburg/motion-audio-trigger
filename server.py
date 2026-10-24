import argparse
import logging
from pathlib import Path
import random
import subprocess

from flask import Flask, request


logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)

AUDIO_ROOT = Path(__file__).resolve().parent / "media/audio"
AUDIO_DIR = AUDIO_ROOT / "halloween"

app = Flask(__name__)


@app.post("/motion")
def motion():
    app.logger.info("Motion request received from %s", request.remote_addr)

    samples = [
        path for path in AUDIO_DIR.rglob("*")
        if path.is_file() and path.suffix.lower() in {".mp3", ".wav"}
    ]
    if not samples:
        app.logger.error("No audio samples found in %s", AUDIO_DIR)
        return {"status": "no audio samples"}, 500

    sound_file = random.choice(samples)
    app.logger.info("Playing audio sample: %s", sound_file.relative_to(AUDIO_ROOT))

    try:
        subprocess.run(["pw-play", str(sound_file)], check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        app.logger.exception("Audio playback failed")
        return {"status": "playback failed"}, 500

    return {"status": "received"}, 202


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description="Play a random themed audio sample on motion.")
    parser.add_argument(
        "-t", "--theme", default="halloween",
        choices=sorted(path.name for path in AUDIO_ROOT.iterdir() if path.is_dir()),
        help="audio theme directory under media/audio (default: halloween)",
    )
    args = parser.parse_args(argv)
    audio_dir = AUDIO_ROOT / args.theme
    if not audio_dir.is_dir():
        parser.error(f"theme directory does not exist: {audio_dir}")
    return args


def main(argv=None):
    global AUDIO_DIR

    args = parse_args(argv)
    AUDIO_DIR = AUDIO_ROOT / args.theme

    app.logger.info("Using audio theme: %s", args.theme)
    app.run(host="0.0.0.0", port=5000)


if __name__ == "__main__":
    main()
