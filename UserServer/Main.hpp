#pragma once

#include <mutex>

#define AUTOLOCK(a) lock_guard<std::mutex> lock(a)
