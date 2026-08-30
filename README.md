# Homelab Portfolio

> **Learning by building. Breaking things carefully. Writing down what I learn.**

This repository documents the beginning of my homelab journey using a repurposed **HP EliteDesk 800 G4 SFF** running **Proxmox VE**.

I am not presenting this repository as if I already know everything about Linux, networking, virtualization, or self-hosting. The point of the lab is the opposite: I am using real hardware and real services to move those subjects from theory into hands-on experience.

Each project includes what I was trying to accomplish, what I configured, what confused me, what broke, how I tested it, and what I learned from getting it working.

> 🔒 **Public portfolio note:** Documentation is sanitized. Example private IP addresses may be substituted for real values, and passwords, tokens, public IP addresses, private keys, private Tailscale hostnames/domains, device IDs, and other sensitive identifiers are not intentionally published.

---

## 🧭 Where I Am Right Now

This is an **active beginner homelab**, not a finished environment.

The current goal is to keep adding useful services while gradually building stronger fundamentals in:

`Linux` · `Networking` · `Proxmox` · `DNS` · `Containers` · `Remote Access` · `Storage` · `Troubleshooting`

### Current project progress

| # | Project | Status | What it pushed me to learn |
|---|---|---|---|
| 1 | [Proxmox Home Server Deployment](projects/01-proxmox-home-server/README.md) | ✅ Running / Expanded | Hypervisors, Linux bridges, static IPs, remote administration, filesystems, persistent storage |
| 2 | [Pi-hole DNS & DHCP Deployment on Proxmox](projects/02-pihole/README.md) | ✅ Running | DNS, DHCP/DNS interaction, LXC networking, `ping`, `dig`, live logs |
| 3 | [Secure Self-Hosted Application Deployment](projects/03-warroom/README.md) | ✅ Running | Tailscale, HTTPS, browser permissions, privacy-conscious self-hosting |
| 4 | [Self-Hosted Obsidian Vault Synchronization](projects/04-obsidian-syncthing/README.md) | ✅ Running | Syncthing, Linux services, multi-device sync, versioning |
| 5 | [Jellyfin Media Server Deployment on Proxmox](projects/05-jellyfin/README.md) | ✅ Running / Remotely Tested | LXC storage bind mounts, Linux permissions, Intel Quick Sync/VA-API, private remote streaming |
| 6 | Containerized DVD ripping / media ingestion | 🚧 In Progress | Optical-device passthrough, media extraction, FFmpeg workflow, file organization |
| 7 | Immich / photo backup | 🔜 Planned | Self-hosted photo storage, backups, data protection |

---

## 🖥️ The Lab

### Main server

| Component | Current hardware |
|---|---|
| System | HP EliteDesk 800 G4 SFF |
| CPU | Intel Core i5-8500 |
| Memory | 32 GB RAM |
| Primary storage | 1 TB NVMe SSD |
| Bulk storage | 4 TB SATA HDD |
| Hypervisor | Proxmox VE |

### Other devices I use while learning

- **Fedora ThinkPad** — Linux workstation and networking practice
- **Windows laptop** — installer creation, administration, general workstation
- **ClockworkPi uConsole** — portable Linux/radio/network experimentation
- **Android phone** — remote administration, synchronized notes, and real-world client testing

### Current high-level layout

```text
                       Internet
                          │
                          ▼
                     Home Router
                          │
                 ┌────────┴────────┐
                 │                 │
           Normal Clients      HP EliteDesk
                                   │
                                   ▼
                              Proxmox VE
                  ┌────────┬───────┼────────┬────────┐
                  │        │       │        │        │
               Pi-hole  War Room Syncthing Jellyfin Media
                 LXC      App     Service    LXC    Storage
                                     │        │       │
                          Fedora ↔ Server ↔ Android   │
                                              │       │
                                              └── 4 TB HDD
```

This diagram is intentionally simplified and uses no sensitive addressing information.

---

# 📚 Projects

## Project #1: Proxmox Home Server Deployment

**The starting point.** I took a used small-form-factor business PC and turned it into my first dedicated virtualization host.

The install itself was only part of the project. The first real lesson came when Proxmox appeared unreachable and I had to work through Ethernet connectivity, static addressing, the gateway, `vmbr0`, and the management port rather than assume the installation had failed.

The project later expanded with a 4 TB SATA HDD that I health-checked, repartitioned, formatted as `ext4`, and mounted persistently for future bulk application data.

**Things I touched:**

`Proxmox VE` · `Linux bridges` · `Static IPv4` · `Tailscale` · `SSH` · `SMART` · `GPT` · `ext4` · `/etc/fstab` · `Storage planning`

