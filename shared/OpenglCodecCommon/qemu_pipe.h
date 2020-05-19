/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !(defined(HOST_BUILD) || defined(ALLOW_DEPRECATED_QEMU_PIPE_HEADERS))
#error include <qemu_pipe_bp.h> instead from libqemupipe.ranchu
#endif

#ifndef ANDROID_INCLUDE_HARDWARE_QEMU_PIPE_H
#define ANDROID_INCLUDE_HARDWARE_QEMU_PIPE_H

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>

#define QEMU_PIPE_RETRY(exp) ({ \
    __typeof__(exp) _rc; \
    do { \
        _rc = (exp); \
    } while (qemu_pipe_try_again(_rc)); \
    _rc; })

#ifdef HOST_BUILD

typedef void* QEMU_PIPE_HANDLE;

#define QEMU_PIPE_INVALID_HANDLE NULL

QEMU_PIPE_HANDLE qemu_pipe_open(const char* pipeName);
void qemu_pipe_close(QEMU_PIPE_HANDLE pipe);

ssize_t qemu_pipe_read(QEMU_PIPE_HANDLE pipe, void* buffer, size_t len);
ssize_t qemu_pipe_write(QEMU_PIPE_HANDLE pipe, const void* buffer, size_t len);

bool qemu_pipe_try_again(int ret);
bool qemu_pipe_valid(QEMU_PIPE_HANDLE pipe);

void qemu_pipe_print_error(QEMU_PIPE_HANDLE pipe);

#else

typedef int QEMU_PIPE_HANDLE;

#define QEMU_PIPE_INVALID_HANDLE (-1)

#ifndef QEMU_PIPE_PATH
#define QEMU_PIPE_PATH "/dev/qemu_pipe"
#endif

#if PLATFORM_SDK_VERSION < 26
#include <cutils/log.h>
#else
#include <log/log.h>
#endif
#ifdef __ANDROID__
#include <sys/cdefs.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>  /* for pthread_once() */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef D
#  define  D(...)   do{}while(0)
#endif

/* Try to open a new Qemu fast-pipe. This function returns a file descriptor
 * that can be used to communicate with a named service managed by the
 * emulator.
 *
 * This file descriptor can be used as a standard pipe/socket descriptor.
 *
 * 'pipeName' is the name of the emulator service you want to connect to.
 * E.g. 'opengles' or 'camera'.
 *
 * On success, return a valid file descriptor
 * Returns -1 on error, and errno gives the error code, e.g.:
 *
 *    EINVAL  -> unknown/unsupported pipeName
 *    ENOSYS  -> fast pipes not available in this system.
 *
 * ENOSYS should never happen, except if you're trying to run within a
 * misconfigured emulator.
 *
 * You should be able to open several pipes to the same pipe service,
 * except for a few special cases (e.g. GSM modem), where EBUSY will be
 * returned if more than one client tries to connect to it.
 */

static __inline__ bool
qemu_pipe_try_again(int ret) {
    return (ret < 0) && (errno == EINTR || errno == EAGAIN);
}

static __inline__ ssize_t
qemu_pipe_write_fully(QEMU_PIPE_HANDLE pipe, const void* buffer, ssize_t len);

static __inline__ QEMU_PIPE_HANDLE
qemu_pipe_open_ns(const char* ns, const char* pipeName, int flags) {
    char  buff[256];
    int   buffLen;
    QEMU_PIPE_HANDLE   fd;

    if (pipeName == NULL || pipeName[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (ns) {
        buffLen = snprintf(buff, sizeof(buff), "pipe:%s:%s", ns, pipeName);
    } else {
        buffLen = snprintf(buff, sizeof(buff), "pipe:%s", pipeName);
    }

    fd = QEMU_PIPE_RETRY(open(QEMU_PIPE_PATH, flags));
    if (fd < 0 && errno == ENOENT) {
        fd = QEMU_PIPE_RETRY(open("/dev/goldfish_pipe", flags));
    }
    if (fd < 0) {
        D("%s: Could not open " QEMU_PIPE_PATH ": %s", __FUNCTION__, strerror(errno));
        //errno = ENOSYS;
        return -1;
    }

    if (qemu_pipe_write_fully(fd, buff, buffLen + 1)) {
        D("%s: Could not connect to %s pipe service: %s", __FUNCTION__, pipeName, strerror(errno));
        return -1;
    }

    return fd;
}

static __inline__ QEMU_PIPE_HANDLE
qemu_pipe_open(const char* pipeName) {
    return qemu_pipe_open_ns(NULL, pipeName, O_RDWR | O_NONBLOCK);
}

static __inline__ void
qemu_pipe_close(QEMU_PIPE_HANDLE pipe) {
    close(pipe);
}

static __inline__ ssize_t
qemu_pipe_read(QEMU_PIPE_HANDLE pipe, void* buffer, size_t len) {
    return read(pipe, buffer, len);
}

static __inline__ ssize_t
qemu_pipe_write(QEMU_PIPE_HANDLE pipe, const void* buffer, size_t len) {
    return write(pipe, buffer, len);
}

static __inline__ bool
qemu_pipe_valid(QEMU_PIPE_HANDLE pipe) {
    return pipe >= 0;
}

static __inline__ void
qemu_pipe_print_error(QEMU_PIPE_HANDLE pipe) {
    ALOGE("pipe error: fd %d errno %d", pipe, errno);
}


#endif // !HOST_BUILD

static __inline__ ssize_t
qemu_pipe_read_fully(QEMU_PIPE_HANDLE pipe, void* buffer, ssize_t len) {
    char* p = (char*)buffer;

    while (len > 0) {
      ssize_t n = QEMU_PIPE_RETRY(qemu_pipe_read(pipe, p, len));
      if (n < 0) return n;

      p += n;
      len -= n;
    }

    return 0;
}

static __inline__ ssize_t
qemu_pipe_write_fully(QEMU_PIPE_HANDLE pipe, const void* buffer, ssize_t len) {
    const char* p = (const char*)buffer;

    while (len > 0) {
      ssize_t n = QEMU_PIPE_RETRY(qemu_pipe_write(pipe, p, len));
      if (n < 0) return n;

      p += n;
      len -= n;
    }

    return 0;
}

#endif /* ANDROID_INCLUDE_HARDWARE_QEMU_PIPE_H */