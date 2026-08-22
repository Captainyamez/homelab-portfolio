# Project 1 — Proxmox Home Server Deployment

## Overview

This project documents the initial deployment of my home lab using an HP EliteDesk 800 G4 SFF as a virtualization server. The goal was to turn inexpensive used enterprise hardware into a flexible platform for learning Linux, virtualization, networking, self-hosting, storage, and infrastructure administration.

> **Public portfolio note:** Network addresses and other environment-specific identifiers in this repository may be sanitized or replaced with documentation examples. No passwords, tokens, public IP addresses, private keys, or remote-access identifiers are intentionally published.

## Hardware

- HP EliteDesk 800 G4 SFF
- Intel Core i5-8500
- 32 GB RAM
- 1 TB NVMe SSD
- 4 TB SATA HDD for bulk storage
- Gigabit Ethernet

## Platform

- Proxmox VE
- Linux containers (LXC) and future virtual machines
- Tailscale for authenticated remote administration

## Objectives

- Install a dedicated virtualization platform on the EliteDesk.
- Configure a stable management interface on the home LAN.
- Learn the Proxmox web interface and command-line tools.
- Build a foundation for multiple isolated self-hosted services.
- Establish secure remote administration without directly exposing Proxmox to the public internet.
- Expand storage for future media, photo, and backup workloads.

## Deployment

### 1. Installation media

A Proxmox installation image was downloaded on a Windows laptop and written to a USB flash drive. The EliteDesk was then booted from USB and Proxmox was installed to the NVMe SSD.

### 2. Host networking

The Proxmox host was configured with a static management address on the local network using the default Linux bridge, `vmbr0`.

Sanitized example:

```text
Interface: vmbr0
Address:   192.168.10.10/24
Gateway:   192.168.10.1
Bridge:    Physical Ethernet NIC
```

Using a static address keeps the hypervisor reachable at a predictable management address and provides a stable bridge for containers and virtual machines.

### 3. Web management

The Proxmox web interface was accessed over HTTPS on port `8006` from another device on the LAN.

```text
https://192.168.10.10:8006
```

### 4. Remote administration

Tailscale was installed so the server could be administered while away from home without port-forwarding the Proxmox management interface through the home router.

This gave me a practical introduction to overlay networking and authenticated remote access while keeping the management service off the open internet.

### 5. Storage expansion

The system was initially brought online with the 1 TB NVMe SSD. A 4 TB SATA HDD was later installed for bulk data such as media, photo storage, and backups.

Separating fast system/application storage from higher-capacity bulk storage gives the lab room to grow while keeping the initial hardware cost low.

## Troubleshooting and Lessons Learned

### Management interface initially unreachable

After the initial installation, the Proxmox web interface was not immediately reachable. Troubleshooting focused on physical Ethernet connectivity, the configured address, the Linux bridge, gateway settings, and the required HTTPS management port.

The process reinforced several networking fundamentals:

- A service can be running correctly while still being unreachable because of addressing or connectivity problems.
- Static IP, subnet mask, and default gateway values must agree with the LAN design.
- Proxmox management uses HTTPS on TCP port `8006`, not the normal HTTPS port `443`.
- The physical NIC is normally attached to `vmbr0`, allowing guests to participate in the LAN through the bridge.

## Result

The EliteDesk is now functioning as the primary Proxmox host for the home lab. It provides a central platform on which individual services can be deployed in isolated containers or virtual machines and documented as separate portfolio projects.

## Skills Demonstrated

- Bare-metal hypervisor installation
- Linux networking and bridges
- Static IPv4 configuration
- Web-based and CLI server administration
- Remote-access design
- Basic storage planning
- Hardware installation and validation
- Troubleshooting network reachability
- Infrastructure documentation

## Next Steps

The Proxmox host is the foundation for later projects including Pi-hole, self-hosted applications, Jellyfin, Immich, monitoring, backups, and increasingly advanced networking labs.
