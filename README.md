# Halloween Motion Audio Project

A small Raspberry Pi + ESP32 project for triggering outdoor audio effects from motion sensors.

The initial architecture is:

```text
PIR Sensor
    |
    | GPIO
    v
ESP32
    |
    | HTTP over Wi-Fi
    v
Raspberry Pi
    |
    | Bluetooth audio
    v
Speaker
```

The Raspberry Pi acts as the central server and audio player. ESP32 devices will eventually detect motion from PIR sensors and send events to the Pi over Wi-Fi.

## Milestone 1: Bluetooth Audio on a Headless Raspberry Pi

The first milestone is getting the Raspberry Pi to reliably play an audio file through a Bluetooth speaker.

This Pi is running Raspberry Pi OS Lite and is used as a headless machine over SSH. That introduces one important Bluetooth-audio complication: Bluetooth pairing can work even when the Linux audio stack is not configured to expose Bluetooth audio devices.

### Audio Stack

The relevant pieces are:

```text
Application / audio player
        |
        v
     PipeWire
        |
        v
    WirePlumber
        |
        v
       BlueZ
        |
        v
 Bluetooth adapter
        |
        v
 Bluetooth speaker
```

- **BlueZ** is the Linux Bluetooth stack. It handles discovery, pairing, trust, connections, Bluetooth profiles, etc.
- **PipeWire** is the multimedia server responsible for routing audio.
- **WirePlumber** manages PipeWire devices, routing, and policy.
- **wpctl** is WirePlumber's command-line control utility.

### Install Bluetooth Audio Support

A minimal Raspberry Pi OS Lite installation may have BlueZ installed without the PipeWire Bluetooth audio components.

Install:

```bash
sudo apt update

sudo apt install \
    pipewire \
    pipewire-pulse \
    wireplumber \
    libspa-0.2-bluetooth
```

The less-obvious package here is:

```text
libspa-0.2-bluetooth
```

This provides PipeWire's Bluetooth plugins, allowing Bluetooth speakers and headsets discovered through BlueZ to become PipeWire audio devices.

After installation, restart the user-level audio services:

```bash
systemctl --user restart wireplumber pipewire pipewire-pulse
```

Check their status with:

```bash
systemctl --user status wireplumber pipewire pipewire-pulse
```

### Pair a Bluetooth Speaker

Start the BlueZ command-line client:

```bash
bluetoothctl
```

Inside `bluetoothctl`:

```text
power on
agent on
default-agent
scan on
```

These commands:

- `agent on` enables the pairing agent that handles Bluetooth pairing/authentication requests.
- `default-agent` makes that agent the default pairing handler.
- `scan on` begins discovering nearby Bluetooth devices.

Once the speaker appears, note its Bluetooth MAC address:

```text
Device 94:4B:F8:7E:A0:FA TOZO PE1
```

Pair it:

```text
pair 94:4B:F8:7E:A0:FA
```

Trust it so future connections do not require manual authorization:

```text
trust 94:4B:F8:7E:A0:FA
```

Then connect:

```text
connect 94:4B:F8:7E:A0:FA
```

Scanning can then be stopped:

```text
scan off
```

Useful inspection commands include:

```text
devices
paired-devices
info 94:4B:F8:7E:A0:FA
```

### Headless Raspberry Pi Bluetooth Issue

On a headless Pi, pairing may succeed while connecting fails with:

```text
Failed to connect:
org.bluez.Error.Failed br-connection-profile-unavailable
```

In this case:

- BlueZ can see the speaker.
- The device can successfully pair and become trusted.
- PipeWire and WirePlumber may both be running.
- The Bluetooth PipeWire plugin may already be installed.
- But WirePlumber may still refuse to expose the Bluetooth audio device.

The cause can be WirePlumber's default **seat monitoring** behavior.

Desktop Linux systems normally use `systemd-logind` to determine which local graphical session owns hardware devices. On a headless Pi accessed exclusively over SSH, the login session may be active but have no physical `seat`.

For example:

```text
SESSION  UID  USER  SEAT
36       1000 kfun  -
```

For a dedicated headless audio server, Bluetooth seat monitoring can be disabled.

Create a WirePlumber configuration directory:

```bash
mkdir -p ~/.config/wireplumber/wireplumber.conf.d
```

Create:

