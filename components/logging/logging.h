#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>

/* Substantially based on https://github.com/rxi/log.c, under MIT license */

/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <time.h>
#include <stdarg.h>

#if ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
static inline uint32_t
logging_now(void)
{
    return esp_timer_get_time()/1000;
}
#elif LIB_PICO_PLATFORM
#include "pico/time.h"
static inline uint32_t
logging_now(void)
{
    return to_ms_since_boot(get_absolute_time());
}
#else
#error unknown platform
#endif

// You should devine this somewhere
extern int LogLevel;

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


static const char *level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif

static inline const char* log_level_string(int level) {
  return level_strings[level];
}

typedef struct {
  va_list ap;
  const char *fmt;
  const char *file;
  uint32_t time;
  void *udata;
  int line;
  int level;
} log_Event;

static void init_event(log_Event *ev, void *udata) {
  if (!ev->time) {
    ev->time = logging_now();
  }
  ev->udata = udata;
}

static inline void
stdout_callback(log_Event *ev) {
#ifdef LOG_USE_COLOR
  fprintf(
    ev->udata, "%08d %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    ev->time, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
  fprintf(
    ev->udata, "%08d %-5s %s:%d: ",
    ev->time, level_strings[ev->level], ev->file, ev->line);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}

static inline void
log_log(int level, const char *file, int line, const char *fmt, ...)
{
    log_Event ev = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
  };

  if (level >= LogLevel) {
    init_event(&ev, stdout);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }
}

#endif // __LOGGING_H__
