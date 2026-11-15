# moby-rabbit

An offline, single-player 2D game in C++20 on SDL2 / SDL2_image, built with CMake.
This repo currently holds the game framework (event loop + actor system); the game itself is still to be designed.

## Build and run

Needs `cmake`, a C++20 compiler, `libsdl2-dev` and `libsdl2-image-dev`.

```sh
./build.sh                         # configures and builds into cmake-build-debug/
./cmake-build-debug/moby-rabbit    # run from the repo root so ./assets/ resolves
```

Images and other assets are **not** stored in git. Get them from the team shared
folder (`/home/box/shared`, ask the Animator which subfolder) and make them
available as `./assets/` in the repo root, e.g.
`ln -s /home/box/shared/<assets-folder> assets` (or copy the files in).
Never commit them. The code loads them at runtime
from `./assets/` (for example `./assets/player.png`). If the player sprite is
missing the game still runs, the player just isn't drawn.

## Layout

| Path | What it is |
| --- | --- |
| `main.cpp` | Entry point: creates `State`, one `Player`, runs the event loop. |
| `State/` | Singleton game state (`State::make/get/destroy`). Owns the SDL environment, actors and loaded animations. `startEventLoop()` polls SDL events, dispatches them to actors, moves and draws actors, runs collisions, applies deferred actor adds/removes, and caps the frame rate. |
| `Actor/Actor.*` | Base class for everything in the world: `position`/`velocity`/`acceleration`, named `animations`, `eventActions` (per event group and key/button), and `onRender` / `onCollision` callbacks. |
| `Actor/Actors/Player/` | The pirate mouse: run, jump, attack (no parry yet). Keyboard: arrows/A,D move, Space/Up/W jump, X/Z attack. Gamepad (works alongside keyboard): D-pad, A jump, X/B attack. Reference example of writing an actor. |
| `Actor/Actors/Level/` | Placeholder level: ground and platforms drawn as white lines (one-way: land on top, jump up through). |
| `Actor/Actors/View/` | The camera/viewport actor; actors are drawn relative to its position. Handles quit. |
| `sdl_tools/` | Thin SDL wrappers: `Config` (defaults), `SDL_Environment` (init/shutdown), `Window`, `Renderer`, `Texture`. |
| `Animation/` | Animation = cyclic list of texture frames + ms per frame. |
| `CyclicList/`, `Vector/`, `Matrix/`, `Error/`, `utils/` | Generic utilities. `Error` is the standard error channel. |

## Writing an actor

1. Subclass `Actor` (see `Player`).
2. In the constructor, load animations with `State::get().loadAnimation(...)` and map them in `animations`, set `currentAnimation`.
3. Bind input in `eventActions[Actor::keyDown][SDLK_...]` (or `controllerButtonDown`, etc.).
4. Optionally set `onRender` (per-frame logic) and `onCollision`.
5. Register with `State::get().registerActor(name, this, err)`. State owns and deletes registered actors. Adding/removing actors during the loop is deferred to the end of the frame.

Control mode (keyboard or gamepad) comes from `Config::controlMode`. In `GAMEPAD` mode State opens the first connected controller.

See `CONVENTIONS.md` for code style.
