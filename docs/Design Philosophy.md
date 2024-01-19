# Inspiration {#Inspiration}
Game audio is dominated by Wwise and FMOD. Both are amazing tools and cater to different markets. Having learnt on FMOD and used Wwise professionaly in the AAA industry, I can see both their strengths and weaknesses. With this in mind, I feel there is a gap in the market for an open-source alternative.

It's near impossible for Sound Bakery to add more features than Wwise or FMOD but I do believe it can lay a better foundation for future development. For example, FMOD has the best API and is emulated in Sound Bakery but if you want to modify the code, you need a development budget over $1.5 and an extra $15k to gain access to the source. With Sound Bakery, if you want a new feature in the program, you can write it yourself. Even visuals and widgets can be tweaked easily without the need for something like WAAPI or a Javascript interface.

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