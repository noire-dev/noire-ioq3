
                                     _                 _              _____ 
                                    (_)               (_)            |____ |
                         _ __   ___  _ _ __ ___ ______ _  ___   __ _     / /
                        | '_ \ / _ \| | '__/ _ \______| |/ _ \ / _` |    \ \
                        | | | | (_) | | | |  __/      | | (_) | (_| |.___/ /
                        |_| |_|\___/|_|_|  \___|      |_|\___/ \__, |\____/ 
                                                                  | |       
                                                                  |_|       

# The engine for Noire's Mod is based on ioquake3.

# Noire-ioq3 key features:
  * Bullet physics
  * JavaScript (duktape)

# Compilation and installation
For *nix,
  1. `git clone git://github.com/ioquake/ioq3.git`
  2. `cd ioq3`
  3. Install dependencies according to your operating system's instructions.  
     for apt-based systems, `sudo apt install cmake libsdl2-dev`
  4. `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  5. `cmake --build build`
  6. The resulting files will be in the `build` directory.

For Windows,
  1. Install Visual Studio Community Edition from Microsoft.
     https://visualstudio.microsoft.com/vs/community/
  2. Install CMake https://cmake.org/cmake/download
  3. Clone our git repository either using the command-line or a GUI tool:  
      `git clone git://github.com/noire-dev/noire-ioq3.git`
  4. Compile using Visual Studio by selecting our CMakeLists.txt and clicking Build.
  5. Or using the command-line: `cmake -S . -B build -G "Visual Studio 17 2022"`
  6. `cmake --build build --config Release`

For macOS,
  1. Install XCode.
  2. Install CMake via homebrew https://brew.sh or your package manager of choice.
  3. `git clone git://github.com/noire-dev/noire-ioq3.git`
  4. `cd noire-ioq3`
  5. `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  6. `cmake --build build`
  7. Copy the resulting `ioquake3.app` in `/build/`
     to your `/Applications/ioquake3` folder.

For Emscripten,
  1. Follow the installation instructions for the Emscripten SDK including
     setting up the environment with emsdk_env. https://emscripten.org/
  2. `git clone git://github.com/noire-dev/noire-ioq3.git`
  3. `cd noire-ioq3`
  4. `emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  5. `cmake --build build`
  3. Copy or symlink your baseq3 pk3 files into the `build/Release/baseq3`
     directory so they can be loaded at run-time. Only game files listed in
     `client-config.json` will be loaded.
  4. Start a web server serving this directory. `python3 -m http.server`
     is an easy default that you may already have installed.
  5. Open `http://localhost:8000/build/Release/ioquake3.html`
     in a web browser. Open the developer console to see errors and warnings.
  6. Debugging the C code is possible using a Chrome extension. For details
     see https://developer.chrome.com/blog/wasm-debugging-2020

Installation, for *nix
  1. Set the CMAKE_INSTALL_PREFIX to your prefered installation directory.
     By default it will be set to /opt/quake3/.
  2. `cmake --install build`.

The following CMake variables may be set, using `-D` on the command line.

```
  BUILD_SERVER            - build the 'ioq3ded' server binary
  BUILD_CLIENT            - build the 'ioquake3' client binary
  BUILD_RENDERER_OPENGL1  - build the opengl1 client / renderer library
  BUILD_RENDERER_OPENGL2  - build the opengl2 client / renderer library
  BUILD_GAME_LIBRARIES    - build the game shared libraries
  BUILD_GAME_QVMS         - build the game qvms
  BUILD_STANDALONE        - build binaries suited for stand-alone games

  USE_RENDERER_DLOPEN     - build and use the renderer in a library
  USE_OPENAL              - use OpenAL where available
  USE_OPENAL_DLOPEN       - link with OpenAL at runtime
  USE_HTTP                - enable http download support
  USE_CODEC_VORBIS        - enable Ogg Vorbis support
  USE_CODEC_OPUS          - enable Ogg Opus support
  USE_MUMBLE              - enable Mumble support
  USE_VOIP                - enable built-in VoIP support
  USE_FREETYPE            - enable FreeType support for rendering fonts

  USE_INTERNAL_LIBS       - build internal libraries instead of dynamically
                            linking against system libraries; this just sets
                            the default for USE_INTERNAL_ZLIB etc.
  USE_INTERNAL_SDL        - link against internal SDL (if available)
  USE_INTERNAL_ZLIB       - build and link against internal zlib
  USE_INTERNAL_JPEG       - build and link against internal JPEG library
  USE_INTERNAL_OGG        - build and link against internal ogg library
  USE_INTERNAL_VORBIS     - build and link against internal Vorbis library
  USE_INTERNAL_OPUS       - build and link against internal opus/opusfile libraries

  EMSCRIPTEN_PRELOAD_FILE - set to 1 to package 'baseq3' (BASEGAME) directory
                            containing pk3s and loose files as a single
                            .data file that is loaded instead of listing
                            individual files in client-config.json

  CMAKE_BUILD_TYPE        - on single config CMake, set to Debug or Release
```

