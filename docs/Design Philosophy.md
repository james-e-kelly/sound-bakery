# Design Philosophy {#Design}

When writing a new audio middleware tool, it is almost impossible not to mention [Wwise](https://www.audiokinetic.com/en/products/wwise/) and [FMOD](https://fmod.com). They dominate the industry and have established well-designed patterns for adaptive audio. Sound Bakery takes heavy inspiration from both. The library also looks at Unreal Engine audio for inspiration. More and more businesses are asking if it right for them, due to the seamless integration with the engine and price point.

At its simplest, adaptive audio solves the problem of 'what sound(s) should I play for this game event?'. This question means an engine must track variable parameters from the game and use them to select one or more sounds. Sound Bakery creates this selection logic with a tree of nodes where each node can either be a sound or hold some logic for choosing a child node. Users can stack nodes to create more and more complex behavior. 

This design doesn't abstract away from the problem too much. For example, FMOD uses timelines and moves the play cursor based on conditions to play a different sound. This is a reasonable approach, and one Sound Bakery might integrate in the future, but not one that is treated as a priority. FMOD chose it deliberately to make the tool approachable for users coming from a DAW. Sound Bakery's approach (inherited from Wwise) might alienate a sound designer at first glance but becomes intuitive once learned.

Once sound selection is solved, an adaptive audio engine must package its audio for efficient performance at runtime. Uncompressed audio files can be incredibly large and bloat any install size. Moreover, audio is usually reserved only a small portion of the platform's memory, requiring audio to stay compressed in memory. Sound Bakery solves this problem with Soundbanks. The standard approach is to compress many audio files into one file to limit the number of calls to the storage device. Wwise has started to pack each event into a unique soundbank, ensuring only the audio required for the scene is loaded. Sound Bakery will explore this approach, but it will not be a priority.

With selection logic and Soundbanks, the core of Sound Bakery is complete. However, having selection logic and packaging sorted does not mean the tool is easy to use. How a user interacts with the tool through code and the visual interface is just as important. We will now discuss Sound Bakery's API and editor design.

## Interacting With Sound Bakery

Sound Bakery can be interacted with via an API and a visual authoring application. The API takes inspiration from FMOD, providing the user with a clean C (and higher level wrappers) API and minimal object types; the authoring application takes some inspiration from Wwise but also goes in a unique direction.

### Editor

Sound designers spend most of their time importing/modifying sounds, defining playback logic, creating events, and packaging soundbanks. Other features like profiling, mixing, and sandboxing are secondary. Sound Bakery makes the most essential features front and center; others are kept in the background until needed. Sound Bakery believes in keeping the editor clean with minimal visual noise.

In comparison, Wwise's default editor is dense. This is intimidating to first-time users. Wwise is the industry leader and it is built for professionals. Wwise's default layout opens to:
- Platform combo box
- Language combo box
- Profiling buttons
- Two search bars
- A project explorer with tabs for every object category
- Audio Devices, Master-Mixer hierarchy, Actor-Mixer hierarchy, and Interactive Music Hierarchy
- Event viewer
- Contextual help window
- Transport/play control
- Large output meter
- Selected object information with:
    - Volume, lowpass, highpass, pitch, and make-up gain values
    - Output bus selection with further tweakable properties
    - Game-defined sends
    - User-defined sends
    - Early reflections
    - Initial delay
    - Unique properties for the selected object
    - Contents or source window
    - Multiple tabs to tweak even more properties for the object

On top of this, Wwise contains seven other layouts and even more views/windows not shown by default on these layouts.

Sound Bakery takes a different approach: easy for first-time users while not removing anything from the offering. The fundamental purpose of the tool — choosing what sound to play for a game event — should be front and centre, with the deeper machinery a click away rather than always on screen. More contextual windows and less static views that fight for screen space.

Sound Bakery displays audio files, nodes, events, and soundbanks in its main explorer windows. These are the main focuses of the program and are kept prominent. Secondly, the play controls and object inspector window are displayed to provide the user with the most important controls and information.

Customization of the editor is crucial, and the user can add, remove, and dock windows to their liking. The user can make the editor their own and match Wwise or FMOD's layouts if desired.

Finally, visual design is also essential. While a program targeted at professionals needs to perform more than it needs to look pretty, visual design still needs to be considered. 

Unreal Engine 5's makeover is a clear example. UE5 keeps the same viewport, content browser, inspector, and more, yet its updated visuals make using the tool more enjoyable.

Sound Bakery aims for a clean and modern visual design that should be familiar and comfortable to users.

If you want to dive deeper into Sound Bakery's visual design, please refer to the @ref UserGuide.

### API

Software is complex, and when an API is also complex, working with it becomes more arduous than it needs to be. A clean and simple API can be just as pleasing as any visual tool. Sound Bakery fully believes in a clean, easy-to-use, yet powerful API.

FMOD's API is an excellent example of a great API. Using FMOD is extremely easy and requires minimal lines of code. For example, starting FMOD and playing a sound takes around four lines of code. This simplicity isn't to say FMOD is a simple tool or has minimal features; instead, FMOD does not expose complexity where it doesn't need to.

Wwise takes a different path. Its setup is more verbose and less is hidden. Starting Wwise requires the caller to initialise several modules by hand — most notably a memory manager and a stream manager. This feels like friction, and the documentation does little to soften it:

> It requires an instance of AK::StreamMgr::IAkFileLocationResolver, and creates a streaming device. This requires an instance of AK::StreamMgr::IAkLowLevelIOHook. This interface is defined in AkStreamMgrModule.h, which contains all definitions that are specific to the default Stream Manager implementation provided with the SDK.

However, this does not mean it is an API to ignore - it is the industry leader for a reason. Wwise is built on trust and gives confidence to the programmers integrating it into their engines. It is not meant to be initialized in a few lines to start playing sounds; it is built to work on the toughest of platforms (hardware-constrained consoles and mobile) and it needs to convince programmers to let audio folk add a bunch of `PostEvent` calls in the game without worrying about failing certification. Wwise refuses to hide the fact users need to take the integration seriously. 

Sound Bakery targets a different audience first — game jams and indies — and so makes the opposite default choice. It is built on a playback library called Sound Chef, which is itself built on miniaudio. Miniaudio creates a resource manager when its engine object is created, and that chain continues down to device initialisation. Sound Bakery hides this chain behind `sbk_system_create` and `sbk_system_init`, so a new project starts in a couple of lines.

Crucially, this is a default behaviour, not an admission that memory and I/O handling is not important to Sound Bakery. The same seams Wwise exposes — allocation, I/O, threading — remain available and swappable underneath, so Sound Bakery can grow toward the same console-grade control as it matures (see the @ref Roadmap) without forcing that complexity on someone writing their first game jam.

It is also important to note a significant advantage Sound Bakery has due to its open-source nature. While FMOD has an excellent API, it cannot expose anything more. In Sound Bakery, if the user desires, they can dig deeper than the public API and reach into internal objects and functions. This transparency means the user can modify and use Sound Bakery in more ways than publically declared. Moreover, users can extend Sound Bakery itself by modifying its source.

If you want to dive deeper into Sound Bakery's API, please refer to the @ref ProgrammerGuide.

## The Runtime

Everything above is about *authoring* audio. None of it matters if the engine that plays that audio cannot be trusted. Sound Bakery treats the runtime as a separate discipline from the tooling, held to a stricter standard.

The rule is simple. The editor is allowed to be rich, reflective, and allocation-happy, because it runs on a designer's workstation. The code that runs inside the audio callback of a shipping game is not. Below that real-time line, the runtime holds to a contract:

- **No allocations** on the audio thread — runtime memory is preallocated and bounded.
- **No locks** on the audio path — communication from the game to the runtime is lock-free.
- **Fast** — data-oriented hot paths designed for hundreds of concurrent voices.
- **Stable** — graceful degradation under load rather than glitches or stalls.
- **Memory-smart** — a predictable, budgetable footprint with no runtime surprises.

This is where Wwise's engineering earns its reputation, and Sound Bakery follows its lead. Wwise keeps the game thread off the audio thread by turning API calls into cheap commands that its own thread drains — the decoupling, not the audio thread doing everything, is the point. Sound Bakery adopts the same discipline: rich objects and coroutines above the real-time line, tight data passes below it, and a clean boundary between the two.

Not all of this is fully realised today (see the @ref Roadmap). It is the bar every release moves toward, and the one place the editor's convenience is never allowed to cross.

## Built for Scale and Teams

Matching Wwise and FMOD on their own terms is not enough to justify a new tool. Creating a random container, wiring up an event, and triggering it from the game is a solved problem — every serious middleware does it well. The problems that are *not* solved are the ones that appear at scale.

A designer rarely struggles to author a single sound. They struggle to manage tens of thousands of them in one project, and to keep a team aligned on quality across all of them. This is where Sound Bakery intends to lead rather than follow:

- **Scale.** Managing tens of thousands of assets is as much a database and search problem as an audio one. Fast, queryable, taggable browsing; project-wide analysis of loudness, peaks, and orphaned or unused assets; dependency graphs and safe bulk operations.
- **Collaboration.** Keeping a team aligned means bringing review to audio content the way it already exists for code — comparing a change before and after inside the editor, approving it, and keeping a history of who changed what. Documentation that lives in the project, beside the assets it describes, so intent is captured rather than lost.
- **Realism made easy.** Environmental audio — occlusion, distance, and reverb — should be powerful by default and approachable to set up, rather than a deep configuration exercise reserved for specialists.

These are deliberately the areas where the incumbents are weakest, and where an open, modern tool has the most room to win. How they land across releases is laid out in the @ref Roadmap.