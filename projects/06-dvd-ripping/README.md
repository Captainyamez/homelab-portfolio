# Project #6: DVD Ripping and Hardware-Accelerated Media Encoding on Proxmox

> **Status:** ✅ Working / Manually tested across multiple discs  
> **Purpose:** Build a dedicated media-ingestion container that can read personally owned DVDs, extract the main feature, encode it efficiently, and place the finished file into the Jellyfin media library  
> **Focus:** Proxmox/LXC, optical-device passthrough, Linux device access, DVD structure, FFmpeg, Intel VA-API, HEVC, audio/subtitle selection, troubleshooting, media verification

## Why I Chose This Project

Once Jellyfin was running, I needed a repeatable way to get media from discs I own into the library without turning the Jellyfin container itself into a general-purpose ripping workstation.

I decided to make the ripping workflow its own Proxmox LXC. That kept media ingestion separate from media serving and gave me a place to experiment with optical-drive passthrough, GPU access, FFmpeg, DVD navigation, subtitles, and encoding without changing the Jellyfin application container every time I tested something.

This project is still intentionally hands-on. I have not wrapped the process in a one-command automation script yet. For now, I am manually inspecting each disc, backing it up, identifying the correct title and streams, choosing an appropriate video filter, encoding it, verifying the result, and only then moving it into Jellyfin.

> **Public portfolio note:** This documentation is about the infrastructure and encoding workflow. It does not publish disc contents, decryption keys, credentials, private addresses, or other sensitive/environment-specific identifiers.

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
| Main tools | `dvdbackup`, FFmpeg/ffprobe, libdvdcss, VA-API, MakeMKV (tested), CCExtractor |

---

## What I Wanted to Accomplish

- Keep DVD ripping separate from the Jellyfin server itself.
- Pass the physical optical drive into an LXC container.
- Give the container access to the Intel integrated GPU for hardware-assisted HEVC encoding.
- Store temporary DVD backups on the large HDD rather than filling the container root filesystem.
- Write completed media directly into the existing Jellyfin storage hierarchy.
- Preserve the correct display aspect ratio rather than stretching video to fill a screen.
- Select the correct main audio track and preserve useful alternate-language audio when appropriate.
- Preserve subtitles when the workflow supports them correctly.
- Handle both progressive and interlaced DVD material.
- Verify runtime, codecs, aspect ratio, field order, language tags, and playback before considering a rip complete.
- Learn how to troubleshoot discs that do not behave like a clean single-file video source.

---

## Container and Device Design

The media-ingestion container is deliberately separate from CT 103, which runs Jellyfin.

Conceptually the design is:

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
                  + encoding      movies / tv / music
