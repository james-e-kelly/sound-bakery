# Design Philosophy {#Design}

When writing a new audio middleware tool, it is almost impossible not to mention [Wwise](https://www.audiokinetic.com/en/products/wwise/) and [FMOD](https://fmod.com). They dominate the industry and have established well-designed patterns for adaptive audio. Sound Bakery takes heavy inspiration from both. While there are alternatives like [SoLoud](https://solhsa.com/soloud/) and in-engine tools like [Unreal's](https://docs.unrealengine.com/5.2/en-US/working-with-audio-in-unreal-engine/), there are downsides to their approaches that Sound Bakery hopes to avoid.

At its simplest, adaptive audio solves the problem of 'what sound(s) should I play for this game event?'. This question means an engine must track variable parameters from the game and use them to select one or more sounds. Sound Bakery creates this selection logic with a tree of nodes where each node can either be a sound or hold some logic for choosing a child node. Users can stack nodes to create more and more complex behavior. 

This design doesn't abstract away from the problem too much. For example, FMOD uses timelines and moves the play cursor based on conditions to play a different sound. This approach is acceptable but slightly cumbersome, adding an extra layer of abstraction that isn't needed. FMOD chose this approach to make the tool approachable for users from a DAW. Sound Bakery's approach (inherited from Wwise) might alienate a sound designer at first glance but becomes intuitive once learned.

Once sound selection is solved, an adaptive audio engine must package its audio for efficient performance at runtime. Uncompressed audio files can be incredibly large and bloat any install size. Moreover, audio is usually reserved only a small portion of the platform's memory, requiring audio to stay compressed in memory. Sound Bakery solves this problem with Soundbanks. The standard approach is to compress many audio files into one file to limit the number of calls to the storage device. Wwise has started to pack each event into a unique soundbank, ensuring only the audio required for the scene is loaded. Sound Bakery will explore this approach, but it will not be a priority.

With selection logic and Soundbanks, the core of Sound Bakery is complete. However, having selection logic and packaging sorted does not mean the tool is easy to use. How a user interacts with the tool through code and the visual interface is just as important. We will now discuss Sound Bakery's API and editor design.

## Interacting With Sound Bakery

Sound Bakery can be interacted with via an API and a visual authoring application. The API takes inspiration from FMOD, providing the user with a clean C (and higher level wrappers) API and minimal object types; the authoring application takes some inspiration from Wwise but also goes in a unique direction.

### Editor

Sound designers spend most of their time importing/modifying sounds, defining playback logic, creating events, and packaging soundbanks. Other features like profiling, mixing, and sandboxing are secondary. Sound Bakery makes the most essential features front and center; others are kept in the background until needed. Sound Bakery believes in keeping the editor clean with minimal visual noise.

Wwise is almost infamous for its busy editor. While the user can modify this, Wwise's default layout opens to:
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

This noise is something Sound Bakery aims to avoid. While a complex program should not shy away from its complexity, when the fundamental purpose of the tool is to select an audio file from a game event, it is better to keep the program focussed.

Sound Bakery displays audio files, nodes, events, and soundbanks in its main explorer windows. These are the main focuses of the program and are kept prominent. Secondly, the play controls and object inspector window are displayed to provide the user with the most important controls and information.

Customization of the editor is crucial, and the user can add, remove, and dock windows to their liking. The user can make the editor their own and match Wwise or FMOD's layouts if desired.

Finally, visual design is also essential. While a program targeted at professionals needs to perform more than it needs to look pretty, visual design still needs to be considered. 

Unreal Engine 5's makeover is a clear example. UE5 keeps the same viewport, content browser, inspector, and more, yet its updated visuals make using the tool more enjoyable.

Sound Bakery aims for a clean and modern visual design that should be familiar and comfortable to users.

If you want to dive deeper into Sound Bakery's visual design, please refer to the @ref UserGuide.

### API

Software is complex, and when an API is also complex, working with it becomes more arduous than it needs to be. A clean and simple API can be just as pleasing as any visual tool. Sound Bakery fully believes in a clean, easy-to-use, yet powerful API.

FMOD's API is an excellent example of a great API. Using FMOD is extremely easy and requires minimal lines of code. For example, starting FMOD and playing a sound takes around four lines of code. This simplicity isn't to say FMOD is a simple tool or has minimal features; instead, FMOD does not expose complexity where it doesn't need to.

On the other hand, Wwise's API is large and unwieldy. Starting Wwise requires the initialization of six different modules. For example, read this passage for initializing the streaming manager:

> It requires an instance of AK::StreamMgr::IAkFileLocationResolver, and creates a streaming device. This requires an instance of AK::StreamMgr::IAkLowLevelIOHook. This interface is defined in AkStreamMgrModule.h, which contains all definitions that are specific to the default Stream Manager implementation provided with the SDK.

API design like this causes friction. Sound Bakery also has complex classes and patterns of behavior but doesn't show this to the user unless needed. For example, Sound Bakery is built on a playback library called Sound Chef. Sound Chef is itself built on miniaudio. Miniaudio creates a resource manager when its engine object is created. This chain continues to device initialization. Sound Bakery hides this complexity behind simple functions like `SB_System_Init`.

It is also important to note a significant advantage Sound Bakery has due to its open-source nature. While FMOD has an excellent API, it cannot expose anything more. In Sound Bakery, if the user desires, they can dig deeper than the public API and reach into internal objects and functions. This transparency means the user can modify and use Sound Bakery in more ways than publically declared. Moreover, users can extend Sound Bakery itself by modifying its source.

If you want to dive deeper into Sound Bakery's API, please refer to the @ref ProgrammerGuide.