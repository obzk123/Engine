#pragma once
#include "engine/ecs/Entity.h"

#include <vector>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cassert>

namespace eng::ecs {

// Interface para manejar pools sin conocer el tipo T
struct IComponentPool {
    virtual ~IComponentPool() = default;
    virtual void removeIfExists(Entity e) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
};

template <typename T>
class ComponentPool final : public IComponentPool {
    // Componentes ECS deben ser movibles para swap-remove en el dense array.
    // Si ves este error, tu componente necesita move constructor y move assignment.
    static_assert(std::is_move_constructible_v<T>,
        "ECS components must be move-constructible (needed for swap-remove).");
    static_assert(std::is_move_assignable_v<T>,
        "ECS components must be move-assignable (needed for swap-remove).");

public:
    ComponentPool() = default;

    bool has(Entity e) const {
        if (e.index >= m_sparse.size()) return false;
        uint32_t denseIndex = m_sparse[e.index];
        if (denseIndex == InvalidDense) return false;
        // Verificamos index Y generation para detectar handles stale.
        // Si la entidad fue destruida y el slot reutilizado, la generation
        // no va a coincidir y retornamos false correctamente.
        return m_denseEntities[denseIndex] == e;
    }

    T& get(Entity e) {
        assert(has(e));
        return m_denseComponents[m_sparse[e.index]];
    }

    const T& get(Entity e) const {
        assert(has(e));
        return m_denseComponents[m_sparse[e.index]];
    }

    template <typename... Args>
    T& emplace(Entity e, Args&&... args) {
        ensureSparseSize(e.index);

        if (has(e)) {
            // si ya existe, lo reasignamos
            T& existing = get(e);
            existing = T(std::forward<Args>(args)...);
            return existing;
        }

        uint32_t denseIndex = static_cast<uint32_t>(m_denseEntities.size());
        m_denseEntities.push_back(e);
        m_denseComponents.emplace_back(std::forward<Args>(args)...);
        m_sparse[e.index] = denseIndex;
        return m_denseComponents.back();
    }

    void remove(Entity e) {
        if (!has(e)) return;

        uint32_t denseIndex = m_sparse[e.index];
        uint32_t lastIndex = static_cast<uint32_t>(m_denseEntities.size() - 1);

        if (denseIndex != lastIndex) {
            // swap-remove para O(1)
            m_denseEntities[denseIndex] = m_denseEntities[lastIndex];
            m_denseComponents[denseIndex] = std::move(m_denseComponents[lastIndex]);

            // actualizar sparse del que movimos
            Entity moved = m_denseEntities[denseIndex];
            m_sparse[moved.index] = denseIndex;
        }

        m_denseEntities.pop_back();
        m_denseComponents.pop_back();
        m_sparse[e.index] = InvalidDense;
    }

    // IComponentPool
    void removeIfExists(Entity e) override { remove(e); }
    void clear() override {
        m_denseEntities.clear();
        m_denseComponents.clear();
        m_sparse.clear();
    }
    size_t size() const override { return m_denseEntities.size(); }

    // Iteración (más adelante la vamos a usar para queries)
    const std::vector<Entity>& denseEntities() const { return m_denseEntities; }
    std::vector<T>& denseComponents() { return m_denseComponents; }
    const std::vector<T>& denseComponents() const { return m_denseComponents; }

private:
    // Limite maximo de entidades para evitar que un bug aloque gigabytes.
    // 1M entidades es mas que suficiente para un juego tipo Stardew Valley.
    // Si necesitas mas, simplemente subi este valor.
    static constexpr uint32_t MaxEntities = 1'048'576; // 2^20

    void ensureSparseSize(uint32_t entityIndex) {
        assert(entityIndex < MaxEntities && "Entity index exceeds MaxEntities limit!");
        if (entityIndex >= m_sparse.size()) {
            m_sparse.resize(entityIndex + 1, InvalidDense);
        }
    }

private:
    static constexpr uint32_t InvalidDense = 0xFFFFFFFFu;

    std::vector<uint32_t> m_sparse;        // [entityIndex] -> denseIndex
    std::vector<Entity>   m_denseEntities; // dense
    std::vector<T>        m_denseComponents;
};

} // namespace eng::ecs