```text
~/.config/wireplumber/wireplumber.conf.d/80-disable-bluetooth-seat-monitoring.conf
```

with:

```text
wireplumber.profiles = {
  main = {
    monitor.bluez.seat-monitoring = disabled
  }
}
```

Restart WirePlumber:

```bash
systemctl --user restart wireplumber
```

Then retry:

```bash
bluetoothctl connect 94:4B:F8:7E:A0:FA
```

A successful connection should now look like:

```text
Connection successful
```

### Inspect Audio Devices

Use:

```bash
wpctl status
```

Once Bluetooth audio is working, the speaker should appear as a BlueZ-backed PipeWire device:

```text
Audio
 ├─ Devices:
 │      73. TOZO PE1          [bluez5]
 │
 ├─ Sinks:
 │  *   77. TOZO PE1          [vol: 0.30]
```

The `*` indicates the current default audio sink.

A **sink** is an audio destination. In this case, the Bluetooth speaker is the destination where applications send playback audio.

### Test Audio Playback

PipeWire includes a simple command-line player:

```bash
pw-play /path/to/audio.wav
```

`pw-play` sends the audio file into PipeWire, which routes it to the current default audio sink.

For this project, that should be the Bluetooth speaker.

### Volume Control

WirePlumber's CLI can control sink volume:

```bash
wpctl set-volume 77 0.90
```

Volume is expressed as a scalar:

```text
0.0 =   0%
0.5 =  50%
1.0 = 100%
```

For example:

```bash
wpctl set-volume 77 0.50
```

sets the speaker to 50%.

```bash
wpctl set-volume 77 0.90
```

sets it to 90%.

Values greater than `1.0` are possible, but represent software amplification and may introduce clipping or distortion.

For outdoor playback, keeping PipeWire near `0.8-1.0` and adjusting the speaker's own hardware volume is preferable to software amplification beyond 100%.

### Useful Bluetooth and Audio Commands

Check Bluetooth state:

```bash
bluetoothctl
```

Check PipeWire/WirePlumber state:

```bash
wpctl status
```

Restart the audio stack:

```bash
systemctl --user restart wireplumber pipewire pipewire-pulse
```

Check audio-service status:

```bash
systemctl --user status wireplumber pipewire pipewire-pulse
```

Connect the known speaker directly:

```bash
bluetoothctl connect 94:4B:F8:7E:A0:FA
```

Play a WAV file:

```bash
pw-play sound.wav
```

Set speaker volume:

```bash
wpctl set-volume 77 0.90
```

## Current Status

Bluetooth audio playback from the headless Raspberry Pi to the TOZO PE1 is working.

The development container can also send audio to the host PipeWire server. A
`POST /motion` request from another device on the local network plays the
randomly selected sample through the host's default audio sink.

Next milestone:

```text
ESP32 PIR event
     |
     v
POST /motion
     |
     v
Raspberry Pi HTTP server
     |
     v
Play audio through Bluetooth speaker
```

The endpoint is tested manually with `curl` before the ESP32 is introduced.

## ESP32 PIR Firmware

`firmware/esp32-pir/` is a standalone ESP-IDF project for an ESP32 connected
to a PIR sensor. It polls the sensor on GPIO27 and mirrors its state to an LED
on GPIO26. The project builds for the original ESP32 target, which QEMU
emulates.

### Wi-Fi Setup

The ESP32 joins a configured 2.4 GHz radio, pinned to its BSSID (Basic Service
Set Identifier). The channel is discovered automatically. The Pi can remain
on the network's 5 GHz radio.

Copy `firmware/esp32-pir/main/wifi/wifi_config.example.h` to
`firmware/esp32-pir/main/wifi/wifi_config.h` and set `WIFI_SSID`, `WIFI_PASSWORD`,
and `WIFI_BSSID` for your network. The local configuration is ignored by Git.
Without a password, the firmware logs that Wi-Fi is disabled and still runs
the PIR/LED loop.

After building, flash from a devcontainer terminal:

```bash
cd firmware/esp32-pir
idf.py -b 115200 -p /dev/ttyUSB0 flash monitor
```

