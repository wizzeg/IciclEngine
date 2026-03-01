#include <engine/core/systems_context.h>
#include <engine/core/game_thread.h>

SystemsContext::SystemsContext(entt::registry& a_registry, std::shared_ptr<WorkerThreadPool> a_entt_thread_pool, std::shared_ptr<WorkerThreadPool> a_general_thread_pool)
	: entt_thread_pool(a_entt_thread_pool), registry(a_registry), general_thread_pool(a_general_thread_pool)
{
}

SystemsContext::~SystemsContext()
{
}

const std::shared_ptr<EngineContext> SystemsContext::get_engine_context()
{
	return engine_context;
}

std::shared_ptr<AssetManager>& SystemsContext::get_asset_manager()
{
	return engine_context->get_asset_manager();
}