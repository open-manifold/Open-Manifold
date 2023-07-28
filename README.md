![](https://raw.githubusercontent.com/open-manifold/Open-Manifold/main/res/readme_logo.svg)

[![Latest Release](https://img.shields.io/github/v/release/open-manifold/Open-Manifold?style=plastic)](https://github.com/open-manifold/Open-Manifold/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/open-manifold/Open-Manifold/makefile.yaml?style=plastic)](https://github.com/open-manifold/Open-Manifold/actions/workflows/makefile.yaml)
[![Total Downloads](https://img.shields.io/github/downloads/open-manifold/Open-Manifold/total?style=plastic)](https://github.com/open-manifold/Open-Manifold/releases/)
---

**Open Manifold** is a free and open-source rhythm game with the goal of cloning the gameplay of the PlayStation game *Rhythm 'N' Face* while offering several additional features, such as high resolutions, custom level support, and enhanced visual effects.

Currently (as of May 2023), it is not yet considered ready for widespread use, as it's missing important features and doesn't include much content.

## Getting Started
For those who want to start playing, visit [the releases page](https://github.com/open-manifold/Open-Manifold/releases/latest) and download the ZIP file with the name that matches your OS. Extract the ZIP's contents somewhere and double-click `OpenManifold` to run the game.

Want to create your own levels? Check out [the wiki](https://github.com/open-manifold/Open-Manifold/wiki) for level format documentation!

If you run into any bugs or have feature suggestions, you can [file an issue ticket on Github](https://github.com/open-manifold/Open-Manifold/issues); just be sure to check [this page](https://github.com/open-manifold/Open-Manifold/wiki/Troubleshooting#common-issues) first to see if your problem's covered there already.

## Background

*Rhythm 'N' Face* was a rhythm game released exclusively in Japan for the [PlayStation](https://en.wikipedia.org/wiki/PlayStation_%28console%29) in 2000. It was developed by Outside Directors, best known for the cult-classic exploration game _[LSD: Dream Emulator](https://en.wikipedia.org/wiki/LSD:_Dream_Emulator)_ and for their geometry-based, psychedelic art style which *Rhythm 'N' Face* is based around.
  
The object of the game was to create "faces" by moving and resizing a series of circles, squares, and triangles - corresponding to the respective buttons on a PlayStation controller - to the beat of a song. (The remaining button, X, is used for a "stall" function, referred to by this game as an "X-plode").

Unlike many other rhythm games, the buttons the player must press aren't directly enforced by the chart; the player can use any sequence of movements on any beat of the song so long as the shape ends up in the right place.  Performance was scored not by timing accuracy, but rather shape placement and how frequently the player performed actions.

## Licensing

Open Manifold's engine/source code is licensed under MIT. The included content however, is licensed under CC-BY-NC 4.0 (see [`LICENSE`](https://raw.githubusercontent.com/open-manifold/Open-Manifold/main/LICENSE) for details on what file are considered "included content").

## Credits 'n' Thanks
 - **Outside Directors/Asmik Ace/Sony**: Creating and publishing *Rhythm 'n' Face*, the direct inspiration of this game.
 - **[SDL Community](https://www.libsdl.org/)**: Creating the SDL2, SDL_Mixer and SDL_Image libraries.
 - **[Niels Lohmann](https://github.com/nlohmann)**: Creating the JSON for Modern C++ library.
 - **mrmanet**: Creating the [VCR OSD Mono](https://www.dafont.com/vcr-osd-mono.font) font.
 - **[Cockos](https://www.cockos.com/)**: Creating [Reaper](https://www.reaper.fm/index.php), which was used to compose the game's soundtrack and sound effects.
 - **All of the contributors, players, issue reporters, etc.**, it means a lot!
 - **You**: Checking out this project and taking the time to read this file. :)
