#pragma once

class State {
public:
    static void init();
    static bool AppRunning();
    static void shutdown();
    static void registerProcUICallbacks();
    static uint32_t ConsoleProcCallbackAcquired(void *context);
    static uint32_t ConsoleProcCallbackReleased(void *context);

private:
    static bool aroma;
};