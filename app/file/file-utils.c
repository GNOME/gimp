/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-utils.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core/core-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in.h"
#include "plug-in/plug-in-proc-def.h"

#include "file-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static PlugInProcDef * file_proc_find_by_prefix    (GSList       *procs,
                                                    const gchar  *uri,
                                                    gboolean      skip_magic);
static PlugInProcDef * file_proc_find_by_extension (GSList       *procs,
                                                    const gchar  *uri,
                                                    gboolean      skip_magic);
static PlugInProcDef * file_proc_find_by_name      (GSList       *procs,
                                                    const gchar  *uri,
                                                    gboolean      skip_magic);
static void            file_convert_string         (const gchar  *instr,
                                                    gchar        *outmem,
                                                    gint          maxmem,
                                                    gint         *nmem);
static gint            file_check_single_magic     (const gchar  *offset,
                                                    const gchar  *type,
                                                    const gchar  *value,
                                                    const guchar *file_head,
                                                    gint          headsize,
                                                    FILE         *ifp);
static gint            file_check_magic_list       (GSList       *magics_list,
                                                    const guchar *head,
                                                    gint          headsize,
                                                    FILE         *ifp);


/*  public functions  */

gchar *
file_utils_filename_to_uri (GSList       *procs,
                            const gchar  *filename,
                            GError      **error)
{
  gchar *absolute;
  gchar *uri;

  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);

  /*  check for prefixes like http or ftp  */
  if (file_proc_find_by_prefix (procs, filename, FALSE))
    {
      if (g_utf8_validate (filename, -1, NULL))
        {
          return g_strdup (filename);
        }
      else
        {
          g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
                       _("Invalid character sequence in URI"));
          return NULL;
        }
    }

  if (! g_path_is_absolute (filename))
    {
      gchar *current;

      current = g_get_current_dir ();
      absolute = g_build_filename (current, filename, NULL);
      g_free (current);
    }
  else
    {
      absolute = g_strdup (filename);
    }

  uri = g_filename_to_uri (absolute, NULL, error);

  g_free (absolute);

  return uri;
}

/**
 * file_utils_filename_from_uri:
 * @uri: a URI
 *
 * A utility function to be used as a replacement for
 * g_filename_from_uri(). It deals with file: URIs with hostname in a
 * platform-specific way. On Win32, a UNC path is created and
 * returned, on other platforms the URI is detected as non-local and
 * NULL is returned.
 *
 * Returns: newly allocated filename or %NULL if @uri is a remote file
 **/
gchar *
file_utils_filename_from_uri (const gchar *uri)
{
  gchar *filename;
  gchar *hostname;

  g_return_val_if_fail (uri != NULL, NULL);

  filename = g_filename_from_uri (uri, &hostname, NULL);

  if (!filename)
    return NULL;

  if (hostname)
    {
      /*  we have a file: URI with a hostname                           */
#ifdef G_OS_WIN32
      /*  on Win32, create a valid UNC path and use it as the filename  */

      gchar *tmp = g_build_filename ("//", hostname, filename, NULL);

      g_free (filename);
      filename = tmp;
#else
      /*  otherwise return NULL, caller should use URI then             */
      g_free (filename);
      filename = NULL;
#endif

      g_free (hostname);
    }

  return filename;
}

PlugInProcDef *
file_utils_find_proc (GSList       *procs,
                      const gchar  *uri)
{
  PlugInProcDef *file_proc;
  GSList        *all_procs = procs;
  gchar         *filename;

  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  /* First, check magicless prefixes/suffixes */
  file_proc = file_proc_find_by_name (all_procs, uri, TRUE);

  if (file_proc)
    return file_proc;

  filename = file_utils_filename_from_uri (uri);

  /* Then look for magics */
  if (filename)
    {
      PlugInProcDef *size_matched_proc = NULL;
      FILE          *ifp               = NULL;
      gint           head_size         = -2;
      gint           size_match_count  = 0;
      gint           match_val;
      guchar         head[256];

      while (procs)
        {
          file_proc = procs->data;
          procs = procs->next;

          if (file_proc->magics_list)
            {
              if (head_size == -2)
                {
                  head_size = 0;
                  if ((ifp = fopen (filename, "rb")) != NULL)
                    head_size = fread ((gchar *) head, 1, sizeof (head), ifp);
                }

              if (head_size >= 4)
                {
                  match_val = file_check_magic_list (file_proc->magics_list,
                                                     head, head_size,
                                                     ifp);

                  if (match_val == 2)  /* size match ? */
                    { /* Use it only if no other magic matches */
                      size_match_count++;
                      size_matched_proc = file_proc;
                    }
                  else if (match_val)
                    {
                      fclose (ifp);
                      g_free (filename);

                      return file_proc;
                    }
                }
            }
        }

      if (ifp)
        fclose (ifp);

      g_free (filename);

      if (size_match_count == 1)
        return size_matched_proc;
    }

  /* As a last resort, try matching by name */
  return file_proc_find_by_name (all_procs, uri, FALSE);
}

