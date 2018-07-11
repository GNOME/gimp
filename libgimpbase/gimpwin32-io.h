/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwin32-io.h
 * Compatibility defines, you mostly need this as unistd.h replacement
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_WIN32_IO_H__
#define __GIMP_WIN32_IO_H__

#include <io.h>
#include <direct.h>

G_BEGIN_DECLS


#define mkdir(n,a)  _mkdir(n)
#define chmod(n,f)  _chmod(n,f)
#define access(f,p) _access(f,p)

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif

#ifndef S_IRGRP
#define S_IRGRP _S_IREAD
#endif
#ifndef S_IXGRP
#define S_IXGRP _S_IEXEC
#endif
#ifndef S_IROTH
#define S_IROTH _S_IREAD
#endif
#ifndef S_IXOTH
#define S_IXOTH _S_IEXEC
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif
#ifndef _O_TEMPORARY
#define _O_TEMPORARY 0
#endif

#ifndef F_OK
#define F_OK 0
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef X_OK
#define X_OK 0 /* not really */
#endif

/*
2004-09-15  Tor Lillqvist  <tml@iki.fi>

        * glib/gwin32.h: Don't define ftruncate as a macro. Was never a
        good idea, and it clashes with newest mingw headers, which have a
        ftruncate implementation as an inline function. Thanks to Dominik R.
 */
/* needs coorection for msvc though ;( */
#ifdef _MSC_VER
#define ftruncate(f,s) g_win32_ftruncate(f,s)
#endif

G_END_DECLS

#endif /* __GIMP_WIN32_IO_H__ */
