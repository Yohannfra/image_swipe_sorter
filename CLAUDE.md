# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make            # Build the project (creates ./image_swipe_sorter)
make clean      # Remove build artifacts
make re         # Clean rebuild
make format     # Format code with clang-format
make lint       # Run cppcheck static analysis
make install    # Install to /usr/local/bin (or PREFIX)
```

## Dependencies

SDL2 and SDL2_image development libraries are required:
- Debian/Ubuntu: `sudo apt install libsdl2-dev libsdl2-image-dev`

## Running

```bash
./image_swipe_sorter <source_dir> --left-dir=<path> --right-dir=<path>
```

## Architecture

This is a single-window SDL2 application for sorting images via keyboard input.

**Module structure:**
- `main.c` - SDL event loop, rendering, application state management
- `files.c/h` - CLI argument parsing (`getopt_long`), directory scanning, file move/undo operations
- `history.c/h` - Circular buffer (50 entries) for undo functionality
- `render.c/h` - Bitmap font rendering and arrow indicators
- `types.h` - Shared types: `Config`, `ImageList`, `MAX_PATH` (4096), `MAX_IMAGES` (10000)

**Key data flow:**
1. `parse_args()` populates `Config` with source/left/right directories
2. `load_image_list()` scans source directory for supported image formats
3. Main loop displays images, handles keyboard/mouse input for sorting
4. `move_file()` moves images and records to `MoveHistory` for undo support
5. `history_push/pop()` manages undo stack as a circular buffer

**Rendering approach:** Uses SDL2's software rendering with a custom bitmap font (no SDL_ttf dependency). Images are scaled to fit window with zoom/pan support.
