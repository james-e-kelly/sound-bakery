# Roadmap {#Roadmap}

Sound Bakery's long-term goal is to be a competitive, open alternative to Wwise
and FMOD. When a team is starting a project and choosing an audio tool, Sound Bakery should be a viable choice. But there is no specific feature or version number that this roadmap can point to for when that happens. Instead, we mark the road with goals of who can trust us.

As we go through major versions, the tool should be trustable to more and more people, and be able to tackle harder and harder problems. We start with the hobbyists and jammers, then work our work up to small titles, then indies, AA, and finally consoles and AAA.

> **Versioning note.** Sound Bakery is pre-1.0. Anything may change, including
> the project and soundbank serialization formats. We will document breaking
> changes, but do not treat pre-1.0 releases as stable to build a shipping title
> on. Within each milestone below there are many smaller minor and patch
> releases.

## The runtime contract

Every milestone is built on the same foundation: the authoring tools are free to inovate and push the boundaries, but once the game is live and in the hands of users, it must hold up. That means:

- **No allocations** on the audio thread — runtime memory is preallocated and
  bounded.
- **No locks** on the audio path — game-to-runtime communication is lock-free.
- **Fast** — data-oriented hot paths designed for hundreds of concurrent voices.
- **Stable** — graceful degradation under load; never a glitch, never a stall.
- **Memory-smart** — predictable, budgetable footprint with no runtime surprises.

---

## 0.1.0 — MVP

> **Ready for:** internal use, experiments, and learning. The first version that
> provides a complete game audio loop — author playback logic in an editor,
> package it, and play it back in a consuming application.

- Audio playback
- Editor application
- Adaptive logic
    - Parameters
    - Audio nodes
    - Events
- Packaging
    - Encoding
    - Soundbanks
- Basic runtime

## 0.2.0 – 0.9.0 — Game Jam Ready

> **Ready for:** game jams, prototypes, and hobby projects — anything where money is not at stake. 
> The tool is usable, but users might encounter a few rough edges.

- Runtime hardening, pass one
    - Allocation-free mix callback
    - Lock-free game-to-runtime boundary
- Voice management
    - Virtualisation and graceful voice limiting under load
- Serialization stable enough to survive a project's lifetime (breaking changes
  documented)
- At least one engine integration — a real path to hearing audio in a game
- Editor stability and core UX polish
- Basic metering and profiling

## 1.0.0 — Indie ready

> **Ready for:** shipping a small commercial title, solo or as a small team.
> At this point, Sound Bakery is reliable and performant. 
> Not every feature exists, but you can trust the tool to do its job.

- **Hardened runtime** — the runtime contract above, held and verified: no
  allocations, no locks, stable under load, budgetable memory
- **Stable serialization** — a versioned project and soundbank format with a
  migration path; no silent breaks
- Core adaptive audio, plus foundational interactive music
- One or two first-class engine integrations (UE5, Unity, or Godot)
- Profiling tools
    - CPU insights
    - Memory insights
    - Voice graph
    - Metering
- Encoding formats (ADPCM, Vorbis, Opus)
- One language binding (C#)
- Source-control-friendly project format — mergeable and diffable in Git
- Documentation and onboarding: time-to-first-sound measured in minutes

## 2.0.0 — AA ready

> **Ready for:** mid-size teams and AA productions. 
> This is where Sound Bakery's unique features start to shine.
> The tool is reliable and has shipped games.
> Now, it starts becoming a viable option for bigger projects.

- **Scale** — project management for tens of thousands of assets
    - Fast searchable, taggable browser with saved smart views
    - First-class metadata and tagging
    - Project-wide analysis: loudness, true peak, clipping, orphans, dead assets
    - Dependency graphs and safe bulk operations
- **Collaboration** — keeping teams aligned on quality
    - In-editor review: A/B before-and-after changes, comment, approve
    - Change history and blame for audio objects
    - Documentation that lives in the project, next to the assets it describes
- Full interactive music system
- **Environmental audio made easy**
    - Occlusion and obstruction with strong defaults and progressive disclosure
    - Perceptual distance modelling (transient softening, air absorption)
    - Reverb zones and portals
- Remote profiling — connect the authoring tool to a running game
- Additional engine integrations
- Additional language bindings (Python, Rust)
- CI-friendly headless tooling and quality gates
- Auxiliary outputs and advanced mixing

## 3.0.0+ — Console & AAA

> **Ready for:** console shipping and AAA-scale productions.
> Not so much a specific feature, but a marker that Sound Bakery is a tool that can be trusted to tackle any challenge.
> Sound Bakery has clear reasons to use it, teams have used it for years to build smaller projects, and it is giving businesses a reason to consider it.

- Controlled, replaceable core libraries (reflection, allocator, threading, I/O)
  behind stable seams
- Cert-readiness: deterministic memory budgets, suspend/resume, platform I/O,
  long-soak reliability
- **Console platform backends** delivered via private, access-controlled repos
  under NDA, implementing the public backend interface — the public core stays
  clean and open
- Deep spatial audio: geometry, diffraction, and both baked and runtime
  simulation