➡️ **[Read Project #1](projects/01-proxmox-home-server/README.md)**

---

## Project #2: Pi-hole DNS & DHCP Deployment on Proxmox

I wanted my second project to actually do something useful for the home network, so I deployed Pi-hole inside its own Debian LXC.

This project became much more about troubleshooting than I originally expected. At different points I had to separate raw internet connectivity from DNS resolution, inspect routes and neighbor behavior, test Pi-hole directly with `dig`, and watch live queries before I could be confident devices were really using it.

**Things I touched:**

`Debian 13` · `LXC` · `Pi-hole` · `DNS` · `DHCP/DNS interaction` · `ping` · `dig` · `Live logs`

➡️ **[Read Project #2](projects/02-pihole/README.md)**

---

## Project #3: Secure Self-Hosted Application Deployment

For the third project, I deployed a third-party War Room web application related to one of my hobbies.

I did **not** develop the application. My work was on the infrastructure side: hosting it, accessing it remotely through Tailscale, moving from the original HTTP access path to HTTPS using Tailscale Serve, and stopping to investigate password handling and browser geolocation permissions instead of blindly clicking through them.

**Things I touched:**

`Self-hosting` · `Tailscale` · `Tailscale Serve` · `HTTPS` · `Browser permissions` · `Privacy review`

➡️ **[Read Project #3](projects/03-warroom/README.md)**

---

## Project #4: Self-Hosted Obsidian Vault Synchronization

As my CCNA and networking notes started becoming something I actually relied on, I wanted the same Obsidian vault available from both my Fedora ThinkPad and Android phone.

For this project I used Syncthing with an always-on copy in the homelab rather than manually transferring notes between devices. I paired each device, located the actual Obsidian vault from Linux, configured folder sharing and versioning, and then tested changes in both directions before considering it finished.

**Things I touched:**

`Syncthing` · `Obsidian` · `systemd` · `Linux permissions` · `Device discovery` · `File versioning` · `Android/Linux sync`

➡️ **[Read Project #4](projects/04-obsidian-syncthing/README.md)**

---

## Project #5: Jellyfin Media Server Deployment on Proxmox

After adding the 4 TB HDD, I wanted to turn that storage into a useful service rather than leave it as unused capacity. I deployed Jellyfin in its own Debian 13 LXC and bind-mounted the media directory from the bulk-storage drive into the container.

This project pulled several parts of the homelab together. I had to work through LXC storage mapping and permissions, fix an initial Debian repository mismatch during installation, pass the Intel UHD 630 render device into the unprivileged container, verify the Intel `iHD` VA-API driver from inside the LXC, and test access from the Android Jellyfin app over Tailscale while away from home.

**Things I touched:**

`Jellyfin` · `Debian 13` · `LXC bind mounts` · `Linux permissions` · `Intel Quick Sync` · `VA-API` · `i915` · `Tailscale` · `Remote client testing`

➡️ **[Read Project #5](projects/05-jellyfin/README.md)**

---

## 🧠 What I Am Trying to Get Better At

A big reason I started documenting this is so I can look back and see the difference between what I understood at the beginning and what I understand later.

Right now I am deliberately working on:

- Understanding *why* a command or configuration works instead of only copying it.
- Troubleshooting one layer at a time.
- Getting more comfortable in Linux terminals.
- Learning IPv4 addressing and subnetting well enough that they become intuitive.
- Understanding DNS, DHCP, routing, switching, VLANs, and firewalling through actual use.
- Building toward CCNA-level networking knowledge.
- Writing documentation that another person could follow without having been there when I built it.

---

## 🔧 What Is Coming Next

The lab is still young, so there is a lot left to build.

The 4 TB bulk-storage integration is complete at the host level and is now actively being used by Jellyfin through an LXC bind mount. The next media-related work is a separate containerized DVD-ripping and ingestion workflow rather than folding that process into the Jellyfin project itself.

Planned or developing projects include:

- Containerized DVD ripping / media ingestion
- Immich photo backup
- Backup and recovery strategy
- Monitoring and resource dashboards
- Local DNS improvements
- VLANs and network segmentation
- Firewall rules and access-control experiments
- Additional virtual networking labs
- CCNA-focused labs and packet analysis
- More detailed architecture diagrams

Some of those plans will probably change as I learn more. That is part of what I want this repository to capture.

---

## 🛠️ Technologies I Have Used So Far

![Proxmox](https://img.shields.io/badge/Proxmox-VE-informational)
![Debian](https://img.shields.io/badge/Debian-13-informational)
![Linux](https://img.shields.io/badge/Linux-CLI-informational)
![Pi-hole](https://img.shields.io/badge/Pi--hole-DNS-informational)
![Tailscale](https://img.shields.io/badge/Tailscale-Remote%20Access-informational)
![Syncthing](https://img.shields.io/badge/Syncthing-File%20Sync-informational)
![Obsidian](https://img.shields.io/badge/Obsidian-Notes-informational)
![Jellyfin](https://img.shields.io/badge/Jellyfin-Media%20Server-informational)
![Intel Quick Sync](https://img.shields.io/badge/Intel-Quick%20Sync-informational)
![SSH](https://img.shields.io/badge/SSH-Administration-informational)

Badges here mean **"I have used this in the lab"**, not **"I am an expert in this technology."**

---

## Why This Repository Exists

I could wait until I know much more and build a polished portfolio afterward, but that would erase the most useful part of the story.

I would rather document the journey from the beginning—including the mistakes and the moments where I had to stop and figure out what was actually happening.

The goal is that, over time, this repository shows two things:

1. The infrastructure becoming more capable.
2. My understanding becoming more capable with it.

---

**Current status:** 🟢 Homelab online and actively evolving.
