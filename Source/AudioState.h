#pragma once

#include <atomic>

struct AudioState
{
    std::atomic<bool> monitorEnabled { false };
    std::atomic<float> cutoffHz { 5000.0f };
    std::atomic<float> vol1 { 0.05f };
    std::atomic<float> vol2 { 0.05f };
    std::atomic<float> vol3 { 0.05f };
    std::atomic<int> filterType { 0 }; // 0 = lowpass, 1 = highpass
};