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

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#include <glib-object.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

#include "plug-in-types.h"

#include "base/tile.h"

#include "core/gimp.h"

#include "plug-in-shm.h"


struct _PlugInShm
{
  gint    shm_ID;
  guchar *shm_addr;

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
  HANDLE  shm_handle;
#endif
};


void
plug_in_shm_init (Gimp *gimp)
{
  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  
#ifdef HAVE_SHM_H
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->plug_in_shm = g_new0 (PlugInShm, 1);
  gimp->plug_in_shm->shm_ID = -1;

  gimp->plug_in_shm->shm_ID = shmget (IPC_PRIVATE,
                                      TILE_WIDTH * TILE_HEIGHT * 4,
                                      IPC_CREAT | 0600);

  if (gimp->plug_in_shm->shm_ID == -1)
    {
      g_message ("shmget() failed: Disabling shared memory tile transport.");
    }
  else
    {
      gimp->plug_in_shm->shm_addr = (guchar *)
        shmat (gimp->plug_in_shm->shm_ID, NULL, 0);

      if (gimp->plug_in_shm->shm_addr == (guchar *) -1)
	{
	  g_message ("shmat() failed: Disabling shared memory tile transport.");
	  shmctl (gimp->plug_in_shm->shm_ID, IPC_RMID, NULL);
	  gimp->plug_in_shm->shm_ID = -1;
	}

#ifdef IPC_RMID_DEFERRED_RELEASE
      if (gimp->plug_in_shm->shm_addr != (guchar *) -1)
	shmctl (gimp->plug_in_shm->shm_ID, IPC_RMID, NULL);
#endif
    }

#else /* ! HAVE_SHM_H */

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
  /* Use Win32 shared memory mechanisms for
   * transfering tile data.
   */
  gint  pid;
  gchar fileMapName[MAX_PATH];
  gint  tileByteSize = TILE_WIDTH * TILE_HEIGHT * 4;
  
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
                                                     tileByteSize, fileMapName);
  
  if (gimp->plug_in_shm->shm_handle)
    {
      /* Map the shared memory into our address space for use */
      gimp->plug_in_shm->shm_addr = (guchar *)
        MapViewOfFile (gimp->plug_in_shm->shm_handle,
                       FILE_MAP_ALL_ACCESS,
                       0, 0, tileByteSize);
      
      /* Verify that we mapped our view */
      if (gimp->plug_in_shm->shm_addr)
	gimp->plug_in_shm->shm_ID = pid;
      else
        g_warning ("MapViewOfFile error: "
                   "%d... disabling shared memory transport",
                   GetLastError());
    }
  else
    {
      g_warning ("CreateFileMapping error: "
                 "%d... disabling shared memory transport",
                 GetLastError());
    }
#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

#endif /* HAVE_SHM_H */
}

void
plug_in_shm_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->plug_in_shm != NULL);

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

  CloseHandle (gimp->plug_in_shm->shm_handle);

#else /* ! (G_OS_WIN32 || G_WITH_CYGWIN) */

#ifdef HAVE_SHM_H
#ifndef	IPC_RMID_DEFERRED_RELEASE
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
#endif /* HAVE_SHM_H */

#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

  g_free (gimp->plug_in_shm);
  gimp->plug_in_shm = NULL;
}

gint
plug_in_shm_get_ID (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), -1);
  g_return_val_if_fail (gimp->plug_in_shm != NULL, -1);

  return gimp->plug_in_shm->shm_ID;
}

guchar *
plug_in_shm_get_addr (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (gimp->plug_in_shm != NULL, NULL);

  return gimp->plug_in_shm->shm_addr;
}
