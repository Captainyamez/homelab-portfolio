# Project #5: Jellyfin Media Server Deployment on Proxmox

> **Status:** ✅ Running and remotely tested  
> **Purpose:** Build a private self-hosted media server using Proxmox, dedicated bulk storage, Intel hardware acceleration, and Tailscale  
> **Focus:** Jellyfin, Proxmox/LXC, storage bind mounts, Linux permissions, Intel Quick Sync/VA-API, remote access, troubleshooting

## Why I Chose This Project

After expanding my Proxmox host with a 4 TB SATA HDD, I wanted to start using that storage for an actual service rather than leaving it as empty capacity.

Jellyfin was a good next step because it brought several parts of the homelab together at once: virtualization, Linux storage, application deployment, media organization, hardware acceleration, and remote access.

The goal was to create a dedicated Jellyfin container that stores the application itself on the NVMe-backed Proxmox storage while reading the actual media library from the larger HDD.

I also wanted to be able to use Jellyfin from my phone both at home and away from home without forwarding its web port directly to the public internet.

> **Public portfolio note:** Real private IP addresses, credentials, Tailscale addresses, and other environment-specific identifiers are intentionally omitted or generalized.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Host system | HP EliteDesk 800 G4 SFF |
| CPU | Intel Core i5-8500 |
| Integrated graphics | Intel UHD Graphics 630 |
| Memory | 32 GB RAM |
| Primary storage | 1 TB NVMe SSD |
| Bulk storage | 4 TB SATA HDD, `ext4` |
| Guest | Dedicated Debian 13 LXC container (CT 103) |
| Jellyfin application storage | Container root filesystem on NVMe-backed Proxmox storage |
| Media storage | Bind-mounted from `/mnt/homelab-storage/jellyfin` |
| Remote access | Tailscale |
| Client tested | Android Jellyfin app |

---

## What I Wanted to Accomplish

- Deploy Jellyfin in its own LXC container.
- Keep the Jellyfin application and operating system separate from the media library.
- Use the 4 TB HDD for movies, TV, and music.
- Expose the Intel UHD 630 render device to the container for hardware transcoding.
- Confirm that Jellyfin could actually access the GPU rather than only configuring it in the web interface.
- Use Jellyfin on the home network from normal clients.
- Use Jellyfin away from home through Tailscale without public port forwarding.
- Document the problems I hit instead of presenting the deployment as if everything worked on the first attempt.

---

## Deployment

### 1. Creating the Jellyfin LXC

I created a dedicated Debian 13 LXC container for Jellyfin and gave it modest starting resources:

- 2 CPU cores
- 2 GB RAM
- 512 MB swap
- 16 GB root filesystem
- DHCP networking through the Proxmox Linux bridge
- Unprivileged container mode
- Automatic startup with the host

The 16 GB root filesystem is not used for the media library. It holds the container operating system, Jellyfin application, database, cache, and configuration.

This kept the design simple and separated application data from bulk media storage.

---

### 2. Mounting the 4 TB Media Storage

On the Proxmox host I prepared the media directory structure:

```text
/mnt/homelab-storage/jellyfin/
├── movies/
├── tv/
└── music/
```

I then bind-mounted the Jellyfin directory into CT 103 at:

```text
/media
```

Inside the container, `df -h /media` showed the large HDD rather than the small container root filesystem, confirming the media library was backed by the 4 TB drive.

The resulting structure inside Jellyfin is:

```text
/media/
├── movies/
├── tv/
└── music/
```

Because the LXC is unprivileged, the bind-mounted folders initially appeared as `nobody:nogroup` inside the container. That was a useful reminder that UID/GID mapping matters when host filesystems are exposed to unprivileged containers.

For the initial deployment, Jellyfin only needed read access to the media folders, so I did not give the service unnecessary write access to the original media files.

---

## Troubleshooting: Debian Repository Mismatch

