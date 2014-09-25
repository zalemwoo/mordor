// Copyright (c) 2009 - Mozy, Inc.

#include "date_time.h"

namespace Mordor {

time_t toTimeT(const std::chrono::system_clock::time_point &ptime)
{
    return std::chrono::system_clock::to_time_t(ptime);
}

}
