# Engine Project Context — Session Recovery File
> Leelo completo antes de hacer cualquier cosa. Contiene TODO el estado del proyecto.

## Proyecto
Motor de juegos 2D estilo Stardew Valley en C++20 con ECS propio (sparse-dense pools).

## Ubicacion
- **Engine repo**: `E:\engine-repo\`
- **Build**: `E:\engine-repo\out\build\windows-debug\`
- **Ejecutable**: `E:\engine-repo\out\build\windows-debug\demo\demo.exe`

## Toolchain
- MSVC 2022 (cl.exe x64), Ninja, CMake 3.24+
- vcpkg (SDL2, glad, glm, stb)
- ImGui vendorizado en `external/imgui/`
- **Para compilar**:
```powershell
cd E:/engine-repo
powershell -Command "Import-Module 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'; Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\2022\Community' -Arch amd64 -SkipAutomaticLocation; cmake --preset windows-debug; cmake --build out/build/windows-debug"
```
- Si falla con LNK1168 (exe abierto): `taskkill /F /IM demo.exe` primero (usar powershell, no bash, por el /IM flag)

## Plan de 10 fases
| Fase | Tema | Estado |
|------|------|--------|
| 0 | Bug fixes | COMPLETADA |
| 1 | Eliminar globals, EngineContext pattern | COMPLETADA |
| 2 | Texturas y Sprites | COMPLETADA |
| 3 | Sprite Animations | COMPLETADA |
| 4 | Tilemap + Tileset + Sprites | COMPLETADA |
| **5** | **Colisiones (AABB, spatial grid)** | **SIGUIENTE** |
| 6 | Audio (SDL_mixer) | Pendiente |
| 7 | Scene management | Pendiente |
| 8 | Serialization (save/load) | Pendiente |
| 9 | Polish (UI, menus, packaging) | Pendiente |

## Arquitectura del Engine

### ECS (Entity Component System)
- `Entity` = {index, generation} — generational IDs para detectar stale handles
- `ComponentPool<T>` = sparse-dense array, O(1) add/remove/get
- `Registry` = maneja entidades + pools + EngineContext
- `EngineContext` = punteros a subsistemas (window, renderer, profiler, scheduler, textures) accesible via `reg.ctx()`
- `SystemScheduler` = ejecuta sistemas por Phase (FixedUpdate, Update, Render) ordenados por prioridad
- `View<Ts...>` = iterador multi-componente que elige el pool mas chico como driver

### Subsistemas
- `Engine` = orquestador principal, dueño de todos los subsistemas
- `Renderer2D` = batch renderer con multi-texture (hasta 16 slots), shaders GLSL 330
- `TextureManager` = carga PNG/JPG via stb_image, cache por path, GL_NEAREST para pixel art
- `Profiler` = rolling average por sistema, visible en ImGui
- `Input` = keyboard con action mapping, edge detection (pressed/released)
- `Time` = semi-fixed timestep, pause/step

### Componentes ECS (en Components.h)
- `Transform2D` = position, prevPosition (para interpolacion), rotation
- `Velocity2D` = velocity vec2
- `PlayerTag` = marca al player
- `Color4` = RGBA float
- `RenderQuad` = quad de color solido (layer, w, h, color)
- `Sprite` = quad texturizado (TextureHandle, uvRect, tint, layer, width, height, flipX)
- `AnimationClip` = secuencia de frames UV (name, frames vector, frameDuration, loop)
- `SpriteAnimator` = vector de clips, currentClip, currentFrame, timer, playing
- `TilemapLayer` = flat array de tile IDs (uint16_t), renderOrder
- `Tilemap` = Tileset + width/height + vector de TilemapLayer

### Sistemas (orden de ejecucion)
| Phase | Priority | Sistema | Que hace |
|-------|----------|---------|----------|
| Update | 10 | InputSystem | Lee pause/step |
| FixedUpdate | 150 | PlayerControlSystem | WASD -> velocidad + cambia clip de animacion |
| FixedUpdate | 200 | MovementSystem | Aplica velocidad a posicion |
| Update | 300 | AnimationSystem | Avanza timer, cambia frame, actualiza sprite.uvRect |
| Update | 900 | DebugUISystem | Panel ImGui de debug |
| Render | 100 | RenderSystem | RenderQuads (flush) + TilemapRenderSystem + Sprites con flipX (flush) |

### Game loop (Engine::run)
```
while running:
    Time::beginFrame()
    Profiler::beginFrame()
    SDL_PollEvent (quit, input, imgui)
    while consumeFixedStep():
        scheduler.fixedUpdate(reg, fixedDt)    // PlayerControl, Movement
    ImGui::NewFrame()  // ANTES de update para que DebugUI pueda usar ImGui
    scheduler.update(reg, dt)                  // Input, Animation, DebugUI
    glViewport + glClear
    scheduler.render(reg, alpha)               // RenderSystem
    ImGui::Render()
    SDL_GL_SwapWindow
    Input::endFrame()
