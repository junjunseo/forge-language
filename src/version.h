#pragma once

#ifndef IEUM_VERSION
#error "IEUM_VERSION must be provided by the build system"
#endif

inline constexpr const char* kIeumVersion = IEUM_VERSION;
