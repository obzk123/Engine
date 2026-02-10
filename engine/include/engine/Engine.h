#pragma once
#include "engine/ecs/Registry.h"
#include "engine/ecs/SystemScheduler.h"
#include "engine/Profiling.h"
#include "engine/render/Renderer2D.h"
#include "engine/render/TextureManager.h"

#include <SDL.h>

namespace eng {

class Engine {
public:
    bool init();
    void run();
    void shutdown();

    // ── Getters para que la demo/juego pueda acceder a los subsistemas ──
    SDL_Window*           window()    { return m_window; }
    ecs::Registry&        registry()  { return m_registry; }
    ecs::SystemScheduler& scheduler() { return m_scheduler; }
    Renderer2D&           renderer()  { return m_renderer; }
    TextureManager&       textures()  { return m_texManager; }
    Profiler&             profiler()  { return m_profiler; }

private:
    bool           m_running   = false;
    SDL_Window*    m_window    = nullptr;
    SDL_GLContext  m_glContext  = nullptr;

    Profiler             m_profiler{240};
    ecs::SystemScheduler m_scheduler{m_profiler};
    ecs::Registry        m_registry;
    Renderer2D           m_renderer;
    TextureManager       m_texManager;
};

} // namespace eng
