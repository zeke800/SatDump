#pragma once

#include <spdlog/spdlog.h>
#include <memory>

#ifdef SATDUMP_EXPORT
#define SATDUMP_EXP __declspec(dllexport)
#else
#define  SATDUMP_EXP __declspec(dllimport)
#endif

// Main logger instance
SATDUMP_EXP extern std::shared_ptr<spdlog::logger> logger;

// Initialize the logger
void initLogger();

// Change output level, eg Debug, Info, etc
void setConsoleLevel(spdlog::level::level_enum level);