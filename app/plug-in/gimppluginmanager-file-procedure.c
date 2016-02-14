/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * gimppluginmanager-file-procedure.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp-utils.h"

#include "gimppluginmanager-file-procedure.h"
#include "gimppluginprocedure.h"

#include "gimp-intl.h"


typedef enum
{
  FILE_MATCH_NONE,
  FILE_MATCH_MAGIC,
  FILE_MATCH_SIZE
} FileMatchType;


/*  local function prototypes  */

static GimpPlugInProcedure * file_proc_find_by_prefix    (GSList       *procs,
                                                          GFile        *file,
                                                          gboolean      skip_magic);
static GimpPlugInProcedure * file_proc_find_by_extension (GSList       *procs,
                                                          GFile        *file,
                                                          gboolean      skip_magic,
                                                          gboolean      uri_procs_only);
static GimpPlugInProcedure * file_proc_find_by_name      (GSList       *procs,
                                                          GFile        *file,
                                                          gboolean      skip_magic);

static void                  file_convert_string         (const gchar  *instr,
                                                          gchar        *outmem,
                                                          gint          maxmem,
                                                          gint         *nmem);
static FileMatchType         file_check_single_magic     (const gchar  *offset,
                                                          const gchar  *type,
                                                          const gchar  *value,
                                                          const guchar *file_head,
                                                          gint          headsize,
                                                          GFile        *file,
                                                          GInputStream *input);
static FileMatchType         file_check_magic_list       (GSList       *magics_list,
                                                          const guchar *head,
                                                          gint          headsize,
                                                          GFile        *file,
                                                          GInputStream *input);


/*  public functions  */

GimpPlugInProcedure *
file_procedure_find (GSList  *procs,
                     GFile   *file,
                     GError **error)
{
  GimpPlugInProcedure *file_proc;
  GimpPlugInProcedure *size_matched_proc = NULL;
  gint                 size_match_count  = 0;

  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* First, check magicless prefixes/suffixes: */

  if (! file_proc_find_by_extension (procs, file, FALSE, TRUE))
    {
      /* If there is not any (with or without magic) file proc that
       * can load the file by extension directly, try to find a proc
       * that can load the prefix
       */
      file_proc = file_proc_find_by_prefix (procs, file, TRUE);
    }
  else
    {
      /* Otherwise try to find a magicless file proc that handles the
       * extension
       */
      file_proc = file_proc_find_by_extension (procs, file, TRUE, FALSE);
    }

  if (file_proc)
    return file_proc;

  /* Then look for magics, but not on remote files */
  if (g_file_is_native (file))
    {
      GSList       *list;
      GInputStream *input     = NULL;
      gboolean      opened    = FALSE;
      gsize         head_size = 0;
      guchar        head[256];

      for (list = procs; list; list = g_slist_next (list))
        {
          file_proc = list->data;

          if (file_proc->magics_list)
            {
              if (G_UNLIKELY (! opened))
                {
                  input = G_INPUT_STREAM (g_file_read (file, NULL, error));

                  if (input)
                    {
                      g_input_stream_read_all (input,
                                               head, sizeof (head),
                                               &head_size, NULL, error);

                      if (head_size < 4)
                        {
                          g_object_unref (input);
                          input = NULL;
                        }
                      else
                        {
                          GDataInputStream *data_input;

                          data_input = g_data_input_stream_new (input);
                          g_object_unref (input);
                          input = G_INPUT_STREAM (data_input);
                        }
                    }

                  opened = TRUE;
                }

              if (head_size >= 4)
                {
                  FileMatchType match_val;

                  match_val = file_check_magic_list (file_proc->magics_list,
                                                     head, head_size,
                                                     file, input);

                  if (match_val == FILE_MATCH_SIZE)
                    {
                      /* Use it only if no other magic matches */
                      size_match_count++;
                      size_matched_proc = file_proc;
                    }
                  else if (match_val != FILE_MATCH_NONE)
                    {
                      g_object_unref (input);

                      return file_proc;
                    }
                }
            }
        }

      if (input)
        {
#if 0
          if (ferror (ifp))
            g_set_error_literal (error, G_FILE_ERROR,
                                 g_file_error_from_errno (errno),
                                 g_strerror (errno));
#endif

          g_object_unref (input);
        }
    }

  if (size_match_count == 1)
    return size_matched_proc;

  /* As a last resort, try matching by name, not skipping magic procs */
  file_proc = file_proc_find_by_name (procs, file, FALSE);

  if (file_proc)
    {
      /* we found a procedure, clear error that might have been set */
      g_clear_error (error);
    }
  else
    {
      /* set an error message unless one was already set */
      if (error && *error == NULL)
	g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
			     _("Unknown file type"));
    }

  return file_proc;
}

