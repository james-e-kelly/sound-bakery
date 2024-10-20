<h1>Sound Bakery</h1>

| Windows | MacOS | Linux | License | C++<br>Standard | Version |
| ------- | ----- | ----- | ------- | --------------- | ------- |
| [![Windows](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml) | [![macOS](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml) |[![Linux](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml)  | [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT) | [![GitHub license](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/compiler_support#cpp20) | ![WIP](https://img.shields.io/badge/Status-WIP-yellow) |

<div align="center">
    <a href="#why-choose-sound-bakery">Why?</a> • <a href="#goal">Goal</a> • <a href="#getting-started">Getting Started</a> • <a href="#contributing">Join</a>
</div>
<p></p>

Sound Bakery is a free and open-source audio middleware tool. The tool comes packaged with an authoring application and C/C++ API for integration with game engines. It supports the entire audio pipeline, from asset management, sound selection, packaging, playback and debugging.

![](docs/img/sound-bakery-wip-02.png)

## Why Choose Sound Bakery?

From Blender and Krita to Godot, more and more developers are choosing open-source alternatives for their projects. Sound Bakery is the open choice for game audio. 

| Effective | Modern | Open |
| --- | --- | --- |
| With a full authoring application, powerful API, and multithreading built in, Sound Bakery is a competitive choice. | Sound Bakery is a tool for the modern era - Github hosting with CI, C++ 20, CMake, modern libraries and more. | Don't be restricted by cost or license. Sound Bakery is free, modifiable and MIT licensed. |

## Goal

Sound Bakery aims to be a competitive option to Wwise and FMOD that is more collaborative and open. Imagine owning your audio engine and being able to customise its look, tools, behaviour, and more. Imagine receiving features and fixes from top studios, all collectively improving the tools of the industry.

Check out the [roadmap](docs/Roadmap.md) for where Sound Bakery is going!

## Getting Started

Looking for binaries? Check out the [releases](https://github.com/KarateKidzz/sound-bakery/releases) page for prebuilt binaries and source code. Otherwise, see how to build from source.

### Prerequisites
Ensure the following are installed:

- A C++ compiler (e.g., GCC, Clang, MSVC)
- CMake (version 3.28 or higher)
- Git

### Installation
Clone the repository and build the project:

```
git clone https://github.com/KarateKidzz/sound-bakery.git
cd sound-bakery
mkdir build
cd build
cmake ..
make
```

### Usage
Here's a simple example to get started with Sound Bakery:

```cpp
#include "sound_chef.h"

int main() {
    sc_system* system = NULL;
    sc_system_create(&system);
    sc_system_init(system);
 
    sc_sound* sound = NULL;
    sc_system_create_sound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 
    sc_sound_instance* instance = NULL;
    sc_system_play_sound(system, sound, &instance, NULL, SC_FALSE);

    return 0;
}
```

### Documentation

For documentation and guides, visit [soundbakery.jameskelly.audio](https://soundbakery.jameskelly.audio).

## Contributing
Sound Bakery needs **you**! From UI/UX artists to DSP programmers, the project needs yours skills.

Found a bug or want to request a feature? Open an [issue](https://github.com/KarateKidzz/sound-bakery/issues).

Want to talk about the project? Start a [discussion](https://github.com/KarateKidzz/sound-bakery/discussions).

Ready to make a change? Create a [fork](https://github.com/KarateKidzz/sound-bakery/fork).

All contributions are welcome!

For more details, check out our [contributing guidelines](CONTRIBUTING.md).

## License
Sound Bakery is licensed under the MIT License. See the LICENSE file for more information.

## Acknowledgements
A big thank you to the creators of miniaudio, concurrencpp, ImGui, rttr, tracy, and spdlog for their incredible libraries that make Sound Bakery possible.