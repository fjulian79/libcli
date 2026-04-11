# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.6.0] - 2026-04-11

### Added
- Added bash like command completion based on tab key
    - Enabled by default, but can be disabled with `#define CLI_TAB_COMPLETION 0` to save flash memory
    - very low RAM impact, more impact on flash memory.
- Now sorting the command table if command completion is enabled to display matches in alphabetical order.
    - Using the standard C library qsort (expected to be optimized and tested)
    - Can be disabled via `Cli::begin()` method parameter.
- Exposed more VT100 control sequences to users for enhanced terminal control (e.g., cursor movement)
- Added Documentation for command registration

### Changed
- Refactored the code to clean up cli.cpp.
- Updated and improved existing documentation

## [4.5.0] - 2026-04-09

### Added
- Optional history support: set `CLI_HISTORYSIZ = 0` to completely disable command history feature
- Memory savings when history is disabled: ~224 bytes RAM + ~824 bytes Flash (measured on RP2040)
- Compile-time validation for `CLI_HISTORYSIZ` with warning for invalid values
- Comprehensive memory usage documentation in CONFIGURATION.md
- Updated README and added CHANGELOG.md, doc/CONFIGURATION.md, doc/API.md for better documentation and clarity

## [4.4.0] - 2026-04-07

### Added
- Duplicate command prevention: consecutive identical commands are no longer stored in history
- Empty command filtering: whitespace-only commands are now skipped during processing

## [4.3.1] - 2026-03-29

### Fixed
- Fixed check for a git repo in pre_version.py

## [4.3.0] - 2026-03-28

### Changed
- Redesigned history management to be more robust, efficient and capable
- History now uses a fixed size ring buffer, storing multiple commands based on their size
- Oldest commands are automatically overwritten when buffer is full
- Navigation through history (back and forth) now supported
- Commands stored as-is to preserve escaped characters
- Invalid commands also stored to allow recall and correction

### Added
- Pre-build script to generate version header from git tags or library.json

## [4.2.0] - 2026-03-28

### Fixed
- Potential out of bounds write in command buffer
- Unhandled missing string terminator in command arguments

### Changed
- Improved code documentation and comments

## [4.1.0] - 2026-01-29

### Added
- Support for escaped characters in command argument strings

## [4.0.0] - 2025-02-07

### Changed
- Complete reimplementation using constructor-based automatic command registration
- API cleanup and modernization
- Official PlatformIO library registration

### Breaking Changes
- Command registration mechanism changed from manual table definition to automatic constructor-based registration
- API changes throughout the library require code updates

## [3.4.0] - 2024-10-21

### Changed
- Command function prototype now includes `Stream& ioStream` as first parameter for direct I/O access
- Improved telnet support with proper `\r\n` line ending handling
- Enhanced echo handling throughout the codebase
- Command buffer initialization moved to `begin()` method

### Added
- Null-pointer check for `pStream` in `loop()` to prevent crashes

### Breaking Changes
- Command signature changed: `int8_t cmd_name(Stream& ioStream, const char *argv[], uint8_t argc)`

## [3.2.0] - 2023-02-12

### Fixed
- Bug in `generateCmdTable.py` where output file was not properly closed

### Changed
- Improved logging functionality

## [3.1.0] - 2023-01-25

### Changed
- `reset()` method is now public for external access

### Fixed
- Recursive search results in `generateCmdTable.py`

## [3.0.0] - 2023-01-15

### Added
- Automatic command table generation via `generateCmdTable.py` script
- Support for both `ascii.del` and `ascii.bs` (backspace) for character deletion

### Changed
- API improvements for better usability

### Breaking Changes
- API changes require code updates in command registration and usage

## Older Versions

For detailed changes in versions prior to 3.0.0, see the [commit history](https://github.com/fjulian79/libcli/commits/).

**v2.2.0** (2021-11-02): Added `CLI_CMD_DEF()` macro for simplified command table definition  
**v1.0.1** (2021-04-25): Fixed broken includes  
**v1.0.0** (2021-04-25): First tagged release

### Pre-release Development (2017-2020)

The library was actively developed for several years before the first official release:

**2020**: Major API improvements - Added command history, echo support, form feed handling, and Stream-based I/O  
**2018**: PlatformIO integration, configuration system improvements  
**2017**: Extracted from a previously created collection of personal Arduino libraries and turned into standalone libCli repository with basic command parsing (argv/argc style)