GimpPlugInProcedure *
file_procedure_find_by_prefix (GSList *procs,
                               GFile  *file)
{
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return file_proc_find_by_prefix (procs, file, FALSE);
}

GimpPlugInProcedure *
file_procedure_find_by_extension (GSList *procs,
                                  GFile  *file)
{
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return file_proc_find_by_extension (procs, file, FALSE, FALSE);
}

GimpPlugInProcedure *
file_procedure_find_by_mime_type (GSList      *procs,
                                  const gchar *mime_type)
{
  GSList *list;

  g_return_val_if_fail (mime_type != NULL, NULL);

  for (list = procs; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->mime_type && ! strcmp (mime_type, proc->mime_type))
        return proc;
    }

  return NULL;
}


/*  private functions  */

static GimpPlugInProcedure *
file_proc_find_by_prefix (GSList   *procs,
                          GFile    *file,
                          gboolean  skip_magic)
{
  gchar  *uri = g_file_get_uri (file);
  GSList *p;

  for (p = procs; p; p = g_slist_next (p))
    {
      GimpPlugInProcedure *proc = p->data;
      GSList              *prefixes;

      if (skip_magic && proc->magics_list)
        continue;

      for (prefixes = proc->prefixes_list;
           prefixes;
           prefixes = g_slist_next (prefixes))
        {
          if (g_str_has_prefix (uri, prefixes->data))
            {
              g_free (uri);
              return proc;
            }
        }
     }

  g_free (uri);

  return NULL;
}

static GimpPlugInProcedure *
file_proc_find_by_extension (GSList   *procs,
                             GFile    *file,
                             gboolean  skip_magic,
                             gboolean  uri_procs_only)
{
  gchar *ext = gimp_file_get_extension (file);

  if (ext)
    {
      GSList *p;

      for (p = procs; p; p = g_slist_next (p))
        {
          GimpPlugInProcedure *proc = p->data;

          if (uri_procs_only && ! proc->handles_uri)
            continue;

          if (skip_magic && proc->magics_list)
            continue;

          if (g_slist_find_custom (proc->extensions_list,
                                   ext + 1,
                                   (GCompareFunc) g_ascii_strcasecmp))
            {
              g_free (ext);

              return proc;
            }
        }

      g_free (ext);
    }

  return NULL;
}

