This is a general game engine capable of creating simple games! This may be a nice introduction to ECS if you want to learn how to use ECS and Data Oriented Programming.

Note! You can play around with the engine with the Demo Game in releases! In the Editor version (check other releases), you can create scenes with existing components and systems.

C++ 20 is required, there is a macro for print incase you use C++23+
using entt ([see skypjack](https://github.com/skypjack/entt)), imgui-docking, opengl through glad, glfw, and glm.

This engine is primarily using data oriented design, with some use of object oriented programming where it makes more sense (or will not impact game performance). Everything that is reasonbly simple to make multi-threaded is multi-threaded.

How to run:
Install Visual Studio 22, and ensure you have at least C++20, but preferably C++ 23 or higher. Also ensure you have windows 10, but it might work on windows 11 too (I pray).
(Projects -> properties -> Configure properties -> general -> C++ language standards)

Download code.

open .sln, go into Project -> properties -> Configure Properties -> Debugging -> Working Directionry ... And set to $(SolutionDir)

Take the imgui.ini in the contents of "MOVE THESE" and move it into the same folder as the .sln.

Then take all of the contents in "MOVE THESE" and go into the x64 folder that lives in the same folder as the .sln file. And paste them into both the x64 -> Release and Debug folder (this should give you a prepared layout for imgui).
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

How to use engine:

You must manually save scenes, right click on a scene object in the scene hierarchy to get up an extra menu, for things like pre-fabs and duplicate etc.

If you press on the main camera buffer the mouse will be locked and invisible in the game window. To get out of it, press ~ (alt + esc?), or alt-tab or press the windows key so that the editor loses focus.

To move the editor camera, hover the editor camera buffer, and hold right click. Then you can move the camera with WASD, Shift to move up, and ctrl to move down, and move the mouse to look around.

To add Components, add them in components.h or elsewhere, but components_data.h must include that directory (if you place them elsewhere you'll have to figure it out). Then go into component_entries and REGISTER_COMPONENT, follow the examples to figure out how to do it.

To add Systems, add them in systems.h or elsewhere, then in system_entires use REGISTER_SYSTEM macro.

To make systems.

Use 

	WithRead<Components...>{}, 
	WithWrite<Components...>{}, 
	WithOut<Components...>{} 
	and WithRef<Components...>. 
	
What you will itterate over are entities with the components in WithRead and WithWrite, but without entities with components in WithOut. WithRef is to signify that you will use ctx.try_get<Component>(entity) to the dependency checking (so that there's no read/write collision).

In your arguments for the lambda, use the following format (const entt::entity entity, const ReadComponent& read_component, const WriteComponent& write_component).

For entt iteration you have the following.

	ctx.each -> simple immediate iteration of the requested entities.
	
	ctx.enqueue_each -> placing the .each iteration in a seperate thread.
	
	ctx.enqueue_parallel_each -> launches parallel threads with chunk_size number of entities.

ctx.enqueue_parallel_data_each ->supply a vector of num_threads number of vectors, all initialized, and supply num_threads and if you wish to record iteration order. When sorting for recorded iteration order, sort by > rather than, as the order appears to be reversed when using entt::handle instead of entt::view for iteration (and recording in parallel_data_each).

To use SystemStorage, use e.g.

	ctx.get_systemstorage().get_object<ObjectType>("object_name"), 

with option to use a size_t if you have multiple entires with same name and type.

If you wish to read from a SystemObjectStorage, you may use a ReadLock, for which you copy into each lambda in a ctx.enqueue_. Create it like this.

	std::shared_ptr<ReadLock<ObjectType>> shared_read_lock = std::make_shared<ReadLock<ObjectType>>(strg_obj_ptr); 
	
-> when they are copied into the lambda the ReadLock will automatically unlock when the last thread is finished with the reading (as the ReadLock goes out of scope).

To spawn entities, use an ecb. First create a system for which you set enabled = false after running. In the system, ensure you do ctx.create_ecb("name);, then in a spawning/destroyer/modifier system, use ctx.get_ecb("name);. The ecb is NOT thread safe.

To use the ecb you can do as following.

            ecb->create_entity(new_entity()
							.with_component<TransformDynamicComponent>(transform)
							.with_child(new_entity()
								.with_component<TransformDynamicComponent>(trans)
								.assemble(), "new_child")
							.assemble(), "new_entity");
              
You can also use ecb->add_component<Component>(entity) or add_component(entity, Component{}), ecb->orphan(entity), ecb->remove_component<Component>(entity), ecb->set_parent(parent, child), ecb->orphan(entity)
