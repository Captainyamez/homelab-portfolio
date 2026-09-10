# Project #7: Physical Homelab Monitoring Dashboard

> **Status:** ✅ Working / Standalone wall-powered display  
> **Purpose:** Build a physical status display that combines live Proxmox telemetry with room temperature and humidity data  
> **Focus:** Raspberry Pi Pico W, Arduino/C++, I²C sensors, Wi-Fi, REST APIs, JSON, Proxmox API authentication, Linux hardware monitoring, systemd

## Why I Chose This Project

After building several services in Proxmox, I wanted a way to see the state of the lab without opening a browser or SSH session every time I walked into the room.

The goal became a small standalone display that could answer two different questions at a glance:

1. Is the homelab itself healthy and are the main services running?
2. What are the server and room conditions right now?

I used a Raspberry Pi Pico W with a 3.5-inch Waveshare LCD and an SHT41 temperature/humidity sensor. The Pico reads the environmental sensor locally, connects to the network over Wi-Fi, queries the Proxmox API for infrastructure data, and queries a small read-only helper service on the Proxmox host for the CPU package temperature.

The result is a physical monitoring appliance that boots automatically from wall power and refreshes itself without needing a laptop attached.

> **Public portfolio note:** Network addresses and API credentials shown in this documentation and example source files are placeholders. The real Wi-Fi credentials, Proxmox token secret, and environment-specific addresses are not published.

---

## Final Dashboard

The current display shows:

- Proxmox host status
- Pi-hole status
- Jellyfin status
- Syncthing status
- War Room status
- DVD-ripping container status
- Host CPU utilization
- Host RAM utilization
- Proxmox uptime
- CPU package temperature
- Room temperature
- Relative humidity

A typical screen looks conceptually like this:

```text
HOMELAB                                      V6
────────────────────────────────────────────────
SERVICES

PVE       ● ONLINE
PIHOLE    ● ONLINE
JELLYFIN  ● ONLINE
SYNC      ● ONLINE
WARROOM   ● ONLINE
RIPPER    ● ONLINE

────────────────────────────────────────────────
CPU 1%       RAM 10%       UP 12d 20h
────────────────────────────────────────────────
SERVER 33.0C       ROOM 68.7F
HUMIDITY 63.7%
```

The values above are only an example representation of the live layout.

---

## Hardware

| Component | Role |
|---|---|
| Raspberry Pi Pico W | Microcontroller, Wi-Fi client, display controller |
| Waveshare Pico-ResTouch-LCD-3.5 | 480x320 physical dashboard |
| SHT41 temperature/humidity sensor | Measures room temperature and relative humidity |
| 52Pi Pico breadboard/breakout setup | Provides a solder-free prototyping and wiring platform |
| Raspberry Pi micro-USB power supply | Standalone wall power with inline on/off switch |
| HP EliteDesk 800 G4 SFF | Proxmox host being monitored |

The project was deliberately built around **zero soldering**. Pre-installed headers, plug-in connectors, a breakout/breadboard, and jumper wiring allowed the prototype to become a functioning standalone device without soldering components directly.

---

## SHT41 Wiring

The environmental sensor uses I²C:

```text
SHT41              Pico W
─────              ──────
SDA   ───────────► GPIO 0
SCL   ───────────► GPIO 1
VCC   ───────────► 3.3 V
GND   ───────────► GND
```

The Pico reads temperature in Celsius from the sensor, converts it to Fahrenheit for the display, and reads relative humidity directly.

---

## Architecture

