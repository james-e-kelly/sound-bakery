# Contributing to Sound Bakery

Thank you for your interest in contributing to Sound Bakery! Contributions are welcome and appreciated. This guide will help you get started with contributing to the project.

## Table of Contents
- [How to Contribute](#how-to-contribute)
- [Reporting Issues](#reporting-issues)
- [Submitting Pull Requests](#submitting-pull-requests)
- [Code Style](#code-style)
- [Testing](#testing)
- [Documentation](#documentation)
- [Community](#community)

## How to Contribute

There are several ways to contribute to Sound Bakery:
- Reporting bugs and issues
- Suggesting new features and enhancements
- Submitting pull requests to fix bugs or add features
- Improving documentation

## Reporting Issues

If any bugs or issues are encountered, please report them using the GitHub [issue tracker](https://github.com/KarateKidzz/sound-bakery/issues). When reporting an issue, please provide as much detail as possible, including:
- A clear and descriptive title
- Steps to reproduce the issue
- Expected and actual results
- Any relevant screenshots or logs
- The version of Sound Bakery and your development environment

## Submitting Pull Requests

To submit a pull request (PR), follow these steps:

1. **Fork the Repository**: Fork the [Sound Bakery repository](https://github.com/KarateKidzz/sound-bakery) to your GitHub account.

2. **Clone Your Fork**: Clone your forked repository to your local machine.
   ```bash
   git clone https://github.com/KarateKidzz/sound-bakery.git
   cd sound-bakery
   ```

3. **Create a Branch**: Create a new branch for your feature or bugfix.
   ```bash
   git checkout -b feature-or-bugfix-name
   ```

4. **Make Changes**: Make your changes to the codebase. Ensure that your changes follow the project's [code style](#code-style) and include tests if applicable.

5. **Commit Changes**: Commit your changes with a clear and descriptive commit message.
   ```bash
   git commit -m "Description of the changes made"
   ```

6. **Push Changes**: Push your changes to your forked repository.
   ```bash
   git push origin feature-or-bugfix-name
   ```

7. **Create a Pull Request**: Create a pull request to the main repository. Provide a detailed description of your changes and any relevant information.

## Code Style

Consistent code style helps maintain readability and quality. Sound Bakery integrates `clang-format` and `clang-tidy` to ensure code quality and consistency. Please follow these guidelines when contributing:
- Use modern C++20 features and standard library components.
- Use descriptive variable and function names.
- Write clear and concise comments where necessary.
- Ensure that your code is properly formatted using `clang-format`.
- Run `clang-tidy` to catch and fix potential issues.

## Testing

Ensuring that your changes do not introduce new issues is crucial. Sound Bakery uses `doctest` and `CTest` for testing. Please follow these testing guidelines:
- Write unit tests for new features and bug fixes using `doctest`.
- Ensure that all existing tests pass before submitting your pull request.
- Use `CTest` to manage and run tests.

To run tests, use the following commands:

```bash
cd build
ctest
```

## Documentation

Good documentation is key to a successful project. Sound Bakery uses Doxygen for documentation generation. When contributing, please:
- Update or add documentation for any new features or changes.
- Ensure that your documentation is clear and easy to understand.
- Use Doxygen comments to document your code.
- Use markdown for formatting other documentation files.

### Using Doxygen

To generate documentation using Doxygen, follow these steps:

1. **Install Doxygen** if you haven't already.
2. **Add Doxygen-style comments** to your code. Here is an example:
   ```cpp
   /**
    * @brief Initializes the Sound Bakery system.
    */
   void Initialize();
   ```
3. **Build the Doxygen Documentation**:
   - Ensure that CMake is set up for the project.
   - Build the `doc_doxygen` target:
     ```bash
     cd build
     cmake --build . --target doc_doxygen
     ```

Generated documentation will be available in the `doxygen` directory.

## Community

Sound Bakery aims to foster a welcoming and inclusive community. When contributing, please:
- Be respectful and considerate of others.
- Provide constructive feedback and support.
- Follow the [Code of Conduct](CODE_OF_CONDUCT.md) to ensure a positive environment for all contributors.

---

Thank you for contributing to Sound Bakery! Your efforts help make this project better for everyone. If you have any questions, feel free to reach out via the issue tracker or the project's communication channels.