```

### Renderer2D detalles
- Vertex = {x, y, r, g, b, a, u, v, texIndex(int)}
- Vertex shader: pixels -> NDC, pasa vColor/vTexCoord/vTexIndex(flat int)
- Fragment shader: `texture(uTextures[vTexIndex], vTexCoord) * vColor`
- **texIndex es int puro** — viaja como entero CPU→GPU→VS→FS sin conversion float. Usa `glVertexAttribIPointer` (con I)
- `submitQuad()` = internamente usa submitTexturedQuad con textura dummy blanca
- `submitTexturedQuad()` = world->screen + pixel-snap (round size + floor position) + 6 vertices (2 triangulos)
- `flushBatch()` = sube VBO, bindea texturas activas, glDrawArrays, limpia
- Alpha blending habilitado (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
- **IMPORTANTE**: RenderSystem hace flush() entre quads de color y sprites texturizados para evitar artefactos de blending

### TextureManager detalles
- Handle 0 = textura dummy blanca 1x1 (siempre valida, fallback)
- stb_image con RGBA forzado (4 canales)
- GL_NEAREST filtering (pixel art crujiente)
- GL_CLAMP_TO_EDGE
- Cache por path (no recarga si ya existe)

### Sprite Animation detalles
- Sprite sheet = una imagen con multiples frames en grilla
- `framesFromGrid(cols, rows, row, startCol, count)` = helper que genera vector de Rects UV
- AnimationSystem itera `<SpriteAnimator, Sprite>`, avanza timer, actualiza sprite.uvRect
- PlayerControlSystem cambia animator.currentClip segun direccion (0=idle, 1=walk_down, 2=walk_up, 3=walk_side)
- Al cambiar clip se resetea currentFrame=0, timer=0
- `flipX` en Sprite: RenderSystem invierte UVs (uv.x += uv.w, uv.w = -uv.w) para espejado horizontal
- PlayerControlSystem setea flipX segun direccion horizontal del movimiento

### Tilemap detalles
- `Tileset` (header-only, `Tileset.h`): mapea tile index a UV rect. Index 0 = vacio, 1+ = tiles reales (row-major)
- `TilemapLayer` = flat array uint16_t [row * width + col], renderOrder para ordenar capas
- `Tilemap` = Tileset + dimensiones + vector de capas
- `TilemapRenderSystem` = funcion llamada DESDE RenderSystem (no es un sistema registrado en scheduler)
  - Recibe camCenter, screenW, screenH como parametros
  - Frustum culling: calcula rango de tiles visibles segun camara, solo dibuja los visibles
  - Itera capas ordenadas por renderOrder, cada tile visible → submitTexturedQuad
  - flush() al final
- Pipeline de assets: `tools/build_tileset.py` combina tiles individuales (Cute_Fantasy_Free) en un tileset atlas

### Demo scene (demo/main.cpp)
- Player: Cute_Fantasy Player.png (6x10 grid, 32x32 frames, 4 clips: idle/walk_down/walk_up/walk_side)
- Skeleton: NPC estatico animado en (5,0), mismo layout que Player
- 4 animales: Chicken, Sheep, Cow, Pig (2x2 grid, 32x32 frames cada uno)
- Tilemap 24x20: pasto, camino horizontal, lago de agua, zona de cultivo
- Tileset: tileset.png (256x96, 16 cols x 6 rows de 16x16)

## Estructura de archivos

```
E:\engine-repo\
  CMakeLists.txt              # Root: standalone detection, add_subdirectory(engine + demo)
  CMakePresets.json            # windows-debug + windows-release presets
  external/
    vcpkg/                     # vcpkg submodule
    imgui/                     # ImGui vendorizado (core + SDL2/OpenGL3 backends)
  engine/
    CMakeLists.txt             # Libreria estatica 'engine'
    include/engine/
      Engine.h                 # Clase principal, dueña de todos los subsistemas
      Time.h                   # Semi-fixed timestep, pause, interpolation
      Input.h                  # Keyboard input con action mapping
      Actions.h                # Enum Action (Pause, Step, MoveLeft/Right/Up/Down)
      Profiling.h              # Profiler + ScopeTimer
      ecs/
        Entity.h               # Entity = {index, generation}
        ComponentPool.h        # Sparse-dense pool template
        Registry.h             # ECS registry + EngineContext + View
        Components.h           # Transform2D, Velocity2D, PlayerTag, Color4, RenderQuad, Sprite(+flipX), AnimationClip, SpriteAnimator, TilemapLayer, Tilemap
        SystemScheduler.h      # Phase-based system execution
        systems/
          InputSystem.h
          PlayerControlSystem.h
          MovementSystem.h
          AnimationSystem.h
          RenderSystem.h
          DebugUISystem.h
          TilemapRenderSystem.h  # Funcion llamada desde RenderSystem
      render/
        Texture.h              # TextureHandle, Rect, Texture, framesFromGrid()
        Tileset.h              # Header-only: tile index -> UV rect mapping
        TextureManager.h       # Carga/cache/GPU upload de texturas
        Renderer2D.h           # Batch renderer con multi-texture
    src/
      Engine.cpp               # init/run/shutdown, game loop
      Time.cpp                 # Timestep implementation
      Input.cpp                # Keyboard state management
      ecs/
        Registry.cpp           # create/destroy/clear/removeAllComponents
        SystemScheduler.cpp    # addSystem/runPhase/sort
        systems/
          InputSystem.cpp      # Pause/Step handling
          PlayerControlSystem.cpp  # WASD + animation clip selection
          MovementSystem.cpp   # position += velocity * dt
          AnimationSystem.cpp  # Timer advance + frame change + uvRect update
          RenderSystem.cpp     # RenderQuad flush + TilemapRenderSystem + Sprite flush (con flipX)
          TilemapRenderSystem.cpp  # Frustum culling + tile rendering
          DebugUISystem.cpp    # ImGui debug panel
      render/
        Renderer2D.cpp         # Shaders, VAO/VBO, texture slots, submit/flush
        TextureManager.cpp     # stb_image loading, GL texture upload
  demo/
    CMakeLists.txt             # Ejecutable demo + post-build asset copy
    src/
      main.cpp                 # Crea engine, player, skeleton, animales, tilemap
    assets/
      player.png               # Sprite sheet 64x64 (4x4 frames de 16x16) [legacy, ya no usado]
      tileset.png              # Tileset combinado (256x96, 16x6 de 16x16) generado por build_tileset.py
      Cute_Fantasy_Free/       # Asset pack: Player, Enemies, Animals, Tiles
  tools/
    build_tileset.py           # Script Python que combina tiles individuales en tileset atlas
