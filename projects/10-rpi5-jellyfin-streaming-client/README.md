# Project #10: Raspberry Pi 5 Jellyfin Streaming Client

> **Status:** ✅ Working / Daily-use capable  
> **Purpose:** Build a small dedicated Jellyfin streaming appliance without relying on a Fire TV, Chromecast, or other large vendor ecosystem  
> **Focus:** Raspberry Pi 5, LibreELEC, Kodi, Jellyfin for Kodi, library synchronization, appliance-style shutdown, thermal management

## Why I Chose This Project

After building the Jellyfin server, I wanted a client device that I could understand and control from the operating system upward.

Commercial streaming boxes are easy to buy, but this project gave me an excuse to build a dedicated endpoint around open-source software and my existing self-hosted media server.

The goal was simple:

- boot directly into a TV-friendly interface
- connect to my Jellyfin server
- present the Jellyfin library natively inside Kodi
- play media reliably
- shut down safely from the couch
- remain portable enough to use with another TV while traveling

> **Public portfolio note:** Private server addresses, credentials, Tailscale identities, and other environment-specific information are intentionally omitted.

---

## Hardware

The client is built around:

- Raspberry Pi 5
- microSD storage
- HDMI connection to the display/TV
- Argon NEO case
- passive thermal pads plus case cooling
- wired Ethernet or Wi-Fi depending on where it is being used
- keyboard during setup, with normal operation designed around the TV interface

The Pi is powerful enough for the client role while still being small enough to pack with my travel networking gear.

---

## Software Stack

```text
Raspberry Pi 5
      │
      ▼
 LibreELEC
      │
      ▼
    Kodi
      │
      ▼
Jellyfin for Kodi
      │
      ▼
Jellyfin Server
```

I chose LibreELEC because it is a minimal operating system built specifically around Kodi. Instead of maintaining a full desktop Linux installation, the Pi behaves much more like a dedicated media appliance.

---

## Initial Bring-Up

After flashing LibreELEC to the microSD card, I booted the Pi and completed the first-run configuration.

I enabled SSH during setup and verified remote shell access before installing the Jellyfin integration.

That gave me two administration paths:

1. Kodi's local TV interface
2. SSH for troubleshooting and configuration

This made it much easier to diagnose problems without repeatedly removing the Pi from the TV setup.

---

## Jellyfin Integration

I installed the **Jellyfin for Kodi** add-on and connected it to the existing Jellyfin server.

At first, the Jellyfin add-on itself could browse the server's movies and shows, but Kodi's normal Movies and TV sections remained empty.

The fix was to explicitly add the Jellyfin libraries and allow the add-on to populate Kodi's local media database.

After adding both the movie and TV libraries and running the library update, the content appeared in Kodi's native sections.

This distinction taught me that:

- being able to browse a server through an add-on is not the same thing as
- synchronizing that server's library into Kodi's own database and interface

---

## Library Synchronization

Once the Jellyfin libraries were linked correctly, new playback activity and media changes began appearing in Kodi as expected.

I also enabled the appropriate synchronization behavior in the Jellyfin add-on so the Pi acts like a real client for the existing server rather than a disconnected local library.

When one newly added movie appeared in the native Jellyfin client but not Kodi, I used the Jellyfin add-on's library-update controls to resynchronize the local Kodi view.

---

## Kodi Interface Customization

I installed and configured the **Arctic Fuse 3** skin to make Kodi feel more like a dedicated streaming appliance.

One of the interface problems I ran into was the default Quit action. In this setup, selecting Quit only closed the sidebar rather than safely powering off the Pi.

I changed that menu action to Kodi's:

```text
Shutdown()
```

After the change, selecting the menu item shut the Raspberry Pi down properly. I verified this by watching the display go black and the Pi's indicator change from active operation to its powered-down state.

That was a small UI customization, but an important operational improvement: the normal visible power option now performs a safe OS shutdown instead of encouraging the device to be unplugged while running.

---

## Case and Thermal Setup

Once the software was stable, I moved the Pi into the Argon NEO case.

The case uses thermal contact between the Raspberry Pi's main heat-generating components and the metal chassis, supplemented by active airflow.

I cut the supplied thermal pad material to fit the important contact points rather than covering the entire board indiscriminately.

The goal is not extreme cooling; it is stable, quiet operation in a compact enclosure while decoding and displaying media.

---

## Network Role

At home, the Pi can reach the Jellyfin server over the local network.

The wider design also fits my travel setup:

```text
Jellyfin server at home
        │
        ▼
     Tailscale
        │
        ▼
 Travel router / remote network
        │
        ▼
 Raspberry Pi 5
        │
        ▼
       TV
```

The Pi is intended to become a portable trusted Jellyfin endpoint that I can connect to a hotel or friend's TV instead of signing into a random smart-TV ecosystem.

Native Tailscale support on the streaming client is a planned next step; it is not documented here as completed until I have installed and validated it on this device.

---

## Verification

I verified the client in stages:

### LibreELEC
- Pi boots successfully from microSD.
- Kodi launches automatically.
- SSH access works for administration.

### Jellyfin
- Jellyfin for Kodi connects to the server.
- Movie and TV libraries synchronize into Kodi.
- Content appears in Kodi's native Movies and TV sections.
- Playback works from the synchronized library.
- Continue-watching behavior updates after playback activity.

### Interface
- Arctic Fuse 3 skin loads correctly.
- The customized power action calls `Shutdown()`.
- The Pi shuts down cleanly from the Kodi interface.

### Hardware
- Pi operates inside the Argon NEO enclosure.
- Thermal pads are installed at the intended chip-to-case contact points.
- HDMI output and network connectivity remain stable during normal playback.

---

## Privacy and Design Decisions

This project was partly motivated by wanting a streaming endpoint that is under my control.

Instead of making the device dependent on a large vendor account ecosystem, the main software layers are:

- LibreELEC
- Kodi
- Jellyfin
- my own self-hosted server

That does not automatically make the device perfectly private or secure, but it gives me much more visibility into what the box is running and how it reaches my media.

I also avoid publishing private server addresses or remote-access identifiers in this portfolio.

---

## What I Learned

This project gave me hands-on experience with:

- Raspberry Pi 5 deployment
- Flashing and booting an appliance OS
- LibreELEC
- Kodi
- Kodi add-ons
- Jellyfin client/server integration
- Media-library synchronization
- SSH administration
- Troubleshooting the difference between add-on browsing and native Kodi libraries
- Kodi action customization
- Safe Linux shutdown behavior
- Raspberry Pi thermal management
- Designing a purpose-built endpoint instead of using a general-purpose desktop OS

The biggest lesson was that the client side of self-hosting matters too. Running a server is only half the system; the endpoint still needs reliable networking, synchronization, UI behavior, and safe power management.

---

## Result

I now have a dedicated Raspberry Pi 5 streaming box that boots directly into Kodi, synchronizes my Jellyfin movies and TV shows into the native interface, plays media from the homelab, and can be shut down safely from the on-screen menu.

It also gives me a portable platform that I can continue extending for private remote Jellyfin access while traveling.

---

## Future Improvements

- Install and validate Tailscale directly on the LibreELEC client
- Test the complete remote/travel workflow through the GL.iNet travel router
- Add a compact remote-control solution
- Measure temperatures and fan behavior during long playback sessions
- Document clean-update/rollback procedures for LibreELEC and Kodi add-ons
- Test additional TVs and HDMI environments
