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
- Python 3 + Pillow (PIL) para herramientas de assets
- **Para compilar**:
```powershell
cd E:/engine-repo
powershell -Command "Import-Module 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'; Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\2022\Community' -Arch amd64 -SkipAutomaticLocation; cmake --preset windows-debug; cmake --build out/build/windows-debug"
```
- Si falla con LNK1168 (exe abierto): `taskkill /F /IM demo.exe` primero (usar powershell, no bash, por el /IM flag)
- Si falla con "SDL2 not found": borrar `out/build/windows-debug` y reconfigurar con `cmake --preset windows-debug`
- Para regenerar el tileset: `python tools/build_tileset.py`

## Plan de 10 fases
| Fase | Tema | Estado |
|------|------|--------|
| 0 | Bug fixes | COMPLETADA |
| 1 | Eliminar globals, EngineContext pattern | COMPLETADA |
| 2 | Texturas y Sprites | COMPLETADA |
| 3 | Sprite Animations | COMPLETADA |
| 4 | Tilemap + Tileset + Sprites + Mapa completo | COMPLETADA |
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
- **No se usa `new`/`delete` manual** — toda la memoria dinámica es via std::vector dentro de los pools (RAII puro, sin leaks)

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
| FixedUpdate | 150 | PlayerControlSystem | WASD -> velocidad + cambia clip de animacion + flipX |
| FixedUpdate | 200 | MovementSystem | Aplica velocidad a posicion |
| Update | 300 | AnimationSystem | Avanza timer, cambia frame, actualiza sprite.uvRect |
| Update | 900 | DebugUISystem | Panel ImGui de debug (FPS, profiler, toggle sistemas, player pos) |
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
- Fragment shader: usa `switch(vTexIndex)` con 16 cases explicitos para indexar `uTextures[]`
  - Indexar sampler arrays con variable no-constante es **undefined behavior en GLSL 330**
  - El switch explicito garantiza que cada `texture()` usa un indice constante en tiempo de compilacion
- **texIndex es int puro** — viaja como entero CPU→GPU→VS→FS sin conversion float. Usa `glVertexAttribIPointer` (con I)
- `getTextureSlot()` retorna `int` (no float)
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
Mapa 40x30 con zonas tematicas. Todo hardcodeado en main() (no hay carga desde archivo aun).

**Player:**
- Cute_Fantasy Player.png (192x320, 6x10 grid, 32x32 frames)
- 4 clips: idle(fila 0), walk_down(fila 3), walk_up(fila 5), walk_side(fila 4)
- Comienza en (0,0) — centro del mapa, cerca de la interseccion de caminos
- width/height = 2.0 world units

**Tilemap (3 capas):**
- Capa 0 (ground): todo pasto (GRASS=1)
- Capa 1 (terrain): caminos, agua, cultivos, acantilados
  - Camino horizontal cruza todo el mapa (fila 14-16)
  - Camino vertical en cruz (col 19-21, filas 2-27)
  - Interseccion rellenada con PATH_C
  - Lago grande 8x6 (sureste, pos 26,20)
  - Lago chico 5x4 (noroeste, pos 2,3)
  - 4 parcelas de cultivo 3x3 (suroeste)
  - Acantilado 9x3 (noreste, pos 28,2)
- Capa 2 (decor): flores, rocas, arbustos, cajones, barriles, carteles, lamparas, cofre, troncos, sacos, hongos, calabazas, enredaderas

**Objetos grandes (sprites posicionados, no tilemap):**
- 1 casa (House_1_Wood_Base_Blue.png, 6x8 world units, noroeste en -14,-10)
- 8 arboles grandes (Oak_Tree.png, 4x5 world units, bosque al este + sueltos)
- 2 arboles chicos (Oak_Tree_Small.png, con UV parcial para seleccionar cada arbolito)

**NPCs y animales:**
- 2 Skeletons (6x10 grid, idle animado, custodiando el bosque)
- 3 gallinas, 2 ovejas, 2 vacas, 1 cerdo (2x2 grid, idle animado, en la granja suroeste)
- 1 gallina + 1 oveja sueltas cerca del lago

**Helpers en main.cpp:**
- `makeStaticSprite()` — crea sprite estatico posicionado
- `makeAnimatedNPC()` — crea entidad con Transform + Sprite + SpriteAnimator
- `fillRect()` — rellena rectangulo de tiles (definido pero no usado actualmente)
- `makeWaterRect()` — dibuja rectangulo de agua con bordes
- `makePathH()` / `makePathV()` — dibuja caminos horizontales/verticales de 3 tiles de ancho

**Tileset (tileset.png, 256x160, 16 cols x 10 rows):**
```
Tile indices (0 = vacio, 1+ = tiles reales):
  Grass:     1
  PathMid:   2
  WaterMid:  3
  FarmLand:  4..12   (3x3 bordes)
  Path:      13..30  (3x6 bordes)
  Water:     31..48  (3x6 bordes)
  Cliff:     49..66  (3x6 bordes)
  Beach:     67..81  (5x3)
  Decor:     82..140 (59 tiles del Outdoor_Decor_Free.png, filtrados vacios)
  Fences:    141..156 (16 tiles del Fences.png)
  Chest:     157
```

