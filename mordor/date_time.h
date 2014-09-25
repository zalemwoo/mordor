#ifndef __MORDOR_DATE_TIME_H__
#define __MORDOR_DATE_TIME_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <chrono>

namespace Mordor {
    time_t toTimeT(const std::chrono::system_clock::time_point &ptime);
};

#endif
