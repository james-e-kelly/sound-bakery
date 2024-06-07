# Sound Bakery

[![linux](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml)
[![macos](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml)
[![windows](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![GitHub license](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/compiler_support#cpp20)
![WIP](https://img.shields.io/badge/Status-WIP-yellow)

Welcome to **Sound Bakery** – an open-source audio middleware tool designed to bring game audio to life! Inspired by industry-leading tools Wwise and FMOD, Sound Bakery combines the robustness of the miniaudio library with the custom-built low-level library Sound Chef and high-level library Sound Bakery to deliver a seamless audio experience.

## Table of Contents
- [About Sound Bakery](#about-sound-bakery)
- [Features](#features)
- [Getting Started](#getting-started)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## About Sound Bakery
Sound Bakery offers an effective solution for integrating high-quality audio into games and interactive media. Leveraging the simplicity and efficiency of miniaudio, along with custom libraries, Sound Bakery provides a comprehensive suite of tools to manage and play audio effortlessly.

Sound Bakery provides an intuitive and flexible environment for crafting immersive soundscapes for both seasoned audio programmers and newcomers.

## Features
- **Seamless Integration:** Integrate Sound Bakery into existing projects with minimal setup.
- **Powerful Audio Engine:** Utilize miniaudio for robust audio playback capabilities.
- **Modular Design:** Use Sound Chef for low-level audio manipulation and Sound Bakery for high-level audio management.
- **Authoring Tool:** Create and modify audio experiences using an authoring tool powered by ImGui.
- **Written in C++20:** Modern C++20 features ensure optimal performance and a great developer experience.
- **Easy API:** An API designed to be as simple to use as FMOD's.
- **Cross-Platform Support:** Develop audio solutions for multiple platforms without hassle.
- **Flexible Media Encoding:** Encode media with Vorbis, Opus, or ADPCM.
- **Human-Readable Data:** Save data in YAML format to minimize source control conflicts.
- **Profiling Tools:** Profile audio performance with the tracy profiler.
- **Open-Source:** Contributions to the project are welcome to help build the future of game audio.

## Getting Started
Ready to start baking some sounds? Follow these steps to set up Sound Bakery in a project.

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

## Contributing
Contributions from the community are welcome! Whether fixing bugs, adding new features, or improving documentation, all help is valuable.

### How to Contribute
1. Fork the repository.
2. Create a new branch for the feature or bug fix.
3. Commit changes and push the branch to the fork.
4. Create a pull request with a detailed description of the changes.

For more details, check out our [contributing guidelines](CONTRIBUTING.MD).

## License
Sound Bakery is licensed under the MIT License. See the LICENSE file for more information.

## Acknowledgements
A big thank you to the creators of miniaudio, concurrencpp, ImGui, rttr, tracy, and spdlog for their incredible libraries and to all our contributors who make Sound Bakery possible.
