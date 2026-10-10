# Project #9: Self-Hosted Media Automation Stack

> **Status:** ✅ Working / End-to-end tested / Remotely accessible  
> **Purpose:** Build a self-hosted media-management workflow around Jellyfin while separating download traffic, application traffic, storage, and remote administration  
> **Focus:** Docker Compose, Gluetun, Proton VPN, qBittorrent, Prowlarr, Radarr, Sonarr, Bazarr+, Provider Hub, Seerr, bind mounts, Tailscale, backup discipline

## Why I Chose This Project

Jellyfin was running, and my DVD workflow was working too. Next I wanted to figure out how the media-management tools fit together instead of handling everything manually.

Knowing the names—Prowlarr, Radarr, Sonarr, qBittorrent, Bazarr, Seerr—was one thing. Making them communicate was another. I built a separate media-stack LXC and connected the applications piece by piece.

The networking and file paths took real attention. I wanted qBittorrent isolated behind Proton VPN through Gluetun while the other apps remained reachable normally. Then there were shared directories, import paths, subtitles, and remote access to test. When a request finally made it all the way through to a playable Jellyfin item, it made sense as a system instead of a bunch of containers. The stack is intended for media I'm authorized to use.

> **Public portfolio note:** This project documents the infrastructure and automation design. Credentials, VPN keys/configuration, private hostnames, API keys, private addresses, and any environment-specific secrets are not published. The workflow is intended for media I am authorized to obtain and manage.

---

## Architecture

```text
                         Remote admin
                             │
                         Tailscale
                             │
                             ▼
                    ┌─────────────────┐
                    │  Media-stack LXC │
                    │    Docker        │
                    │                  │
Request ───────────►│ Seerr            │
                    │   │              │
                    │   ├──► Radarr ───┼──┐
                    │   └──► Sonarr ───┼──┤
                    │                  │  │
                    │ Prowlarr ────────┼──┤
                    │ Bazarr+          │  │
                    │                  │  ▼
                    │ Gluetun ◄────────┴ qBittorrent
                    │   │
                    └───┼──────────────┘
                        │
                    Proton VPN
                        │
                        ▼
                     Internet

4 TB HDD
├── jellyfin/
│   ├── movies/
│   └── tv/
├── backups/
├── ripping/
└── shared/

Jellyfin LXC
    │
    └── reads final movie/TV library from the same HDD
```

The important design choice is that **qBittorrent is the component routed through the VPN gateway**. The rest of the automation applications stay on the normal container network so local APIs, Tailscale access, and service-to-service communication remain predictable.

---

## Container Design

The stack runs in a dedicated Debian LXC with Docker Compose.

Current services include:

- **Gluetun** — VPN gateway
- **qBittorrent** — download client
- **Prowlarr** — indexer management
- **Radarr** — movie library automation
- **Sonarr** — TV library automation
- **Bazarr+** — subtitle automation with Provider Hub support
- **Seerr** — request interface

The LXC has access to the bulk-storage filesystem through controlled bind mounts rather than storing large media files inside the LXC root disk.

---

## VPN Isolation

One of the most important parts of the project was separating VPN-routed traffic from the rest of the stack.

Conceptually:

```text
qBittorrent
    │
    ▼
Gluetun
    │
    ▼
VPN tunnel
    │
    ▼
Internet
```

qBittorrent uses Gluetun's network namespace, which means it does not simply "prefer" the VPN route. Its network path depends on the VPN gateway container.

This gave me a much clearer understanding of the difference between:

- putting an entire host behind a VPN
- putting an entire Docker stack behind a VPN
- routing only one application through a VPN gateway

For this environment, isolating only the download client kept the remaining services easier to administer.

---

## Port Forwarding

The VPN provider supports forwarded peer ports on compatible sessions.

During testing, Gluetun obtained a forwarded port and qBittorrent was configured to use the active value rather than leaving its incoming listening port unset.

This became a useful lesson in application networking because the public-facing peer port belongs to the VPN path, not to the normal home-router NAT path.

No inbound qBittorrent port is intentionally exposed through the home router.

---

## Application Flow

The working request path is:

```text
Seerr
  │
  ├── movie request ──► Radarr
  │
  └── TV request ─────► Sonarr
                         │
                         ▼
                     qBittorrent
                         │
                         ▼
                  completed download
                         │
              ┌──────────┴──────────┐
              ▼                     ▼
           Radarr                 Sonarr
              │                     │
              └──────────┬──────────┘
                         ▼
                  organized library
                         │
                         ▼
                      Jellyfin
```

Prowlarr supplies indexer configuration to the library managers, while Bazarr+ handles subtitle acquisition for supported library content.

I tested the full path with a movie request and confirmed that the media was downloaded, imported into the correct library, and played successfully through Jellyfin.

---

## Storage and Path Mapping

The media-stack LXC mounts the shared bulk-storage tree from the Proxmox host.

