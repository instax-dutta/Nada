# NADA ARCHITECTURE BLUEPRINT

## Purpose

This document defines the target architecture for Nada Beta.

The primary goal is separation of concerns.

Current issues likely originate from:

- UI coupled with playback state
- Queue logic spread across views
- Background workers not centrally managed
- Lifecycle shutdown issues
- Resource ownership ambiguity

---

# TARGET SYSTEM

UI Layer
↓
ViewModels
↓
Application Services
↓
Core Engine
↓
Storage + Audio Backend

---

# MODULES

## Core Engine

PlaybackEngine

Responsibilities:

- Playback
- Seeking
- Pause
- Resume
- Output device management
- Playback state machine

Must NOT:

- Access UI
- Access Widgets

---

## QueueManager

Single source of truth.

Responsibilities:

- Current queue
- Queue order
- Shuffle
- Repeat
- Context generation

Must own:

- Current track index
- Queue state

No queue logic inside views.

---

## LibraryManager

Responsibilities:

- Scanning
- Metadata
- Database access
- Artwork discovery

Must not:

- Block UI thread

---

## ArtworkManager

Responsibilities:

- Artwork extraction
- Thumbnail generation
- Disk cache
- Memory cache

Requirements:

- Async only
- LRU cache

---

## WaveformManager

Responsibilities:

- Waveform generation
- Persistent cache

Must never block playback.

---

## DSPManager

Responsibilities:

- EQ
- ReplayGain
- Crossfeed
- Future DSP modules

---

# UI LAYER

Views should contain no business logic.

Views:

- LibraryView
- AlbumView
- ArtistView
- QueueView
- SettingsView
- NowPlayingView

Views only:

- Display state
- Forward events

---

# STATE FLOW

User Action
↓
View
↓
ViewModel
↓
Manager
↓
Engine
↓
State Update
↓
ViewModel
↓
View

No direct View → Engine access.

---

# THREADING RULES

Main Thread:

- UI

Worker Threads:

- Scanning
- Artwork extraction
- Waveform generation
- Metadata loading

Forbidden:

- Updating widgets from worker threads

---

# SHUTDOWN SEQUENCE

1. Stop scans
2. Stop artwork workers
3. Stop waveform workers
4. Stop playback
5. Flush database
6. Destroy UI

Shutdown must be deterministic.

---

# FUTURE EXPANSION

Planned modules:

- MusicBrainzService
- LastFmService
- ListenBrainzService
- RemoteControlService

These must remain optional services.
