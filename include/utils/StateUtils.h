#pragma once

class State {
public:
    static void init();
    static bool AppRunning();
    static void shutdown();
    static void registerProcUICallbacks();

private:
    static bool aroma;
};