# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Raw sounds have a "Create Sound Node" right-click option
- Drag & Dropping .wav files copies them to the project
- Drag & Dropping .bakery files opens the project
- The editor has a new "Create New Project" dialog window
- The database_object class has a get_database_name and get_database_path function for generating unique object identifiers

### Removed

- Removed the Git commit ID from builds and version files

### Changed

- Editor opens in fullscreen upon start
- Editor strings are shown in alphabetical order instead of random
- The sound_bakery and sound_chef libraries are replaced in favour of xxx_shared and xxx_static libraries
- Object names are moved from the database_object class to the object class
- **BREAKING**: Public type names have changed and Sound Bakery will not be able to load old files

### Fixed

- Fixed assert failing when a parent node is stopping a child
- Fixed saving sounds when no encoded sound exists
- Fixed the editor save icons displaying a question mark
- Sound nodes without sounds don't trigger an assert and correctly stop

## [0.3.0]

### Added

- Editor - Add profiling with Tracy
- Editor - Add layouts
- Editor - When opening a project, a default layout is loaded

## [0.2.0] - 2024-02-18

### Added

- Editor - Add support for re-parenting a node
- Editor - Add support for changing switches on preview/listener GameObject
- Sound Bakery - Add switches/int parameters
- Sound Bakery - Switch containers choose child containers based on switches
- Sound Bakery - Properties have min/max values to clamp set values
- Sound Chef - Simple "Play Sound" API with sounds and sound instances
- Docs - Fix Roadmap title in doxygen

### Changed

- Sound Bakery - IntParameter is now named NamedParameter

### Fixed

- Fixed compilation on MacOS and Linux

## [0.1.0] - 2023-01-10

### Added

- Sound Bakery - Initial commit
- Sound Chef - Initial commit
- Editor - Initial commit