The defaults for these variables may differ depending on the target platform.


# OpenGL ES support

The opengl2 renderer (the default) supports OpenGL ES 2+. Though there
are many missing features and the performance may not be sufficient for
embedded System-on-a-Chip and mobile platforms.

The opengl1 renderer does not have OpenGL ES support.

The opengl2 renderer will try both OpenGL and OpenGL ES APIs to find one that
works. The `r_preferOpenGLES` cvar controls which API to try first.
Set it to -1 for auto (default), 0 for OpenGL, and 1 for OpenGL ES. It should be
set using command line arguments:

    ioquake3 +set cl_renderer opengl2 +set r_preferOpenGLES 1


# Filesystem

Compared to the original release, user configuration and data files are stored
in more modern locations. If you want a different behaviour a specific path
can be provided by adding `+set fs_homepath <path>` to the command line.

### Windows

`C:\Users\<username>\AppData\Roaming\Quake3`

### macOS

`/Users/<username>/Library/Application Support/Quake3`

### Linux

`/home/<username>/.config/Quake3` Configuration files.
`/home/<username>/.local/share/Quake3` Data files (pk3s etc.).
`/home/<username>/.local/state/Quake3` Other internal runtime files.

These directories correspond to the Free Desktop XDG Base Directory
Specification. The original release used `/home/.q3a`. This will be used if
present, however in this case a prompt will be shown suggesting migration to
the above locations, if desired.

# Console

## New cvars

