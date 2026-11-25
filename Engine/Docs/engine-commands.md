# Engine commands

Windows examples below start from the repository root. Build the binaries if needed.

```cmd
cmake -S . -B build
cmake --build build --config Release --target Sandbox onedude
```

## Capture and exit

```cmd
start "" /wait ".\Bin\Sandbox.exe" --scene Sandbox/Scenes/TerrainTest.scene --mode play --capture-viewport "%CD%\terrain.png"
echo %ERRORLEVEL%
```

- `--scene <path>` selects a saved scene. Paths such as `Sandbox/Scenes/PBR.scene` are relative to the project's `Content` folder. The command-line scene workflow defaults to PBR when no scene is specified.
- `--mode edit|play` selects the editor camera or the running game's camera.
- `--capture-viewport <png>` waits for a rendered frame, saves the PNG, and exits. Wait for exit code `0` before opening the image.

## Control a running editor

Start the game in one terminal:

```cmd
.\Bin\Sandbox.exe --live-commands --mode edit
```

Use another terminal:

```cmd
.\Bin\onedude.exe status
.\Bin\onedude.exe list
.\Bin\onedude.exe list --json
.\Bin\onedude.exe command scene open Sandbox/Scenes/PBR.scene
.\Bin\onedude.exe command play
.\Bin\onedude.exe command capture-viewport play.png
.\Bin\onedude.exe command stop
```

Live captures save relative paths from the client terminal. Stop play before opening a different scene. The client discovers a running session automatically. If several match, use `onedude --pid <pid> status` or put the same selector before another command. Exit codes are `0` for success, `1` for a command failure, and `2` for a client or connection error.

## Use live commands in another project

Rebuild your project against the updated engine. If it uses `OD/Entry.h`, the
engine consumes `--live-commands` before the project's argument parser runs.
If it defines its own `main`, call `OD::Application::SetArgs(argc, argv)` first
and read the remaining arguments with `OD::Application::GetArgs()`.

```cmd
MyGame.exe --live-commands
```

From another terminal, run `onedude list` or `onedude list --json` to see what
that process exposes. The Editor module adds `status`, `scene open`, `play`,
`stop`, and `capture-viewport`. The Sandbox startup flags `--scene`, `--mode`,
and `--capture-viewport` belong to Sandbox. A project can use live commands
with its own modules and scene system.

## Register a custom command

In an existing module's `OnInit`, register a unique name and a handler.
Add the module through your project's existing module setup so `OnInit` runs.

```cpp
#include "OD/Core/LiveCommands.h"
#include "OD/Core/Log.h"

void MyGameModule::OnInit(){
    const bool registered = OD::LiveCommands::Register(
        *this, "game.echo",
        [](const OD::LiveCommands::Arguments& args, OD::LiveCommands::Reply reply){
            if(args.size() != 1){
                reply(false, "Usage: game.echo <message>");
                return;
            }
            reply(true, args[0]);
        });
    if(!registered) LogError("Could not register game.echo");
}
```

```cmd
onedude command game.echo "hello from my project"
```

The handler receives only the arguments after the command name. Reply with
`true` for exit code `0` or `false` for exit code `1`. Handlers and replies run
on the application thread. For work that spans frames, retain the reply and
invoke it on that thread once the result is ready. The engine removes a
command when its owning module is removed.

To make this discoverable in a separate project, link to this guide from
that project's README or `AGENTS.md` and list the commands its modules add.
