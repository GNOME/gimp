/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <sys/types.h>

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

#include <glib-object.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define USE_WIN32_SHM 1

#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

#include "plug-in-types.h"

#include "base/tile.h"

#include "core/gimp.h"

#include "plug-in-shm.h"


struct _PlugInShm
{
  gint    shm_ID;
  guchar *shm_addr;

#if defined(USE_WIN32_SHM)
  HANDLE  shm_handle;
#endif
};


#define TILE_MAP_SIZE (TILE_WIDTH * TILE_HEIGHT * 4)

#define ERRMSG_SHM_DISABLE "Disabling shared memory tile transport"

void
plug_in_shm_init (Gimp *gimp)
{
  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */

#if defined(USE_SYSV_SHM)
  /* Use SysV shared memory mechanisms for transferring tile data. */

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->plug_in_shm = g_new0 (PlugInShm, 1);
  gimp->plug_in_shm->shm_ID = -1;

  gimp->plug_in_shm->shm_ID = shmget (IPC_PRIVATE, TILE_MAP_SIZE,
                                      IPC_CREAT | 0600);

  if (gimp->plug_in_shm->shm_ID != -1)
    {
      gimp->plug_in_shm->shm_addr = (guchar *)
        shmat (gimp->plug_in_shm->shm_ID, NULL, 0);

      if (gimp->plug_in_shm->shm_addr == (guchar *) -1)
        {
          g_warning ("shmat() failed: %s\n" ERRMSG_SHM_DISABLE,
                     g_strerror (errno));
          shmctl (gimp->plug_in_shm->shm_ID, IPC_RMID, NULL);
          gimp->plug_in_shm->shm_ID = -1;
        }

#ifdef IPC_RMID_DEFERRED_RELEASE
      if (gimp->plug_in_shm->shm_addr != (guchar *) -1)
        shmctl (gimp->plug_in_shm->shm_ID, IPC_RMID, NULL);
#endif
    }
  else
    {
      g_warning ("shmget() failed: %s\n" ERRMSG_SHM_DISABLE,
                 g_strerror (errno));
    }

#elif defined(USE_WIN32_SHM)

  /* Use Win32 shared memory mechanisms for transferring tile data. */

  gint  pid;
  gchar fileMapName[MAX_PATH];

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->plug_in_shm = g_new0 (PlugInShm, 1);
  gimp->plug_in_shm->shm_ID = -1;

  /* Our shared memory id will be our process ID */
  pid = GetCurrentProcessId ();

  /* From the id, derive the file map name */
  g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", pid);

  /* Create the file mapping into paging space */
  gimp->plug_in_shm->shm_handle = CreateFileMapping ((HANDLE) 0xFFFFFFFF, NULL,
                                                     PAGE_READWRITE, 0,
                                                     TILE_MAP_SIZE,
                                                     fileMapName);

  if (gimp->plug_in_shm->shm_handle)
    {
      /* Map the shared memory into our address space for use */
      gimp->plug_in_shm->shm_addr = (guchar *)
        MapViewOfFile (gimp->plug_in_shm->shm_handle,
                       FILE_MAP_ALL_ACCESS,
                       0, 0, TILE_MAP_SIZE);

      /* Verify that we mapped our view */
      if (gimp->plug_in_shm->shm_addr)
        gimp->plug_in_shm->shm_ID = pid;
      else
        g_warning ("MapViewOfFile error: %d... " ERRMSG_SHM_DISABLE,
                   GetLastError ());
    }
  else
    {
      g_warning ("CreateFileMapping error: %d... " ERRMSG_SHM_DISABLE,
                 GetLastError ());
    }

#elif defined(USE_POSIX_SHM)

  /* Use POSIX shared memory mechanisms for transferring tile data. */

  gint  pid;
  gchar shm_handle[32];
  gint  shm_fd;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->plug_in_shm = g_new0 (PlugInShm, 1);
  gimp->plug_in_shm->shm_ID = -1;

  /* Our shared memory id will be our process ID */
  pid = getpid ();

  /* From the id, derive the file map name */
  g_snprintf (shm_handle, sizeof (shm_handle), "/gimp-shm-%d", pid);

  /* Create the file mapping into paging space */
  shm_fd = shm_open (shm_handle, O_RDWR | O_CREAT, 0600);

  if (shm_fd != -1)
    {
      if (ftruncate (shm_fd, TILE_MAP_SIZE) != -1)
        {
          /* Map the shared memory into our address space for use */
          gimp->plug_in_shm->shm_addr = (guchar *)
            mmap (NULL, TILE_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                  shm_fd, 0);

          /* Verify that we mapped our view */
          if (gimp->plug_in_shm->shm_addr != MAP_FAILED)
            {
              gimp->plug_in_shm->shm_ID = pid;
            }
          else
            {
              g_warning ("mmap() failed: %s\n" ERRMSG_SHM_DISABLE,
                         g_strerror (errno));

              shm_unlink (shm_handle);
            }
        }
      else
        {
          g_warning ("ftruncate() failed: %s\n" ERRMSG_SHM_DISABLE,
                     g_strerror (errno));

          shm_unlink (shm_handle);
        }

      close (shm_fd);
    }
  else
    {
      g_warning ("shm_open() failed: %s\n" ERRMSG_SHM_DISABLE,
                 g_strerror (errno));
    }

#endif
}

void
plug_in_shm_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->plug_in_shm != NULL);

#if defined (USE_SYSV_SHM)

#ifndef IPC_RMID_DEFERRED_RELEASE
  if (gimp->plug_in_shm->shm_ID != -1)
    {
      shmdt (gimp->plug_in_shm->shm_addr);
      shmctl (gimp->plug_in_shm->shm_ID, IPC_RMID, NULL);
    }
#else /* IPC_RMID_DEFERRED_RELEASE */
  if (gimp->plug_in_shm->shm_ID != -1)
    {
      shmdt (gimp->plug_in_shm->shm_addr);
    }
#endif

#elif defined(USE_WIN32_SHM)

  if (gimp->plug_in_shm->shm_handle)
    {
      CloseHandle (gimp->plug_in_shm->shm_handle);
    }

#elif defined(USE_POSIX_SHM)

  if (gimp->plug_in_shm->shm_ID != -1)
    {
      gchar shm_handle[32];

      munmap (gimp->plug_in_shm->shm_addr, TILE_MAP_SIZE);

      g_snprintf (shm_handle, sizeof (shm_handle), "/gimp-shm-%d",
                  gimp->plug_in_shm->shm_ID);

      shm_unlink (shm_handle);
    }

#endif

  g_free (gimp->plug_in_shm);
  gimp->plug_in_shm = NULL;
}

gint
plug_in_shm_get_ID (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), -1);

  return gimp->plug_in_shm ? gimp->plug_in_shm->shm_ID : -1;
}

guchar *
plug_in_shm_get_addr (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (gimp->plug_in_shm != NULL, NULL);

  return gimp->plug_in_shm->shm_addr;
}