My first Jellyfin installation attempt failed because I manually configured the Jellyfin repository for Debian 12 `bookworm` while the container itself was Debian 13 `trixie`.

APT reported unsatisfied dependencies including older library packages that were not available on Debian 13.

Examples included dependencies for:

- `libvpx7`
- `libx265-199`
- `libicu72`

Instead of trying to force old packages into the container, I removed the incorrect repository configuration and used Jellyfin's official Debian/Ubuntu installer script.

Before running it, I verified the downloaded script against its published SHA256 checksum.

The installer correctly detected:

```text
Real OS:            debian
Repository OS:      debian
Repository Release: trixie
CPU Architecture:   amd64
```

After using the correct repository, Jellyfin installed successfully and the service reported:

```text
Active: active (running)
```

This was one of the more useful parts of the project because the failure was not caused by Jellyfin itself. The repository configuration did not match the operating system release.

---

## Initial Jellyfin Configuration

From the Jellyfin setup wizard I created separate libraries for:

```text
Movies  -> /media/movies
Shows   -> /media/tv
Music   -> /media/music
```

I kept the initial metadata and artwork settings fairly conservative rather than enabling every optional processing feature immediately.

I also allowed remote connections in Jellyfin but did not configure direct public port forwarding on the home router.

---

## Intel Quick Sync / Hardware Acceleration

One of the main technical goals was to use the Intel UHD 630 integrated GPU in the i5-8500 for Jellyfin transcoding.

### 1. Verifying the GPU on the Proxmox host

I first confirmed that Proxmox could see the render devices:

```bash
ls -lah /dev/dri
```

The host exposed:

```text
card0
renderD128
```

I also verified the GPU and active driver with `lspci`.

The host identified the Intel Coffee Lake UHD 630 and showed the `i915` kernel driver in use.

### 2. Passing the render device into CT 103

I exposed `/dev/dri/renderD128` to the Jellyfin container through Proxmox device passthrough.

After rebooting the container, the render device existed inside CT 103.

The first pass exposed it as `root:root`, which meant the Jellyfin service account could see the device path but did not have the correct group access.

I adjusted the device configuration so the device appeared inside the container as:

```text
root:render
```

with mode `0660`.

The Jellyfin user already belonged to the container's `render` group, so I tested the permission directly rather than assuming it worked:

```bash
runuser -u jellyfin -- test -r /dev/dri/renderD128
```

The test succeeded.

### 3. Verifying VA-API inside the container

I used Jellyfin's bundled `vainfo` utility against the render device:

```bash
/usr/lib/jellyfin-ffmpeg/vainfo --display drm --device /dev/dri/renderD128
```

The important result was:

```text
va_openDriver() returns 0
Intel iHD driver for Intel(R) Gen Graphics
```

The output also reported hardware support for several useful video profiles, including:

- MPEG-2
- H.264
- VC-1
- HEVC
- HEVC Main 10
- VP8
- VP9
- VP9 Profile 2

That confirmed that the Intel media driver could initialize the GPU from inside the LXC.

### 4. Jellyfin transcoding settings

In Jellyfin I selected Intel Quick Sync as the hardware acceleration method and enabled the formats supported by this generation of Intel graphics.

I deliberately left unsupported or unnecessary options such as AV1 hardware acceleration disabled.

At the time of this documentation update, the GPU stack has been validated from inside the container. A full end-to-end transcode test will be documented separately once the library contains suitable media that requires transcoding.

---

## Remote Access with Tailscale

At home, Jellyfin is reachable over the normal private LAN.

For access away from home, I use Tailscale rather than forwarding Jellyfin's port directly through the router.

The tested path is conceptually:

```text
Android Phone
     │
     │ Cellular / external network
     ▼
  Tailscale
     │
     ▼
Home private network
     │
     ▼
Jellyfin LXC
```

I tested this from the Android Jellyfin app while away from the home LAN. With Tailscale connected, the app successfully reached the Jellyfin server and logged in normally.

