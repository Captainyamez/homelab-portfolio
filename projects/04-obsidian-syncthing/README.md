# Project #4: Self-Hosted Obsidian Vault Synchronization

> **Status:** ✅ Running  
> **Purpose:** Keep my learning notes synchronized between Linux and Android using my homelab  
> **Focus:** Syncthing, Proxmox/LXC, Linux services, multi-device synchronization, file versioning

## Why I Chose This Project

As I started studying networking and CCNA material more seriously, my Obsidian vault became something I wanted available on more than one device.

My notes originally lived on my Fedora ThinkPad. I wanted to be able to read or edit those same notes from my Android phone without maintaining separate copies or manually transferring files.

Instead of subscribing to another synchronization service, I decided to see whether I could use my own homelab as part of the solution.

The goal was simple to describe:

```text
Fedora ThinkPad
      ↕
   Syncthing
      ↕
 Home Server
      ↕
   Syncthing
      ↕
 Android Phone
      ↕
   Obsidian
```

Getting there gave me more hands-on practice with Linux services, file paths, permissions, device identities, synchronization, and the difference between synchronization and backup.

> **Public portfolio note:** Device IDs, private IP addresses, credentials, and other environment-specific identifiers are intentionally omitted or generalized.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Server role | Dedicated Linux environment for Syncthing |
| Server vault path | `/srv/obsidian` |
| Linux client | Fedora ThinkPad |
| Linux vault | `Documents/IT-Learning` |
| Mobile client | Android phone |
| Notes application | Obsidian |
| Synchronization | Syncthing |
| Sync mode | Send & Receive |
| Server versioning | Staggered File Versioning |

---

## What I Wanted to Accomplish

- Keep one Obsidian vault synchronized across my ThinkPad and phone.
- Use my always-on home server as a synchronization point.
- Avoid manually copying notes between devices.
- Keep the notes available locally so they still work offline.
- Add some protection against accidental replacement or deletion through Syncthing versioning.
- Understand what Syncthing was actually doing instead of treating it like a black box.

---

## Deployment

### 1. Preparing the server-side vault

I configured Syncthing on my Proxmox environment and created a dedicated location for the synchronized vault:

```text
/srv/obsidian
```

The Syncthing service owns the synchronized files rather than running everything as root.

Once synchronization began, I verified the directory directly from Linux. The server copy contained the same Obsidian configuration and learning directories that existed on the workstation, including my CCNA notes and lab/reference folders.

This was useful because it confirmed that I was not only seeing a green status indicator in a web interface—the files actually existed where I expected them to exist on the server.

### 2. Configuring file versioning

On the server copy I enabled **Staggered File Versioning**.

The versioning policy keeps versions more frequently when they are new and gradually reduces the frequency for older versions. I left the maximum age at one year.

I chose this because my notes are small compared with media files, and keeping older revisions gives me another recovery option if a synchronized change replaces or deletes something I wanted to keep.

This also taught me an important distinction:

> Syncthing keeps devices synchronized. It is not, by itself, the same thing as maintaining an independent backup.

I still plan to add a separate backup strategy as the homelab develops.

### 3. Installing Syncthing on Fedora

On the Fedora ThinkPad I installed Syncthing through the package manager and verified the installed version.

I then enabled the user service so Syncthing starts automatically for my account:

```bash
systemctl --user enable --now syncthing.service
```

I checked the service afterward with:

```bash
systemctl --user status syncthing.service
```

The service reported `active (running)` and exposed its local management interface on the loopback address.

That was another useful Linux lesson: Syncthing did not need to be launched manually every time I wanted my notes synchronized.

### 4. Finding the real Obsidian vault

Before sharing anything, I needed to make sure I was synchronizing the actual Obsidian vault rather than an empty directory with a similar name.

I used Linux to locate the `.obsidian` directory:

```bash
find ~ -maxdepth 3 -type d -name ".obsidian" -printf '%h\n'
```

That confirmed my vault was located under:

```text
/home/<user>/Documents/IT-Learning
```

