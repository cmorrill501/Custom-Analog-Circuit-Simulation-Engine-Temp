#ifndef Logger_H
#define Logger_H

#include <iostream>
#include <functional>
#include <string>

enum class MessageLevel {
    success,
    warning,
    error,
    info
};

using LogCallBack = std::function<void(const std::string& message,MessageLevel level)>;

class Logger {
    private:
        LogCallBack m_callback = nullptr;
        static Logger& get() {
            static Logger instance;
            return instance;
        }
        void log(const std::string& message,MessageLevel level) {
            if(m_callback) {
                m_callback(message,level);
            } else {
                std::cout << "[Fallback] " << message << std::endl;
            }
        }
    public:
        static void registerCallback(LogCallBack callback) {get().m_callback = callback;}
        static void info(const std::string& message) {get().log(message,MessageLevel::info);}
        static void success(const std::string& message) {get().log(message,MessageLevel::success);}
        static void warning(const std::string& message) {get().log(message,MessageLevel::warning);}
        static void error(const std::string& message) {get().log(message,MessageLevel::error);}
};

#endif