PlugInProcDef *
file_utils_find_proc_by_extension (GSList      *procs,
                                   const gchar *uri)
{
  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  return file_proc_find_by_extension (procs, uri, FALSE);
}

gchar *
file_utils_uri_to_utf8_basename (const gchar *uri)
{
  gchar *filename;
  gchar *basename;

  g_return_val_if_fail (uri != NULL, NULL);

  filename = file_utils_uri_to_utf8_filename (uri);

  if (strstr (filename, G_DIR_SEPARATOR_S))
    {
      basename = g_path_get_basename (filename);

      g_free (filename);

      return basename;
    }
  else if (strstr (filename, "://"))
    {
      basename = strrchr (uri, '/');

      basename = g_strdup (basename + 1);

      g_free (filename);

      return basename;
    }

  return filename;
}

gchar *
file_utils_uri_to_utf8_filename (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  if (g_str_has_prefix (uri, "file:"))
    {
      gchar *filename = file_utils_filename_from_uri (uri);

      if (filename)
        {
          GError *error = NULL;
          gchar  *utf8;

          utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, &error);
          g_free (filename);

          if (utf8)
            return utf8;

          g_warning ("%s: cannot convert filename to UTF-8: %s",
                     G_STRLOC, error->message);
          g_error_free (error);
        }
    }

  return g_strdup (uri);
}


/*  private functions  */

static PlugInProcDef *
file_proc_find_by_prefix (GSList      *procs,
                          const gchar *uri,
                          gboolean     skip_magic)
{
  GSList *p;

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *prefixes;

      if (skip_magic && proc->magics_list)
	continue;

      for (prefixes = proc->prefixes_list;
	   prefixes;
	   prefixes = g_slist_next (prefixes))
	{
	  if (strncmp (uri, prefixes->data, strlen (prefixes->data)) == 0)
	    return proc;
	}
     }

  return NULL;
}

static PlugInProcDef *
file_proc_find_by_extension (GSList      *procs,
                             const gchar *uri,
                             gboolean     skip_magic)
{
  GSList      *p;
  const gchar *ext;

  ext = strrchr (uri, '.');

  if (ext)
    ext++;

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *extensions;

      for (extensions = proc->extensions_list;
	   ext && extensions;
	   extensions = g_slist_next (extensions))
	{
	  const gchar *p1 = ext;
	  const gchar *p2 = extensions->data;

          if (skip_magic && proc->magics_list)
	    continue;

	  while (*p1 && *p2)
	    {
	      if (g_ascii_tolower (*p1) != g_ascii_tolower (*p2))
		break;

	      p1++;
	      p2++;
	    }

	  if (!(*p1) && !(*p2))
	    return proc;
	}
    }

  return NULL;
}

static PlugInProcDef *
file_proc_find_by_name (GSList      *procs,
		        const gchar *uri,
		        gboolean     skip_magic)
{
  PlugInProcDef *proc;

  proc = file_proc_find_by_prefix (procs, uri, skip_magic);

  if (! proc)
    proc = file_proc_find_by_extension (procs, uri, skip_magic);

  return proc;
}

static void
file_convert_string (const gchar *instr,
		     gchar       *outmem,
		     gint         maxmem,
		     gint        *nmem)
{
  /* Convert a string in C-notation to array of char */
  const guchar *uin  = (const guchar *) instr;
  guchar       *uout = (guchar *) outmem;
  guchar        tmp[5], *tmpptr;
  guint         k;

  while ((*uin != '\0') && ((((gchar *) uout) - outmem) < maxmem))
    {
      if (*uin != '\\')   /* Not an escaped character ? */
        {
          *(uout++) = *(uin++);
          continue;
        }

      if (*(++uin) == '\0')
        {
          *(uout++) = '\\';
          break;
        }

      switch (*uin)
        {
          case '0':  case '1':  case '2':  case '3': /* octal */
            for (tmpptr = tmp; (tmpptr - tmp) <= 3;)
              {
                *(tmpptr++) = *(uin++);
                if (   (*uin == '\0') || (!g_ascii_isdigit (*uin))
                    || (*uin == '8') || (*uin == '9'))
                  break;
              }

            *tmpptr = '\0';
            sscanf ((gchar *) tmp, "%o", &k);
            *(uout++) = k;
            break;

          case 'a': *(uout++) = '\a'; uin++; break;
          case 'b': *(uout++) = '\b'; uin++; break;
          case 't': *(uout++) = '\t'; uin++; break;
          case 'n': *(uout++) = '\n'; uin++; break;
          case 'v': *(uout++) = '\v'; uin++; break;
          case 'f': *(uout++) = '\f'; uin++; break;
          case 'r': *(uout++) = '\r'; uin++; break;

          default : *(uout++) = *(uin++); break;
        }
    }

  *nmem = ((gchar *) uout) - outmem;
}

