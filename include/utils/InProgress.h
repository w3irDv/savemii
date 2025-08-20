#pragma once

#include <ApplicationState.h>

class InProgress {
public:
    inline static std::string titleName{};
    inline static int currentStep = 0;
    inline static int totalSteps = 0;
    inline static bool abortTask = false;
    inline static Input *input = nullptr;
    inline static eJobType jobType = NONE;
    inline static bool abortCopy = false;
    inline static int copyErrorsCounter = 0;
};
