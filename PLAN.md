# NADA MVP RECOVERY PLAN

## Objective

Transform Nada from a technically impressive prototype into a stable, usable beta music player.

Success criteria:

- No unexpected crashes during normal usage.
- Stable playback for large music libraries.
- Responsive UI.
- Intuitive navigation.
- Album-focused music browsing.
- Cross-session reliability.
- Ready for public beta testing.

---

# PHASE 0 — PROJECT AUDIT (MANDATORY)

Before implementing new features:

1. Build project in Debug mode.
2. Enable Address Sanitizer.
3. Enable Undefined Behavior Sanitizer.
4. Enable Qt debug logging.
5. Fix ALL crashes before implementing UI changes.

Required build flags:

- -fsanitize=address
- -fsanitize=undefined
- -fno-omit-frame-pointer

Deliverables:

- Crash report document.
- Memory leak report.
- Startup profiling report.
- UI responsiveness report.

Exit criteria:

- Zero sanitizer errors.
- Zero reproducible crashes.

---

# PHASE 1 — STABILIZATION

## 1. Library Loading

Current issue:

Entire library appears to be loaded eagerly.

Requirements:

- Background loading.
- Incremental loading.
- Progress reporting.
- Cancel scan support.

Implementation:

- Paginated model.
- Lazy fetch metadata.
- Lazy artwork extraction.
- Background thumbnail generation.

Target:

100,000 tracks must remain responsive.

---

## 2. Database Layer

Requirements:

- Add indexes.
- Avoid duplicate scans.
- Faster startup.

Required indexes:

- path
- album
- artist
- genre
- title

Add migration system.

Target startup:

<2 seconds with existing library.

---

## 3. Playback Stability

Requirements:

Audit:

- Queue handling
- Track switching
- Seeking
- Gapless transitions
- Engine shutdown
- Application shutdown

Prevent:

- Dangling pointers
- Invalid queue index access
- Race conditions
- Double deletion

Target:

24-hour playback stress test without crash.

---

## 4. Thread Safety

Audit:

- QtConcurrent
- FutureWatcher
- Library scanning
- Waveform generation
- Artwork generation

Rules:

- UI thread must never block.
- UI updates only from main thread.

---

# PHASE 2 — COMPLETE UI REWORK

Current UI Problems:

- Permanent right panel wastes space.
- Sidebar is too minimal.
- Album discovery is missing.
- Search is hidden.
- Layout feels like an audio tool.

Goal:

Modern desktop music player.

---

## New Layout

Replace:

Sidebar + Content + Fixed Right Panel

With:

Sidebar + Main Content
Bottom Playback Bar
Optional Now Playing Drawer

Layout:

[Sidebar] [Main Content]

--------------------------
Bottom Playback Controls
--------------------------

Now Playing opens as:

- Slide-out panel
OR
- Overlay drawer

---

## Sidebar

Add:

- Library
- Albums
- Artists
- Genres
- Queue
- DSP
- Settings

Support:

- Expanded mode
- Compact mode

---

## Search

Permanent global search.

Top of window.

Placeholder:

Search songs, albums, artists...

Requirements:

- Instant filtering
- Fuzzy search
- Keyboard focus shortcut

---

## Album View

Highest priority feature.

Grid layout.

Display:

- Cover art
- Album title
- Artist
- Year

Features:

- Sort
- Filter
- Open album
- Play album
- Queue album

---

## Artist View

Display:

- Artist image if available
- Albums grouped by artist

---

## Queue View

Requirements:

- Drag reorder
- Remove tracks
- Save queue
- Clear queue

Current behavior where selecting a track clears queue should be removed.

Expected behavior:

Selecting a track creates contextual queue.

Example:

Album track 5 selected.

Queue becomes:

5
6
7
8
...

---

# PHASE 3 — NOW PLAYING EXPERIENCE

## Bottom Bar

Must contain:

- Play
- Pause
- Next
- Previous
- Shuffle
- Repeat
- Timeline
- Volume

---

## Metadata

Display:

- Track title
- Album
- Artist
- Cover art

---

## Audiophile Stats

Optional section.

Display:

- Codec
- Sample rate
- Bit depth
- Bitrate
- Output device

---

## Signal Chain View

Display:

Source
↓
ReplayGain
↓
EQ
↓
Crossfeed
↓
Output

---

# PHASE 4 — PERFORMANCE

## Artwork Cache

Requirements:

- Async generation
- Persistent disk cache
- Memory limit
- LRU eviction

---

## Waveforms

Requirements:

- Generate once
- Cache forever
- Background processing

Never block playback.

---

## Model Optimization

Replace full table refreshes.

Use:

- Incremental updates
- View virtualization
- Batched signals

---

# PHASE 5 — BETA FEATURES

## Smart Playlists

Examples:

- Recently added
- Most played
- Never played
- Favorites

---

## Metadata Editing

Support:

- Title
- Artist
- Album
- Genre
- Cover art

---

## MusicBrainz Integration

Automatic metadata lookup.

---

# PHASE 6 — TESTING

Mandatory automated tests.

Coverage:

## Library

- Scan
- Rescan
- Remove files

## Queue

- Add
- Remove
- Reorder

## Playback

- Play
- Pause
- Resume
- Seek
- Next
- Previous

## DSP

- EQ
- ReplayGain
- Crossfeed

## UI

Smoke tests.

---

# RELEASE CHECKLIST

Application may be called Beta only when:

[ ] No sanitizer errors
[ ] No reproducible crashes
[ ] No memory leaks
[ ] Album view complete
[ ] Artists view complete
[ ] Search complete
[ ] Queue behavior fixed
[ ] Bottom playback bar implemented
[ ] Library loads asynchronously
[ ] Artwork cache working
[ ] Waveform cache working
[ ] 100k track stress test passes
[ ] 24 hour playback stress test passes
[ ] Test suite passing

---

# NON-GOALS FOR MVP

Do NOT spend time on:

- Android remote
- DSD support
- Social features
- Streaming services
- Visualizers
- Fancy animations

Priority order:

Stability > Performance > Usability > Features > Eye Candy
