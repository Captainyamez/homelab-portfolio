# Project #8: Self-Hosted Bitwarden Lite Password Manager

> **Status:** ✅ Running / Backed Up / Restore Tested  
> **Purpose:** Deploy a private self-hosted password manager with secure remote access, multi-device synchronization, automated backups, and a verified recovery path  
> **Focus:** Bitwarden Lite, Docker, SQLite, Debian LXC, Tailscale, HTTPS, 2FA, systemd, backup automation, restore testing

## Why I Chose This Project

After building several self-hosted services, I wanted to try a service where security, availability, and recovery mattered more than convenience alone.

A password manager is very different from a media server or dashboard. If a media service is unavailable, it is inconvenient. If a password manager is unavailable or its data is lost, it can affect access to many other services and accounts.

I chose **Bitwarden Lite** because it provides the official Bitwarden server in a lightweight single-container deployment suitable for a small homelab. I paired it with SQLite to keep the database layer simple and used Tailscale for private remote access rather than exposing the service directly to the public Internet.

> **Public portfolio note:** This documentation is intentionally sanitized. It does not publish real passwords, vault contents, installation credentials, recovery codes, authentication seeds, private IP addresses, private Tailscale hostnames/domains, account email addresses, device identifiers, or the specific authenticator application used for two-factor authentication.

---

## Architecture

```text
                    Remote client devices
                 ┌──────────┴──────────┐
                 │                     │
             Mobile app          Browser extension
                 │                     │
                 └──────────┬──────────┘
                            │
                      Private Tailscale
                         HTTPS access
                            │
                            ▼
                 ┌─────────────────────┐
                 │   Debian 13 LXC     │
                 │                     │
                 │      Docker         │
                 │        │            │
                 │        ▼            │
                 │  Bitwarden Lite     │
                 │        │            │
                 │        ▼            │
                 │      SQLite         │
                 └─────────┬───────────┘
                           │
                           │ automated backup
                           ▼
                 ┌─────────────────────┐
                 │ Separate bulk HDD   │
                 │ Dated backup sets   │
                 └─────────────────────┘
```

The password-manager service runs in its own unprivileged Debian LXC. Docker hosts the official Bitwarden Lite image, and the application data is persisted outside the container filesystem used by the Docker image itself.

The application listens only on the LXC loopback interface. Tailscale Serve provides the HTTPS access path for authenticated devices on the private tailnet.

---

## Container Design

I deployed the service in a dedicated unprivileged Debian 13 LXC rather than adding it to an existing application container.

The container uses:

- Debian 13
- Docker Engine and Docker Compose
- Bitwarden Lite official container image
- SQLite database
- Tailscale
- LXC nesting support for Docker
- TUN device passthrough for Tailscale
- automatic start with the Proxmox host

Separating the password manager from unrelated services reduces the number of dependencies that can affect it and makes backup, restore, and troubleshooting easier to reason about.

---

## DNS Troubleshooting During Deployment

The first package installation failed even though the new LXC had received an address and a default gateway.

The failure looked like:

```text
Temporary failure resolving 'deb.debian.org'
```

I tested connectivity separately from name resolution. Raw IP connectivity worked, and the container could reach the local DNS service, which showed that the network path itself was healthy.

The problem was that the container had inherited DNS resolvers that were not working correctly for the deployment. I changed the LXC DNS configuration to use the homelab's local DNS service and verified resolution before continuing.

This reinforced an important troubleshooting habit: **working IP connectivity does not prove DNS is working**.

---

## Docker and Bitwarden Lite

I installed Docker from Docker's Debian repository and verified that both the Docker daemon and Compose plugin were active before deploying Bitwarden.

The persistent layout is conceptually:

```text
/opt/bitwarden/
├── settings.env
├── docker-compose.yml
└── bwdata/
    ├── vault.db
    ├── attachments/
    ├── data-protection/
    └── application support files
```

The Compose deployment binds Bitwarden's HTTP service to loopback only:

```text
127.0.0.1:<local-port> -> Bitwarden Lite
```

