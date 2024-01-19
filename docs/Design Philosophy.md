# Design Philosophy {#Design}

When writing a new audio middleware tool, it is almost impossible not to mention [Wwise](https://www.audiokinetic.com/en/products/wwise/) and [FMOD](https://fmod.com). They dominate the industry and have established well-designed patterns for adaptive audio. Sound Bakery takes heavy inspiration from both. While there are alternatives like [SoLoud](https://solhsa.com/soloud/) and in-engine tools like [Unreal's](https://docs.unrealengine.com/5.2/en-US/working-with-audio-in-unreal-engine/), there are downsides to their approaches that Sound Bakery hopes to avoid.

At its simplest, adaptive audio solves the problem of 'what sound(s) should I play for this game event?'. This question means an engine must track variable parameters from the game and use them to select one or more sounds. Sound Bakery creates this selection logic with a tree of nodes where each node can either be a sound or hold some logic for choosing a child node. Users can stack nodes to create more and more complex behavior. 

This design doesn't abstract away from the problem too much. For example, FMOD uses timelines and moves the play cursor based on conditions to play a different sound. This approach is acceptable but slightly cumbersome, adding an extra layer of abstraction that isn't needed. FMOD chose this approach to make the tool approachable for users from a DAW. Sound Bakery's approach (inherited from Wwise) might alienate a sound designer at first glance but becomes intuitive once learned.

Once sound selection is solved, an adaptive audio engine must package its audio for efficient performance at runtime. Uncompressed audio files can be incredibly large and bloat any install size. Moreover, audio is usually reserved only a small portion of the platform's memory, requiring audio to stay compressed in memory. Sound Bakery solves this problem with Soundbanks. The standard approach is to compress many audio files into one file to limit the number of calls to the storage device. Wwise has started to pack each event into a unique soundbank, ensuring only the audio required for the scene is loaded. Sound Bakery will explore this approach, but it will not be a priority.

With selection logic and Soundbanks, the core of Sound Bakery is complete.

## Low-Level Audio Playback

How an audio engine sends its audio to the user is essential. Sound Bakery uses the well-regarded node graph approach, where an output node iterates over its inputs to create output. Check out miniaudio for the library that implements Sound Bakery's node graph.