# Project #4: Self-Hosted Obsidian Vault Synchronization

> **Status:** ✅ Running and remotely tested  
> **Purpose:** Keep my learning notes synchronized between Linux and Android using my homelab  
> **Focus:** Syncthing, Proxmox/LXC, Tailscale, Linux services, multi-device synchronization, file versioning

## Why I Chose This Project

As I started studying networking and CCNA material more seriously, my Obsidian vault became something I wanted available on more than one device.

My notes originally lived on my Fedora ThinkPad. I wanted to read or edit those same notes from my Android phone, and eventually from additional devices, without maintaining separate copies or manually transferring files.

Instead of subscribing to another synchronization service, I decided to see whether I could use my own homelab as part of the solution. I also wanted the setup to continue working when my phone or ThinkPad was away from my home network.

The final goal became:

```text
Fedora ThinkPad ─┐
                 │
                 ├── Syncthing over local network or Tailscale ── Home Server
                 │                                                   │
Android Phone ───┘                                             /srv/obsidian
```

Getting there gave me hands-on practice with Linux services, file paths, permissions, device identities, synchronization, TUN device passthrough, private overlay networking, and the difference between synchronization and backup.

> **Public portfolio note:** Device IDs, private/Tailscale IP addresses, credentials, tailnet information, and other environment-specific identifiers are intentionally omitted or generalized.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Guest | Dedicated Linux LXC container (CT 102) |
| Server role | Always-on Syncthing peer |
| Server vault path | `/srv/obsidian` |
| Linux client | Fedora ThinkPad |
| Linux vault | `Documents/IT-Learning` |
| Mobile client | Android phone |
| Notes application | Obsidian |
| Synchronization | Syncthing |
| Remote private connectivity | Tailscale |
| Sync mode | Send & Receive |
| Server versioning | Staggered File Versioning |

---

## What I Wanted to Accomplish

- Keep one Obsidian vault synchronized across my ThinkPad and phone.
- Use my always-on home server as a synchronization point.
- Keep notes available locally so they still work offline.
- Synchronize while away from my home LAN without publicly exposing the service.
- Avoid manually copying notes between devices.
- Add some protection against accidental replacement or deletion through Syncthing versioning.
- Understand what Syncthing and Tailscale were actually doing instead of treating them like black boxes.

---

## Deployment

### 1. Building the Syncthing container

I created a dedicated LXC container in Proxmox for Syncthing and initially allocated a single CPU core because the workload is lightweight.

After confirming network connectivity and DNS resolution from inside the container, I installed Syncthing and configured it to run as a dedicated `syncthing` user through systemd.

I verified the service with:

```bash
systemctl status syncthing@syncthing.service
```

The service reported `active (running)` and was configured to start automatically.

### 2. Preparing the server-side vault

I created a dedicated location for the synchronized vault:

```text
/srv/obsidian
```

The Syncthing service owns the synchronized files rather than running the synchronization workload as root.

Once synchronization began, I verified the directory directly from Linux. The server copy contained the Obsidian configuration and learning directories that existed on the workstation, including my CCNA notes and lab/reference folders.

This confirmed that I was not only seeing a green status indicator in a web interface—the files actually existed where I expected them to exist on the server.

### 3. Configuring file versioning

On the server copy I enabled **Staggered File Versioning**.

The versioning policy keeps versions more frequently when they are new and gradually reduces the frequency for older versions. I left the maximum age at one year.

I chose this because my notes are small compared with media files, and keeping older revisions gives me another recovery option if a synchronized change replaces or deletes something I wanted to keep.

This also taught me an important distinction:

> Syncthing keeps devices synchronized. It is not, by itself, the same thing as maintaining an independent backup.

I still plan to add a separate backup strategy as the homelab develops.

### 4. Installing Syncthing on Fedora

On the Fedora ThinkPad I installed Syncthing through the package manager and verified the installed version.

I enabled the user service so Syncthing starts automatically for my account:

```bash
systemctl --user enable --now syncthing.service
```

I checked it afterward with:

```bash
systemctl --user status syncthing.service
```

The service reported `active (running)` and exposed its management interface only on the local loopback address.

### 5. Finding the real Obsidian vault

Before sharing anything, I wanted to make sure I was synchronizing the actual Obsidian vault rather than an empty directory with a similar name.

I used Linux to locate the `.obsidian` directory:

```bash
find ~ -maxdepth 3 -type d -name ".obsidian" -printf '%h\n'
```

That confirmed my vault was located under:

```text
/home/<user>/Documents/IT-Learning
```

This small verification prevented me from guessing at a path and synchronizing the wrong directory.

### 6. Pairing Fedora with the home server

Syncthing identifies peers using device IDs.

The server and Fedora device were explicitly approved on each side, and I shared the **Obsidian Vault** folder between them.

Once both devices reported **Up to Date**, I checked `/srv/obsidian` from the server console and confirmed that the vault contents had physically arrived.

### 7. Adding Android and Obsidian

I installed a Syncthing-compatible Android client and paired the phone with the home server using the same explicit device-approval process.

I left automatic device introduction and automatic folder acceptance disabled because I wanted to deliberately control which devices and folders were trusted.

The Android folder was configured as **Send & Receive**. After Syncthing populated it, I installed Obsidian and chose **Use my existing vault** rather than creating a new one.

I selected the synchronized `IT-Learning` folder as the local vault. Obsidian immediately displayed the same CCNA notes and learning folders that existed on Fedora.

---

## Adding Private Remote Synchronization with Tailscale

Local synchronization solved only part of the problem. I also wanted to take notes away from home and have them synchronize back to my server without opening Syncthing directly to the public internet.

### 1. Giving the LXC container access to TUN

Before Tailscale could run inside CT 102, I checked for the Linux TUN device:

```bash
ls -l /dev/net/tun
```

Initially it returned:

```text
No such file or directory
```

I added `/dev/net/tun` to the container through Proxmox device passthrough and restarted the container.

Running the command again showed the expected character device, confirming the container could now support Tailscale.

### 2. Installing Tailscale on the server

The container did not initially have `curl`, so the first installation attempt failed. I installed the missing package and then installed Tailscale.

After authenticating the container to my tailnet, I verified it with:

```bash
tailscale status
tailscale ip -4
```

The Syncthing container could see the other authorized devices on the private Tailscale network.

### 3. Pointing Syncthing at the private Tailscale path

Instead of relying only on Syncthing's dynamic discovery for the server connection, I configured the clients to use the Syncthing container's Tailscale address on TCP port `22000`.

The actual private address is intentionally omitted from this public repository.

Conceptually, the client address is:

```text
tcp://<syncthing-tailscale-ip>:22000
```

This gave the clients a direct private path to the always-on server while away from the home LAN.

### 4. Adding Tailscale to Fedora

I installed Tailscale on the Fedora ThinkPad and authenticated it to the same tailnet.

I confirmed that Fedora could see the Syncthing container with `tailscale status`, then configured Fedora's Syncthing peer entry to use the server's Tailscale address.

This meant the ThinkPad could continue reaching the synchronization server even when it was no longer connected to my home Wi-Fi.

---

## Troubleshooting: Remote Android Sync Showed Disconnected

My first Android remote test failed.

I turned off Wi-Fi, left mobile data enabled, connected Tailscale, and expected Syncthing to reconnect to the server. Instead, the Syncthing client showed the Home Server as **Disconnected**.

Rather than immediately changing the server configuration, I tested the layers separately.

Tailscale showed that the phone could successfully ping the Syncthing container and reported a **direct connection**. That told me the private network path itself was working.

The problem turned out to be much simpler: **cellular-data synchronization was disabled in the Android Syncthing client's run conditions.**

After allowing Syncthing to operate over cellular data, the server connection came up and synchronization worked normally through Tailscale.

That was a useful troubleshooting lesson because the visible symptom was "server disconnected," but the server and network tunnel were both healthy. The restriction was on the client application.

---

## Verification

I did not consider the project complete after seeing the files appear once. I tested each important path.

### Local bidirectional test

I created/changed a test note on one device, confirmed it appeared on the other, edited it from the second device, and confirmed the change synchronized back.

### Server filesystem verification

I inspected `/srv/obsidian` directly from the container console and confirmed the expected Obsidian configuration and learning folders existed on disk.

### Android remote test

I disabled Wi-Fi on the phone, used cellular data with Tailscale connected, and confirmed that Syncthing could reach CT 102 and synchronize changes.

### Fedora remote test

I installed Tailscale on the ThinkPad, moved it away from the normal home-LAN path, and confirmed that Obsidian changes continued synchronizing through the server over Tailscale.

These tests confirmed that the system worked both locally and remotely rather than merely looking correct in the dashboards.

---

## Current Design

```text
                     Away from Home

       Android Phone                 Fedora ThinkPad
       Obsidian Vault                Obsidian Vault
             │                             │
        Syncthing                     Syncthing
             │                             │
             └──────────┐       ┌──────────┘
                        │       │
                       Tailscale
                           │
                           ▼
                 ┌─────────────────────┐
                 │     Home Server     │
                 │      Proxmox VE     │
                 │                     │
                 │ CT 102: Syncthing   │
                 │   /srv/obsidian     │
                 │ staggered versions  │
                 └─────────────────────┘

At home, the same clients can synchronize over available private connectivity.
```

Each client keeps a local copy of the vault. Obsidian therefore remains useful offline, while the server acts as an always-on synchronization peer so the ThinkPad and phone do not have to be online at exactly the same time.

Tailscale provides the private remote path without intentionally exposing Syncthing's listening port through the home router.

---

## What I Learned

This project gave me practical experience with:

- Creating and operating another LXC workload in Proxmox
- Linux file ownership and permissions
- Linux systemd services and user-level services
- Verifying services with `systemctl`
- Finding application data from the command line instead of guessing paths
- Syncthing device identities and explicit peer approval
- Folder sharing and Send & Receive synchronization
- Android/Linux interoperability
- Obsidian's local-vault model
- File versioning
- TUN device passthrough into an LXC container
- Installing and validating Tailscale on Debian and Fedora
- Private overlay networking between devices on different physical networks
- Testing network layers independently during troubleshooting
- Android application run conditions and cellular-data restrictions
- Testing synchronization in both directions
- Separating synchronization from a true backup strategy

One thing I liked about this project is that it solved an actual problem for me. The homelab is no longer only something I experiment on—it is beginning to provide infrastructure that supports the way I study and work.

---

## Result

My `IT-Learning` Obsidian vault now stays synchronized between my Fedora ThinkPad, Android phone, and an always-on Syncthing service in my Proxmox homelab.

At home I can work normally on either device. Away from home, Tailscale provides a private route back to CT 102 so both the Android phone and Fedora ThinkPad can continue synchronizing without intentionally exposing Syncthing to the public internet.

I verified local two-way synchronization, the server-side files, Android synchronization over cellular/Tailscale, and Fedora synchronization away from the home LAN.

The server also keeps versioned copies of replaced/deleted synchronized files, giving me some recovery capability while I work toward a more complete independent backup design.

---

## Future Improvements

- Add an independent scheduled backup of `/srv/obsidian`
- Test and document restoration from Syncthing file versions
- Include the vault in the broader homelab backup strategy
- Monitor Syncthing and Tailscale service health
- Document recovery procedures for replacing a lost or failed client device
- Add additional authorized devices as needed
- Revisit network segmentation and access controls as the homelab grows