This means the application is not directly exposed on the LAN through that port. Private HTTPS access is provided separately through Tailscale.

The real installation ID, installation key, private hostname, and other environment-specific values are deliberately omitted from this repository.

---

## SQLite

For this personal deployment I chose SQLite instead of running a separate PostgreSQL or MySQL database service.

That choice keeps the architecture small:

```text
Bitwarden Lite
      │
      ▼
   vault.db
```

The deployment has a single user and a small number of client devices, so a separate database server would add operational complexity without providing a meaningful benefit for the current scale.

---

## Private Remote Access

I did not forward Bitwarden through the home router or expose the login page directly to the public Internet.

Instead, Tailscale runs inside the LXC. Tailscale Serve proxies the Bitwarden loopback service over private HTTPS to authenticated devices on the tailnet.

Conceptually:

```text
Internet
   │
   X  no public Bitwarden port-forward

Authorized device
   │
   ▼
Tailscale
   │
   ▼
Private HTTPS endpoint
   │
   ▼
Bitwarden Lite on loopback
```

I verified access from both a mobile client and a browser extension on a Linux workstation.

---

## Multi-Device and Offline Testing

After creating the account, I connected multiple Bitwarden clients to the self-hosted environment.

I created a harmless test credential on the mobile client and confirmed that it synchronized to the browser extension on another device.

I then tested the opposite failure condition: the mobile device was moved away from the home network, Tailscale was disabled, and the Bitwarden server became unreachable.

The already-synchronized test credential remained accessible from the locally cached encrypted vault on the mobile device.

That test proved two different behaviors:

1. Live synchronization requires access to the self-hosted server.
2. Previously synchronized credentials remain available locally when the server cannot be reached.

No real account credentials are included in this repository.

---

## Account Hardening

Before migrating real credentials, I hardened the deployment.

I enabled two-factor authentication for the Bitwarden account using a standards-based one-time-password method. The specific authenticator application, seed, generated codes, and recovery code are not documented publicly.

I also disabled open user registration after the initial account was created. This prevents another user who can reach the private service from simply registering a new account.

The security choices for this deployment include:

- Strong master password
- Two-factor authentication
- Recovery code stored separately from the vault
- Open account registration disabled
- No public router port-forward
- Private HTTPS access through Tailscale
- Application bound to loopback behind the private access layer
- Dedicated LXC for the password manager

---

## Backup Design

A password manager is only useful if its data can be recovered.

I created a dedicated backup location on a separate physical bulk-storage drive attached to the Proxmox host. Only the Bitwarden backup directory is bind-mounted into the password-manager LXC rather than exposing the entire bulk-storage filesystem to the container.

Conceptually:

```text
Bitwarden LXC
     │
     │ dedicated bind mount
     ▼
Separate HDD
└── backups/
    └── bitwarden/
        ├── bitwarden-<timestamp>.tar.gz
        └── bitwarden-<timestamp>.tar.gz
```

The backup archive contains the persistent Bitwarden data plus the deployment configuration required for recovery. Secrets contained in those files are not committed to GitHub.

---

## Automated Backups

I wrote a small shell script that performs a consistent backup workflow:

```text
Stop Bitwarden container
        ↓
Archive persistent data + deployment configuration
        ↓
Write dated archive to separate HDD
        ↓
Start Bitwarden container
        ↓
Remove backups older than retention period
```

The script also uses shell exit handling so Bitwarden is started again if a later backup step fails after the application has been stopped.

A systemd oneshot service executes the script, and a systemd timer runs it daily. The timer is persistent so a missed run can execute after the LXC comes back online.

Current local retention is approximately 30 days.

Example public documentation intentionally excludes the real installation secrets and private addressing information.

---

## Restore Test

I did not consider the backup system complete just because archive files existed.

I created a temporary, isolated Debian LXC specifically for a restore test. The production password-manager container remained untouched.

The restore-test workflow was:

```text
Fresh Debian LXC
      │
      ├─ Install Docker
      │
      ├─ Mount backup directory read-only
      │
      ├─ Extract latest backup
      │
      ├─ Restore deployment files
      │
      ├─ Start Bitwarden Lite
      │
      └─ Verify restored SQLite database
```

