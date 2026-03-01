C++ 20 is required, there is a macro for print incase you use C++23+
using entt ([see skypjack](https://github.com/skypjack/entt)), imgui-docking, opengl through glad, glfw, and glm.

This engine is primarily using data oriented design, with some use of object oriented programming where it makes more sense (or will not impact game performance).

I'm unsure if my gitignore is too aggressive or not, so I might've been blocking too many files for this to build from a pull. Earlier I had problems because it was too lax, making me unable to upload to github.

How to run:
Install Visual Studio 22, and ensure you have at least C++, but preferably C++ 23 or higher.
(Projects -> properties -> Configure properties -> general -> C++ language standards)

Download code.
After extracting, take the contents of "MOVE THESE OUTSIDE" and move them into the same folder as you extracted initially into.
Run the .sln.
Press the Play button in either release or debug.
If you want to play the game through the editor outside of the .sln you must ensure you copy the assets folder into the debug/release folder where it is exported.