```

The container root filesystem stays relatively small because the large temporary DVD backup and final media files live on the dedicated HDD.

---

## Optical Drive Passthrough

Passing a physical DVD drive into an LXC was one of the first new parts of this project.

The drive needed more than the normal block-device path. The container was given access to both:

```text
/dev/sr0
/dev/sg0
```

The LXC configuration also needed matching device permissions so applications inside the container could communicate with the optical hardware.

I verified the device from inside the container before trying to build the ripping workflow around it.

This helped me understand that device passthrough is not just about making a path appear in a container. The device type, major/minor numbers, cgroup permissions, mount entry, and application access all have to line up.

---

## Intel GPU Passthrough and VA-API

The host's Intel UHD 630 is also exposed to the ripping container through `/dev/dri`.

I installed the Intel media driver and used `vainfo` to confirm that the container could initialize the GPU and that HEVC encoding was available.

The working hardware-encode path uses FFmpeg with the VA-API render device, for example:

```bash
-vaapi_device /dev/dri/renderD128
-c:v hevc_vaapi
```

For a normal progressive DVD source, the video filter is kept simple:

```text
format=nv12,hwupload
```

For material that is actually interlaced or mixed, I tested deinterlacing before hardware upload instead of blindly applying the same filter to every disc.

That distinction became important because DVD video is not always stored the same way even when the final playback looks similar on a television.

---

## The Workflow I Ended Up Trusting

My first assumption was that ripping a DVD would mostly be a matter of pointing an encoder at the disc and choosing the longest file.

The testing showed that DVD-Video is much more structured than that. VOB files, IFO navigation, titles, chapters, audio tracks, subtitles, angles, timestamps, and disc authoring choices all matter.

The workflow that proved the most reliable is now:

### 1. Inspect the disc

I start with `dvdbackup` in information mode to identify:

- Disc title
- Title sets
- Likely main feature
- Aspect ratio
- Number of angles
- Audio tracks
- Subtitle tracks
- Chapter count

### 2. Create a full DVD backup

Instead of encoding directly from the optical drive, I make a full mirror to the HDD first.

This gives the encoder a stable local copy and means the optical drive does not need to be continuously involved during the encoding stage.

### 3. Read the backup as DVD-Video

One of the biggest improvements was using FFmpeg's dedicated `dvdvideo` demuxer against the backed-up DVD directory.

That allowed FFmpeg to follow DVD navigation and expose proper titles, chapter markers, language-tagged audio, subtitles, runtime, and aspect-ratio information.

This was significantly more reliable than simply concatenating raw VOB files.

### 4. Inspect the streams with `ffprobe`

Before encoding, I check the selected title for:

- Runtime
- Video format
- Frame rate
- Progressive/interlaced state
- Sample and display aspect ratio
- Audio languages and channel layouts
- Subtitle languages

I do not assume that stream 0 is always the audio track I want.

### 5. Choose the video path

For progressive content, I normally use Intel VA-API HEVC encoding directly.

For interlaced or mixed sources, I use a deinterlacing filter before handing the frames to VA-API.

I tested inverse-telecine approaches as well, but one full encode demonstrated that a short sample can look correct while the complete title develops timestamp problems. Because of that, I now favor the safer full-runtime result unless I have verified the entire pipeline.

### 6. Copy audio and subtitles when practical

DVD AC3 audio can usually be copied rather than re-encoded.

Where a disc contains useful alternate audio, such as original-language and English tracks, both can be retained in the MKV and tagged by language.

### 7. Verify before moving into Jellyfin

After encoding, I use `ffprobe` to confirm the final file before it is added to the library.

I check:

- Codec
- Resolution
- Aspect ratio
- Field order
- Frame rate
- Audio channels
- Language tags
- Subtitle presence
- Runtime
- File size

I then test actual playback through Jellyfin instead of treating successful FFmpeg exit status as the only acceptance test.

---

## Troubleshooting: Why Raw VOB Concatenation Was Not Enough

One of the early discs did not behave correctly with the first tools I tried.

A direct MakeMKV attempt failed, and concatenating the movie's VOB files produced broken timing behavior.

That was a useful lesson because the VOB files contained the video data, but the DVD navigation information was still important for reconstructing the title correctly.

Using FFmpeg's `dvdvideo` demuxer against the full DVD backup solved the problem by reading the title through the DVD structure instead of treating the VOBs as unrelated generic MPEG files.

This became the default approach for the project.

---

## Troubleshooting: Progressive, Interlaced, and Telecined Material

Different discs reported different video characteristics.

Some newer DVDs were clean progressive sources and did not need deinterlacing.

Other discs contained interlaced or mixed material. For those, I tested decoded frames with FFmpeg's field-detection tools and used deinterlacing when appropriate.

At one point a short inverse-telecine test looked excellent and produced a clean 23.976 fps sample. However, the complete encode developed timestamp discontinuities and ended significantly shorter than the actual movie.

I discarded that result and kept the complete, safer encode instead.

That changed the way I test video filters: a successful two-minute sample is useful, but it is not proof that an entire DVD title will survive the same processing chain.

---

## Troubleshooting: Mid-Stream Video Parameter Change

One of the strongest troubleshooting cases came from a newer 16:9 progressive DVD.

The normal Intel VA-API encode ran successfully for more than 80 minutes and then failed at the same point every time.

FFmpeg reported that the decoded video parameters changed mid-stream, including a color-metadata change to `smpte170m`. The VA-API filter graph could not reinitialize across that transition.

I first tried normalizing the frames with a software scale operation before uploading them to the GPU. A short test across the problem area succeeded, but the full encode still failed at exactly the same location.

I then switched to software HEVC encoding with `libx265`. The encoder itself handled the format change, but the full pass still stopped at the same timestamp.

Instead of assuming the disc was unreadable, I tested the final several minutes as a separate segment. That segment encoded successfully and proved the ending was intact.

The workaround was:

1. Encode the first portion up to several seconds before the transition.
2. Start a second encode after that cut point and run it to the end.
3. Verify both segment runtimes.
4. Concatenate the two compatible encoded segments without another video encode.
5. Verify the final runtime against the source title.
6. Test playback in Jellyfin.

The resulting file was within about one second of the source title's runtime and played correctly.

That failure ended up being more educational than a normal successful encode because it forced me to isolate whether the problem was the physical disc, DVD navigation, GPU encoder, software encoder, filter graph, or the particular point in the stream.

---

## What I Tested

I deliberately used multiple DVDs rather than deciding the project worked after one successful disc.

The test set included a mix of:

- Older and newer DVD releases
- 4:3 and 16:9 content
- Progressive sources
- Interlaced/mixed sources
- Stereo and 5.1 AC3 audio
- Multiple spoken-language tracks
- Multiple subtitle tracks
- Multi-angle DVD structure
- Discs with unusual navigation/timestamp behavior
- Hardware HEVC encoding
- Software HEVC fallback
- Split-encode and concat recovery

Completed files were checked in Jellyfin rather than only inspected from the command line.

---

## Verification

### Container and storage

- The dedicated Debian LXC runs independently of Jellyfin.
- `/work` maps to bulk HDD storage for temporary DVD data and encodes.
- `/media` maps to the Jellyfin media hierarchy on the same bulk-storage disk.

### Optical drive

- The physical DVD drive is visible inside the container.
- Both the block-device and SCSI-generic interfaces are available where required.
- `dvdbackup` can inspect and mirror supported discs to local storage.

### GPU

- The Intel render device is visible inside the container.
- `vainfo` initializes the Intel media driver.
- FFmpeg can perform HEVC encoding through VA-API.

### Media output

Across the tested discs I verified combinations of:

- HEVC 720x480 video
- Correct 4:3 or 16:9 display aspect ratio
- Progressive output where appropriate
- English stereo and 5.1 AC3
- Multiple audio languages in one MKV
- DVD subtitle streams
- Full movie runtime
- Jellyfin playback on a real client

---

## What I Learned

This project gave me hands-on experience with:

- Proxmox LXC device passthrough
- Linux block and character devices
- LXC cgroup device permissions
- Bind mounts between the Proxmox host and containers
- DVD-Video title/VTS/chapter structure
- CSS-capable DVD access tools
- FFmpeg and `ffprobe`
- FFmpeg's `dvdvideo` demuxer
- Intel VA-API and HEVC hardware encoding
- Software HEVC fallback with `libx265`
- Progressive vs interlaced video
- Field detection and deinterlacing
- Aspect-ratio preservation
- Audio stream selection and language tagging
- Subtitle stream handling
- Diagnosing timestamp and filter-graph failures
- Segmenting and concatenating video as a recovery technique
- Verifying output at both the file and application layers

The biggest lesson was that media ingestion is not one operation. Reading the physical disc, understanding DVD structure, decoding video correctly, choosing streams, encoding, storing the result, and validating playback are separate stages that can fail independently.

---

## Result

I now have a dedicated Proxmox LXC that can ingest personally owned DVDs into the same bulk-storage hierarchy used by Jellyfin.

The working process mirrors a disc to the HDD, reads the backup using DVD-aware navigation, inspects the title and streams, encodes the video to HEVC using Intel VA-API when the source allows it, preserves selected AC3 audio/subtitles, verifies the finished MKV, and places the result into the Jellyfin movie library.

The workflow has been tested across several discs with different aspect ratios, audio layouts, language tracks, field structures, and authoring quirks. I also documented failure cases where the normal hardware pipeline was not sufficient and a software or segmented fallback was required.

It is intentionally still a manual workflow. I want the underlying process to remain understandable before I turn it into automation.

---

## Future Improvements

- Create a safe wrapper such as `ripdvd "Movie Title" YEAR` after the manual workflow is stable enough to automate
- Automatically identify likely main features while still allowing manual confirmation
- Add safer automatic selection of English/original-language audio tracks
- Improve subtitle classification and preservation
- Add structured logging for each rip and encode
- Automatically run `ffprobe` validation and reject obviously truncated output
- Add cleanup logic for temporary DVD backups only after final verification
- Add duplicate/file-exists protection before writing into the Jellyfin library
- Test additional discs to expand the progressive/interlaced edge-case coverage
- Document the final automation separately once it exists rather than claiming it is automated today
