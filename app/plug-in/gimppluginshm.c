/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginhsm.c
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

#include <gio/gio.h>
#include <gegl.h>

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

#include "core/gimp-utils.h"

#include "gimppluginshm.h"

#include "gimp-log.h"


#define TILE_MAP_SIZE (GIMP_PLUG_IN_TILE_WIDTH * GIMP_PLUG_IN_TILE_HEIGHT * 32)

#define ERRMSG_SHM_DISABLE "Disabling shared memory tile transport"


struct _GimpPlugInShm
{
  gint    shm_id;
  guchar *shm_addr;

#if defined(USE_WIN32_SHM)
  HANDLE  shm_handle;
#endif
};


GimpPlugInShm *
gimp_plug_in_shm_new (void)
{
  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */

  GimpPlugInShm *shm = g_slice_new0 (GimpPlugInShm);

  shm->shm_id = -1;

#if defined(USE_SYSV_SHM)

  /* Use SysV shared memory mechanisms for transferring tile data. */
  {
    shm->shm_id = shmget (IPC_PRIVATE, TILE_MAP_SIZE, IPC_CREAT | 0600);

    if (shm->shm_id != -1)
      {
        shm->shm_addr = (guchar *) shmat (shm->shm_id, NULL, 0);

        if (shm->shm_addr == (guchar *) -1)
          {
            g_printerr ("shmat() failed: %s\n" ERRMSG_SHM_DISABLE,
                        g_strerror (errno));
            shmctl (shm->shm_id, IPC_RMID, NULL);
            shm->shm_id = -1;
          }

#ifdef IPC_RMID_DEFERRED_RELEASE
        if (shm->shm_addr != (guchar *) -1)
          shmctl (shm->shm_id, IPC_RMID, NULL);
#endif
      }
    else
      {
        g_printerr ("shmget() failed: %s\n" ERRMSG_SHM_DISABLE,
                    g_strerror (errno));
      }
  }

#elif defined(USE_WIN32_SHM)

  /* Use Win32 shared memory mechanisms for transferring tile data. */
  {
    gint     pid;
    gchar    fileMapName[MAX_PATH];
    wchar_t *w_fileMapName         = NULL;

    /* Our shared memory id will be our process ID */
    pid = GetCurrentProcessId ();

    /* From the id, derive the file map name */
    g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", pid);

    w_fileMapName = g_utf8_to_utf16 (fileMapName, -1, NULL, NULL, NULL);

    /* Create the file mapping into paging space */
    shm->shm_handle = CreateFileMappingW (INVALID_HANDLE_VALUE, NULL,
                                          PAGE_READWRITE, 0,
                                          TILE_MAP_SIZE,
                                          w_fileMapName);

    g_free (w_fileMapName);
    w_fileMapName = NULL;

    if (shm->shm_handle)
      {
        /* Map the shared memory into our address space for use */
        shm->shm_addr = (guchar *) MapViewOfFile (shm->shm_handle,
                                                  FILE_MAP_ALL_ACCESS,
                                                  0, 0, TILE_MAP_SIZE);

        /* Verify that we mapped our view */
        if (shm->shm_addr)
          {
            shm->shm_id = pid;
          }
        else
          {
            g_printerr ("MapViewOfFile error: %u... " ERRMSG_SHM_DISABLE,
                        (unsigned) GetLastError ());
          }
      }
    else
      {
        g_printerr ("CreateFileMapping error: %u... " ERRMSG_SHM_DISABLE,
                    (unsigned) GetLastError ());
      }
  }

#elif defined(USE_POSIX_SHM)

  /* Use POSIX shared memory mechanisms for transferring tile data. */
  {
    gint  pid;
    gchar shm_handle[32];
    gint  shm_fd;

    /* Our shared memory id will be our process ID */
    pid = gimp_get_pid ();

    /* From the id, derive the file map name */
    g_snprintf (shm_handle, sizeof (shm_handle), "/gimp-shm-%d", pid);

    /* Create the file mapping into paging space */
    shm_fd = shm_open (shm_handle, O_RDWR | O_CREAT, 0600);

    if (shm_fd != -1)
      {
        if (ftruncate (shm_fd, TILE_MAP_SIZE) != -1)
          {
            /* Map the shared memory into our address space for use */
            shm->shm_addr = (guchar *) mmap (NULL, TILE_MAP_SIZE,
                                             PROT_READ | PROT_WRITE, MAP_SHARED,
                                             shm_fd, 0);

            /* Verify that we mapped our view */
            if (shm->shm_addr != MAP_FAILED)
              {
                shm->shm_id = pid;
              }
            else
              {
                g_printerr ("mmap() failed: %s\n" ERRMSG_SHM_DISABLE,
                            g_strerror (errno));

                shm_unlink (shm_handle);
              }
          }
        else
          {
            g_printerr ("ftruncate() failed: %s\n" ERRMSG_SHM_DISABLE,
                        g_strerror (errno));

            shm_unlink (shm_handle);
          }

        close (shm_fd);
      }
    else
      {
        g_printerr ("shm_open() failed: %s\n" ERRMSG_SHM_DISABLE,
                    g_strerror (errno));
      }
  }

#endif

  if (shm->shm_id == -1)
    {
      g_slice_free (GimpPlugInShm, shm);
      shm = NULL;
    }
  else
    {
      GIMP_LOG (SHM, "attached shared memory segment ID = %d", shm->shm_id);
    }

  return shm;
}

void
gimp_plug_in_shm_free (GimpPlugInShm *shm)
{
  g_return_if_fail (shm != NULL);

  if (shm->shm_id != -1)
    {

#if defined (USE_SYSV_SHM)

      shmdt (shm->shm_addr);

#ifndef IPC_RMID_DEFERRED_RELEASE
      shmctl (shm->shm_id, IPC_RMID, NULL);
#endif

#elif defined(USE_WIN32_SHM)

      if (shm->shm_handle)
        CloseHandle (shm->shm_handle);

#elif defined(USE_POSIX_SHM)

      gchar shm_handle[32];

      munmap (shm->shm_addr, TILE_MAP_SIZE);

      g_snprintf (shm_handle, sizeof (shm_handle), "/gimp-shm-%d",
                  shm->shm_id);

      shm_unlink (shm_handle);

#endif

      GIMP_LOG (SHM, "detached shared memory segment ID = %d", shm->shm_id);
    }

  g_slice_free (GimpPlugInShm, shm);
}

gint
gimp_plug_in_shm_get_id (GimpPlugInShm *shm)
{
  g_return_val_if_fail (shm != NULL, -1);

  return shm->shm_id;
}

guchar *
gimp_plug_in_shm_get_addr (GimpPlugInShm *shm)
{
  g_return_val_if_fail (shm != NULL, NULL);

  return shm->shm_addr;
}
