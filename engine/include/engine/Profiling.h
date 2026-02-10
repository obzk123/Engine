#pragma once
#include <SDL.h>
#include <string>
#include <unordered_map>
#include <deque>
#include <numeric>
#include <algorithm>

namespace eng {

struct ProfileStats {
    double lastMs = 0.0;
    double avgMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
    size_t samples = 0;
};

class Profiler {
public:
    explicit Profiler(size_t window = 120)
        : m_window(window) {}

    void beginFrame() {
        m_frameSubmitted.clear();
    }

    void submitSample(const std::string& name, double ms) {
        m_frameSubmitted[name] = ms;

        auto& dq = m_history[name];
        dq.push_back(ms);
        if (dq.size() > m_window) dq.pop_front();

        // actualizar stats cacheados
        ProfileStats st;
        st.lastMs = ms;
        st.samples = dq.size();
        st.avgMs = average(dq);
        auto [mn, mx] = minmax(dq);
        st.minMs = mn;
        st.maxMs = mx;
        m_stats[name] = st;
    }

    // Estadísticas por sistema (promedio móvil)
    const std::unordered_map<std::string, ProfileStats>& stats() const {
        return m_stats;
    }

    // Último frame (si lo querés mostrar)
    const std::unordered_map<std::string, double>& lastFrameSamples() const {
        return m_frameSubmitted;
    }

    void setWindow(size_t window) {
        m_window = std::max<size_t>(1, window);
        // opcional: recortar colas actuales
        for (auto& kv : m_history) {
            auto& dq = kv.second;
            while (dq.size() > m_window) dq.pop_front();
        }
        // stats se recalculan al próximo submit
    }

    size_t window() const { return m_window; }

private:
    static double average(const std::deque<double>& dq) {
        if (dq.empty()) return 0.0;
        double sum = 0.0;
        for (double v : dq) sum += v;
        return sum / static_cast<double>(dq.size());
    }

    static std::pair<double, double> minmax(const std::deque<double>& dq) {
        if (dq.empty()) return {0.0, 0.0};
        auto mm = std::minmax_element(dq.begin(), dq.end());
        return { *mm.first, *mm.second };
    }

private:
    size_t m_window;

    // último frame
    std::unordered_map<std::string, double> m_frameSubmitted;

    // historial por sistema
    std::unordered_map<std::string, std::deque<double>> m_history;

    // stats precomputados por sistema
    std::unordered_map<std::string, ProfileStats> m_stats;
};

class ScopeTimer {
public:
    ScopeTimer(Profiler& profiler, std::string name)
        : m_profiler(profiler), m_name(std::move(name)) {
        m_start = SDL_GetPerformanceCounter();
    }

    ~ScopeTimer() {
        const uint64_t end = SDL_GetPerformanceCounter();
        const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
        const double ms = (static_cast<double>(end - m_start) / freq) * 1000.0;
        m_profiler.submitSample(m_name, ms);
    }

private:
    Profiler& m_profiler;
    std::string m_name;
    uint64_t m_start = 0;
};

} // namespace eng
