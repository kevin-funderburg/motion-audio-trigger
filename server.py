import logging

from flask import Flask, request


logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)

app = Flask(__name__)


@app.post("/motion")
def motion():
    app.logger.info("Motion request received from %s", request.remote_addr)
    return {"status": "received"}, 202


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
