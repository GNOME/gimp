/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp-shm.c
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

#include "config.h"

#include <errno.h>

#if defined(USE_SYSV_SHM)

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#elif defined(USE_POSIX_SHM)

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/mman.h>

#endif /* USE_POSIX_SHM */

#include <glib.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  ifdef STRICT
#  undef STRICT
#  endif
#  define STRICT

#  include <windows.h>
#  include <tlhelp32.h>
#  undef RGB
#  define USE_WIN32_SHM 1
#endif

#include "gimp.h"
#include "gimp-shm.h"


#define TILE_MAP_SIZE     (gimp_tile_width () * gimp_tile_height () * 32)
#define ERRMSG_SHM_FAILED "Could not attach to gimp shared memory segment"


#ifdef USE_WIN32_SHM
static HANDLE  _shm_handle;
#endif

static gint    _shm_ID   = -1;
static guchar *_shm_addr = NULL;


guchar *
_gimp_shm_addr (void)
{
  return _shm_addr;
}

void
_gimp_shm_open (gint shm_ID)
{
  _shm_ID = shm_ID;

  if (_shm_ID != -1)
    {
#if defined(USE_SYSV_SHM)

      /* Use SysV shared memory mechanisms for transferring tile data. */

      _shm_addr = (guchar *) shmat (_shm_ID, NULL, 0);

      if (_shm_addr == (guchar *) -1)
        {
          g_error ("shmat() failed: %s\n" ERRMSG_SHM_FAILED,
                   g_strerror (errno));
        }

#elif defined(USE_WIN32_SHM)

      /* Use Win32 shared memory mechanisms for transferring tile data. */

      gchar    fileMapName[128];
      wchar_t *w_fileMapName;

      /* From the id, derive the file map name */
      g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", _shm_ID);

      w_fileMapName = g_utf8_to_utf16 (fileMapName, -1, NULL, NULL, NULL);
      if (!w_fileMapName)
        g_error ("Cannot convert to UTF16");

      /* Open the file mapping */
      _shm_handle = OpenFileMappingW (FILE_MAP_ALL_ACCESS, 0, w_fileMapName);

      g_clear_pointer (&w_fileMapName, g_free);

      if (_shm_handle)
        {
          /* Map the shared memory into our address space for use */
          _shm_addr = (guchar *) MapViewOfFile (_shm_handle,
                                                FILE_MAP_ALL_ACCESS,
                                                0, 0, TILE_MAP_SIZE);

          /* Verify that we mapped our view */
          if (!_shm_addr)
            {
              g_error ("MapViewOfFile error: %lu... " ERRMSG_SHM_FAILED,
                       GetLastError ());
            }
        }
      else
        {
          g_error ("OpenFileMapping error: %lu... " ERRMSG_SHM_FAILED,
                   GetLastError ());
        }

#elif defined(USE_POSIX_SHM)

      /* Use POSIX shared memory mechanisms for transferring tile data. */

      gchar map_file[32];
      gint  shm_fd;

      /* From the id, derive the file map name */
      g_snprintf (map_file, sizeof (map_file), "/gimp-shm-%d", _shm_ID);

      /* Open the file mapping */
      shm_fd = shm_open (map_file, O_RDWR, 0600);

      if (shm_fd != -1)
        {
          /* Map the shared memory into our address space for use */
          _shm_addr = (guchar *) mmap (NULL, TILE_MAP_SIZE,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       shm_fd, 0);

          /* Verify that we mapped our view */
          if (_shm_addr == MAP_FAILED)
            {
              g_error ("mmap() failed: %s\n" ERRMSG_SHM_FAILED,
                       g_strerror (errno));
            }

          close (shm_fd);
        }
      else
        {
          g_error ("shm_open() failed: %s\n" ERRMSG_SHM_FAILED,
                   g_strerror (errno));
        }

#endif
    }
}

void
_gimp_shm_close (void)
{
#if defined(USE_SYSV_SHM)

  if ((_shm_ID != -1) && _shm_addr)
    shmdt ((char *) _shm_addr);

#elif defined(USE_WIN32_SHM)

  if (_shm_handle)
    CloseHandle (_shm_handle);

#elif defined(USE_POSIX_SHM)

  if ((_shm_ID != -1) && (_shm_addr != MAP_FAILED))
    munmap (_shm_addr, TILE_MAP_SIZE);

#endif
}
