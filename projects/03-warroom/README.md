# Project #3: Secure Self-Hosted Application Deployment

> **Status:** ✅ Running  
> **Application:** Third-party War Room web application  
> **Focus:** Self-hosting, Tailscale, HTTPS, browser permissions, privacy

## Why I Chose This Project

For this project, I wanted to go beyond installing a network utility and host a real web application that I could actually use.

The application was **War Room**, a third-party project related to one of my hobbies. I did **not** develop War Room itself. My part of the project was deploying it on my homelab, making it reachable remotely, improving how I accessed it, and thinking through the security and privacy implications before using it.

This was still early in my homelab journey, so I was learning as I went—especially around HTTP vs HTTPS, reverse proxying, private remote access, and browser site permissions.

> **Public portfolio note:** Private hostnames, Tailscale domains, credentials, tokens, public IP addresses, and other sensitive identifiers are omitted or generalized.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Guest | Linux environment hosting War Room |
| Application | Third-party War Room web app |
| Remote access | Tailscale |
| HTTPS access | Tailscale Serve |
| Client testing | Browser on remote devices |

---

## What I Wanted to Accomplish

- Host a third-party web application on my own server.
- Reach it while away from home.
- Avoid exposing the application directly to the public internet.
- Replace the original insecure browser access path with HTTPS.
- Understand what browser permissions the site was requesting.
- Keep application deployment separate from claims of software development.

---

## Deployment

### 1. Getting the application running

War Room was installed on a Linux environment inside the Proxmox homelab and configured to listen on a local web port.

At first, I accessed it using the basic web address provided by the service.

The application loaded, but the browser showed the warning/indicator associated with an insecure or untrusted connection.

That immediately gave me another problem to solve instead of simply accepting that the page worked.

### 2. Remote access with Tailscale

Because Tailscale was already part of my homelab, I used it to reach the War Room host remotely.

I preferred this over forwarding the application port through my home router because I did not want a hobby web application sitting openly on the public internet just so I could use it from my phone.

### 3. Using Tailscale Serve

I then configured Tailscale Serve to proxy the application's local HTTP listener:

```bash
tailscale serve --bg http://127.0.0.1:8000
```

Tailscale returned a private HTTPS address that was only available within my tailnet.

I intentionally do not publish that hostname in this repository.

When I opened War Room through the new address, the browser no longer showed the same insecure connection warning. That was a very visible way for me to see the difference between directly browsing to a raw HTTP service and placing it behind an HTTPS access layer.

---

## Security and Privacy Questions I Stopped to Ask

This project made me slow down and think about things that I probably would have clicked through in the past.

### Where is my password going?

Before creating the War Room password, I stopped to consider whether the password would be stored locally by the application, sent elsewhere, or handled in some other way.

I did not want to assume that "self-hosted" automatically meant every part of an application was private or secure.

No application credentials are included in this portfolio.

### Why does the site want my location?

While looking at War Room's map functionality, the browser asked for location access.

Initially, I could not click the permission options in the pop-up the way I expected.

I opened the browser's site controls and discovered that I could manage the permission directly from the site's settings.

That helped me understand that there were several separate layers involved:

```text
Server hosting the application
          │
          ▼
Web application
          │
          ▼
Browser
          │
          ▼
Device location permission
```

The fact that I host the server does not automatically mean the application gets unrestricted access to everything on the client device.

---

## What Did Not Work Smoothly

### The first access method looked insecure

The application worked, but the browser security indicator made it clear that my first access method was not the way I wanted to leave it.

Instead of exposing another port or ignoring the warning, I used the tools already available in my environment and put the local service behind Tailscale Serve.

### Browser location permissions were confusing

When the geolocation prompt first appeared, the expected controls were not responding normally.

Rather than repeatedly clicking the prompt, I opened the browser's site settings and changed the permission from there.

That was a good reminder that troubleshooting a self-hosted application sometimes means looking beyond the Linux server itself. The issue may be in the browser or client device.

---

## Current Access Design

```text
Remote Device
     │
     ▼
Tailscale Private Network
     │
     ▼
HTTPS / Tailscale Serve
     │
     ▼
War Room Local HTTP Service
     │
     ▼
Linux Guest on Proxmox
```

There is no intentional public port-forwarding path to the application.

---

## What I Learned

This project helped me get hands-on experience with:

- Deploying someone else's application on infrastructure I manage
- Local web services
- Private overlay networking
- The practical difference between HTTP and HTTPS
- Tailscale Serve as a simple HTTPS proxy
- Browser site permissions
- Client-side geolocation controls
- Thinking about credential handling before entering credentials
- Separating application development from application deployment and administration

I am not presenting this project as proof that I am a web-security expert. What it shows is that I started asking better questions as I built the lab instead of treating "it loads in the browser" as the end of the project.

---

## Result

War Room is now hosted inside my Proxmox environment and accessible remotely through my private Tailscale network using HTTPS.

More importantly, the project introduced me to the idea that self-hosting is not just installing software. It also involves deciding **how it should be reached, what should be exposed, what permissions it needs, and what data I am comfortable giving it.**

---

## Future Improvements

- Persistent application-data backups
- Centralized logging
- Resource monitoring
- Recovery documentation
- More granular access for additional users
- Network segmentation as the homelab grows
