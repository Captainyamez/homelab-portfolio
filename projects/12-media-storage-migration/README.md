# Project #12: Dedicated 8 TB Media Storage Migration on Proxmox

> **Status:** ✅ Completed / Verified after host reboot  
> **Scope:** Install a dedicated media HDD, migrate an active multi-container Jellyfin environment, preserve application paths and validate service recovery  
> **Skills:** SMART, GPT, ext4, UUID mounts, rsync, Linux permissions/ACLs, Proxmox LXC bind mounts, systemd, Docker, VPN troubleshooting, validation

## Goal

I wanted to separate my Jellyfin movies, TV shows, downloads and DVD ripping workspace from the disk I plan to use for personal documents, notes, photographs and backups. I installed an 8 TB WD Red CMR HDD alongside my existing 4 TB Toshiba and 1 TB NVMe system disk, then migrated the live media data without rebuilding Jellyfin or the surrounding applications.

I performed the software migration remotely over private Tailscale SSH. This project documents **my infrastructure work**, not the development of Jellyfin or the other third-party applications.

## Architecture

```text
Proxmox VE — HP EliteDesk 800 G4 SFF
├── 1 TB NVMe — OS, LXC root disks, application configurations
├── 4 TB Toshiba — /mnt/homelab-storage
│   ├── backups/ (including service configuration backups)
│   ├── personal/  documents/  photos/  notes/
│   └── old media directories temporarily retained as rollback copies
└── 8 TB WD Red — /mnt/homelab-media
    ├── jellyfin/
    │   ├── movies/
    │   ├── tv/
    │   └── music/
    ├── downloads/
    │   ├── complete/
    │   └── incomplete/
    └── ripping/

CT 103 Jellyfin:   /mnt/homelab-media/jellyfin → /media
CT 104 DVD Ripper: /mnt/homelab-media/ripping → /work
                   /mnt/homelab-media/jellyfin → /media
CT 106 Media Stack:/mnt/homelab-media → /data
```

The LXC-facing paths did not change. The host-side paths did, avoiding the need to rebuild Jellyfin's library or reconfigure every Radarr/Sonarr path.

## 1. Hardware inspection and health checks

After safely shutting down Proxmox, I installed the 8 TB SATA drive and verified it was detected using:

```bash
lsblk -o NAME,SIZE,MODEL,FSTYPE,MOUNTPOINTS
smartctl -a /dev/sdX
smartctl -t short /dev/sdX
smartctl -l selftest /dev/sdX
```

`/dev/sdX` here is a **placeholder**. The actual device was positively identified by model and capacity before writing to it; device letters can change between boots.

The WD Red reported SMART overall health **PASSED**, no reallocated or pending sectors, and a short self-test **Completed without error**. The absence of logged errors is encouraging but not a substitute for a full-drive diagnostic or a backup.

## 2. GPT, ext4 and persistent mounting

I verified that the new disk had no existing filesystem signatures using `wipefs -n`, created a GPT partition spanning the disk with `sgdisk`, and formatted it:

```bash
# Demonstration only — destructive operations, replace device carefully
sgdisk --zap-all /dev/sdX
sgdisk -n 1:0:0 -t 1:8300 -c 1:"Jellyfin-Media" /dev/sdX
partprobe /dev/sdX
mkfs.ext4 -L homelab-media -m 0 /dev/sdX1
```

The `-m 0` setting avoids reserving 5% of a large, dedicated media filesystem for root. Linux shows the nominal 8 TB drive as approximately 7.3 TiB.

I created `/mnt/homelab-media` and configured the filesystem in `/etc/fstab` by UUID, rather than relying on a potentially changing `/dev/sdX` name:

```fstab
UUID=<MEDIA-FILESYSTEM-UUID> /mnt/homelab-media ext4 defaults,nofail 0 2
```

I checked the mount with `findmnt`, `df -hT` and `findmnt --verify --verbose`. The latter returned **Success, no errors or warnings detected**. The UUID above is intentionally redacted.

## 3. Migrate the data with rollback protection

At the time of migration, the three media trees occupied approximately **450 GB** in total (roughly 437 GB Jellyfin, 13 GB ripping workspace and a small downloads directory, after downloads changed during copying).

First I prepared directories on the new disk and used `rsync` to copy the old media while keeping the source intact:

```bash
rsync -aHAX --numeric-ids --info=progress2 \
  /mnt/homelab-storage/jellyfin \
  /mnt/homelab-storage/downloads \
  /mnt/homelab-storage/ripping \
  /mnt/homelab-media/
```

I used `tmux` to manage the long-running job over SSH. The archive, hard-link, ACL, extended-attribute, and numeric-ID options mattered because the media stack runs in an unprivileged LXC.

After the initial transfer, I stopped CT 103, 104 and 106, repeated the synchronization to capture changes made during the first pass, and confirmed a successful `rsync` exit status of `0`. Directory sizes matched. I also performed a fast metadata/size/timestamp dry-run comparison using `rsync -aHAXni --numeric-ids --itemize-changes`, with no reported differences. I began a full checksum dry-run but canceled it due to the time required: **a completed byte-for-byte checksum verification is not claimed**.

The original data was retained on the Toshiba as a short-term rollback copy.

## 4. Reconfigure Proxmox bind mounts

Before changing mount paths I backed up the three LXC configuration files. With the media containers stopped, I redirected their host paths using `pct set`:

