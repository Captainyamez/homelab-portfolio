# Project #3: Secure Self-Hosted Application Deployment

## Overview

This project documents the deployment and administration of a third-party War Room web application on my Proxmox home server.

I did **not** develop the War Room application itself. The project focuses on infrastructure work: deploying the application in my homelab, making it reachable through a private remote-access network, troubleshooting browser access, and reviewing privacy/security behavior before using the service.

> **Public portfolio note:** Network addresses, hostnames, and other environment-specific identifiers shown here are sanitized or generalized. Credentials, public IP addresses, tokens, private keys, and private remote-access domains are not published.

## Environment

- Hypervisor: Proxmox VE
- Linux guest/container hosting the application
- Third-party War Room web application
- Tailscale private overlay network
- Tailscale Serve for HTTPS access within the tailnet
- Browser-based administration from remote devices

## Objectives

- Deploy a third-party web application inside the homelab.
- Keep the application off the public internet.
- Provide remote access through an authenticated private overlay network.
- Improve browser security by using HTTPS instead of directly browsing to an untrusted HTTP endpoint.
- Evaluate application permissions before enabling location-dependent features.
- Document the difference between application development and infrastructure deployment.

## Deployment

### 1. Application hosting

The War Room application was installed on a Linux environment hosted by Proxmox and configured to listen locally on a web port.

The service was first reachable through a direct internal web address. The browser displayed a security warning because the initial connection method did not provide a trusted HTTPS context.

### 2. Private remote access

Rather than expose the application through router port forwarding, Tailscale was used to provide authenticated remote connectivity between approved devices.

This reduced the public attack surface and allowed access while away from the home network.

### 3. HTTPS with Tailscale Serve

Tailscale Serve was configured to proxy the application's local HTTP listener behind a Tailscale-provided HTTPS endpoint.

Representative command:

```bash
tailscale serve --bg http://127.0.0.1:8000
```

The resulting private HTTPS URL is intentionally omitted from this public portfolio.

Moving from the direct HTTP address to the HTTPS Tailscale Serve endpoint removed the browser's insecure-connection warning and provided a cleaner access path for the application.

## Security and Privacy Review

Before creating credentials or granting browser permissions, I reviewed how the application was being accessed and what data could potentially be exposed.

### Password handling

Before creating an application password, I considered whether credentials would be stored by the third-party application and avoided publishing any authentication information in documentation.

### Browser geolocation

While testing the application's map functionality, the browser requested permission to access device location.

Instead of granting permission automatically, I reviewed the browser's site-permission controls and verified that location access could be explicitly allowed or denied for the private War Room site.

This demonstrated an important distinction between:

- Server-side application deployment
- Browser permissions on the client device
- Application access to data exposed by the browser

The geolocation permission was controlled at the browser/site level rather than assumed to be inherently required by the server deployment.

## Troubleshooting

### Browser showed an insecure connection indicator

The original access method used a direct web endpoint and produced a browser warning.

Resolution:

- Kept the application listening locally.
- Used Tailscale Serve as an HTTPS reverse proxy.
- Connected through the private Tailscale HTTPS hostname instead of the raw HTTP endpoint.

### Site permissions were not obvious

When the application requested location access, the expected permission options were not immediately selectable from the prompt.

Resolution:

- Opened the browser's site controls.
- Located the permissions for the currently loaded private site.
- Adjusted geolocation access from the site-specific permission interface.

This reinforced the value of understanding both server-side infrastructure and client-side browser security controls when troubleshooting web applications.

## Result

War Room is now self-hosted inside the Proxmox environment and can be reached remotely through a private Tailscale connection using HTTPS.

The application remains separated from direct public internet exposure, and sensitive hostnames and credentials are excluded from public documentation.

## Skills Demonstrated

- Third-party application deployment
- Linux service administration
- Proxmox-hosted service management
- Private overlay networking
- Tailscale remote access
- HTTPS reverse proxying with Tailscale Serve
- Web-service troubleshooting
- Browser security and permission management
- Privacy-conscious deployment practices
- Infrastructure documentation

## Key Takeaway

This project demonstrates the operational side of self-hosting: taking software developed by someone else, deploying it on managed infrastructure, providing secure remote connectivity, troubleshooting user access, and evaluating privacy implications.

That distinction is important in this portfolio: the work documented here represents **deployment, configuration, administration, and troubleshooting**, not authorship of the War Room application's source code.

## Future Improvements

Potential future work includes:

- Persistent application-data backups
- Centralized logging and monitoring
- Resource-usage monitoring
- Documented recovery procedures
- More granular access control for additional users
- Network segmentation as the homelab architecture grows