### Asset pack: Cute_Fantasy_Free
Ubicacion: `demo/assets/Cute_Fantasy_Free/`
```
Player/
  Player.png           (192x320) 6x10 grid, 32x32 frames
  Player_Actions.png   (96x576) acciones extra (no usado aun)
Enemies/
  Skeleton.png         (192x320) 6x10 grid, 32x32 frames
  Slime_Green.png      (512x192) slime animado (no usado aun)
Animals/
  Chicken/Chicken.png  (64x64) 2x2 grid, 32x32 frames
  Cow/Cow.png          (64x64) 2x2 grid, 32x32 frames
  Pig/Pig.png          (64x64) 2x2 grid, 32x32 frames
  Sheep/Sheep.png      (64x64) 2x2 grid, 32x32 frames
Tiles/
  Grass_Middle.png     (16x16) tile suelto
  Path_Middle.png      (16x16) tile suelto
  Water_Middle.png     (16x16) tile suelto
  FarmLand_Tile.png    (48x48) 3x3 tiles
  Path_Tile.png        (48x96) 3x6 tiles
  Water_Tile.png       (48x96) 3x6 tiles
  Cliff_Tile.png       (48x96) 3x6 tiles
  Beach_Tile.png       (80x48) 5x3 tiles
Outdoor decoration/
  House_1_Wood_Base_Blue.png (96x128) casa completa
  Oak_Tree.png         (64x80) arbol grande
  Oak_Tree_Small.png   (96x48) 2 arbolitos
  Fences.png           (64x64) 4x4 grid de cercas
  Bridge_Wood.png      (144x64) piezas de puente (no usado aun)
  Chest.png            (16x16) cofre
  Outdoor_Decor_Free.png (112x192) 50+ decoraciones en grid 16x16
```

## Estructura de archivos

```
E:\engine-repo\
  CMakeLists.txt              # Root: standalone detection, add_subdirectory(engine + demo)
  CMakePresets.json            # windows-debug + windows-release presets
  CONTEXT.md                   # ESTE ARCHIVO — estado completo del proyecto
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
        Renderer2D.h           # Batch renderer con multi-texture (Vertex con texIndex int)
    src/
      Engine.cpp               # init/run/shutdown, game loop
      Time.cpp                 # Timestep implementation
      Input.cpp                # Keyboard state management
      ecs/
        Registry.cpp           # create/destroy/clear/removeAllComponents
        SystemScheduler.cpp    # addSystem/runPhase/sort
        systems/
          InputSystem.cpp      # Pause/Step handling
          PlayerControlSystem.cpp  # WASD + animation clip selection + flipX
          MovementSystem.cpp   # position += velocity * dt
          AnimationSystem.cpp  # Timer advance + frame change + uvRect update
          RenderSystem.cpp     # RenderQuad flush + TilemapRenderSystem + Sprite flush (con flipX)
          TilemapRenderSystem.cpp  # Frustum culling + tile rendering
          DebugUISystem.cpp    # ImGui debug panel (FPS, profiler, toggle sistemas, bindings, player pos)
      render/
        Renderer2D.cpp         # Shaders (switch-based sampler), VAO/VBO, texture slots, submit/flush
        TextureManager.cpp     # stb_image loading, GL texture upload
  demo/
    CMakeLists.txt             # Ejecutable demo + post-build asset copy
    src/
      main.cpp                 # Mapa 40x30, casa, arboles, granja, lago, NPCs, animales
    assets/
      player.png               # [legacy, ya no usado]
      tileset.png              # Tileset combinado (256x160, 16x10 de 16x16) generado por build_tileset.py
      Cute_Fantasy_Free/       # Asset pack completo (ver seccion arriba)
  tools/
    build_tileset.py           # Python: combina tiles + decoraciones + fences + chest en tileset atlas
```

## Bugs conocidos y resueltos
1. **SDL2 x86 vs x64**: Necesita `-Arch amd64` en Enter-VsDevShell para que detecte el compiler x64
2. **Texture bleeding**: Mezclar texturas dummy blanca con texturas reales en el mismo batch causaba artefactos blancos fantasma. Solucion: flush() entre quads de color y sprites texturizados
3. **texIndex como float + sampler array UB**: Dos problemas combinados:
   - Pasar texIndex como float al shader y convertir con `int(round())` no es confiable en todos los GPU
   - Indexar `uTextures[variable]` es undefined behavior en GLSL 330
   - Solucion: texIndex a `int` puro con `glVertexAttribIPointer`, y `switch` explicito en fragment shader con 16 cases constantes
4. **Asset path**: demo.exe no encontraba player.png porque el working directory no coincidia. Solucion: post-build copy command en demo/CMakeLists.txt
5. **Sub-pixel artifacts**: Posicion sub-pixel causaba sampling inconsistente con GL_NEAREST. Solucion: pixel-snap con round(size) + floor(position)
6. **Animals wrong size**: Animales eran 2x2 grid de 32x32 (64x64 total), no 4x4 de 16x16. Solucion: uvRect {0,0,0.5,0.5} y framesFromGrid(2,2,0)
7. **CMake cache stale**: Cuando falla con "SDL2 not found" o "Ninja not found", borrar `out/build/windows-debug` y reconfigurar

## Idioma
El usuario habla en español. Responder en español.

## Notas para la proxima sesion
- El usuario esta aprendiendo — le gusta entender cada concepto antes de implementar
- A veces hace cambios independientes al codigo y pide que se revisen/corrijan
- Errores comunes del usuario: namespaces sin 's', bugs logicos en expresiones (+=  a + b en vez de += b), push_back() vacios
- Siempre compilar despues de cada cambio y verificar 0 errores
- El usuario confirma visualmente con screenshots que todo funciona
- Prefiere RAII y evitar new/delete — toda la memoria via containers STL
- El mapa actual tiene algunas decoraciones "sin sentido" (tile IDs asignados a ojo) — se puede refinar despues
