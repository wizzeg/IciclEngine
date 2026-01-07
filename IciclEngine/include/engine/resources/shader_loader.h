//#pragma once
//#include <engine/renderer/render_info.h>
//#include <engine/utilities/macros.h>

// load from string
// initialize


// the material jobber has to first check materials if there's dupe, if not, check through shaders to see if it's loaded
// after it has found that no material exists, generate new material, set that it's started handling, lock shaders,
// look throug shaders, if there is a shader, unlock shaders and use that program ... otherwise, create the new shader,
// mark it as started loading, mark material as "loading shader" status...once shader loaded, go back to material, mark it texture loading
// lock textures, check which textures need to be loaded...look if loaded ... if all loaded then it's ready, add num references +1.
// if they've not been loaded, then do a forloop for the material.