static gint
file_check_single_magic (const gchar  *offset,
                         const gchar  *type,
                         const gchar  *value,
                         const guchar *file_head,
                         gint          headsize,
                         FILE         *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  glong         offs;
  gulong        num_testval, num_operatorval;
  gulong        fileval;
  gint          numbytes, k;
  gint          c     = 0;
  gint          found = 0;
  const gchar  *num_operator_ptr;
  gchar         num_operator;
  gchar         num_test;
  guchar        mem_testval[256];

  /* Check offset */
  if (sscanf (offset, "%ld", &offs) != 1) return (0);
  if (offs < 0) return (0);

  /* Check type of test */
  num_operator_ptr = NULL;
  num_operator     = '\0';
  num_test         = '=';

  if (strncmp (type, "byte", 4) == 0)
    {
      numbytes = 1;
      num_operator_ptr = type + 4;
    }
  else if (strncmp (type, "short", 5) == 0)
    {
      numbytes = 2;
      num_operator_ptr = type + 5;
    }
  else if (strncmp (type, "long", 4) == 0)
    {
      numbytes = 4;
      num_operator_ptr = type + 4;
    }
  else if (strncmp (type, "size", 4) == 0)
    {
      numbytes = 5;
    }
  else if (strcmp (type, "string") == 0)
    {
      numbytes = 0;
    }
  else return (0);

  /* Check numerical operator value if present */
  if (num_operator_ptr && (*num_operator_ptr == '&'))
    {
      if (g_ascii_isdigit (num_operator_ptr[1]))
        {
          if (num_operator_ptr[1] != '0')      /* decimal */
            sscanf (num_operator_ptr+1, "%lu", &num_operatorval);
          else if (num_operator_ptr[2] == 'x') /* hexadecimal */
            sscanf (num_operator_ptr+3, "%lx", &num_operatorval);
          else                                 /* octal */
            sscanf (num_operator_ptr+2, "%lo", &num_operatorval);
          num_operator = *num_operator_ptr;
        }
    }

  if (numbytes > 0)   /* Numerical test ? */
    {
      /* Check test value */
      if ((value[0] == '=') || (value[0] == '>') || (value[0] == '<'))
      {
        num_test = value[0];
        value++;
      }
      if (!g_ascii_isdigit (value[0])) return (0);

      /*
       * to anybody reading this: is strtol's parsing behaviour
       * (e.g. "0x" prefix) broken on some systems or why do we
       * do the base detection ourselves?
       * */
      if (value[0] != '0')      /* decimal */
        num_testval = strtol(value, NULL, 10);
      else if (value[1] == 'x') /* hexadecimal */
        num_testval = (unsigned long)strtoul(value+2, NULL, 16);
      else                      /* octal */
        num_testval = strtol(value+1, NULL, 8);

      fileval = 0;
      if (numbytes == 5)    /* Check for file size ? */
        {
	  struct stat buf;

          if (fstat (fileno (ifp), &buf) < 0) return (0);
          fileval = buf.st_size;
        }
      else if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          for (k = 0; k < numbytes; k++)
          fileval = (fileval << 8) | (long)file_head[offs+k];
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (c = getc (ifp));
          if (c == EOF) return (0);
        }
      if (num_operator == '&')
        fileval &= num_operatorval;

      if (num_test == '<')
        found = (fileval < num_testval);
      else if (num_test == '>')
        found = (fileval > num_testval);
      else
        found = (fileval == num_testval);

      if (found && (numbytes == 5)) found = 2;
    }
  else if (numbytes == 0) /* String test */
    {
      file_convert_string (value,
                           mem_testval, sizeof (mem_testval),
                           &numbytes);

      if (numbytes <= 0) return (0);

      if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          found = (memcmp (mem_testval, file_head+offs, numbytes) == 0);
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          found = 1;
          for (k = 0; found && (k < numbytes); k++)
            {
              c = getc (ifp);
              found = (c != EOF) && (c == (int)mem_testval[k]);
            }
        }
    }

  return found;
}

/*
 *  Return values are 0: no match, 1: magic match, 2: size match
 */
static gint
file_check_magic_list (GSList       *magics_list,
		       const guchar *head,
		       gint          headsize,
		       FILE         *ifp)

{
  const gchar *offset;
  const gchar *type;
  const gchar *value;
  gint         and   = 0;
  gint         found = 0;
  gint         match_val;

  while (magics_list)
    {
      if ((offset      = magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((type        = magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((value       = magics_list->data) == NULL) break;

      magics_list = magics_list->next;

      match_val = file_check_single_magic (offset, type, value,
                                           head, headsize,
                                           ifp);
      if (and)
	found = found && match_val;
      else
	found = match_val;

      and = (strchr (offset, '&') != NULL);

      if ((!and) && found)
	return match_val;
    }

  return 0;
}
