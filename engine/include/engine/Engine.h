#pragma once

namespace eng {

    class Engine {
        public:
            bool init();
            void run();
            void shutdown();
        
        private:
            bool m_running = false;
    };

} // namespace eng