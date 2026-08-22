# Homelab Portfolio

This repository documents the ongoing build-out of my home lab and the practical infrastructure skills I am developing through hands-on projects.

The lab is built around a repurposed HP EliteDesk 800 G4 SFF running Proxmox VE. Rather than treating the server as a single-purpose machine, I am using it as a platform for learning virtualization, Linux administration, networking, self-hosting, storage, DNS, remote access, troubleshooting, and infrastructure documentation.

This portfolio is intentionally project-based. Each milestone is documented as a separate project with the objective, architecture, implementation process, troubleshooting, lessons learned, and future improvements.

> **Security note:** Public documentation is sanitized. Example private addresses may be substituted for real environment values, and passwords, API keys, tokens, public IP addresses, private keys, private remote-access domains, and other sensitive identifiers are not intentionally published.

## Current Homelab Platform

### Server

- **System:** HP EliteDesk 800 G4 SFF
- **CPU:** Intel Core i5-8500
- **Memory:** 32 GB RAM
- **Primary storage:** 1 TB NVMe SSD
- **Bulk storage:** 4 TB SATA HDD
- **Hypervisor:** Proxmox VE

### Supporting Devices

- Windows laptop
- Fedora Workstation ThinkPad
- ClockworkPi uConsole
- Android mobile devices for remote administration and testing

### Technologies in Use

- Proxmox VE
- Debian Linux
- Linux Containers (LXC)
- Pi-hole
- Tailscale
- Tailscale Serve
- SSH
- DNS
- IPv4 networking
- Linux bridges

## Projects

### [Project 1 — Proxmox Home Server Deployment](projects/01-proxmox-home-server/README.md)

Built the foundation of the home lab by installing Proxmox VE on repurposed enterprise hardware, configuring static host networking, enabling remote administration, and preparing the system for future virtualized services.

**Skills:** virtualization, Linux networking, static IPv4 configuration, storage planning, remote access, troubleshooting.

---

### [Project 2 — Pi-hole DNS Filtering](projects/02-pihole/README.md)

Deployed Pi-hole inside an unprivileged Debian LXC container and integrated it with the home network for DNS filtering. Troubleshot routing, DNS resolution, container networking, and router-side DNS behavior.

**Skills:** DNS, LXC, Debian administration, routing, `ping`, `dig`, live-log analysis, network troubleshooting.

---

### [Project 3 — Self-Hosted War Room Deployment](projects/03-warroom/README.md)

Deployed and administered a third-party War Room web application inside the Proxmox environment, provided private remote access through Tailscale, and used Tailscale Serve to place the local application behind HTTPS.

This project documents infrastructure deployment and administration; it does **not** claim authorship of the War Room software itself.

**Skills:** application deployment, private overlay networking, HTTPS proxying, browser security permissions, service troubleshooting, privacy-conscious administration.

## What I Am Building Toward

The current projects are the beginning of a larger home infrastructure environment. Planned and developing areas include:

- Jellyfin media server
- Immich self-hosted photo management
- Centralized backups
- Expanded storage management
- System and network monitoring
- VLANs and network segmentation
- Firewall policy and access control
- Additional DNS redundancy
- Virtual networking labs
- CCNA-focused networking practice
- Infrastructure diagrams and architecture documentation

## Why I Built This Portfolio

My goal is to demonstrate practical ability rather than only list technologies on a resume. The projects here show the complete process of learning and operating infrastructure: planning, deployment, mistakes, troubleshooting, validation, security decisions, and documentation.

The environment will continue to evolve, and this repository will be updated as new services and networking projects reach meaningful milestones.

## Skills Demonstrated Across the Lab

- Linux system administration
- Proxmox virtualization
- Linux containers
- IPv4 addressing and subnetting fundamentals
- DNS administration
- Linux network bridges
- Remote-access networking
- SSH administration
- Service deployment
- Web-service troubleshooting
- Storage planning
- Security-conscious infrastructure design
- Technical troubleshooting
- Technical documentation

---

**Status:** Active homelab — continuously expanding as new projects are completed.