The backup mount was deliberately read-only so the temporary restore environment could not modify or delete the production backup set.

The restored instance successfully:

- launched the Bitwarden Lite container
- returned an HTTP 200 response from its local web service
- started the identity, API, admin, icons, notifications, and nginx processes
- passed a SQLite database integrity check
- restored the expected persistent database and support files

After verification, I stopped and deleted the temporary restore-test LXC so an unnecessary additional copy of the real vault would not remain on the server.

This was the most important part of the backup project: **I proved the backup could actually be used to recover the service.**

---

## Verification

I verified the system at multiple layers.

### Container and application

- Debian LXC boots successfully.
- Docker service starts automatically.
- Bitwarden Lite container starts successfully.
- Local web endpoint returns HTTP 200.
- Persistent SQLite data survives container restarts.

### Private access

- Tailscale runs successfully inside the unprivileged LXC.
- Private HTTPS endpoint is reachable from authorized remote devices.
- No public router port-forward is required.

### Client synchronization

- Mobile client connects to the self-hosted instance.
- Browser extension connects to the same instance.
- Test entries synchronize between devices.
- Previously synchronized entries remain accessible offline on the mobile client.

### Security

- Two-factor authentication enabled.
- Recovery information stored separately.
- New-user registration disabled after account creation.
- Sensitive environment values are not published.

### Backup and recovery

- Manual backup succeeds.
- Automated daily backup succeeds.
- Backup is stored on a separate physical HDD.
- Retention cleanup is configured.
- Fresh restore environment successfully starts from a backup.
- SQLite integrity check passes after restoration.
- Temporary restore environment is destroyed after testing.

---

## What I Learned

This project gave me hands-on experience with:

- Running a security-sensitive self-hosted service
- Debian LXC deployment
- Docker and Docker Compose
- SQLite
- LXC nesting
- TUN device passthrough
- Tailscale inside an unprivileged container
- Tailscale Serve and private HTTPS
- Separating DNS failure from general network failure
- Multi-device password-manager synchronization
- Offline client behavior
- Two-factor authentication concepts
- Recovery-code planning
- Disabling unnecessary account registration
- LXC bind mounts and UID/GID mapping
- Limiting a container to a specific backup directory
- Shell backup automation
- systemd services and timers
- Backup retention
- SQLite integrity testing
- Performing a real disaster-recovery exercise
- Removing temporary sensitive environments after testing

The biggest lesson was that **backups are not finished when the file is created**. A backup becomes trustworthy only after I can restore it into a fresh environment and verify that the application and database actually work.

---

## Security and Privacy Notes

Because this is a public portfolio, several details are intentionally generalized or omitted.

Not published:

- Master password
- Vault contents
- User/account email address
- Bitwarden installation ID or key
- Private IP addresses
- Private Tailscale hostname/domain
- Tailscale device identity information
- Two-factor authentication seed
- Two-factor authentication codes
- Recovery code
- The specific authenticator application used
- Any real login credentials

The goal is to document the architecture, troubleshooting, and operational decisions without publishing information that could weaken the real deployment.

---

## Result

I now have a self-hosted Bitwarden Lite password manager running as a private homelab service.

The service synchronizes credentials across authorized devices, remains usable from already-synchronized clients during a home-server connectivity outage, requires two-factor authentication, blocks open account registration, and is reachable remotely without directly exposing Bitwarden to the public Internet.

The deployment also has automated daily backups stored on a separate physical disk and, more importantly, a restore procedure that has been successfully tested against a fresh temporary environment.

That makes this project less about simply deploying a password manager and more about **operating a security-sensitive service with an actual recovery plan**.

---

## Future Improvements

- Add an encrypted off-site backup copy
- Periodically repeat the restore test
- Add backup success/failure alerting
- Review Bitwarden Lite release notes before controlled upgrades
- Back up immediately before application upgrades
- Consider additional monitoring for service availability and backup age
