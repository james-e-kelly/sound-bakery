# Sound Bakery

[![linux](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/linux.yaml)
[![macos](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/macos.yaml)
[![windows](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml/badge.svg?branch=dev)](https://github.com/KarateKidzz/sound-bakery/actions/workflows/windows.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![GitHub license](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/compiler_support#cpp20)
![WIP](https://img.shields.io/badge/Status-WIP-yellow)

Welcome to Sound Bakery – a powerful, open-source audio middleware tool designed to bring your game audio to life! Inspired by industry-leading tools like Wwise and FMOD, Sound Bakery combines the robustness of the [miniaudio](https://miniaud.io/index.html) library with our custom-built low-level library Sound Chef and high-level library Sound Bakery to deliver a seamless audio experience for developers.

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
Sound Bakery is your go-to solution for integrating high-quality audio into your games and interactive media. By leveraging the simplicity and efficiency of miniaudio, along with our custom libraries, Sound Bakery offers a comprehensive suite of tools to manage and play audio effortlessly.

Whether you're a seasoned audio programmer or just getting started, Sound Bakery provides an intuitive and flexible environment to craft immersive soundscapes.

## Features
- **Seamless Integration:** Easily integrate Sound Bakery into your existing projects with minimal setup.
- **Powerful Audio Engine:** Utilize miniaudio for robust audio playback capabilities.
- **Modular Design:** Employ Sound Chef for low-level audio manipulation and Sound Bakery for high-level audio management.
- **Authoring Tool:** Create and modify audio experiences using our authoring tool, powered by ImGui.
- **Written in C++20:** Leverage modern C++20 features for optimal performance and developer experience.
- **Easy API:** Enjoy an API that is as simple to use as FMOD's.
- **Cross-Platform Support:** Develop your audio solutions for multiple platforms without hassle.
- **Flexible Media Encoding:** Encode your media with Vorbis, Opus, or ADPCM.
- **Human-Readable Data:** Save data in YAML format to minimize source control conflicts.
- **Profiling Tools:** Profile your audio performance with the tracy profiler.
- **Open-Source:** Contribute to the project and help us build the future of game audio.

## Getting Started
Ready to start baking some sounds? Follow these steps to set up Sound Bakery in your project.

### Prerequisites
Ensure you have the following installed:

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
Here's a simple example to get you started with Sound Bakery:

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
We welcome contributions from the community! Whether you're fixing bugs, adding new features, or improving documentation, your help is valuable.

### How to Contribute
Fork the repository.
Create a new branch for your feature or bugfix.
Commit your changes and push your branch to your fork.
Create a pull request with a detailed description of your changes.
For more details, check out our contributing guidelines.

## License
Sound Bakery is licensed under the MIT License. See the LICENSE file for more information.

## Acknowledgements
A big thank you to the creators of miniaudio, concurrencpp, ImGui, rttr, tracy, and spdlog for their incredible libraries, and to all our contributors who make Sound Bakery possible.