static GimpPlugInProcedure *
file_proc_find_by_name (GSList   *procs,
                        GFile    *file,
                        gboolean  skip_magic)
{
  GimpPlugInProcedure *proc;

  proc = file_proc_find_by_extension (procs, file, skip_magic, TRUE);

  if (! proc)
    proc = file_proc_find_by_prefix (procs, file, skip_magic);

  if (! proc)
    proc = file_proc_find_by_extension (procs, file, skip_magic, FALSE);

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

static FileMatchType
file_check_single_magic (const gchar  *offset,
                         const gchar  *type,
                         const gchar  *value,
                         const guchar *file_head,
                         gint          headsize,
                         GFile        *file,
                         GInputStream *input)

{
  FileMatchType found = FILE_MATCH_NONE;
  glong         offs;
  gulong        num_testval;
  gulong        num_operatorval;
  gint          numbytes, k;
  const gchar  *num_operator_ptr;
  gchar         num_operator;

  /* Check offset */
  if (sscanf (offset, "%ld", &offs) != 1)
    return FILE_MATCH_NONE;

  /* Check type of test */
  num_operator_ptr = NULL;
  num_operator     = '\0';

  if (g_str_has_prefix (type, "byte"))
    {
      numbytes = 1;
      num_operator_ptr = type + strlen ("byte");
    }
  else if (g_str_has_prefix (type, "short"))
    {
      numbytes = 2;
      num_operator_ptr = type + strlen ("short");
    }
  else if (g_str_has_prefix (type, "long"))
    {
      numbytes = 4;
      num_operator_ptr = type + strlen ("long");
    }
  else if (g_str_has_prefix (type, "size"))
    {
      numbytes = 5;
    }
  else if (strcmp (type, "string") == 0)
    {
      numbytes = 0;
    }
  else
    {
      return FILE_MATCH_NONE;
    }

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
      gchar   num_test = '=';
      gulong  fileval  = 0;

      /* Check test value */
      if ((value[0] == '>') || (value[0] == '<'))
        {
          num_test = value[0];
          value++;
        }

      errno = 0;
      num_testval = strtol (value, NULL, 0);

      if (errno != 0)
        return FILE_MATCH_NONE;

      if (numbytes == 5)    /* Check for file size ? */
        {
          GFileInfo *info = g_file_query_info (file,
                                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                               G_FILE_QUERY_INFO_NONE,
                                               NULL, NULL);
          if (! info)
            return FILE_MATCH_NONE;

          fileval = g_file_info_get_size (info);
          g_object_unref (info);
        }
      else if (offs >= 0 &&
               (offs + numbytes <= headsize)) /* We have it in memory ? */
        {
          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (glong) file_head[offs + k];
        }
      else   /* Read it from file */
        {
          if (! g_seekable_seek (G_SEEKABLE (input), offs,
                                 (offs >= 0) ? G_SEEK_SET : G_SEEK_END,
                                 NULL, NULL))
            return FILE_MATCH_NONE;

          for (k = 0; k < numbytes; k++)
            {
              guchar  byte;
              GError *error = NULL;

              byte = g_data_input_stream_read_byte (G_DATA_INPUT_STREAM (input),
                                                    NULL, &error);
              if (error)
                {
                  g_clear_error (&error);
                  return FILE_MATCH_NONE;
                }

              fileval = (fileval << 8) | byte;
            }
        }

      if (num_operator == '&')
        fileval &= num_operatorval;

      if (num_test == '<')
        found = (fileval < num_testval);
      else if (num_test == '>')
        found = (fileval > num_testval);
      else
        found = (fileval == num_testval);

      if (found && (numbytes == 5))
        found = FILE_MATCH_SIZE;
    }
  else if (numbytes == 0) /* String test */
    {
      gchar mem_testval[256];

      file_convert_string (value,
                           mem_testval, sizeof (mem_testval),
                           &numbytes);

      if (numbytes <= 0)
        return FILE_MATCH_NONE;

      if (offs >= 0 &&
          (offs + numbytes <= headsize)) /* We have it in memory ? */
        {
          found = (memcmp (mem_testval, file_head + offs, numbytes) == 0);
        }
      else   /* Read it from file */
        {
          if (! g_seekable_seek (G_SEEKABLE (input), offs,
                                 (offs >= 0) ? G_SEEK_SET : G_SEEK_END,
                                 NULL, NULL))
            return FILE_MATCH_NONE;

          found = FILE_MATCH_MAGIC;

          for (k = 0; found && (k < numbytes); k++)
            {
              guchar  byte;
              GError *error = NULL;

              byte = g_data_input_stream_read_byte (G_DATA_INPUT_STREAM (input),
                                                    NULL, &error);
              if (error)
                {
                  g_clear_error (&error);
                  return FILE_MATCH_NONE;
                }

              if (byte != mem_testval[k])
                found = FILE_MATCH_NONE;
            }
        }
    }

  return found;
}

static FileMatchType
file_check_magic_list (GSList       *magics_list,
                       const guchar *head,
                       gint          headsize,
                       GFile        *file,
                       GInputStream *input)

{
  const gchar   *offset;
  const gchar   *type;
  const gchar   *value;
  gboolean       and   = FALSE;
  gboolean       found = FALSE;
  FileMatchType  match_val;

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
                                           file, input);
      if (and)
        found = found && (match_val != FILE_MATCH_NONE);
      else
        found = (match_val != FILE_MATCH_NONE);

      and = (strchr (offset, '&') != NULL);

      if (! and && found)
        return match_val;
    }

  return FILE_MATCH_NONE;
}