```text
                    ┌─────────────────────────┐
                    │      Proxmox Host       │
                    │                         │
                    │  Proxmox REST API       │
                    │  ├─ CPU usage           │
                    │  ├─ RAM usage           │
                    │  ├─ uptime              │
                    │  └─ LXC/VM status       │
                    │                         │
                    │  lm-sensors             │
                    │       │                 │
                    │       ▼                 │
                    │ homelab-temp.service    │
                    │       │                 │
                    │       ▼                 │
                    │ read-only HTTP endpoint │
                    └──────────┬──────────────┘
                               │
                         Wi-Fi / HTTP(S)
                               │
                               ▼
                    ┌─────────────────────────┐
                    │ Raspberry Pi Pico W     │
                    │                         │
SHT41 ── I²C ─────► │ Arduino/C++ firmware    │
                    │                         │
                    └──────────┬──────────────┘
                               │
                               ▼
                    ┌─────────────────────────┐
                    │ Waveshare 3.5-inch LCD  │
                    │ Live physical dashboard │
                    └─────────────────────────┘
```

This design separates data collection by responsibility. Proxmox remains the authoritative source for virtualization and resource data, Linux reads hardware temperature locally, and the Pico directly measures the room environment.

---

## Proxmox API Access

I did not place the normal Proxmox administrative account credentials on the Pico.

Instead, I created a dedicated read-only account and API token for the dashboard. The token has only the permissions needed to read the host and guest status information used by the display.

The firmware sends the token through the Proxmox `Authorization` header and consumes JSON responses from endpoints such as:

```text
/api2/json/nodes/<node>/status
/api2/json/cluster/resources?type=vm
```

The first endpoint supplies host statistics such as CPU utilization, memory use, and uptime. The cluster resources endpoint supplies the running/stopped state for the LXCs displayed on the panel.

The public source file intentionally contains placeholders instead of the real token secret or Wi-Fi credentials.

---

## Why CPU Temperature Needed a Separate Path

The Proxmox node-status API provided the resource statistics I wanted, but it did not directly expose the CPU package temperature in the response I was using.

Linux already had the measurement available through `lm-sensors`:

```text
coretemp-isa-0000
Package id 0:  +35.0°C
Core 0:        +29.0°C
Core 1:        +35.0°C
...
```

For the dashboard I chose `Package id 0` as the single representative CPU temperature rather than trying to fit six individual core readings onto the small screen.

I created a small Python HTTP service on the Proxmox host that reads the JSON output from `sensors -j` and returns only the package temperature:

```json
{
  "cpu_temp": 35.0
}
```

The helper runs under systemd and starts automatically with the host.

Example source: [`server/homelab-temp.py`](server/homelab-temp.py)

Example service unit: [`server/homelab-temp.service`](server/homelab-temp.service)

---

## systemd Integration

The helper endpoint is managed as a normal Linux service rather than being started manually after each reboot.

The unit is enabled for `multi-user.target`, and I verified it with:

```bash
systemctl daemon-reload
systemctl enable --now homelab-temp.service
systemctl status homelab-temp.service
```

A successful test showed the service as `active (running)` and confirmed that the temperature endpoint continued to respond independently of an interactive shell.

This was useful because the physical dashboard should behave like infrastructure, not like a script that depends on me remembering to start it.

---

## Firmware

The Pico firmware is written as an Arduino `.ino` sketch and uses:

- `WiFi`
- `WiFiClientSecure`
- `HTTPClient`
- `ArduinoJson`
- `Wire`
- `Adafruit_SHT4x`
- `TFT_eSPI`

The current firmware is based on the sixth working iteration of the dashboard.

Example source: [`firmware/homelab_display_v6.ino`](firmware/homelab_display_v6.ino)

The firmware intentionally handles several failure cases rather than assuming every data source is always available. For example, a failed server-temperature request results in a placeholder value rather than crashing the entire dashboard.

---

## Iterative Build Process

This project changed substantially as I tested each layer.

### Early versions

The first versions focused on proving the physical hardware:

- Bring up the 3.5-inch LCD
- Verify the Pico/display wiring
- Verify the SHT41 over I²C
- Read room temperature and humidity

### Proxmox integration

The next iterations connected the Pico to Wi-Fi and replaced placeholder server values with real data from the Proxmox API.

This required:

- A dedicated API account/token
- Read-only API permissions
- HTTPS requests from the Pico
- JSON parsing
- Mapping Proxmox VMIDs to friendly service names
- Handling `running`, `stopped`, and unknown states

