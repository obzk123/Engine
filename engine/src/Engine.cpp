#include "engine/Engine.h"
#include "engine/Time.h"
#include "engine/ecs/DemoWorld.h"
#include "engine/ecs/Components.h"
#include "engine/Profiling.h"
#include "engine/ecs/SystemScheduler.h"
#include "engine/Input.h"
#include "engine/render/Renderer2D.h"

#include <SDL.h>
#include <glad/glad.h>

#include <cstdint>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

SDL_Window* g_window = nullptr;

namespace eng {

static SDL_GLContext g_glContext = nullptr;
static eng::ecs::DemoWorld g_demoWorld;
static eng::Profiler g_profiler(240);
static eng::ecs::SystemScheduler g_scheduler(g_profiler);

bool Engine::init() {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


    g_window = SDL_CreateWindow(
        "My Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    g_glContext = SDL_GL_CreateContext(g_window);
    if (!g_glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "gladLoadGLLoader failed\n";
        return false;
    }

    // Viewport se actualiza cada frame en run() para soportar resize.

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(g_window, g_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    eng::Time::init();
    eng::Input::init();
    eng::Input::bind(eng::Action::Pause, SDL_SCANCODE_P);
    eng::Input::bind(eng::Action::Step,  SDL_SCANCODE_O);
    eng::Input::bind(eng::Action::MoveLeft,  SDL_SCANCODE_A);
    eng::Input::bind(eng::Action::MoveRight, SDL_SCANCODE_D);
    eng::Input::bind(eng::Action::MoveUp,    SDL_SCANCODE_W);
    eng::Input::bind(eng::Action::MoveDown,  SDL_SCANCODE_S);
    eng::Renderer2D::instance().init();
    g_demoWorld.init(g_scheduler);
    m_running = true;
    return true;
}

void Engine::run() {
    while (m_running) {
        eng::Time::beginFrame(); // Medir el tiempo desde el frame anterior
        g_profiler.beginFrame(); // Resetea el almacenamiento de mediciones del frame
        SDL_Event e;             // Poll de eventos de SDL
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) m_running = false;
            eng::Input::onEvent(e);
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        int fixedStepsThisFrame = 0;
        while (eng::Time::consumeFixedStep()) {
            g_scheduler.fixedUpdate(g_demoWorld.registry(), eng::Time::fixedDeltaTime());
            fixedStepsThisFrame++;
        }

        g_scheduler.update(g_demoWorld.registry(), eng::Time::deltaTime());
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Entities alive: %zu", g_demoWorld.registry().aliveCount());
        ImGui::Text("Transform2D: %zu", g_demoWorld.registry().componentCount<eng::ecs::Transform2D>());
        ImGui::Text("Velocity2D: %zu", g_demoWorld.registry().componentCount<eng::ecs::Velocity2D>());
        ImGui::Text("Fixed steps this frame: %d", fixedStepsThisFrame);
        ImGui::Text("Fixed dt: %.4f", eng::Time::fixedDeltaTime());
        auto e0 = eng::ecs::Entity{0, 0};
        if (g_demoWorld.registry().isAlive(e0) && g_demoWorld.registry().has<eng::ecs::Transform2D>(e0)) {
            auto& t = g_demoWorld.registry().get<eng::ecs::Transform2D>(e0);
            ImGui::Text("E0 pos: (%.2f, %.2f)", t.position.x, t.position.y);
        }
        ImGui::Text("Simulation: %s", eng::Time::isPaused() ? "PAUSED" : "RUNNING");        
        static int window = (int)g_profiler.window();
        ImGui::SliderInt("Profiler window", &window, 10, 600);
        g_profiler.setWindow((size_t)window);

        ImGui::Separator();
        ImGui::Text("Profiler (rolling window: %zu)", g_profiler.window());

        for (const auto& kv : g_profiler.stats()) {
            const auto& name = kv.first;
            const auto& st = kv.second;

            ImGui::BulletText(
                "%s | last: %.3f ms | avg: %.3f ms | min: %.3f | max: %.3f | n=%zu",
                name.c_str(), st.lastMs, st.avgMs, st.minMs, st.maxMs, st.samples
            );
        }
        
        
        ImGui::Separator();
        ImGui::Text("Systems order:");
        for (const auto& s : g_scheduler.systems()) {
            bool enabled = s.enabled;

            ImGui::PushID(s.name.c_str());
            if (ImGui::Checkbox("##enabled", &enabled)) {
                g_scheduler.setEnabled(s.name, enabled);
            }
            ImGui::SameLine();
            ImGui::Text("[%s] p=%d  %s",
                (s.phase == eng::ecs::Phase::FixedUpdate) ? "Fixed" :
                (s.phase == eng::ecs::Phase::Update)      ? "Update" : "Render",
                s.priority,
                s.name.c_str()
            );
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("Bindings:");
        SDL_Scancode sc = eng::Input::binding(eng::Action::Pause);
        const char* name = SDL_GetScancodeName(sc);
        ImGui::Text("Pause -> %s", name && name[0] ? name : "<unbound>");
        
        SDL_Scancode step = eng::Input::binding(eng::Action::Step);
        const char* nameStep = SDL_GetScancodeName(step);
        ImGui::Text("Step  -> %s", nameStep && nameStep[0] ? nameStep : "<unbound>");
        
        ImGui::Separator();
        auto view = g_demoWorld.registry().view<eng::ecs::PlayerTag, eng::ecs::Transform2D>();
        for (auto [e, tag, t] : view) {
            (void)tag;
            ImGui::Text("Player pos: (%.2f, %.2f)", t.position.x, t.position.y);
            break;
        }

        ImGui::Separator();
        ImGui::Text("alpha: %.3f", eng::Time::interpolation());
        ImGui::Text("accum: %.4f", eng::Time::getAccumulator());

        ImGui::End();
        
        // Actualizar viewport al tamano real de la ventana cada frame.
        // Esto es necesario porque la ventana es resizable (SDL_WINDOW_RESIZABLE).
        {
            int w, h;
            SDL_GL_GetDrawableSize(g_window, &w, &h);
            glViewport(0, 0, w, h);
        }

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // RENDER
        float alpha = eng::Time::interpolation();
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        g_scheduler.render(g_demoWorld.registry(), alpha);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        SDL_GL_SwapWindow(g_window);

        // Cerrar el frame de input DESPUES de todo el procesamiento.
        // Esto copia s_curr -> s_prev para que el proximo frame
        // pueda detectar edges (wasPressed / wasReleased).
        eng::Input::endFrame();
    }
}

void Engine::shutdown() {

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    eng::Renderer2D::instance().shutdown();
    ImGui::DestroyContext();
    if (g_glContext) {
        SDL_GL_DeleteContext(g_glContext);
        g_glContext = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    SDL_Quit();
}

} // namespace eng