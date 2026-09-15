# Project #6: DVD Ripping and Hardware-Accelerated Media Encoding on Proxmox

> **Status:** ✅ Working / Automated workflow tested across multiple DVDs  
> **Purpose:** Build a dedicated media-ingestion container that can read DVDs, identify and encode a selected title, verify the result, and safely place the finished file into the Jellyfin media library  
> **Focus:** Proxmox/LXC, optical-device passthrough, Linux device access, DVD-Video structure, FFmpeg, Intel VA-API, HEVC, stream selection, automation, validation, and troubleshooting

## Why I Chose This Project

Once Jellyfin was running, I needed a repeatable way to move media from DVDs into the library without turning the Jellyfin container itself into a ripping workstation.

I built the ripping workflow in its own Proxmox LXC. That kept media ingestion separate from media serving and gave me a place to experiment with optical-drive passthrough, GPU access, FFmpeg, DVD navigation, subtitles, encoding, recovery, and validation without changing the Jellyfin application container every time I tested something.

The project started as a completely manual workflow. I intentionally learned and tested each stage first: inspect the disc, create a backup, identify the correct DVD title, inspect streams, choose filters, encode, verify the result, and only then move it into Jellyfin.

After the manual process had been tested across multiple DVDs, I turned the proven workflow into an interactive `ripdvd` script. The automation still pauses for human decisions where guessing could be dangerous, but it now handles most of the repetitive work and includes several failure-safe recovery paths.

> **Public portfolio note:** This documentation focuses on infrastructure, automation, and troubleshooting. It does not publish disc contents, credentials, private addresses, device-specific secrets, or other sensitive/environment-specific identifiers. DVD examples are intentionally described generically rather than by title.

---

## Environment

| Item | Configuration |
|---|---|
| Hypervisor | Proxmox VE |
| Host system | HP EliteDesk 800 G4 SFF |
| CPU | Intel Core i5-8500 |
| Integrated graphics | Intel UHD Graphics 630 |
| Memory | 32 GB RAM |
| Bulk storage | 4 TB SATA HDD, `ext4` |
| Guest | Dedicated Debian 13 LXC container |
| Container resources | 2 CPU cores, 2 GB RAM, 512 MB swap, 16 GB root filesystem |
| Optical drive | Internal DVD writer passed through from the Proxmox host |
| GPU device | Intel `/dev/dri` render device passed through to the container |
| Working storage | Bind-mounted HDD directory at `/work` |
| Jellyfin media target | Bind-mounted Jellyfin storage at `/media` |
| Main tools | `dvdbackup`, FFmpeg/ffprobe, libdvdcss, VA-API, `libx265`, `vainfo` |
| Automation | Interactive Bash wrapper: `ripdvd` |

---

## What I Wanted to Accomplish

- Keep DVD ripping separate from the Jellyfin server itself.
- Pass the physical optical drive into an LXC container.
- Give the container access to the Intel integrated GPU for hardware-assisted HEVC encoding.
- Store temporary DVD backups on the large HDD rather than filling the container root filesystem.
- Write completed media into the existing Jellyfin storage hierarchy.
- Preserve the correct display aspect ratio rather than stretching video to fill a screen.
- Select the correct audio and subtitle streams instead of assuming a fixed stream layout.
- Handle both progressive and interlaced DVD material.
- Prefer fast hardware encoding, but recover automatically when a DVD exposes a hardware-filter edge case.
- Detect obviously truncated output before it reaches Jellyfin.
- Reuse an existing DVD backup after an interrupted run rather than forcing another full optical read.
- Avoid overwriting an existing library file.
- Log successful and failed runs.
- Keep the workflow understandable enough that I can troubleshoot it instead of treating the script as a black box.

---

## Container and Device Design

The media-ingestion container is deliberately separate from the LXC that runs Jellyfin.

```text
                     HP EliteDesk 800 G4 SFF
                              │
                              ▼
                         Proxmox VE
                              │
             ┌────────────────┴────────────────┐
             │                                 │
             ▼                                 ▼
      DVD Ripping LXC                    Jellyfin LXC
      Debian 13                          Debian 13
             │                                 │
      /dev/sr0 optical drive                    │
      /dev/sg0 SCSI generic                     │
      /dev/dri render device                    │
             │                                 │
             └────── 4 TB HDD bind mounts ─────┘
                       │              │
                     /work          /media
                       │              │
                  DVD backup      Jellyfin library
                  + encoding
```