```
  cl_autoRecordDemo                 - record a new demo on each map change
  cl_aviFrameRate                   - the framerate to use when capturing video
  cl_aviMotionJpeg                  - use the mjpeg codec when capturing video
  cl_guidServerUniq                 - makes cl_guid unique for each server
  cl_cURLLib                        - filename of cURL library to load (non-Windows)
  cl_consoleKeys                    - space delimited list of key names or
                                      characters that toggle the console
  cl_mouseAccelStyle                - Set to 1 for QuakeLive mouse acceleration
                                      behaviour, 0 for standard q3
  cl_mouseAccelOffset               - Tuning the acceleration curve, see below

  con_autochat                      - Set to 0 to disable sending console input
                                      text as chat when there is not a slash
                                      at the beginning
  con_autoclear                     - Set to 0 to disable clearing console
                                      input text when console is closed
  con_scale                         - Scales console text to make it legible at
                                      high resolutions. Defaults to 1. Maximum
                                      is 4. Accepts fractional values (1.5).
  con_notifylines                   - The number of lines to display in the
                                      notify area

  in_joystickUseAnalog              - Do not translate joystick axis events
                                      to keyboard commands

  j_forward                         - Joystick analogue to m_forward,
                                      for forward movement speed/direction.
  j_side                            - Joystick analogue to m_side,
                                      for side movement speed/direction.
  j_up                              - Joystick up movement speed/direction.
  j_pitch                           - Joystick analogue to m_pitch,
                                      for pitch rotation speed/direction.
  j_yaw                             - Joystick analogue to m_yaw,
                                      for yaw rotation speed/direction.
  j_forward_axis                    - Selects which joystick axis
                                      controls forward/back.
  j_side_axis                       - Selects which joystick axis
                                      controls left/right.
  j_up_axis                         - Selects which joystick axis
                                      controls up/down.
  j_pitch_axis                      - Selects which joystick axis
                                      controls pitch.
  j_yaw_axis                        - Selects which joystick axis
                                      controls yaw.

  s_useOpenAL                       - use the OpenAL sound backend if available
  s_alPrecache                      - cache OpenAL sounds before use
  s_alGain                          - the value of AL_GAIN for each source
  s_alSources                       - the total number of sources (memory) to
                                      allocate
  s_alDopplerFactor                 - the value passed to alDopplerFactor
  s_alDopplerSpeed                  - the value passed to alDopplerVelocity
  s_alMinDistance                   - the value of AL_REFERENCE_DISTANCE for
                                      each source
  s_alMaxDistance                   - the maximum distance before sounds start
                                      to become inaudible.
  s_alRolloff                       - the value of AL_ROLLOFF_FACTOR for each
                                      source
  s_alGraceDistance                 - after having passed MaxDistance, length
                                      until sounds are completely inaudible
  s_alDriver                        - which OpenAL library to use
  s_alDevice                        - which OpenAL device to use
  s_alAvailableDevices              - list of available OpenAL devices
  s_alInputDevice                   - which OpenAL input device to use
  s_alAvailableInputDevices         - list of available OpenAL input devices
  s_sdlBits                         - SDL bit resolution
  s_sdlSpeed                        - SDL sample rate
  s_sdlChannels                     - SDL number of channels
  s_sdlDevSamps                     - SDL DMA buffer size override
  s_sdlMixSamps                     - SDL mix buffer size override
  s_backend                         - read only, indicates the current sound
                                      backend
  s_muteWhenMinimized               - mute sound when minimized
  s_muteWhenUnfocused               - mute sound when window is unfocused
  sv_dlRate                         - bandwidth allotted to PK3 file downloads
                                      via UDP, in kbyte/s

  com_ansiColor                     - enable use of ANSI escape codes in the tty
  com_altivec                       - enable use of altivec on PowerPC systems
  com_standalone (read only)        - If set to 1, quake3 is running in
                                      standalone mode
  com_basegame                      - Use a different base than baseq3. If no
                                      original Quake3 or TeamArena pak files
                                      are found, this will enable running in
                                      standalone mode
  com_homepath                      - Specify name that is to be appended to the
                                      home path
  com_legacyprotocol                - Specify protocol version number for
                                      legacy Quake3 1.32c protocol, see
                                      "Network protocols" section below
                                      (startup only)
  com_maxfpsUnfocused               - Maximum frames per second when unfocused
  com_maxfpsMinimized               - Maximum frames per second when minimized
  com_busyWait                      - Will use a busy loop to wait for rendering
                                      next frame when set to non-zero value
  com_pipefile                      - Specify filename to create a named pipe
                                      through which other processes can control
                                      the server while it is running.
                                      Nonfunctional on Windows.
  com_gamename                      - Gamename sent to master server in
                                      getservers[Ext] query and infoResponse
                                      "gamename" infostring value. Also used
                                      for filtering local network games.
  com_protocol                      - Specify protocol version number for
                                      current ioquake3 protocol, see
                                      "Network protocols" section below
                                      (startup only)

  in_joystickNo                     - select which joystick to use
  in_availableJoysticks             - list of available Joysticks
  in_keyboardDebug                  - print keyboard debug info

  sv_dlURL                          - the base of the HTTP or FTP site that
                                      holds custom pk3 files for your server
  sv_banFile                        - Name of the file that is used for storing
                                      the server bans

  net_ip6                           - IPv6 address to bind to
  net_port6                         - port to bind to using the ipv6 address
  net_enabled                       - enable networking, bitmask. Add up
                                      number for option to enable it:
                                      enable ipv4 networking:    1
                                      enable ipv6 networking:    2
                                      prioritise ipv6 over ipv4: 4
                                      disable multicast support: 8
  net_mcast6addr                    - multicast address to use for scanning for
                                      ipv6 servers on the local network
  net_mcastiface                    - outgoing interface to use for scan

  r_allowResize                     - make window resizable
  r_ext_texture_filter_anisotropic  - anisotropic texture filtering
  r_zProj                           - distance of observer camera to projection
                                      plane in quake3 standard units
  r_greyscale                       - desaturate textures, useful for anaglyph,
                                      supports values in the range of 0 to 1
  r_stereoEnabled                   - enable stereo rendering for techniques
                                      like shutter glasses (untested)
  r_anaglyphMode                    - Enable rendering of anaglyph images
                                      red-cyan glasses:    1
                                      red-blue:            2
                                      red-green:           3
                                      green-magenta:       4
                                      To swap the colors for left and right eye
                                      just add 4 to the value for the wanted
                                      color combination. For red-blue and
                                      red-green you probably want to enable
                                      r_greyscale
  r_stereoSeparation                - Control eye separation. Resulting
                                      separation is r_zProj divided by this
                                      value in quake3 standard units.
                                      See also
                                      http://wiki.ioquake3.org/Stereo_Rendering
                                      for more information
  r_marksOnTriangleMeshes           - Support impact marks on md3 models, MOD
                                      developers should increase the mark
                                      triangle limits in cg_marks.c if they
                                      intend to use this.
  r_sdlDriver                       - read only, indicates the SDL driver
                                      backend being used
  r_noborder                        - Remove window decoration from window
                                      managers, like borders and titlebar.
  r_screenshotJpegQuality           - Controls quality of jpeg screenshots
                                      captured using screenshotJPEG
  r_aviMotionJpegQuality            - Controls quality of video capture when
                                      cl_aviMotionJpeg is enabled
  r_mode -2                         - This new video mode automatically uses the
                                      desktop resolution.
```