### Hardware-temperature integration

The final major addition was exposing Linux CPU package temperature through a minimal helper service and reading that value from the Pico alongside the Proxmox API and SHT41 data.

Breaking the project into small versions made troubleshooting much easier. Each version proved one new layer before adding another.

---

## Verification

I verified the dashboard at several layers instead of assuming a successful compile meant the system worked.

### Pico / LCD

- Arduino sketch compiled without errors.
- Firmware uploaded successfully.
- LCD initialized in landscape orientation.
- Dashboard redraws on its configured refresh interval.
- Device boots and operates from a wall adapter without the development laptop connected.

### Environmental sensor

- SHT41 detected over I²C on GPIO 0/1.
- Temperature and humidity values update on the screen.
- Sensor readings are also visible through the serial debug output during development.

### Proxmox API

- Dedicated API-token authentication succeeds.
- PVE host status is read live.
- CPU, RAM, and uptime values change with the host.
- Container status values match Proxmox state.

### CPU temperature

- `lm-sensors` reports the Intel CPU package temperature on the host.
- The helper endpoint returns valid JSON.
- The systemd service remains active independently of the terminal session.
- The Pico displays the returned package temperature.

### Standalone operation

After the firmware was uploaded, I disconnected the development laptop and powered the display from a Raspberry Pi-branded micro-USB wall adapter. The Pico booted, reconnected to Wi-Fi, read both network and physical sensors, and rebuilt the dashboard automatically.

---

## Security Decisions

A monitoring display is not worth weakening the management plane of the homelab, so I deliberately kept its access narrow.

- The Pico does not contain the normal Proxmox administrator password.
- A dedicated API identity is used instead of the root account.
- The API token is limited to read-only monitoring permissions.
- Published firmware contains placeholders rather than the real token secret or Wi-Fi password.
- The temperature helper exposes only a single sensor value rather than arbitrary shell execution.
- The helper is intended for the trusted LAN, not direct public Internet exposure.

One limitation in the current firmware is that the Pico accepts the Proxmox host's self-signed HTTPS certificate without CA validation. The connection is encrypted, but the Pico does not independently validate the server certificate identity. Tightening certificate validation is a possible future improvement.

---

## What I Learned

This project gave me hands-on experience with:

- Raspberry Pi Pico development
- Arduino/C++ firmware
- SPI display integration
- I²C sensors
- GPIO planning
- Wi-Fi clients on embedded hardware
- REST API consumption
- API-token authentication
- JSON parsing on a microcontroller
- Proxmox API endpoints
- Mapping VM/LXC IDs to application status
- Linux `lm-sensors`
- Python HTTP services
- systemd service creation and enablement
- Separating local sensor data from network telemetry
- Designing failure behavior for a monitoring interface
- Sanitizing public source code so credentials are not committed

The biggest lesson was that a physical dashboard can be treated like a small distributed system. The display itself is only the last step. The data comes from several independent sources, each with its own protocol, failure modes, and security considerations.

---

## Result

I now have a standalone physical homelab dashboard that can be switched on at the wall and automatically reconnect to the network, query the Proxmox environment, read the room sensor, and display current infrastructure and environmental status.

The project combines embedded hardware with the virtualization environment I had already built instead of existing as an isolated microcontroller exercise.

That makes the screen useful day-to-day, but it also gave me a practical reason to work with APIs, authentication, JSON, Linux sensors, systemd, I²C, embedded networking, and fault handling in one project.

---

## Future Improvements

- Add storage utilization to the display
- Add warning thresholds for CPU temperature and room humidity
- Change status colors when thresholds are exceeded
- Add a display blanking/dimming mode to reduce unnecessary LCD runtime
- Add touch controls for switching between summary and detailed pages
- Add a second page for storage/network statistics
- Consider certificate validation instead of `setInsecure()` for Proxmox HTTPS
- Restrict the temperature helper further with host firewall rules if the network design changes
- Add a finished mounting plate or enclosure while keeping the build serviceable
