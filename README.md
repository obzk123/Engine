# Engine

Motor grafico 2D con arquitectura ECS (Entity Component System) escrito en C++20.

Diseñado para desarrollar juegos estilo top-down (Stardew Valley, Zelda). El motor se enfoca en separar datos (componentes) de logica (sistemas) para lograr buena performance con miles de entidades y un codigo facil de extender.

## Requisitos

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** con el workload "Desarrollo para escritorio con C++"
- **CMake** 3.24 o superior
- **Git** con soporte de submodules

Ninja, vcpkg y las dependencias se manejan automaticamente.

## Clonar el repositorio

```bash
git clone --recursive https://github.com/obzk123/Engine.git
cd Engine
```

El flag `--recursive` es importante porque descarga los submodules (imgui, vcpkg).

Si ya clonaste sin `--recursive`:

```bash
git submodule update --init --recursive
```

## Compilar

### 1. Abrir Developer Command Prompt

Abri **"Developer PowerShell for VS 2022"** o **"x64 Native Tools Command Prompt for VS 2022"** desde el menu de inicio. Esto configura el compilador (cl.exe) y las herramientas en el PATH.

### 2. Bootstrap vcpkg (solo la primera vez)

```bash
.\external\vcpkg\bootstrap-vcpkg.bat
```

### 3. Configurar y compilar

```bash
cmake --preset windows-debug
cmake --build out/build/windows-debug
```

Para release:

```bash
cmake --preset windows-release
cmake --build out/build/windows-release
```

## Usarlo desde un juego

