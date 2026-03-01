#include <engine/core/system_dependencies.h>
#include <engine/core/systems_context.h>

void SystemsContextDependencies::clear_all()
{
	std::lock_guard lock(components_mutex);
	writing_components.clear();
	reading_components.clear();
}