That confirmed the remote access path worked without intentionally exposing Jellyfin directly to the public internet.

---

## Current Design

```text
                     HP EliteDesk 800 G4 SFF
                              │
                              ▼
                         Proxmox VE
                              │
                  ┌───────────┴───────────┐
                  │                       │
                  ▼                       ▼
            NVMe Storage             4 TB SATA HDD
                  │                       │
                  ▼                       │
            CT 103: Jellyfin              │
          Debian 13 + Jellyfin             │
          DB / cache / config              │
                  │                       │
                  └──── bind mount ───────┘
                              │
                              ▼
                           /media
                    ┌────────┼────────┐
                    │        │        │
                  movies     tv      music

Intel UHD 630
     │
     └── /dev/dri/renderD128 ──> CT 103 ──> Jellyfin Quick Sync

Remote Android client ──> Tailscale ──> Home network ──> Jellyfin
```

---

## Verification

I verified the deployment at several layers rather than relying on a single dashboard indicator.

### Container and storage

- CT 103 reports as running in Proxmox.
- The Jellyfin LXC receives normal network connectivity.
- `/media` inside the container maps to the 4 TB HDD.
- Movies, TV, and Music directories are visible from inside the LXC.

### Jellyfin service

- `jellyfin.service` reports `active (running)`.
- Jellyfin listens on its expected web interface port.
- The first-run wizard completed successfully.
- The Movies, Shows, and Music libraries were created.

### GPU access

- Proxmox detects the Intel UHD 630.
- The host uses the `i915` driver.
- `/dev/dri/renderD128` is present inside CT 103.
- The `jellyfin` account can read the render device.
- Jellyfin's bundled `vainfo` successfully initializes the Intel `iHD` driver.

### Remote access

- Jellyfin works from an Android phone.
- Remote access was tested through Tailscale away from the home LAN.
- No direct public Jellyfin port forwarding was required.

---

## What I Learned

This project gave me practical experience with:

- Creating and configuring another Proxmox LXC workload
- Planning separate application and bulk-storage locations
- Bind-mounting host storage into an LXC container
- Understanding how unprivileged containers affect ownership and permissions
- Diagnosing APT dependency failures caused by the wrong distribution repository
- Verifying a downloaded installer with a SHA256 checksum
- Managing a Linux systemd service
- Passing a Linux character device into an LXC container
- Linux device ownership, groups, and permission modes
- Intel `i915`, VA-API, and the `iHD` media driver
- Testing hardware access as the actual service account
- Configuring Jellyfin libraries and transcoding options
- Using Tailscale as a private remote-access path
- Testing an application from a real remote mobile client

The main lesson for me was that "the service is running" is only one part of a deployment. Storage, permissions, networking, hardware access, and client connectivity all had to be tested separately before I could be confident the system was actually usable.

---

## Result

Jellyfin is now running in a dedicated Debian 13 LXC on my Proxmox host.

The application and its configuration live on the faster NVMe-backed container storage, while the media library is provided from the 4 TB HDD through a bind mount.

The Intel UHD 630 render device is available inside the container and the Intel media driver has been successfully validated for hardware-accelerated video processing.

I can access Jellyfin locally from the home network and remotely from the Android Jellyfin app through Tailscale without intentionally exposing the service directly to the public internet.

The separate DVD-ripping/media-ingestion workflow is being documented as its own project rather than being folded into this Jellyfin deployment.

---

## Future Improvements

- Perform and document an end-to-end Quick Sync transcoding test with media that requires conversion
- Add additional client testing from TVs and other devices
- Review whether HEVC output encoding is useful for remote streaming
- Add backup coverage for Jellyfin configuration and database data
- Add monitoring for Jellyfin service health and resource usage
- Improve media naming and organization standards as the library grows
- Revisit permissions if Jellyfin later needs controlled write access to media directories
- Evaluate network segmentation and access controls as the homelab becomes more complex