The container root filesystem stays relatively small because the large temporary DVD backup and final media files live on the bulk-storage HDD.

---

## Optical Drive Passthrough

The DVD drive needs more than a filesystem path. The container is given access to both the optical block device and its SCSI-generic interface:

```text
/dev/sr0
/dev/sg0
```

The LXC configuration also needs matching device permissions so applications inside the container can communicate with the physical drive.

This was useful practice with Linux device types, major/minor numbers, cgroup permissions, and the difference between making a device path visible and actually making the device usable.

---

## Intel GPU Passthrough and VA-API

The host's Intel UHD 630 is exposed to the ripping container through `/dev/dri`.

I installed the Intel media driver and used `vainfo` to confirm that the container could initialize the GPU and that HEVC encoding was available.

The hardware encode path uses FFmpeg with the render device and VA-API:

```bash
-vaapi_device /dev/dri/renderD128
-c:v hevc_vaapi
-qp 24
```

For progressive material, the filter path is:

```text
format=nv12,hwupload
```

For top-field-first interlaced material, the safe path is:

```text
bwdif=mode=send_frame:parity=tff:deint=interlaced,format=nv12,hwupload
```

Bottom-field-first material can use the same pattern with `parity=bff`.

The script now selects the appropriate path from `ffprobe` field-order information rather than applying one filter to every DVD.

---

# Automation Evolution

## Stage 1: Manual Workflow

I did not start by writing automation.

The first working process was deliberately manual:

1. Inspect a DVD with `dvdbackup`.
2. Create a full HDD mirror of the disc.
3. Use FFmpeg's `dvdvideo` demuxer against the backup.
4. Probe the main title with `ffprobe`.
5. Choose audio and subtitle streams.
6. Decide whether the source was progressive or needed deinterlacing.
7. Encode with Intel VA-API HEVC.
8. Verify the output runtime and streams.
9. Test playback in Jellyfin.
10. Only then remove the temporary DVD backup.

That manual phase exposed several failure modes that would have been easy to hide inside a one-command script.

---

## Stage 2: `ripdvd` Version 1

Once the manual workflow was repeatable, I wrote the first interactive wrapper.

Version 1 automated the repetitive stages while keeping the risky decisions visible. It could:

- Inspect the inserted DVD.
- Detect the disc label.
- Detect the main title set and likely main title.
- Ask for a clean movie name and release year.
- Create the DVD backup on bulk storage.
- Probe source duration and field order.
- Ask which audio and subtitle streams to keep.
- Select a progressive or deinterlacing path.
- Encode with Intel VA-API HEVC.
- Compare source and output runtime.
- Refuse to move a suspiciously short file into Jellyfin.
- Place a verified file into the expected Jellyfin movie directory.

The first full end-to-end test completed successfully, including chapters, audio, subtitles, and playback on multiple Jellyfin clients.

---

## Stage 3: `ripdvd` Version 1.1

Real-world testing then exposed usability and recovery cases that Version 1 did not handle well enough.

Version 1.1 added the following.

### Deliberate movie naming

The disc label is displayed as reference, but the user must enter a clean title instead of silently accepting whatever text the DVD author used as the volume label.

This keeps the filesystem clean even when Jellyfin would have been able to repair the displayed metadata later.

### Existing-file protection

Before doing expensive work, the script checks whether the destination file already exists.

If it does, the script stops rather than overwriting a known-good library file.

### Existing-backup resume

If a working directory already exists, the script offers:

```text
[R] Resume using existing backup
[D] Delete backup and start over
[C] Cancel
```

This was added after a remote shell disconnected during a DVD backup. The process had stopped, but the backup data was already present on disk.

Instead of reading the physical DVD again, the resumed run reused the existing backup and continued into probing and encoding.

### Clear audio and subtitle menus

Instead of requiring the user to interpret raw FFmpeg stream numbers, Version 1.1 presents type-relative choices such as:

```text
[0] eng | ac3 | 6 ch | 5.1(side)
[1] eng | dts | 7 ch | 6.1
[2] eng | ac3 | 2 ch | stereo
```

Multiple tracks can be selected with a comma-separated input such as:

```text
0,1
```

Subtitle selection uses the same style and can also retain multiple streams.

### Encode-plan confirmation

Before encoding starts, the script prints the selected plan:

```text
Movie:       Example Movie (Year)
Title:       1
Angle:       1
Video mode:  Progressive
Audio:       0,1
Subtitles:   0
Destination: /media/movies/Example Movie (Year)/Example Movie (Year).mkv
```

This gives one final opportunity to catch a wrong title, stream selection, or destination before a long encode begins.

### Field-order aware filtering

Version 1.1 distinguishes:

- progressive
- top-field-first
- bottom-field-first
- unknown/mixed cases

The script selects the corresponding FFmpeg filter rather than assuming every older DVD needs the same deinterlacing path.

### Duration verification

The source duration is recorded before encoding. After encoding, `ffprobe` checks the output duration and calculates the absolute difference.

If the difference exceeds the configured tolerance, the script treats the result as failed verification and does **not** move it into Jellyfin.

This is important because a media encoder can exit successfully even when the produced file is shorter than the selected DVD title.

### Rip logging

Successful and failed attempts are written to a log including useful fields such as:

- title
- source duration
- output duration
- duration difference
- encoder used
- video mode
- audio selection
- subtitle selection
- output size

### Optional cleanup

After a verified successful encode, the script can delete the temporary DVD backup, but the default workflow encourages playback testing in Jellyfin first.

---

# Automatic Hardware-to-Software Recovery

One of the most useful changes came from repeated failures involving a mid-stream MPEG-2 parameter change.

On some DVDs, FFmpeg would encode almost the entire title through VA-API and then report a message similar to:

```text
Reconfiguring filter graph because video parameters changed to yuv420p(tv, smpte170m)
Impossible to convert between the formats supported by the filter 'hwupload' and an automatically inserted scale filter
Error reinitializing filters
Conversion failed
```

The important observation was that the physical DVD and backup were still readable. The failure occurred specifically in the hardware-upload/filter path when the decoded stream changed parameters.

I verified this by encoding the same affected region in software. Software encoding handled the parameter transition correctly.

Version 1.1 now reacts to a failed VA-API encode by preserving the backup and selections and offering:

```text
HARDWARE ENCODE FAILED

Retry using software HEVC (libx265)? [Y/n]:
```

If accepted, it retries using:

```text
-c:v libx265
-crf 22
-preset medium
```

The same selected audio, subtitle, title, angle, and deinterlacing decisions are reused.

The software result still has to pass the same runtime verification before it can enter the Jellyfin library.

This fallback has now recovered multiple real DVD encodes that failed through VA-API, making it a tested recovery path rather than a theoretical feature.

---

# Current Automated Workflow

The normal workflow is now simply:

```bash
ripdvd
```

The script then performs the following sequence:

```text
Insert DVD
   │
   ▼
Inspect DVD structure
   │
   ▼
Identify likely main feature
   │
   ▼
Ask for clean title + year
   │
   ▼
Check destination for duplicates
   │
   ▼
Existing backup?
   ├── Yes → Resume / delete / cancel
   └── No  → Full dvdbackup mirror
   │
   ▼
Probe runtime + field order + streams
   │
   ▼
Choose audio/subtitles
   │
   ▼
Review encode plan
   │
   ▼
Try Intel VA-API HEVC
   │
   ├── Success ──────────────┐
   │                         │
   └── Failure              │
        │                    │
        ▼                    │
   Offer libx265 retry       │
        │                    │
        └────────────────────┘
                 │
                 ▼
          Verify duration
                 │
        ┌────────┴────────┐
        │                 │
      Pass              Fail
        │                 │
        ▼                 ▼
 Move to Jellyfin   Preserve work files
        │            Do not publish result
        ▼
 Playback test
        │
        ▼
 Optional backup cleanup
```

---

# Troubleshooting Guide

This section documents the problems that changed the design of the workflow and the checks I would use again on another system.

## 1. `dvdbackup` sees the disc, but the script cannot determine the main title set

First inspect the DVD directly:

```bash
dvdbackup -i /dev/sr0 -I
```

Look for a line similar to:

```text
Title set containing the main feature is 1
```

If it exists, test the parser independently:

```bash
dvdbackup -i /dev/sr0 -I 2>&1 | \
awk '/Title set containing the main feature is/ {
    print "DETECTED MAIN SET:", $NF
}'
```

If the direct command works but the script failed once, rerun the inspection before changing code. Optical-disc initialization can occasionally produce a transient read problem.

If the DVD genuinely has no obvious main feature, it may need manual title selection instead of an automatic main-feature assumption.

---

