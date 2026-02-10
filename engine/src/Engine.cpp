#include "engine/Engine.h"
#include "engine/Time.h"
#include "engine/Input.h"

#include <SDL.h>
#include <glad/glad.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

namespace eng {

bool Engine::init() {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(
        "My Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!m_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
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
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    eng::Time::init();
    eng::Input::init();

    // Inicializar renderer
    m_renderer.init();

    // Setear el contexto en el registry para que los sistemas puedan
    // acceder a window, renderer, profiler y scheduler sin globals.
    m_registry.setContext({m_window, &m_renderer, &m_profiler, &m_scheduler});

    m_running = true;
    return true;
}

void Engine::run() {
    while (m_running) {
        eng::Time::beginFrame();
        m_profiler.beginFrame();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) m_running = false;
            eng::Input::onEvent(e);
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        // Fixed update loop
        while (eng::Time::consumeFixedStep()) {
            m_scheduler.fixedUpdate(m_registry, eng::Time::fixedDeltaTime());
        }

        // ImGui new frame — ANTES de update para que DebugUISystem pueda usar ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Update (incluye InputSystem y DebugUISystem)
        m_scheduler.update(m_registry, eng::Time::deltaTime());

        // Actualizar viewport al tamano real de la ventana cada frame.
        // Esto es necesario porque la ventana es resizable (SDL_WINDOW_RESIZABLE).
        {
            int w, h;
            SDL_GL_GetDrawableSize(m_window, &w, &h);
            glViewport(0, 0, w, h);
        }

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render
        float alpha = eng::Time::interpolation();
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        m_scheduler.render(m_registry, alpha);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(m_window);

        // Cerrar el frame de input DESPUES de todo el procesamiento.
        // Esto copia s_curr -> s_prev para que el proximo frame
        // pueda detectar edges (wasPressed / wasReleased).
        eng::Input::endFrame();
    }
}

void Engine::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    m_renderer.shutdown();
    ImGui::DestroyContext();
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

} // namespace eng
