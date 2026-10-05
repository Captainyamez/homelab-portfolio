# Project #10: Raspberry Pi 5 Jellyfin Streaming Client

> **Status:** ✅ Working / Daily-use capable  
> **Purpose:** Build a portable, privacy-focused Jellyfin streaming appliance that avoids the account, advertising, and behavioral-tracking ecosystems of mainstream streaming-device vendors  
> **Focus:** Raspberry Pi 5, LibreELEC, Kodi, Jellyfin for Kodi, privacy-conscious client design, travel networking, K06 mini keyboard/IR remote, library synchronization, appliance-style shutdown, thermal management

## Why I Chose This Project

After building the Jellyfin server, I wanted a client device that I could understand and control from the operating system upward.

Commercial streaming boxes are easy to buy, but I specifically wanted to avoid making my primary streaming client part of a corporate ecosystem built around vendor accounts, advertising, telemetry, and viewing/behavioral data collection. This project gave me a way to build a dedicated endpoint around open-source software and my existing self-hosted media server instead.

The goal was simple:

- boot directly into a TV-friendly interface
- connect to my Jellyfin server
- present the Jellyfin library natively inside Kodi
- play media reliably
- shut down safely from the couch
- remain portable enough to use with another TV while traveling
- avoid unnecessary advertising and behavioral/data tracking from mainstream streaming-device vendors
- use one compact input device for both Kodi control and basic TV control when traveling

> **Public portfolio note:** Private server addresses, credentials, Tailscale identities, and other environment-specific information are intentionally omitted.

---

## Hardware

The client is built around:

- Raspberry Pi 5
- microSD storage
- HDMI connection to the display/TV
- Argon NEO case
- passive thermal pads plus case cooling
- **K06 mini keyboard/remote** with keyboard keys, integrated trackpad, and IR-learning controls
- wired Ethernet or Wi-Fi depending on where it is being used

The Pi is powerful enough for the client role while still being small enough to pack with my travel networking gear.

---

## K06 Mini Keyboard and IR-Learning Remote

For normal use, I paired the streaming box with a **K06 mini keyboard/remote** instead of carrying a full keyboard and a separate TV remote.

The device combines:

- a compact keyboard for text entry and Kodi shortcuts
- an integrated trackpad for pointer control
- IR-learning controls on the reverse side

The IR-learning side is especially useful when traveling. At a house, hotel, or other temporary location, I can teach the K06 the basic infrared commands from the TV's original remote—at minimum power and volume—so I do not need to juggle a separate TV remote just to control the display.

That gives the travel kit a simple operating model:

```text
K06 mini keyboard
├── keyboard / trackpad ──► Raspberry Pi 5 / Kodi
└── learned IR commands ──► TV power / volume
```

The result is one handheld controller for both the streaming box and the basic TV functions I need most often.

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

The wider design also fits my travel setup, and I have now tested that workflow away from home:

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

I tested the streaming box away from home using my travel router in both of the network modes I expect to use in practice:

- **Wi-Fi:** the Raspberry Pi 5 connected wirelessly to the travel router and streamed successfully.
- **Ethernet:** the Raspberry Pi 5 was connected directly to the travel router by Ethernet and streamed successfully.

That confirmed the client is not just an at-home proof of concept. It works as a portable endpoint on a remote network and can use either wireless or wired LAN connectivity depending on the room and equipment available.

The Pi is intended to remain a trusted Jellyfin endpoint that I can connect to a hotel or friend's TV instead of signing into a smart-TV or mainstream streaming-device ecosystem that I do not control.

Native Tailscale support on the streaming client itself is still a planned next step; the completed remote test used the travel-router setup.

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
- K06 keyboard and trackpad provide local Kodi control.
- K06 IR learning provides basic TV power/volume control after learning commands from the local TV remote.

### Remote / travel testing
- Streaming was tested successfully away from home through the travel router.
- Wi-Fi connectivity from the Pi 5 to the travel router was verified.
- Ethernet connectivity from the Pi 5 to the travel router was verified.
- The same client and Jellyfin interface worked outside the home LAN.

---

## Privacy and Design Decisions

This project was strongly motivated by wanting a streaming endpoint that is under my control and that does not depend on the advertising and data-collection model common to mainstream streaming hardware.

Instead of making the device dependent on a large vendor account ecosystem, the main software layers are:

- LibreELEC
- Kodi
- Jellyfin
- my own self-hosted server

That does not automatically make the device perfectly private or secure, but it removes a major reason I did not want to use a mainstream streaming stick or box: I do not need the client itself to be tied to a large vendor's advertising profile, viewing telemetry, or account ecosystem just to reach my own media. It also gives me much more visibility into what the box is running and how it reaches my media.

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
- Travel-router testing over both Wi-Fi and Ethernet
- IR-learning remote control
- Combining keyboard, trackpad, and TV controls into one portable input device
- Designing a purpose-built endpoint instead of using a general-purpose desktop OS
- Privacy-oriented client design that minimizes dependence on advertising-driven streaming platforms

The biggest lesson was that the client side of self-hosting matters too. Running a server is only half the system; the endpoint still needs reliable networking, synchronization, UI behavior, and safe power management.

---

## Result

I now have a dedicated Raspberry Pi 5 streaming box that boots directly into Kodi, synchronizes my Jellyfin movies and TV shows into the native interface, plays media from the homelab, and can be shut down safely from the on-screen menu.

I have also verified it away from home through my travel router over both Wi-Fi and Ethernet. The K06 mini keyboard gives me keyboard and trackpad control for Kodi while its IR-learning side can take over basic TV power and volume controls at a hotel or another house.

That makes the project both a privacy-conscious alternative to mainstream streaming hardware and a practical portable Jellyfin client.

---

## Future Improvements

- Install and validate Tailscale directly on the LibreELEC client
- Test additional remote-network and hotel captive-portal scenarios
- Expand the K06 IR profile beyond power/volume where useful
- Measure temperatures and fan behavior during long playback sessions
- Document clean-update/rollback procedures for LibreELEC and Kodi add-ons
- Test additional TVs and HDMI environments
