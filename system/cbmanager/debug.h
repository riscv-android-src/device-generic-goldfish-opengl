/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_GOLDFISH_OPENGL_SYSTEM_CBMANAGER_DEBUG_H
#define ANDROID_GOLDFISH_OPENGL_SYSTEM_CBMANAGER_DEBUG_H

#include <log/log.h>

#define CRASH(MSG) \
    do { \
        ALOGE("%s:%d crashed with '%s'", __func__, __LINE__, MSG); \
        ::abort(); \
    } while (false)

#define CRASH_IF(COND, MSG) \
    do { \
        if ((COND)) { \
            ALOGE("%s:%d crashed on '%s' with '%s'", __func__, __LINE__, #COND, MSG); \
            ::abort(); \
        } \
    } while (false)

#define RETURN_ERROR_CODE(X) \
    do { \
        ALOGE("%s:%d failed with '%s' (%d)", \
              __func__, __LINE__, strerror(-(X)), -(X)); \
        return (X); \
    } while (false)

#define RETURN_ERROR(X) \
    do { \
        ALOGE("%s:%d failed with '%s'", __func__, __LINE__, #X); \
        return (X); \
    } while (false)

#define RETURN(X) return (X)

#endif  // ANDROID_GOLDFISH_OPENGL_SYSTEM_CBMANAGER_DEBUG_H