## New commands

```
  video [filename]        - start video capture (use with demo command)
  stopvideo               - stop video capture
  stopmusic               - stop background music
  minimize                - Minimize the game and show desktop
  togglemenu              - causes escape key event for opening/closing menu, or
                            going to a previous menu. works in binds, even in UI

  print                   - print out the contents of a cvar
  unset                   - unset a user created cvar

  banaddr <range>         - ban an ip address range from joining a game on this
                            server, valid <range> is either playernum or CIDR
                            notation address range.
  exceptaddr <range>      - exempt an ip address range from a ban.
  bandel <range>          - delete ban (either range or ban number)
  exceptdel <range>       - delete exception (either range or exception number)
  listbans                - list all currently active bans and exceptions
  rehashbans              - reload the banlist from serverbans.dat
  flushbans               - delete all bans

  net_restart             - restart network subsystem to change latched settings
  game_restart <fs_game>  - Switch to another mod

  which <filename/path>   - print out the path on disk to a loaded item

  execq <filename>        - quiet exec command, doesn't print "execing file.cfg"

  kicknum <client number> - kick a client by number, same as clientkick command
  kickall                 - kick all clients, similar to "kick all" (but kicks
                            everyone even if someone is named "all")
  kickbots                - kick all bots, similar to "kick allbots" (but kicks
                            all bots even if someone is named "allbots")

  tell <client num> <msg> - send message to a single client (new to server)

  cvar_modified [filter]  - list modified cvars, can filter results (such as "r*"
                            for renderer cvars) like cvarlist which lists all cvars

  addbot random           - the bot name "random" now selects a random bot
```


# README for Developers

## pk3dir

_ioquake3_ has a useful new feature for mappers. Paths in a game directory with
the extension ".pk3dir" are treated like pk3 files. This means you can keep
all files specific to your map in one directory tree and easily zip this
folder for distribution.

## 64bit mods

If you wish to compile external mods as shared libraries on a 64bit platform,
and the mod source is derived from the id Q3 SDK, you will need to modify the
interface code a little. Open the files ending in _syscalls.c and change
every instance of int to intptr_t in the declaration of the syscall function
pointer and the dllEntry function. Also find the vmMain function for each
module (usually in cg_main.c g_main.c etc.) and similarly replace the return
value in the prototype with intptr_t (arg0, arg1, ...stay int).

Add the following code snippet to q_shared.h:

```c
#ifdef Q3_VM
typedef int intptr_t;
#else
#include <stdint.h>
#endif
```

Note if you simply wish to run mods on a 64bit platform you do not need to
recompile anything since by default Q3 uses a virtual machine system.

# Credits

Noire-ioq3 Maintainers

  * noire.dev <opennoire@gmail.com>

Maintainers

  * Tim Angus <tim@ngus.net>
  * Jack "Mr. Nuclear Monster" Slater <jack@ioquake.org>

Former Maintainers

  * James Canete <use.less01@gmail.com>
  * Ludwig Nussel <ludwig.nussel@suse.de>
  * Thilo Schulz <arny@ats.s.bawue.de>
  * Tony J. White <tjw@tjw.org>
  * Zack Middleton <zturtleman@gmail.com>

Significant contributions from

  * Ryan C. Gordon <icculus@icculus.org>
  * Andreas Kohn <andreas@syndrom23.de>
  * Joerg Dietrich <Dietrich_Joerg@t-online.de>
  * Stuart Dalton <badcdev@gmail.com>
  * Vincent S. Cojot <vincent at cojot dot name>
  * optical <alex@rigbo.se>
  * Aaron Gyes <floam@aaron.gy>
  * surrealchemist
