# Inspiration
Sound Bakery sees the amazing things Wwise and FMOD are doing but also sees their limitations.

Wwise has powerful features but is held back by its age. The authoring application is Windows only and runs through an emulator on Mac, its graphics are dated and off-putting to newcomers, and its API is a little unwieldy.

FMOD has a wonderful API and a friendly authoring application but fails to keep up with Wwise's next-gen features like ambisonics. Though, in recent versions, support for controller haptics has arrived.

# Philosophy
Given the above, it's possible to see how powerful an engine would be if it combined the best parts of Wwise and FMOD.

Sound Bakery aims to be that engine. To explain how, it's easiest to break up the library into three parts: features, editor, and integration.

## Features
At the most basic, an adaptive audio engine needs an abstraction over audio files so it can play one or many based on parameters from the game.

Sound Bakery takes inspiration from Wwise and its database approach. While less approachable than linear tracks with audio placed upon them, it is far more powerful. Moreover, game audio is inherently non-linear and tools should reflect this.

However, while Wwise's database approach is powerful, limitations have been placed on it. For example, aux sends and effects number limits. This is not copied in Sound Bakery.

## Editor

## Integrations
An audio engine is only as good as its integration with a game engine. Wwise is a poor example for integrating with an engine.

To integrate Wwise with Unreal, for example, a custom launcher must be used that downloads and adds thousands of files (mainly plugins) to the engine.

Secondly, the Wwise integration has a bloated interface with a million-and-one ways to post an event. Modifying Wwise to suit a game's needs can be very cumbersome.

Conversely, FMOD has great integrations and has generally been ahead of Wwise for many years. I vividly remember my confusion when Wwise didn't auto-import events to Unreal like FMOD did.

Sound Bakery aims to emulate FMOD's success in this regard and provide an easy to use API and minimal files for linking to the engine.