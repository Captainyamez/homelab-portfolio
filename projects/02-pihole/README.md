# Project #2: Pi-hole DNS & DHCP Deployment on Proxmox

> **Status:** ✅ Running  
> **Environment:** Debian 13 LXC on Proxmox  
> **Focus:** DNS, DHCP/DNS interaction, container networking, troubleshooting

## Why I Chose This Project

After getting Proxmox running, I wanted the next project to be something that would actually affect my home network instead of just existing as a lab exercise.

Pi-hole seemed like a good fit because it gave me a reason to learn more about DNS, static IP addressing, containers, router settings, and troubleshooting while also providing a useful service for the devices in my house.

I did not start this project already understanding every part of DNS or DHCP. A large part of the project was learning what each layer was doing as problems came up.

> **Public portfolio note:** IP addresses and other environment-specific details shown here are sanitized documentation examples. Credentials, public IP addresses, private keys, and remote-access identifiers are not published.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Guest type | Unprivileged LXC |
| OS | Debian 13 |
| Application | Pi-hole |
| CPU | 1 vCPU |
| Memory | 512 MB RAM |
| Swap | 512 MB |
| Disk | 8 GB |
| Network | `vmbr0` bridge |

Sanitized addressing example:

```text
Proxmox host: 192.168.10.10
Pi-hole LXC:  192.168.10.54
Gateway:      192.168.10.1
Subnet:       192.168.10.0/24
```

---

## What I Wanted to Accomplish

- Create a lightweight Linux container for Pi-hole.
- Give it a predictable static IP address.
- Verify that the container could reach the internet.
- Install Pi-hole and confirm that its DNS service worked locally.
- Point devices on my home network toward Pi-hole through router-side network settings.
- Watch live DNS queries to confirm that devices were actually using it.
- Learn how DHCP-delivered network settings and DNS resolution relate to each other.

---

## Deployment

### 1. Creating the Debian LXC

I downloaded a Debian 13 LXC template through Proxmox and created a dedicated container for Pi-hole.

The container was configured to start automatically with the host and was given a static LAN address.

Representative sanitized configuration:

```text
Architecture: amd64
CPU cores:    1
Memory:       512 MB
Swap:         512 MB
Disk:         8 GB
Unprivileged: yes
On boot:      yes
Network:      vmbr0
IPv4:         192.168.10.54/24
Gateway:      192.168.10.1
```

### 2. Installing Pi-hole

Pi-hole was installed inside the Debian container and configured to answer DNS requests from the local network.

Upstream DNS resolvers were then configured so queries that were not blocked could continue to external resolvers.

### 3. Testing the network before blaming DNS

One of the most useful troubleshooting habits I picked up during this project was testing raw IP connectivity separately from DNS.

For example:

```bash
ping 1.1.1.1
```

If that failed, I knew the problem was not simply "DNS is broken." The container itself did not have working external connectivity yet.

### 4. Testing Pi-hole directly

Once network connectivity was working, I tested Pi-hole's DNS service directly through loopback:

```bash
dig @127.0.0.1 example.com
```

That let me verify Pi-hole itself independently from the router and other clients.

### 5. Watching live traffic

After adjusting the network/router settings, I used:

```bash
pihole tail
```

Seeing queries start to scroll by was the moment I knew devices were actually reaching the service rather than the dashboard merely being installed and available.

I also watched the Pi-hole dashboard query counts increase while normal devices such as my phone and streaming TV were sitting on the network.

---

## What Went Wrong

This project was not a clean install-and-done deployment.

### The container initially could not reach the internet

At one point, the Pi-hole container could not successfully ping an external IP address.

I had to work through:

- The container's IP address and prefix
- Default gateway
- Proxmox bridge configuration
- Neighbor/ARP behavior
- Whether the gateway was reachable
- Whether external IP connectivity worked

Eventually, direct connectivity to `1.1.1.1` worked successfully.

### DNS queries were going somewhere unexpected

During troubleshooting, I saw DNS behavior involving another resolver associated with my remote-access networking setup.

Rather than guessing, I tested Pi-hole directly against `127.0.0.1`. That helped separate **"Pi-hole is broken"** from **"the system is currently asking a different resolver."**

### Router DNS changes temporarily disrupted Wi-Fi

When I changed DNS settings on the router, my phone briefly showed **connected without internet**.

That was uncomfortable because the change affected the whole home network, but the connection recovered and Pi-hole queries began flowing afterward.

That experience taught me to make network-wide changes carefully and to validate them in layers instead of changing several things at once.

### Secondary DNS can bypass Pi-hole

I also learned that supplying a normal external resolver as a secondary DNS server does not necessarily mean clients will use Pi-hole first and the secondary only when Pi-hole fails.

Some clients may choose between them. That can result in DNS traffic bypassing Pi-hole entirely.

---

## How I Learned to Test It

The basic troubleshooting order that made the most sense to me became:

```text
1. Can the container reach the gateway?
2. Can it reach an external IP address?
3. Does Pi-hole answer a direct DNS query locally?
4. What DNS settings are clients receiving?
5. Are queries actually reaching Pi-hole?
```

That was probably more valuable than simply getting the final dashboard working.

---

## DNS & DHCP — What I Actually Learned

The project title includes DNS & DHCP because the project forced me to understand the relationship between them.

At this stage of the lab, my focus was on Pi-hole as the DNS service and on how the router/network distributes DNS information to clients. I am not trying to claim that one early project made me an expert in DHCP or that I redesigned an enterprise DHCP environment.

What I came away understanding much better is that DHCP can provide clients with information such as:

- Their IP configuration
- Default gateway
- DNS server addresses

That means a perfectly functioning Pi-hole server is not useful network-wide if clients are never told to use it.

---

## Current Result

Pi-hole is running inside its own Proxmox LXC and is receiving DNS queries from devices on the home network.

```text
Internet
   │
   ▼
Home Router
   │
   ├── Network clients
   │        │
   │        └──── DNS requests ────┐
   │                               │
   ▼                               ▼
Proxmox Host                  Pi-hole LXC
                                 │
                                 └── Upstream DNS
```

The most satisfying part of the project was watching the live query log begin to populate after troubleshooting the network path.

---

## What I Learned

- How to create and configure a Proxmox LXC.
- How static IPv4 settings affect a service that other devices depend on.
- The difference between internet connectivity and DNS resolution.
- How `ping` and `dig` can answer different troubleshooting questions.
- How DNS settings supplied to clients determine whether they actually use Pi-hole.
- Why a secondary resolver can unintentionally bypass filtering.
- How useful live logs are when verifying whether a service is actually receiving traffic.

I still have plenty to learn about DNS and DHCP, but this project moved both subjects from abstract networking terms into something I had configured and troubleshot myself.

---

## Future Improvements

- Local DNS records for homelab services
- More deliberate DHCP/DNS architecture
- A second filtering DNS server for redundancy
- VLAN segmentation
- DNS metrics and centralized monitoring
- Configuration backups
- Documented recovery procedures
