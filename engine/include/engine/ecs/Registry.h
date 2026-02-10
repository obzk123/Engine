#pragma once
#include "engine/ecs/Entity.h"
#include "engine/ecs/ComponentPool.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <cstdint>
#include <cassert>
#include <tuple>
#include <initializer_list>
#include <limits>

namespace eng::ecs {

class Registry {
public:
    Registry() = default;

    Entity create();
    void destroy(Entity e);

    bool isAlive(Entity e) const;

    size_t aliveCount() const { return m_aliveCount; }

    template <typename T>
    bool has(Entity e) {
        auto* pool = tryGetPool<T>();
        return pool ? pool->has(e) : false;
    }

    template <typename T>
    T& get(Entity e) {
        auto* pool = getOrCreatePool<T>();
        return pool->get(e);
    }

    template <typename T, typename... Args>
    T& emplace(Entity e, Args&&... args) {
        assert(isAlive(e));
        auto* pool = getOrCreatePool<T>();
        return pool->emplace(e, std::forward<Args>(args)...);
    }

    template <typename T>
    void remove(Entity e) {
        auto* pool = tryGetPool<T>();
        if (pool) pool->remove(e);
    }

    template <typename T>
    size_t componentCount() const {
        auto* pool = tryGetPoolConst<T>();
        return pool ? pool->size() : 0;
    }

    void clear();

    template <typename... Ts>
    class View {
    public:
        View(Registry& reg) : m_reg(reg) {
            // Elegimos el pool "driver": el más chico para iterar menos
            pickDriverPool();
        }

        class Iterator {
        public:
            // Unico constructor: requiere la referencia al vector de entidades
            // del driver pool. Las referencias en C++ DEBEN inicializarse,
            // por eso no puede existir un constructor sin este parametro.
            Iterator(Registry& reg,
                     const std::vector<Entity>& driverEntities,
                     size_t i,
                     size_t end)
                : m_reg(reg),
                  m_driverEntities(driverEntities),
                  m_i(i),
                  m_end(end) {
                advanceToValid();
            }

            Iterator& operator++() {
                ++m_i;
                advanceToValid();
                return *this;
            }

            bool operator!=(const Iterator& other) const {
                return m_i != other.m_i;
            }

            auto operator*() const {
                Entity e = m_driverEntities[m_i];
                return std::tuple<Entity, Ts&...>(e, m_reg.get<Ts>(e)...);
            }

        private:
            void advanceToValid() {
                while (m_i < m_end) {
                    Entity e = m_driverEntities[m_i];
                    // Verificamos que la entidad tenga TODOS los componentes
                    // requeridos. El driver pool garantiza al menos uno,
                    // pero necesitamos chequear los demas.
                    if ((m_reg.has<Ts>(e) && ...)) {
                        return;
                    }
                    ++m_i;
                }
            }

            Registry& m_reg;
            const std::vector<Entity>& m_driverEntities;
            size_t m_i = 0;
            size_t m_end = 0;
        };

        Iterator begin() {
            if (!m_valid) {
                static const std::vector<Entity> empty;
                return Iterator(m_reg, empty, 0, 0);
            }
            return Iterator(m_reg, *m_driverEntities, 0, m_driverEntities->size());
        }

        Iterator end() {
            if (!m_valid) {
                static const std::vector<Entity> empty;
                return Iterator(m_reg, empty, 0, 0);
            }
            return Iterator(m_reg, *m_driverEntities, m_driverEntities->size(), m_driverEntities->size());
        }

        bool valid() const { return m_valid; }

    private:
        void pickDriverPool() {
            // Si falta cualquiera de los pools, la view es vacía.
            if (!allPoolsExist()) {
                m_valid = false;
                return;
            }

            // Elegir el pool con menor size() como driver.
            // Necesitamos obtener los denseEntities() del pool driver.
            size_t bestSize = std::numeric_limits<size_t>::max();
            pickDriverImpl<Ts...>(bestSize);

            m_valid = (m_driverEntities != nullptr);
        }

        bool allPoolsExist() {
            return ((m_reg.poolOrNull<Ts>() != nullptr) && ...);
        }

        template <typename T>
        void considerDriver(size_t& bestSize) {
            auto* p = m_reg.poolOrNull<T>();
            if (!p) return;
            size_t s = p->size();
            if (s < bestSize) {
                bestSize = s;
                m_driverEntities = &p->denseEntities();
            }
        }

        template <typename T0, typename... Rest>
        void pickDriverImpl(size_t& bestSize) {
            considerDriver<T0>(bestSize);
            if constexpr (sizeof...(Rest) > 0) {
                pickDriverImpl<Rest...>(bestSize);
            }
        }

    private:
        Registry& m_reg;
        bool m_valid = false;
        const std::vector<Entity>* m_driverEntities = nullptr;
    };

    template <typename... Ts>
    View<Ts...> view() {
        return View<Ts...>(*this);
    }

private:
    struct Slot {
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot> m_slots;
    std::vector<uint32_t> m_freeList;
    size_t m_aliveCount = 0;

    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;

    template <typename T>
    ComponentPool<T>* getOrCreatePool() {
        std::type_index key(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            ComponentPool<T>* raw = pool.get();
            m_pools.emplace(key, std::move(pool));
            return raw;
        }
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T>
    ComponentPool<T>* tryGetPool() {
        std::type_index key(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) return nullptr;
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T>
    const ComponentPool<T>* tryGetPoolConst() const {
        std::type_index key(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const ComponentPool<T>*>(it->second.get());
    }

    void removeAllComponents(Entity e);

    template <typename T>
    ComponentPool<T>* poolOrNull() {
        return tryGetPool<T>();
    }

    template <typename T>
    const ComponentPool<T>* poolOrNull() const {
        return tryGetPoolConst<T>();
    }

};

} // namespace eng::ecs
