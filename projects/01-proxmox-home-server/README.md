# Project #1: Proxmox Home Server Deployment

> **Status:** ✅ Running  
> **Role:** First homelab / first Proxmox deployment  
> **Focus:** Virtualization, Linux networking, storage, troubleshooting

## Why I Built This

This was the starting point of my homelab journey.

I wanted a small home server that I could actually learn on instead of only watching videos or reading about servers and networking. I picked up a used **HP EliteDesk 800 G4 SFF** and decided to turn it into a Proxmox host that I could keep expanding over time.

At this point I was not coming into the project as a Proxmox expert. A lot of the value of this project was figuring out what the different pieces were doing, breaking things down when something did not work, and learning how the hardware, network, and hypervisor fit together.

> **Public portfolio note:** IP addresses and other environment-specific identifiers in this repository are sanitized or replaced with documentation examples. Passwords, tokens, public IP addresses, private keys, and private remote-access identifiers are not published.

---

## Hardware

| Component | Hardware |
|---|---|
| System | HP EliteDesk 800 G4 SFF |
| CPU | Intel Core i5-8500 |
| Memory | 32 GB RAM |
| Primary storage | 1 TB NVMe SSD |
| Bulk storage | 4 TB SATA HDD |
| Network | Gigabit Ethernet |

The EliteDesk appealed to me because it was inexpensive used enterprise hardware with enough RAM, storage expansion, and CPU capability to run several services without needing a large or power-hungry server.

---

## What I Wanted to Accomplish

- Install Proxmox VE on bare metal.
- Give the server a predictable management address on my home network.
- Learn how Proxmox bridges physical networking to containers and VMs.
- Access the server remotely without exposing the management page directly to the internet.
- Create a foundation for future projects such as Pi-hole, Jellyfin, Immich, backups, and networking labs.

---

## Build Log

### 1. Creating the installer

I downloaded the Proxmox installation image on my Windows laptop and wrote it to a USB flash drive.

The EliteDesk was then booted from the USB drive and Proxmox was installed to the NVMe SSD.

This was my first time turning a normal small-form-factor PC into a dedicated hypervisor rather than installing a typical desktop operating system.

### 2. Initial network configuration

The Proxmox host was configured with a static management address and the default Linux bridge, `vmbr0`.

Sanitized example:

```text
Interface: vmbr0
Address:   192.168.10.10/24
Gateway:   192.168.10.1
Bridge:    Physical Ethernet NIC
```

One of the things I had to understand here was that `vmbr0` is not just another physical Ethernet port. It acts as a Linux bridge so the Proxmox host and future guests can connect through the physical NIC.

### 3. Reaching the Proxmox dashboard

The web interface is served over HTTPS on port `8006`:

```text
https://192.168.10.10:8006
```

This sounds simple after the fact, but during setup the server initially appeared unreachable and I had to work through the network configuration rather than assume Proxmox itself had failed.

### 4. Updates and basic verification

After getting access to the host, I worked through repository/update configuration and rebooted the server to make sure the installation came back normally.

I also began using the Proxmox shell and Linux commands to look at CPU, memory, disks, and resource usage instead of relying only on the web dashboard.

### 5. Remote access with Tailscale

I installed Tailscale so I could reach the server securely while away from home.

I deliberately wanted to avoid opening the Proxmox management interface directly to the public internet. Tailscale gave me a practical introduction to private overlay networking while also making the lab more useful day-to-day.

### 6. Storage expansion

The server was initially brought online using the 1 TB NVMe SSD. A 4 TB SATA hard drive was planned as bulk storage for media, photos, and backups.

That gave me a simple separation between faster system/application storage and larger-capacity data storage.

---

## The Part That Did Not Work Immediately

### Proxmox was installed, but the web interface was unreachable

This was the first real troubleshooting moment of the project.

Instead of reinstalling everything, I worked through the problem layer by layer:

- Checked that Ethernet was physically connected.
- Verified the host address and subnet.
- Checked the default gateway.
- Confirmed the bridge configuration.
- Remembered that Proxmox uses TCP port `8006` for the management interface.

Once the addressing and connection details were correct, the dashboard became reachable.

### What that taught me

The biggest lesson was that **"the server is unreachable" does not automatically mean "the server is broken."**

It could be the physical link, the host IP, the subnet, the gateway, the bridge, the service port, or the client I am connecting from. That basic troubleshooting mindset has carried into the later projects.

---

## What I Learned

This project gave me my first hands-on experience with:

- Bare-metal virtualization
- Proxmox VE administration
- Static IPv4 addressing
- Linux bridges
- Basic Linux CLI administration
- Remote administration
- Storage planning
- Separating a service problem from a network problem
- Documenting infrastructure as I build it

I would not describe myself as an expert in all of those areas from one project. The value was getting past the point where the terminology was purely theoretical and seeing how the pieces behave on a real system.

---

## Current Result

The EliteDesk is now the foundation of the homelab and is running Proxmox successfully.

```text
Home Network
     │
     ▼
HP EliteDesk 800 G4 SFF
     │
     ├── Proxmox VE
     │     ├── LXC containers
     │     └── Future VMs
     │
     ├── 1 TB NVMe
     └── 4 TB bulk storage
```

The important result for me is not just that Proxmox boots. I now have a platform where every new service can become another hands-on project instead of starting from scratch each time.

---

## Next Steps

The next milestones built on this host include:

- Pi-hole
- Secure self-hosted applications
- Jellyfin
- Immich
- Backups
- Monitoring
- VLANs and network segmentation
- CCNA-focused networking labs