El engine esta pensado para ser consumido como **git submodule** desde otro proyecto. Ver [Game](https://github.com/obzk123/Game.git) como ejemplo.

En tu proyecto:

```bash
git submodule add https://github.com/obzk123/Engine.git engine
```

En tu `CMakeLists.txt` raiz:

```cmake
cmake_minimum_required(VERSION 3.24)
project(MiJuego LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)   # trae el target "engine"
add_subdirectory(game)     # tu codigo
```

En tu `game/CMakeLists.txt`:

```cmake
add_executable(game src/main.cpp)
target_link_libraries(game PRIVATE engine)
```

Con `PRIVATE engine` heredas automaticamente todos los includes y dependencias (SDL2, OpenGL, glad, glm, stb).

En tu `CMakePresets.json` apunta vcpkg al submodule:

```json
"CMAKE_TOOLCHAIN_FILE": "${sourceDir}/engine/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

## Arquitectura

### ECS (Entity Component System)

El motor usa el patron ECS donde:

- **Entidades** son identificadores (un index + una generation para detectar handles invalidos)
- **Componentes** son estructuras de datos puras, sin logica
- **Sistemas** son funciones que operan sobre entidades que tengan ciertos componentes

Los sistemas se actualizan en orden. Las entidades no se actualizan — no tienen metodos ni herencia. Esto da buena localidad de cache y permite agregar comportamiento sin tocar codigo existente.

```
Entidad = solo un ID
Componentes = datos (posicion, velocidad, sprite, etc.)
Sistemas = logica (mover, renderizar, detectar colisiones, etc.)
```

### Entity

Handle liviano de 64 bits:

```cpp
eng::ecs::Entity e = registry.create();   // crear
registry.destroy(e);                       // destruir
registry.isAlive(e);                       // verificar si sigue viva
```

El campo `generation` se incrementa cada vez que un slot se reutiliza. Si guardas un handle viejo y la entidad fue destruida, `isAlive()` retorna false y las operaciones de componentes lo detectan.

### Componentes

Son structs planas que se agregan a entidades:

```cpp
// Componentes incluidos en el engine
struct Transform2D {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 prevPosition{0.0f, 0.0f};  // para interpolacion
    float rotation = 0.0f;
};

struct Velocity2D {
    glm::vec2 velocity{0.0f, 0.0f};
};

struct PlayerTag {};  // tag: sin datos, solo marca entidades

struct RenderQuad {
    int layer = 0;
    float w = 1.0f, h = 1.0f;
    Color4 color{};
};
```

Crear tus propios componentes es simplemente definir un struct:

```cpp
struct Health {
    int current = 100;
    int max = 100;
};

struct Enemy {
    float aggroRange = 5.0f;
};
```

### Registry

Centro del ECS. Maneja entidades y componentes:

```cpp
eng::ecs::Registry registry;

// Crear entidad y agregarle componentes
Entity player = registry.create();
registry.emplace<Transform2D>(player, glm::vec2{0, 0}, glm::vec2{0, 0}, 0.0f);
registry.emplace<Velocity2D>(player);
registry.emplace<PlayerTag>(player);

// Consultar y modificar
if (registry.has<Transform2D>(player)) {
    auto& t = registry.get<Transform2D>(player);
    t.position.x += 1.0f;
}

// Remover componente
registry.remove<Velocity2D>(player);
```

### Views (queries)

Para iterar sobre entidades que tengan un conjunto especifico de componentes:

```cpp
// Todas las entidades con Transform2D Y Velocity2D
auto view = registry.view<Transform2D, Velocity2D>();
for (auto [entity, transform, vel] : view) {
    transform.position += vel.velocity * dt;
}

// Buscar el player
auto players = registry.view<PlayerTag, Transform2D>();
for (auto [e, tag, t] : players) {
    // solo hay uno, pero la API es generica
}
```

El view internamente elige el pool mas chico como "driver" para iterar lo minimo posible.

### ComponentPool

Cada tipo de componente tiene su propio pool con almacenamiento **sparse-dense**:

- **Sparse array**: `[entity_index] -> dense_index` (lookup O(1))
- **Dense arrays**: componentes empaquetados contiguamente en memoria (iteracion cache-friendly)
- **Insercion, eliminacion, busqueda**: todas O(1) gracias a swap-and-pop

Esto significa que iterar 10,000 entidades con `Transform2D` es tan rapido como recorrer un array contiguo.

### Sistemas y el Scheduler

Los sistemas son funciones con firma `void(Registry&, float)` que se registran en un scheduler con fase y prioridad:

```cpp
// Definir un sistema
void MiSistemaDeMovimiento(eng::ecs::Registry& reg, float dt) {
    auto view = reg.view<Transform2D, Velocity2D>();
    for (auto [e, t, v] : view) {
        t.prevPosition = t.position;
        t.position += v.velocity * dt;
    }
}

// Registrarlo
scheduler.addSystem("Movement", Phase::FixedUpdate, 200, MiSistemaDeMovimiento);
```

**Fases de ejecucion** (en orden):

| Fase | Parametro `float` | Frecuencia | Uso |
|------|-------------------|------------|-----|
| `FixedUpdate` | `fixedDt` (1/60) | 60 veces por segundo, fijo | Fisica, movimiento, logica de juego |
| `Update` | `deltaTime` | 1 vez por frame, variable | Input, UI, cosas visuales |
| `Render` | `alpha` (0.0 - 1.0) | 1 vez por frame | Dibujar, interpolar posiciones |

Dentro de cada fase, los sistemas se ejecutan por prioridad (menor numero = se ejecuta primero).

El parametro `alpha` en Render es el factor de interpolacion. Sirve para renderizar posiciones suaves entre el estado fisico anterior y el actual:

```cpp
void MiRenderSystem(eng::ecs::Registry& reg, float alpha) {
    for (auto [e, t, rq] : reg.view<Transform2D, RenderQuad>()) {
        // Interpolar entre posicion anterior y actual
        glm::vec2 renderPos = t.prevPosition + (t.position - t.prevPosition) * alpha;
        renderer.submitQuad(renderPos, rq.w, rq.h, rq.color);
    }
}
```

### Timestep fijo con interpolacion

El motor usa un **semi-fixed timestep**:

1. Cada frame, el tiempo delta se acumula
2. El acumulador se consume en pasos fijos de 1/60 segundo
3. La fisica corre siempre a 60Hz, independiente del framerate
4. El render interpola entre el estado anterior y el actual con `alpha`

Esto da:
- Fisica deterministica (siempre el mismo dt)
- Render suave a cualquier FPS
- Sin "spiral of death" (el delta se clampea a 0.25s)

```
Frame 1 (dt=0.018s): accumulator=0.018 -> 1 fixed step
Frame 2 (dt=0.015s): accumulator=0.016 -> 0 fixed steps (se acumula)
Frame 3 (dt=0.017s): accumulator=0.033 -> 2 fixed steps
```

### Input

Sistema de input con dos capas:

**Capa baja** — scancodes directos de SDL:

```cpp
Input::isDown(SDL_SCANCODE_W);      // esta presionada ahora?
Input::wasPressed(SDL_SCANCODE_W);   // se presiono este frame?
Input::wasReleased(SDL_SCANCODE_W);  // se solto este frame?
```

**Capa alta** — acciones logicas (rebindeables):

```cpp
Input::bind(Action::MoveUp, SDL_SCANCODE_W);

Input::actionDown(Action::MoveUp);      // abstraido del scancode
Input::actionPressed(Action::Pause);
```

Acciones definidas actualmente:

| Accion | Tecla default |
|--------|---------------|
| `Pause` | P |
| `Step` | O |
| `MoveLeft` | A |
| `MoveRight` | D |
| `MoveUp` | W |
| `MoveDown` | S |

### Renderer2D

Renderer basico de quads 2D con OpenGL 3.3:

```cpp
auto& r = Renderer2D::instance();
r.beginFrame(screenW, screenH);
r.setCamera(playerPos, 64.0f);         // 64 pixels por unidad de mundo
r.submitQuad({0, 0}, 1.0f, 1.0f, {1, 0, 0, 1});  // quad rojo en el origen
r.flush();                              // enviar todo al GPU
```

Coordenadas de mundo se transforman a pantalla automaticamente segun la camara y la escala (pixels-per-unit).

### Profiling

Cada sistema se mide automaticamente. El profiler guarda una ventana de muestras y calcula estadisticas:

```cpp
// Medicion automatica con RAII
{
    ScopeTimer timer(profiler, "MiSistema");
    // ... codigo del sistema ...
}  // al destruirse, registra el tiempo

// Consultar estadisticas
for (auto& [name, stats] : profiler.stats()) {
    // stats.lastMs, stats.avgMs, stats.minMs, stats.maxMs
}
```

El debug overlay de ImGui muestra estas metricas en tiempo real.

## Sistemas incluidos

El engine viene con estos sistemas de ejemplo:

| Sistema | Fase | Lo que hace |
|---------|------|-------------|
| `InputSystem` | Update | Detecta pausa (P) y step (O) |
| `PlayerControlSystem` | FixedUpdate | Lee WASD, normaliza diagonal, setea velocidad |
| `MovementSystem` | FixedUpdate | `position += velocity * dt` para toda entidad |
| `RenderSystem` | Render | Interpola posiciones y dibuja quads con la camara siguiendo al player |

## Estructura del proyecto

```
Engine/
├── engine/
│   ├── include/engine/          # Headers publicos
│   │   ├── Engine.h             # Clase principal del motor
│   │   ├── Time.h               # Manejo de tiempo y timestep fijo
│   │   ├── Input.h              # Input con action binding
│   │   ├── Actions.h            # Enum de acciones
│   │   ├── Profiling.h          # Profiler y ScopeTimer
│   │   ├── ecs/
│   │   │   ├── Entity.h         # Handle de entidad (index + generation)
│   │   │   ├── Components.h     # Componentes base del motor
│   │   │   ├── ComponentPool.h  # Pool sparse-dense por tipo
│   │   │   ├── Registry.h       # Registro central de entidades/componentes
│   │   │   ├── SystemScheduler.h# Scheduler de fases y prioridades
│   │   │   ├── DemoWorld.h      # Mundo de prueba
│   │   │   └── systems/         # Sistemas incluidos
│   │   └── render/
│   │       └── Renderer2D.h     # Renderer de quads 2D
│   └── src/                     # Implementaciones (.cpp)
├── external/
│   ├── imgui/                   # Dear ImGui (submodule)
│   └── vcpkg/                   # Gestor de paquetes (submodule)
├── tools/                       # Herramientas futuras
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

## Dependencias

Manejadas automaticamente por vcpkg:

| Libreria | Version | Uso |
|----------|---------|-----|
| SDL2 | 2.32 | Ventana, eventos, input, audio |
| glad | 0.1.36 | Carga de funciones OpenGL |
| glm | 1.0.3 | Matematica (vectores, matrices) |
| stb | 2024-07 | Carga de imagenes (header-only) |
| Dear ImGui | latest | UI de debug (vendorizado como submodule) |

## Licencia

MIT
