<h1>Sound Bakery</h1>

| Windows | MacOS | Linux | License | C++<br>Standard | Version |
| ------- | ----- | ----- | ------- | --------------- | ------- |
| [![windows](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml) | [![macos](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml) |[![linux](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml)  | [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT) | [![GitHub license](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/compiler_support#cpp20) | ![WIP](https://img.shields.io/badge/Status-WIP-yellow) |

<div align="center">
    <a href="#why-choose-sound-bakery">Why?</a> • <a href="#goal">Goal</a> • <a href="#getting-started">Getting Started</a> • <a href="#contributing">Contributing</a>
</div>
<p></p>

Sound Bakery is a free and open-source event-based audio middleware tool. The tool comes packaged with an authoring application and C/C++ API for integration with game engines. It supports the entire audio pipeline, from asset management, sound selection, and packaging to playback and debugging.

## Why Choose Sound Bakery?

From Blender and Krita to Godot, more and more developers are choosing open-source alternatives for their projects. Sound Bakery is the open choice for game audio. 

The options for audio developers have always been restricted. The choice has been between expensive tools, engine tools, or free software that lacks essential features. That is not the case with Sound Bakery - it is fully featured from an authoring application to a robust API for integration.

| Effective | Modern | Open |
| --- | --- | --- |
| Sound Bakery provides a powerful authoring application and clean API that is as powerful as any commercial tool. It is built with multithreading and performance in mind. | Instead of building atop decades-old codebases, Sound Bakery can start anew and use modern language features, libraries and development techniques. | Tired of spending thousands on your middleware tool just for it to ask for more? Sound Bakery is yours to use and modify - free of charge. |

## Goal

Sound Bakery aims to be a competitive option to Wwise and FMOD that is more collaborative and open. Imagine owning your audio engine and being able to customise its look, tools, behaviour, and more. Imagine receiving features and fixes from top studios, all collectively improving the tools of the industry.

Check out the [roadmap](docs/Roadmap.md) for where Sound Bakery is going!

## Getting Started

Looking to get started instantly? Check out the [releases](https://github.com/KarateKidzz/sound-bakery/releases) page for prebuilt binaries and source code.

### Prerequisites
Ensure the following are installed:

- A C++ compiler (e.g., GCC, Clang, MSVC)
- CMake (version 3.10 or higher)
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
Contributions from the community are welcome! Whether fixing bugs, adding new features, or improving documentation, all help is valuable.

### How to Contribute
1. Fork the repository.
2. Create a new branch for the feature or bug fix.
3. Commit changes and push the branch to the fork.
4. Create a pull request with a detailed description of the changes.

For more details, check out our [contributing guidelines](CONTRIBUTING.md).

## License
Sound Bakery is licensed under the MIT License. See the LICENSE file for more information.

## Acknowledgements
A big thank you to the creators of miniaudio, concurrencpp, ImGui, rttr, tracy, and spdlog for their incredible libraries and to all our contributors who make Sound Bakery possible.
