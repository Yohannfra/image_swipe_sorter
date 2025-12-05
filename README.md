# Image Swipe Sorter

A fast, keyboard-driven image sorting tool built with SDL2. Quickly sort through large collections of images by swiping them into two categories using arrow keys.

## Features

- **Fast sorting workflow** - Sort images with single keypresses
- **Undo support** - Made a mistake? Undo up to 50 moves
- **Zoom & pan** - Inspect image details before deciding
- **Lightweight** - Minimal dependencies, fast startup
- **Keyboard-driven** - No mouse required for sorting
- **Progress tracking** - Visual progress bar shows completion

## Installation

### Dependencies

Install SDL2 and SDL2_image development libraries:

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev libsdl2-image-dev

# Fedora
sudo dnf install SDL2-devel SDL2_image-devel

# Arch Linux
sudo pacman -S sdl2 sdl2_image

# macOS (Homebrew)
brew install sdl2 sdl2_image
```

### Building

```bash
git clone https://github.com/Yohannfra/image_swipe_sorter.git
cd image_swipe_sorter
make
```

The binary `image_swipe_sorter` will be created in the project root.

## Usage

```bash
./image_swipe_sorter <source_directory> --left-dir=<path> --right-dir=<path>
```

### Arguments

| Argument | Description |
|----------|-------------|
| `<source_directory>` | Directory containing images to sort |
| `--left-dir=<path>` | Destination for left-swiped images |
| `--right-dir=<path>` | Destination for right-swiped images |

### Example

```bash
# Sort photos into "keep" and "delete" folders
./image_swipe_sorter ~/Pictures/unsorted --left-dir=~/Pictures/delete --right-dir=~/Pictures/keep

# Sort screenshots by category
./image_swipe_sorter ./screenshots --left-dir=./work --right-dir=./personal
```

**Note:** Left and right directories will be created automatically if they don't exist.

## Controls

### Sorting

| Key | Action |
|-----|--------|
| `←` Left Arrow | Move image to left directory |
| `→` Right Arrow | Move image to right directory |
| `↓` Down Arrow | Skip image (leave in source) |
| `Space` | Undo last move |
| `Q` / `Esc` | Quit |

### Viewing

| Input | Action |
|-------|--------|
| Mouse Wheel | Zoom in/out |
| Left Click + Drag | Pan image |
| Middle Click | Reset zoom and pan |

## Supported Formats

- PNG
- JPEG / JPG
- BMP
- GIF
- WebP
- TIFF / TIF

## Project Structure

```
image_swipe_sorter/
├── src/
│   ├── main.c      # Application entry point and main loop
│   ├── files.c/h   # File operations and directory handling
│   ├── history.c/h # Undo history (circular buffer)
│   ├── render.c/h  # SDL rendering (text, arrows)
│   └── types.h     # Shared type definitions
├── Makefile
└── README.md
```

## How It Works

1. The program scans the source directory for image files
2. Images are displayed one at a time in a resizable window
3. Press left/right arrow to move the current image to the corresponding directory
4. The next image is automatically loaded
5. Press space to undo and restore the last moved image
6. When all images are sorted, the program exits (or you can undo to continue)

## Tips

- **Use meaningful directory names** - Name your left/right directories based on your sorting criteria (e.g., `keep`/`trash`, `good`/`bad`, `work`/`personal`)
- **Zoom to check details** - Use mouse wheel to zoom in on image details before deciding
- **Undo is your friend** - Don't worry about mistakes, you can undo up to 50 moves
- **Skip uncertain images** - Use Down arrow to skip images you're unsure about, then review them in a second pass

## Building for Development

```bash
# Clean build
make clean && make

# Format code (requires clang-format)
make format
```

## License

MIT License - feel free to use, modify, and distribute.
