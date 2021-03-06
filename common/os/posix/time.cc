/**
 * @file
 *
 * Platform specific header files that defines time related functions
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/platform.h>

#include <time.h>
#include <stdio.h>

#include <qcc/time.h>

#if defined(QCC_OS_DARWIN)

#include <sys/time.h>
#include <mach/mach_time.h>
#include <mach/clock.h>
#include <mach/mach.h>

#endif

using namespace qcc;

static void platform_gettime(struct timespec* ts, bool useMonotonic)
{
#if defined(QCC_OS_DARWIN)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(useMonotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, ts);
#endif
}

static time_t s_clockOffset = 0;

uint32_t qcc::GetTimestamp(void)
{
    struct timespec ts;
    uint32_t ret_val;

    platform_gettime(&ts, true);

    if (0 == s_clockOffset) {
        s_clockOffset = ts.tv_sec;
    }

    ret_val = ((uint32_t)(ts.tv_sec - s_clockOffset)) * 1000;
    ret_val += (uint32_t)ts.tv_nsec / 1000000;

    return ret_val;
}

uint64_t qcc::GetTimestamp64(void)
{
    struct timespec ts;
    uint64_t ret_val;

    platform_gettime(&ts, true);

    if (0 == s_clockOffset) {
        s_clockOffset = ts.tv_sec;
    }

    ret_val = ((uint64_t)(ts.tv_sec - s_clockOffset)) * 1000;
    ret_val += (uint64_t)ts.tv_nsec / 1000000;

    return ret_val;
}

uint64_t qcc::GetEpochTimestamp(void)
{
    struct timespec ts;
    uint64_t ret_val;

    platform_gettime(&ts, false);

    ret_val = ((uint64_t)(ts.tv_sec)) * 1000;
    ret_val += (uint64_t)ts.tv_nsec / 1000000;

    return ret_val;
}

void qcc::GetTimeNow(Timespec<MonotonicTime>* ts)
{
    struct timespec _ts;
    platform_gettime(&_ts, true);
    ts->seconds = _ts.tv_sec;
    ts->mseconds = _ts.tv_nsec / 1000000;
}

qcc::String qcc::UTCTime()
{
    static const char* Day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char* Month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    char buf[32];
    time_t t;
    time(&t);
    struct tm utc;
    gmtime_r(&t, &utc);
    snprintf(buf, 32, "%s, %02d %s %04d %02d:%02d:%02d GMT",
             Day[utc.tm_wday],
             utc.tm_mday,
             Month[utc.tm_mon],
             1900 + utc.tm_year,
             utc.tm_hour,
             utc.tm_min,
             utc.tm_sec);

    return buf;
}

int64_t qcc::ConvertStructureToTime(struct tm* timeptr)
{
    return mktime(timeptr);
}

QStatus qcc::ConvertTimeToStructure(const int64_t* timer, struct tm* tm) {
    time_t t = static_cast<time_t>(*timer);
    if (gmtime_r(&t, tm)) {
        return ER_OK;
    }
    return ER_FAIL;
}

QStatus qcc::ConvertToLocalTime(const int64_t* timer, struct tm* tm) {
    time_t t = static_cast<time_t>(*timer);
    if (localtime_r(&t, tm)) {
        return ER_OK;
    }
    return ER_FAIL;
}

size_t qcc::FormatTime(char* strDest, size_t maxSize, const char* format, const struct tm* timeptr)
{
    return strftime(strDest, maxSize, format, timeptr);
}