The important lesson was that multiple containers can refer to the same physical data using different internal paths. Radarr, Sonarr, qBittorrent, and Jellyfin all have to agree on where completed files exist from their own point of view.

I configured root folders for the final libraries and added remote-path mapping where required so the automation applications could correctly translate the download client's paths.

This was also where I ran into a real troubleshooting case: a movie associated with an existing TV/anime title was initially imported under the TV library path. Fixing it required understanding the difference between an application's root folder and the content type being managed, not just moving files blindly.

---

## Subtitle Automation

The stack originally used the standard LinuxServer Bazarr image. It worked, but the built-in provider selection left the subtitle workflow too dependent on a small number of sources and exposed practical provider download limits when adding full TV seasons.

I migrated the existing service to **Bazarr+** using the `ghcr.io/lavx/bazarr:latest` image while preserving the existing `/config` bind mount, media paths, Sonarr/Radarr integrations, and port mapping. Before the change, I created both the normal stack backup and an additional pre-migration copy of the Bazarr configuration.

The first Bazarr+ startup adopted the existing upstream Bazarr database and completed its schema migration successfully. Because that migration included a one-way database change, the pre-migration backup is the rollback point if I ever need to return to the original Bazarr image.

Bazarr+ added a **Provider Hub**, giving me a broader set of subtitle sources without manually patching the application. I also tuned subtitle behavior to reduce unnecessary provider usage by disabling automatic subtitle upgrades and increasing missing-subtitle search intervals.

After the migration I verified that:

- Bazarr+ started healthy on the existing port.
- Existing Sonarr and Radarr connections remained active.
- Existing subtitle history and library state were preserved.
- New external subtitles were detected by Jellyfin.
- External subtitles were successfully rendered through my Kodi/Jellyfin client after the client refreshed.

Earlier in the project, standard Bazarr also returned server errors during initial setup. Resetting only Bazarr rather than rebuilding the entire stack was a useful reminder to isolate the failing component before changing a working multi-service environment.

---

## Tailscale Remote Administration

The media-stack LXC also runs Tailscale.

I tested access while away from the home network and confirmed that the stack remained reachable privately without exposing the administration interfaces directly through the router.

This gives me remote access to management interfaces while keeping normal service traffic and VPN-routed download traffic as separate concerns.

---

## Backup Strategy

The live Docker Compose deployment is stored under:

```text
/opt/media-stack/
```

A copy is maintained on the bulk-storage backup path.

The refresh command I use is:

```bash
rsync -rlD --delete /opt/media-stack/ /data/backups/media-stack/
```

The `--delete` option makes the destination a mirror of the source, so files removed from the live configuration are also removed from the backup mirror.

The backup includes deployment configuration such as the Compose files and environment file. Because that environment file can contain sensitive values, its real contents are **not** published in this repository.

This is a configuration backup, not a substitute for backing up irreplaceable media or other critical data.

---

## Verification

I tested the stack at multiple layers:

### Networking
- Gluetun establishes the VPN tunnel.
- qBittorrent traffic uses the Gluetun network path.
- The other automation services remain reachable on the normal container network.
- Remote administration works through Tailscale.

### Applications
- Prowlarr can communicate with configured indexers.
- Radarr and Sonarr can communicate with qBittorrent.
- Root-folder tests pass.
- Seerr can send requests to Radarr/Sonarr.
- Bazarr+ can retrieve subtitles and use Provider Hub-installed sources.

### End-to-end workflow
- A test request reached the correct library manager.
- The download completed.
- The library manager imported the file.
- Jellyfin detected the final media.
- Playback completed successfully.
- Subtitle automation was verified separately, including playback of externally downloaded subtitles through Jellyfin/Kodi.

---

## What I Learned

This project gave me hands-on experience with:

- Docker Compose
- Multi-container application design
- Linux bind mounts
- Shared storage across LXCs
- Container networking
- VPN gateways
- Network namespaces
- Port forwarding through a VPN provider
- qBittorrent configuration
- Prowlarr/Radarr/Sonarr integration
- Request-management workflows
- Subtitle automation
- Application migration with persistent Docker bind-mounted configuration
- Provider Hub / provider diversification
- Remote-path mapping
- Tailscale remote administration
- `rsync` configuration backups
- Troubleshooting one service without unnecessarily rebuilding the entire stack

The biggest lesson was that a stack like this is really a collection of APIs, paths, and network boundaries. Once I understood which component owns each responsibility, troubleshooting became much easier.

---

## Result

I now have a working media-management stack that connects request handling, library management, Bazarr+ subtitle automation, download handling, shared storage, and Jellyfin.

The project also gave me practical experience separating traffic by security/privacy requirement instead of treating every service in a container as if it should share the same network path.

---

## Future Improvements

- Add health monitoring for the individual Docker services
- Add backup-age/status alerts
- Improve automated recovery after VPN port changes
- Document controlled update procedures
- Add storage-capacity alerts
- Continue refining quality profiles and library organization