This was a small step, but it prevented me from guessing at a path and synchronizing the wrong directory.

### 5. Pairing Fedora with the home server

Syncthing identifies peers using device IDs.

The server appeared through local discovery, but I still had to explicitly approve the relationship on both sides. I added the server to Fedora and then accepted Fedora on the server.

After that, I shared the **Obsidian Vault** folder with the Fedora device.

Once both devices reported **Up to Date**, I checked the server filesystem and confirmed that the vault contents had arrived successfully.

### 6. Adding Android

The next step was getting the same vault onto my phone.

I installed a Syncthing-compatible Android client and paired the phone with the home server using the same device-approval process.

I shared the Obsidian vault with the phone while leaving features such as automatic device introduction and automatic folder acceptance disabled. I wanted to explicitly approve what each device could access while I was still learning how the system worked.

For the Android copy I used a local directory under the phone's document storage and kept the folder configured as **Send & Receive**.

### 7. Connecting Obsidian to the synchronized Android folder

After Syncthing populated the Android directory, I installed Obsidian and chose **Use my existing vault** rather than creating a new one.

I selected the synchronized `IT-Learning` folder as the local vault.

Obsidian immediately displayed the same learning material that existed on Fedora, including my CCNA folders and notes.

---

## Verification

Seeing the files on the phone proved that the initial synchronization worked, but I wanted to verify that the design was actually bidirectional.

I performed a simple two-way test:

1. Created/changed a test note on one device.
2. Confirmed the change appeared on the other device.
3. Edited it from the second device.
4. Confirmed that change synchronized back.

The test succeeded in both directions.

At that point I considered the basic deployment working rather than merely installed.

---

## Current Design

```text
                 ┌─────────────────────┐
                 │   Fedora ThinkPad   │
                 │                     │
                 │ Obsidian IT-Learning│
                 └──────────┬──────────┘
                            │
                         Syncthing
                            │
                            ▼
                 ┌─────────────────────┐
                 │     Home Server     │
                 │      Proxmox VE     │
                 │                     │
                 │  Syncthing service  │
                 │   /srv/obsidian     │
                 │  staged versioning  │
                 └──────────┬──────────┘
                            │
                         Syncthing
                            │
                            ▼
                 ┌─────────────────────┐
                 │    Android Phone    │
                 │                     │
                 │ Local IT-Learning   │
                 │   Obsidian vault    │
                 └─────────────────────┘
```

Each client keeps a local copy of the vault. The server provides an always-on synchronization peer rather than requiring the ThinkPad and phone to be online at exactly the same time.

---

## What I Learned

This project gave me practical experience with:

- Running another useful service inside my Proxmox environment
- Linux file ownership and permissions
- Linux user-level systemd services
- Verifying services with `systemctl`
- Finding application data from the command line instead of guessing paths
- Syncthing device identities and peer approval
- Local device discovery
- Folder sharing and Send & Receive synchronization
- Android/Linux interoperability
- Obsidian's local-vault model
- File versioning
- Testing synchronization in both directions
- Separating synchronization from a true backup strategy

One thing I liked about this project is that it solved an actual problem for me. The homelab is no longer only something I experiment on—it is beginning to provide infrastructure that supports the way I study and work.

---

## Result

My `IT-Learning` Obsidian vault now stays synchronized between my Fedora ThinkPad and Android phone through Syncthing, with the home server acting as an always-available peer.

I can work on networking notes from the ThinkPad, leave the house, and still have the same notes available locally on my phone. Changes made from either device synchronize back to the other.

The server also keeps versioned copies of replaced/deleted synchronized files, giving me some recovery capability while I work toward a more complete backup design.

---

## Future Improvements

- Add an independent scheduled backup of the server-side vault
- Test and document restoration from Syncthing file versions
- Include the vault in the broader homelab backup strategy
- Monitor Syncthing service health and storage usage
- Evaluate remote synchronization behavior away from the home LAN
- Document recovery procedures for replacing a lost or failed client device
