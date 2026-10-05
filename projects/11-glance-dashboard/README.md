# Project #11: Centralized Homelab Dashboard with Glance

> **Status:** ✅ Running / Remotely Accessible  
> **Purpose:** Build one browser-based landing page for homelab health, service status, and fast access to self-hosted web interfaces  
> **Focus:** Glance, Docker Compose, Proxmox LXC, Tailscale Serve, custom HTML/CSS, lightweight JSON telemetry, service monitoring

## Why I Chose This Project

As the homelab grew, I ended up with more web interfaces to remember: Proxmox, Pi-hole, Jellyfin, Syncthing, Bitwarden, War Room, and the media-automation applications.

I wanted a single place that answered two questions quickly:

1. Is the lab healthy?
2. Where do I click to manage a service?

Instead of replacing the native administration interfaces, Glance acts as the **front door** to them.

> **Public portfolio note:** The examples in this repository use sanitized addresses and hostnames. Real LAN addresses, Tailscale IPs, private tailnet names, credentials, and tokens are intentionally omitted.

---

## Architecture

Glance runs in its own Debian 13 LXC on Proxmox and is deployed with Docker Compose.

```text
Remote device
     │
     ▼
  Tailscale
     │
     ▼
Tailscale Serve
     │
     ▼
Glance LXC
     │
     ├── System-health JSON endpoint ──► Proxmox host
     │
     ├── Service monitors
     │
     └── Launcher cards
          ├── LAN links
          └── Tailscale links
```

The dashboard does not replace Proxmox, Pi-hole, Jellyfin, or the other service interfaces. It centralizes their status and access paths.

---

## Deployment

I created a dedicated unprivileged Debian 13 LXC for Glance with a small resource footprint:

- 1 vCPU
- 512 MB RAM
- 8 GB root disk
- nesting enabled for Docker
- start-on-boot enabled

Inside the LXC I installed Docker and Docker Compose, then deployed Glance from the official Docker Compose template under:

```text
/opt/glance
```

The main configuration is split between:

```text
/opt/glance/config/home.yml
/opt/glance/config/glance.yml
/opt/glance/assets/custom.css
```

---

## Remote Access

The Glance container also runs Tailscale.

I exposed the local Glance service through **Tailscale Serve**, which gives me private HTTPS access without opening a router port or publishing the dashboard to the public internet.

The traffic path is:

```text
Browser
  │
  ▼
Private Tailscale HTTPS URL
  │
  ▼
Tailscale Serve reverse proxy
  │
  ▼
Glance :8080
```

This keeps the dashboard private to the tailnet while still making it easy to reach when I am away from home.

---

## System Health Widget

I reused a small Python telemetry service that was originally created for my physical Pico dashboard.

I expanded that service so Glance can retrieve a compact JSON response containing:

- CPU utilization
- RAM utilization
- CPU package temperature
- bulk-storage utilization
- host uptime

Glance's custom API widget reads that endpoint and renders the information into a mobile-friendly **System Health** card.

The same host telemetry can therefore serve more than one client:

```text
Proxmox host
    │
    ├──► Physical Pico dashboard
    │
    └──► Glance web dashboard
```

That was a useful design improvement because I did not need to build a second monitoring stack just to display the same data in a browser.

---

## Service Monitoring

The homepage uses Glance's monitor widget to check the important web services.

The current dashboard watches services such as:

- Proxmox
- Pi-hole
- Jellyfin
- War Room
- Syncthing
- Bitwarden
- Seerr
- Sonarr
- Radarr
- Prowlarr
- qBittorrent

The monitor gives me a quick green/red status view and response-time information before I open the underlying service.

Some services are checked over their LAN endpoint while others use their private Tailscale HTTPS endpoint when that is the cleaner or more reliable path.

---

## LAN and Tailscale Launcher

The part I use most is the custom service launcher.

Each application card has two explicit access choices:

```text
Service Name

[ LAN ]   [ TAILSCALE ]
```

I deliberately kept both paths visible instead of trying to make the dashboard automatically choose one.

That makes the launcher useful for troubleshooting:

- **LAN** tests the direct home-network path.
- **TAILSCALE** tests the remote/private-overlay path.

The launcher is divided into two groups:

### Core

- Proxmox
- Pi-hole
- War Room
- Syncthing
- Bitwarden

### Media

- Jellyfin
- Seerr
- Sonarr
- Radarr
- Prowlarr
- qBittorrent

---

## Custom UI

The stock bookmarks widget worked, but it rendered as a simple vertical list.

I wanted the dashboard to be easier to use from a phone, so I replaced that section with a custom HTML widget and added CSS for:

- responsive service cards
- larger mobile text
- LAN and Tailscale buttons
- Core and Media section headings
- responsive two-column/one-column layout
- a clearer System Health block

The goal was not to turn Glance into a completely custom application. I kept the styling close to Glance's native dark interface and only changed the parts that improved usability.

---

## Troubleshooting

This project produced several useful troubleshooting moments.

### Broken YAML after a mobile paste

My first custom `home.yml` failed because indentation was lost and list markers were transformed while pasting from a phone.

Glance entered a restart loop and logged:

```text
Config has errors: yaml: line 11: did not find expected key
```

I verified the failure with:

```bash
docker-compose ps
curl -I http://127.0.0.1:8080
docker-compose logs --tail=100
```

The fix was to restore valid YAML formatting and use a shell heredoc for larger replacements so whitespace and list markers were preserved exactly.

### Tailscale was not the problem

At one point the dashboard would not open remotely. Testing the Tailscale IP also failed.

The key check was:

```bash
curl -I http://127.0.0.1:8080
```

That failed too, which proved the problem was local to Glance rather than MagicDNS or Tailscale. The Docker logs then exposed the YAML error immediately.

### Monitor endpoints

War Room and Bitwarden initially showed monitor errors even though the services themselves were usable.

Direct `curl` tests from the Glance LXC showed which endpoint was actually reachable, and I changed the monitor entries to use the working private HTTPS paths.

This reinforced a useful habit: test the exact path the monitoring service is using instead of assuming that a service being reachable from my phone means it is reachable from another container in the same way.

---

## Sanitized Configuration Examples

This project directory includes sanitized examples of:

- `home.yml`
- `custom.css`

They demonstrate the layout and structure without publishing my real internal addressing or private Tailscale identifiers.

---

## What I Learned

This project gave me hands-on experience with:

- deploying a lightweight dashboard in its own LXC
- Docker Compose inside an unprivileged container
- Tailscale device access and Tailscale Serve
- reverse-proxy concepts
- YAML structure and whitespace-sensitive configuration
- custom Glance widgets
- HTML and CSS customization
- responsive mobile layouts
- JSON API consumption
- reusing telemetry from an existing monitoring project
- service-health checks
- differentiating LAN access from overlay-network access
- troubleshooting from the application layer downward

The biggest lesson was that a dashboard is most useful when it stays focused. I did not try to make Glance replace every administration interface. I made it the place where I can quickly see the state of the lab and jump to the right tool.

---

## Result

I now have a centralized homelab homepage that is available both at home and remotely.

At a glance I can see:

- live Proxmox host health
- whether important services are responding
- response times
- direct links to each web interface
- separate LAN and Tailscale access paths

The result is a much cleaner way to operate a growing homelab without memorizing every address and port.

---

## Future Improvements

- Back up the Glance configuration alongside other critical service configuration
- Add additional dashboard pages only when they solve a real operational need
- Consider alerting for failures that matter enough to require notification
- Continue improving local DNS so service names can replace more raw addresses
- Add additional infrastructure monitoring as the homelab grows