```bash
pct set 103 -mp0 /mnt/homelab-media/jellyfin,mp=/media
pct set 104 -mp0 /mnt/homelab-media/ripping,mp=/work
pct set 104 -mp1 /mnt/homelab-media/jellyfin,mp=/media
pct set 106 -mp0 /mnt/homelab-media,mp=/data
```

I compared the source and destination numeric ownership and permissions. The media stack's ACLs and shifted UID mapping remained visible after migration. Container-level listings showed `nobody:nogroup` on bind-mounted directories, consistent with the unprivileged container model.

I added per-container systemd service drop-ins for CT 103, 104 and 106:

```ini
[Unit]
RequiresMountsFor=/mnt/homelab-media
```

The dependencies were visible using `systemctl show`. This is a startup dependency, not a substitute for testing all Proxmox startup paths or monitoring drive failure. CT 103 and CT 106 were configured to autostart; the DVD-ripper CT 104 remains manually started.

## 5. Application and VPN validation

Jellyfin, the media-stack LXC, and eight Docker containers restarted successfully. I verified:

- Movie and TV library directories were visible from the new bind mounts.
- Jellyfin playback worked both before and **after a full Proxmox host reboot**.
- Radarr recognized its library and Sonarr recognized existing episode files.
- Gluetun and Bazarr reported healthy states; the other media containers were running.
- qBittorrent's observed public address matched Gluetun's VPN exit address.
- qBittorrent's Docker network mode pointed at Gluetun's container namespace.

A post-migration check found qBittorrent's listening port did not match Gluetun's forwarded port. Restarting the dedicated `port-sync` helper synchronized the values. After the subsequent full host reboot, the forwarded and listening ports **matched automatically**, showing that the synchronization recovered without manual intervention in that test.

A reboot also verified that both drives mounted, CT 103 and CT 106 started, and CT 104 remained stopped by design. This confirms the successful **normal-boot case**; I have not simulated a missing-drive failure remotely.

## 6. Post-migration observation and Toshiba cleanup — November 2026

After more than two weeks of normal operation, Jellyfin continued to play media successfully, new DVD rips worked, and additional media was imported through the automation workflow. I then retired the old media directories on the 4 TB Toshiba.

Before deleting anything I verified:

- Both drives were mounted as ext4 with the expected device sources.
- LXC 103 (Jellyfin), 104 (DVD Ripper) and 106 (Media Stack) referenced the **8 TB media drive**; the 4 TB Toshiba remained referenced for Bitwarden and media-stack backup mounts.
- `rsync -rni --size-only` comparisons exposed an obsolete older movie rip and historical download files. I checked these discrepancies rather than treating the comparison as clean. The old movie rip had already been replaced by a better version.
- The old ripping tree matched its new counterpart in the size-based dry-run, and the old optical-disc recovery/temporary processing files were no longer needed.
- `findmnt -R` showed no nested mounts under the old Toshiba media directories. `fuser -vm` reported no userspace processes accessing them in that check.
- Docker inspection confirmed qBittorrent's `/downloads` maps to `/data/downloads` in CT 106, which is on the 8 TB disk. An inactive item was reviewed and removed from the client before cleanup.

I deleted only the old Toshiba `jellyfin/`, `ripping/`, and finally `downloads/` directories, after confirming the files not present at identical paths were obsolete or otherwise no longer needed. The 8 TB live-media files were not targeted.

### Measured results

| Check | Before cleanup | After cleanup |
|---|---:|---:|
| Toshiba used (`df -hT`) | 450G | 541M |
| Toshiba capacity | 3.6T | 3.6T |
| WD Red used | 524G | 524G |
| WD Red available | 6.8T | 6.8T |

The Toshiba retained `backups/` (about 544 MB measured during inspection), `documents/`, `immich/`, `notes/`, `personal/`, `photos/`, `shared/`, and `lost+found/`. Its old media and download trees no longer appear in the final directory listing. Values are reported as printed by `df`/`du`, which apply rounded human-readable units.

**Limitations:** The comparisons were based on filenames and file sizes, not a completed end-to-end checksum audit of every media file. This cleanup permanently retired the initial rollback copies; ongoing backups are selective rather than a full replica of the media disk.

## Backup strategy and follow-ups

The 8 TB disk is dedicated primarily to Jellyfin and is **not fully mirrored**. My plan is to back up important or difficult-to-replace rips selectively when I build a separate backup node with a planned 12 TB HDD. Personal photographs, documents, notes and sensitive service data deserve a higher backup priority.

The previous media directories were retained on the Toshiba as a temporary rollback copy during the initial cutover. **On November 5, 2026, after over two weeks of successful operation, I completed the cleanup** as documented below.

Additional future checks: monitor disk health and mount availability, verify selective backups by restoring them, and avoid treating a second disk in the same building as an off-site backup.

## What I learned

The key lesson was that a storage migration is more than copying files. The filesystem, persistent mounts, LXC user mapping, Docker paths, VPN-dependent services and restart behavior all have to work together. Keeping source data intact, verifying a second sync with writers stopped, and testing playback after a real reboot made the cutover controlled and recoverable.

> **Security note:** Published examples omit actual serial numbers, UUIDs, network IPs, VPN exit IPs, private tailnet identifiers, authentication secrets and private application configuration.
