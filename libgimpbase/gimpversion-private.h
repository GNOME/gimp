/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpversion-private.h
 * Copyright (C) 2025 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_VERSION_PRIVATE_H__
#define __GIMP_VERSION_PRIVATE_H__

#include <libgimpbase/gimpversion.h>

G_BEGIN_DECLS

/**
 * GIMP_WARNING_API_BREAK:
 * @message: the message to output either as build WARNING or ERROR.
 *
 * Internal macro to use when we consider an API break, so that we don't
 * forget to look at it when the time comes:
 *
 * - When the minor version reaches 99 (e.g. 3.99), the message will be
 *   outputted as a compilation warning.
 * - When the minor and micro versions are 0.0 (e.g. 4.0.0), the message
 *   will be outputted as compilation error, hence forbidding a major
 *   release unless we resolve all the API break warnings (in any way,
 *   it can be by actually breaking the API, or by deciding that this is
 *   not a valid change anymore).
 *
 * Note: this macro relies on the "GCC warning" pragma which was tested
 * to work on clang too. Assuming/hoping it works on other compilers.
 **/

#if GIMP_CHECK_VERSION(GIMP_MAJOR_VERSION, 99, 0)
 #define GIMP_PRAGMA(pragma)       _Pragma(#pragma)
 #define GIMP_WARNING_MSG(message) GIMP_PRAGMA(GCC warning #message)

 #define GIMP_WARNING_API_BREAK(message) GIMP_WARNING_MSG(message)
#elif GIMP_MINOR_VERSION == 0 && GIMP_MICRO_VERSION == 0
 #define GIMP_PRAGMA(pragma)     _Pragma(#pragma)
 #define GIMP_ERROR_MSG(message) GIMP_PRAGMA(GCC error #message)

 #define GIMP_WARNING_API_BREAK(message) \
    GIMP_ERROR_MSG("API break warning needs to be handled before releasing: " message)
#else
 #define GIMP_WARNING_API_BREAK(message)
#endif


G_END_DECLS

#endif /* __GIMP_VERSION_PRIVATE_H__ */