## 2. A bonus/extras DVD contains several meaningful titles

A DVD can contain many titles inside one title set. `dvdbackup -I` will show the title structure and chapter counts.

Do not assume that "main feature" always means "the only content worth ripping." Bonus discs, documentaries, and collections can contain several independent pieces of content.

The current movie workflow is designed primarily around one selected feature. Manual title-selection support is a logical future extension for complex extras discs.

---

## 3. FFmpeg prints `Couldn't find device name` when reading the HDD backup

When the `dvdvideo` demuxer reads a DVD directory rather than the physical optical drive, messages such as these can appear:

```text
libdvdread: Couldn't find device name.
libdvdnav: Can't read name block. Probably not a DVD-ROM device.
```

In this workflow, these warnings are expected when FFmpeg is reading the backed-up DVD directory. They are not by themselves evidence that the backup is broken.

The more important checks are whether FFmpeg can enumerate the title, chapters, streams, and duration.

---

## 4. Raw VOB concatenation produces broken timing

A DVD is not just a collection of MPEG files. IFO navigation data, title boundaries, chapters, angles, and timestamps matter.

If concatenating VOB files produces timing problems, use FFmpeg's DVD-aware demuxer against the complete backup instead:

```bash
ffprobe \
-f dvdvideo \
-title 1 \
-preindex 1 \
-i "/work/example-dvd/Disc Directory"
```

`-preindex 1` takes longer because FFmpeg scans the title, but it produces much more reliable chapter and duration indexing.

---

## 5. VA-API fails near the end of an otherwise normal encode

A typical failure signature is:

```text
Reconfiguring filter graph because video parameters changed ...
Impossible to convert between the formats supported by ... hwupload ...
Error reinitializing filters
Conversion failed
```

This can happen when MPEG-2 video metadata changes mid-stream and the hardware filter chain cannot reinitialize.

Do not immediately assume the DVD is damaged.

A useful isolation test is to encode the problem area with a software encoder. If the same section completes in software, the source is readable and the problem is likely the hardware/filter path.

The automated workflow now offers a full software HEVC retry with `libx265`.

---

## 6. FFmpeg exits successfully but the movie is too short

Successful process exit is not enough.

Compare the source title duration with the finished file:

```bash
ffprobe \
-v error \
-show_entries format=duration \
-of default=noprint_wrappers=1:nokey=1 \
"output.mkv"
```

The automation performs this comparison automatically and rejects output that differs beyond the configured tolerance.

This check was added because some processing paths can produce a technically valid MKV that is missing part of the title.

---

## 7. A short inverse-telecine test looks correct, but the full encode breaks

A short sample is useful, but it does not prove that an entire DVD has consistent cadence or timestamps.

I tested an inverse-telecine path that looked excellent on a sample but produced major timestamp problems across the complete title.

For the automated workflow, I prefer the safer deinterlacing path unless the complete source has been proven suitable for IVTC.

The lesson is simple: validate the **full runtime**, not just picture quality in a short sample.

---

## 8. SSH disconnects during a rip

If the process was attached directly to the remote shell, it may stop when that session disappears.

Before reripping the DVD, inspect `/work`:

```bash
du -sh /work/* 2>/dev/null
find /work -maxdepth 2 -type d
```

If a substantial DVD backup already exists, verify that it contains a `VIDEO_TS` directory and use the script's resume option instead of reading the whole disc again.

A terminal multiplexer such as `tmux` can also protect a long-running job from SSH disconnects, but it is optional in my normal workflow.

---

## 9. Progressive vs interlaced handling

Check field order with `ffprobe` rather than guessing based on the age of the DVD:

```bash
ffprobe \
-v error \
-f dvdvideo \
-title 1 \
-i "/work/example-dvd/Disc Directory" \
-select_streams v:0 \
-show_entries stream=field_order \
-of default=noprint_wrappers=1:nokey=1
```

Common results include:

```text
progressive
tt
tb
bb
bt
```

The automation maps those values to progressive, TFF, BFF, or automatic deinterlacing behavior.

---

## 10. No subtitle tracks are present

Not every DVD has usable subtitle streams.

If `ffprobe` finds no subtitle streams, the script simply continues without mapping subtitles. That is not an error.

---

# Why the Validation Layer Matters

The biggest design change in this project was moving from "FFmpeg finished" to "the output passed validation."

A file is not considered ready just because an encoder created it.

The current acceptance path is:

1. Source title is successfully indexed.
2. User confirms title and stream choices.
3. Encoder completes.
4. Output duration is compared with source duration.
5. Only a verified file is moved into Jellyfin.
6. Playback is tested on an actual Jellyfin client.
7. Temporary backup is removed only after confidence in the final result.

This protects the library from partial or silently truncated files.

---

## What I Tested

The workflow has now been exercised across multiple DVDs with combinations of:

- 4:3 and 16:9 content
- Progressive video
- Top-field-first interlaced video
- Stereo and 5.1 AC3 audio
- DTS audio
- Multiple audio tracks
- Multiple spoken languages
- Multiple subtitle tracks
- No subtitle tracks
- Multi-angle DVD structure
- Bonus/documentary discs
- Intel VA-API HEVC encoding
- Automatic `libx265` software fallback
- Interrupted sessions with existing-backup recovery
- Mid-stream video-parameter changes
- Runtime verification failures
- Jellyfin playback on phone and television clients

---

## Verification

### Container and storage

- The dedicated Debian LXC runs independently of Jellyfin.
- `/work` maps to bulk HDD storage for temporary DVD data and encodes.
- `/media` maps to the Jellyfin media hierarchy on the bulk-storage disk.

### Optical drive

- The physical DVD drive is visible inside the container.
- Both the block-device and SCSI-generic interfaces are available where required.
- `dvdbackup` can inspect and mirror supported DVDs to local storage.

### GPU

- The Intel render device is visible inside the container.
- `vainfo` initializes the Intel media driver.
- FFmpeg can perform HEVC encoding through VA-API.

### Media output

Across the tested DVDs I verified combinations of:

- HEVC 720x480 video
- Correct 4:3 or 16:9 display aspect ratio
- Progressive output after processing
- Stereo, 5.1, and selected alternate audio tracks
- DVD subtitle streams
- Chapter preservation
- Full-runtime verification
- Jellyfin playback on multiple client types

---

## What I Learned

This project gave me hands-on experience with:

- Proxmox LXC device passthrough
- Linux block and character devices
- LXC cgroup device permissions
- Bind mounts between the Proxmox host and containers
- DVD-Video title/VTS/chapter structure
- FFmpeg and `ffprobe`
- FFmpeg's `dvdvideo` demuxer
- Intel VA-API and HEVC hardware encoding
- Software HEVC fallback with `libx265`
- Progressive vs interlaced video
- Field-order detection and deinterlacing
- Aspect-ratio preservation
- Audio stream selection and language tagging
- Subtitle stream handling
- Bash scripting and interactive automation
- Input validation and overwrite protection
- Resume/recovery design
- Structured logging
- Diagnosing filter-graph failures
- Diagnosing truncated output
- Separating source problems from encoder problems
- Verifying output at both the file and application layers

The biggest lesson was that media ingestion is not one operation. Reading the physical disc, understanding DVD structure, selecting the correct content, decoding video correctly, choosing streams, encoding, storing the result, and validating playback are separate stages that can fail independently.

The automation became reliable because each of those stages was understood manually before it was scripted.

---

## Result

I now have a dedicated Proxmox LXC with an interactive `ripdvd` workflow for ingesting DVDs into Jellyfin.

The script can inspect a DVD, create or resume a backup, identify a likely main title, request clean metadata, display audio/subtitle choices, select an appropriate progressive or deinterlacing path, attempt fast Intel VA-API HEVC encoding, retry automatically with `libx265` when the hardware path fails, verify the finished runtime, log the result, and only then place the file into the Jellyfin movie library.

The automation has been tested repeatedly across different DVD structures and has recovered successfully from real hardware-encoding failures.

This project evolved from a manual media-processing lab into a small fault-aware automation tool rather than a one-off collection of commands.

---

## Future Improvements

- Add an explicit manual title-selection mode for complex bonus/extras DVDs.
- Improve automatic classification of commentary versus primary audio tracks.
- Improve subtitle classification such as full subtitles versus forced/foreign-language-only streams.
- Add more detailed per-run logs and optional diagnostic log files.
- Continue testing uncommon DVD authoring structures and multi-angle behavior.
- Consider a separate TV-series workflow rather than forcing episode-based discs through the movie-oriented script.
- For TV DVDs, investigate automatic episode discovery, per-episode naming, episode-order verification, and safe multi-file output.
- Keep movie and TV automation separate if that produces clearer and safer behavior.