```

## Bugs conocidos y resueltos
1. **SDL2 x86 vs x64**: Necesita `-Arch amd64` en Enter-VsDevShell para que detecte el compiler x64
2. **Texture bleeding**: Mezclar texturas dummy blanca con texturas reales en el mismo batch causaba artefactos blancos fantasma. Solucion: flush() entre quads de color y sprites texturizados
3. **texIndex como float**: Pasar texIndex como float al shader causaba artefactos intermitentes en sprites con multiples texturas. `int(round(vTexIndex))` no es confiable en todos los GPU. Solucion: cambiar texIndex a `int` en Vertex, usar `glVertexAttribIPointer`, y `flat in int` en el shader. Cero conversiones float
4. **Asset path**: demo.exe no encontraba player.png porque el working directory no coincidia. Solucion: post-build copy command en demo/CMakeLists.txt
5. **Sub-pixel artifacts**: Posicion sub-pixel causaba sampling inconsistente con GL_NEAREST. Solucion: pixel-snap con round(size) + floor(position)

## Idioma
El usuario habla en español. Responder en español.

## Notas para la proxima sesion
- El usuario esta aprendiendo — le gusta entender cada concepto antes de implementar
- A veces hace cambios independientes al codigo y pide que se revisen/corrijan
- Errores comunes del usuario: namespaces sin 's', bugs logicos en expresiones (+=  a + b en vez de += b), push_back() vacios
- Siempre compilar despues de cada cambio y verificar 0 errores
- El usuario confirma visualmente con screenshots que todo funciona
