NOTE! I will wake up very late on monday march 2nd. So I cannot showcase it that day.

C++ 20 is required, there is a macro for print incase you use C++23+
using entt ([see skypjack](https://github.com/skypjack/entt)), imgui-docking, opengl through glad, glfw, and glm.

This engine is primarily using data oriented design, with some use of object oriented programming where it makes more sense (or will not impact game performance). Everything that is reasonbly simple to make multi-threaded is multi-threaded.

How to run:
Install Visual Studio 22, and ensure you have at least C++, but preferably C++ 23 or higher. Also ensure you have windows 10, but it might work on windows 11 too (I pray).
(Projects -> properties -> Configure properties -> general -> C++ language standards)

Download code.

open .sln, go into Project -> properties -> Configure Properties -> Debugging -> Working Directionry ... And set to $(SolutionDir)

Take the imgui.ini in the contents of "MOVE THESE" and move it into the same folder as the .sln.

Then take all of the contents in "MOVE THESE" and go into the x64 folder that lives in the same folder as the .sln file. And paste them into both the Release and Debug folder (this should give you a prepared layout for imgui).
Then you may also copy the assets folder from the .sln file folder, into the x64 -> Release and Debug folders.

Run the .sln.
Press the Play button in either release or debug.
Then exit.

Go into the x64 folder, then into Release or Debug, ensure all the files above are there, and run the .exe

To build to game:

Go into engine -> core -> build_state.h

Uncomment //#define GAME_BUILD

uncomment //#define DEFAULT_PATH "./assets/scenes/mainmenu.scn"

comment #define DEFAULT_PATH "./assets/temp/temp_scene.scn"


That is:

#define GAME_BUILD

//#define DEFAULT_PATH "./assets/temp/temp_scene.scn"

#define DEFAULT_PATH "./assets/scenes/mainmenu.scn"


Note, Assimp .dlls should not be necessary.

How to use engine:

You must manually save scenes, right click on a scene object in the scene hierarchy to get up an extra menu, for things like pre-fabs and duplicate etc.

If you press on the main camera buffer the mouse will be locked and invisible in the game window. To get out of it, press ~ (alt + esc?), or alt-tab or press the windows key so that the editor loses focus.

To move the editor camera, hover the editor camera buffer, and hold right click. Then you can move the camera with WASD, Shift to move up, and ctrl to move down, and move the mouse to look around.

To add Components, add them in components.h or elsewhere, but components_data.h must include that directory (if you place them elsewhere you'll have to figure it out). Then go into component_entries and REGISTER_COMPONENT, follow the examples to figure out how to do it.

To add Systems, add them in systems.h or elsewhere, then in system_entires use REGISTER_SYSTEM macro.
