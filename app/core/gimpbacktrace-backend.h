/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace-backend.h
 * Copyright (C) 2018 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_BACKTRACE_BACKEND_H__
#define __GIMP_BACKTRACE_BACKEND_H__


#ifdef __gnu_linux__
# define GIMP_BACKTRACE_BACKEND_LINUX
#elif defined (G_OS_WIN32) && defined (ARCH_X86)
# define GIMP_BACKTRACE_BACKEND_WINDOWS
#else
# define GIMP_BACKTRACE_BACKEND_NONE
#endif


#endif  /*  __GIMP_BACKTRACE_BACKEND_H__  */