The serial log reports the connected radio address, channel, assigned Internet
Protocol (IP) address, netmask, gateway, and signal strength in dBm (decibels
relative to one milliwatt). Address assignment uses DHCP (Dynamic Host
Configuration Protocol). Disconnections trigger retries after five seconds;
the PIR/LED loop continues while reconnecting. Pinning the radio prevents
fallback to another mesh node if that radio is unavailable.

On the Pi host, try `ping <esp32-ip-address>` using the logged address to
check reachability. Test reconnection by temporarily turning off the chosen
access point and restoring it. QEMU does not emulate the physical Wi-Fi radio;
Wi-Fi and sensor behavior must be verified on the board.

### Send Motion Events to the Pi

The firmware sends an empty HTTP (Hypertext Transfer Protocol) `POST /motion`
on each low-to-high PIR transition. The LED continues to mirror the sensor
while a separate task waits for the server response.

Start the Python server in one devcontainer terminal from the repository root:

```bash
python3 server.py
```

In another devcontainer terminal, configure the Pi's endpoint and flash:

```bash
cd firmware/esp32-pir
idf.py -DMOTION_SERVER_URL="http://<pi-ip-address>:5000/motion" build
idf.py -b 115200 -p /dev/ttyUSB0 flash monitor
```

Use the Pi host's LAN address, not `localhost` or a Docker container address.
`MOTION_SERVER_URL` is retained in the ignored build directory until changed
or the build is cleaned. With no URL configured, motion requests are disabled
and the PIR/LED behavior still works.

Wave in front of the sensor after Wi-Fi receives an address. The server should
log the ESP32's source address and play the sample; the firmware should log
`Motion accepted (HTTP 202)`. The server responds after playback finishes,
so the firmware allows 30 seconds for each request.

Motion while offline is skipped. Multiple detections while a request is in
progress are combined into at most one pending request. Failed requests are
logged without automatic retries, because the server may have played the audio
even if its response was lost. Falling transitions only turn off the LED.

From a devcontainer terminal, build it once:

```bash
cd firmware/esp32-pir
idf.py set-target esp32
idf.py build
```

Run the built firmware in QEMU. Its serial console is attached to the terminal:

```bash
idf.py qemu
```

The boot log ends with:

```text
I (...) motion: PIR input on GPIO27; LED output on GPIO26
```

Use `Ctrl+A`, then `X` to stop QEMU. `build/` and `sdkconfig` are generated
files and are intentionally not tracked.

### Motion Endpoint

The Flask server accepts a motion event, logs its source address, and plays the
randomly selected sample with `pw-play`. Flask and `pw-play` are installed in the
development image.

Audio is grouped by theme under `media/audio/`. Place your own MP3 and WAV samples
in a theme's `local/` directory, which is ignored by Git. On each request,
`server.py` gathers all MP3 and WAV files recursively under the selected theme,
including both `examples/` and `local/`, and chooses randomly from the combined
list. The selected path is logged. New files are picked up without restarting
the server, and consecutive requests may select the same sample. If the theme
contains no samples, the endpoint returns `500` without attempting playback.
See [the audio README](media/audio/README.md) for the directory layout.

`pw-play` runs inside the container, but connects to the host user's PipeWire
socket through the `/run/host-user` bind mount. PipeWire on the Pi then routes
the audio to its current default sink, such as the paired Bluetooth speaker.

From a devcontainer terminal, start the server:

```bash
python3 server.py -t halloween
```

Use `-t <theme>` or `--theme <theme>` to select an existing theme directory,
such as `christmas`. Omitting the argument defaults to `halloween`.

```bash
./dockerw -p python3 server.py -t halloween
```

Plain `dockerw` runs do not publish service ports. Only one container can own
the Pi's port `5000` at a time. If the devcontainer is running, use its terminal
to run the server, or stop it from the Pi host before starting the separate
server container:

```bash
docker compose --project-directory docker -f docker/compose.yaml stop motion-audio-trigger
./dockerw -p python3 server.py -t halloween
```

From another device on the local network, send a test event to the Pi:

```bash
curl --include --request POST http://<pi-ip-address>:5000/motion
```

Docker maps the Pi's port `5000` to the container's port `5000`, allowing LAN
clients to reach the server. The request waits for `pw-play` to finish. A
successful request returns `202 Accepted`; a playback failure returns `500`.

Rebuild the devcontainer after changing its Dockerfile or Compose configuration
so new packages, mounts, and port mappings are applied.
