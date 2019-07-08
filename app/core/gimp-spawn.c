/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-spawn.c
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

#include "config.h"

#include <glib-object.h>

#ifdef HAVE_VFORK
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>
#endif

#include "core-types.h"

#include "gimp-spawn.h"

#include "gimp-intl.h"


#ifdef HAVE_VFORK

/* copied from glib */
static gint
exec_err_to_g_error (gint en)
{
  switch (en)
    {
#ifdef EACCES
    case EACCES:
      return G_SPAWN_ERROR_ACCES;
      break;
#endif

#ifdef EPERM
    case EPERM:
      return G_SPAWN_ERROR_PERM;
      break;
#endif

#ifdef E2BIG
    case E2BIG:
      return G_SPAWN_ERROR_TOO_BIG;
      break;
#endif

#ifdef ENOEXEC
    case ENOEXEC:
      return G_SPAWN_ERROR_NOEXEC;
      break;
#endif

#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
      return G_SPAWN_ERROR_NAMETOOLONG;
      break;
#endif

#ifdef ENOENT
    case ENOENT:
      return G_SPAWN_ERROR_NOENT;
      break;
#endif

#ifdef ENOMEM
    case ENOMEM:
      return G_SPAWN_ERROR_NOMEM;
      break;
#endif

#ifdef ENOTDIR
    case ENOTDIR:
      return G_SPAWN_ERROR_NOTDIR;
      break;
#endif

#ifdef ELOOP
    case ELOOP:
      return G_SPAWN_ERROR_LOOP;
      break;
#endif

#ifdef ETXTBUSY
    case ETXTBUSY:
      return G_SPAWN_ERROR_TXTBUSY;
      break;
#endif

#ifdef EIO
    case EIO:
      return G_SPAWN_ERROR_IO;
      break;
#endif

#ifdef ENFILE
    case ENFILE:
      return G_SPAWN_ERROR_NFILE;
      break;
#endif

#ifdef EMFILE
    case EMFILE:
      return G_SPAWN_ERROR_MFILE;
      break;
#endif

#ifdef EINVAL
    case EINVAL:
      return G_SPAWN_ERROR_INVAL;
      break;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_SPAWN_ERROR_ISDIR;
      break;
#endif

#ifdef ELIBBAD
    case ELIBBAD:
      return G_SPAWN_ERROR_LIBBAD;
      break;
#endif

    default:
      return G_SPAWN_ERROR_FAILED;
      break;
    }
}

#endif /* HAVE_VFORK */

gboolean
gimp_spawn_async (gchar       **argv,
                  gchar       **envp,
                  GSpawnFlags   flags,
                  GPid         *child_pid,
                  GError      **error)
{
  g_return_val_if_fail (argv != NULL, FALSE);
  g_return_val_if_fail (argv[0] != NULL, FALSE);

#ifdef HAVE_VFORK
  if (flags == (G_SPAWN_LEAVE_DESCRIPTORS_OPEN |
                G_SPAWN_DO_NOT_REAP_CHILD      |
                G_SPAWN_CHILD_INHERITS_STDIN))
    {
      pid_t pid;

      pid = vfork ();

      if (pid < 0)
        {
          gint errsv = errno;

          g_set_error (error,
                       G_SPAWN_ERROR,
                       G_SPAWN_ERROR_FORK,
                       _("Failed to fork (%s)"),
                       g_strerror (errsv));

          return FALSE;
        }
      else if (pid == 0)
        {
          if (envp)
            execve (argv[0], argv, envp);
          else
            execv (argv[0], argv);

          _exit (errno);
        }
      else
        {
          int   status = -1;
          pid_t result;

          result = waitpid (pid, &status, WNOHANG);

          if (result)
            {
              if (result < 0)
                {
                  g_warning ("waitpid() should not fail in "
                             "gimp_spawn_async()");
                }

              if (WIFEXITED (status))
                status = WEXITSTATUS (status);
              else
                status = -1;

              g_set_error (error,
                           G_SPAWN_ERROR,
                           exec_err_to_g_error (status),
                           _("Failed to execute child process “%s” (%s)"),
                           argv[0],
                           g_strerror (status));

              return FALSE;
            }

          if (child_pid) *child_pid = pid;

          return TRUE;
        }
    }
#endif /* HAVE_VFORK */

  return g_spawn_async (NULL, argv, envp, flags, NULL, NULL, child_pid, error);
}

void
gimp_spawn_set_cloexec (gint fd)
{
#if defined (G_OS_WIN32)
  SetHandleInformation ((HANDLE) _get_osfhandle (fd), HANDLE_FLAG_INHERIT, 0);
#elif defined (HAVE_FCNTL_H)
  fcntl (fd, F_SETFD, fcntl (fd, F_GETFD, 0) | FD_CLOEXEC);
#elif defined (__GNUC__)
#warning gimp_spawn_set_cloexec() is not implemented for the target platform
#endif
}
