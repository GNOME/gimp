/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-file.c
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

#include <errno.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"

#include "gimpplugin.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-file.h"
#include "gimppluginprocedure.h"

#include "gimp-log.h"

#include "gimp-intl.h"

#ifdef G_OS_WIN32
#include <stdlib.h>
#endif


typedef enum
{
  /*  positive values indicate the length of a matching magic  */

  FILE_MATCH_NONE = 0,
  FILE_MATCH_SIZE = -1
} FileMatchType;


/*  local function prototypes  */

static gboolean              file_proc_in_group (GimpPlugInProcedure    *file_proc,
                                                 GimpFileProcedureGroup  group);

static GimpPlugInProcedure * file_proc_find              (GSList       *procs,
                                                          GFile        *file,
                                                          GError      **error);
static GimpPlugInProcedure * file_proc_find_by_prefix    (GSList       *procs,
                                                          GFile        *file,
                                                          gboolean      skip_magic);
static GimpPlugInProcedure * file_proc_find_by_extension (GSList       *procs,
                                                          GFile        *file,
                                                          gboolean      skip_magic);
static GimpPlugInProcedure * file_proc_find_by_mime_type (GSList       *procs,
                                                          const gchar  *mime_type);
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

void
gimp_plug_in_manager_add_load_procedure (GimpPlugInManager   *manager,
                                         GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (! g_slist_find (manager->load_procs, proc))
    manager->load_procs = g_slist_prepend (manager->load_procs, proc);
}

void
gimp_plug_in_manager_add_save_procedure (GimpPlugInManager   *manager,
                                         GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (file_proc_in_group (proc, GIMP_FILE_PROCEDURE_GROUP_SAVE))
    {
      if (! g_slist_find (manager->save_procs, proc))
        manager->save_procs = g_slist_prepend (manager->save_procs, proc);
    }

  if (file_proc_in_group (proc, GIMP_FILE_PROCEDURE_GROUP_EXPORT))
    {
      if (! g_slist_find (manager->export_procs, proc))
        manager->export_procs = g_slist_prepend (manager->export_procs, proc);
    }
}

GSList *
gimp_plug_in_manager_get_file_procedures (GimpPlugInManager      *manager,
                                          GimpFileProcedureGroup  group)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_NONE:
      return NULL;

    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return manager->display_load_procs;

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return manager->display_save_procs;

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return manager->display_export_procs;

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find (GimpPlugInManager      *manager,
                                          GimpFileProcedureGroup  group,
                                          GFile                  *file,
                                          GError                **error)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_proc_find (manager->load_procs, file, error);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_proc_find (manager->save_procs, file, error);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_proc_find (manager->export_procs, file, error);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_prefix (GimpPlugInManager      *manager,
                                                    GimpFileProcedureGroup  group,
                                                    GFile                  *file)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_proc_find_by_prefix (manager->load_procs, file, FALSE);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_proc_find_by_prefix (manager->save_procs, file, FALSE);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_proc_find_by_prefix (manager->export_procs, file, FALSE);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_extension (GimpPlugInManager      *manager,
                                                       GimpFileProcedureGroup  group,
                                                       GFile                  *file)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_proc_find_by_extension (manager->load_procs, file, FALSE);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_proc_find_by_extension (manager->save_procs, file, FALSE);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_proc_find_by_extension (manager->export_procs, file, FALSE);

    default:
      g_return_val_if_reached (NULL);
    }
}

GimpPlugInProcedure *
gimp_plug_in_manager_file_procedure_find_by_mime_type (GimpPlugInManager      *manager,
                                                       GimpFileProcedureGroup  group,
                                                       const gchar            *mime_type)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      return file_proc_find_by_mime_type (manager->load_procs, mime_type);

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      return file_proc_find_by_mime_type (manager->save_procs, mime_type);

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      return file_proc_find_by_mime_type (manager->export_procs, mime_type);

    default:
      g_return_val_if_reached (NULL);
    }
}


/*  private functions  */

static gboolean
file_proc_in_group (GimpPlugInProcedure    *file_proc,
                    GimpFileProcedureGroup  group)
{
  const gchar *name        = gimp_object_get_name (file_proc);
  gboolean     is_xcf_save = FALSE;
  gboolean     is_filter   = FALSE;

  is_xcf_save = (strcmp (name, "gimp-xcf-save") == 0);

  is_filter   = (strcmp (name, "file-gz-export")  == 0 ||
                 strcmp (name, "file-bz2-export") == 0 ||
                 strcmp (name, "file-xz-export")  == 0);

  switch (group)
    {
    case GIMP_FILE_PROCEDURE_GROUP_NONE:
      return FALSE;

    case GIMP_FILE_PROCEDURE_GROUP_SAVE:
      /* Only .xcf shall pass */
      return is_xcf_save || is_filter;

    case GIMP_FILE_PROCEDURE_GROUP_EXPORT:
      /* Anything but .xcf shall pass */
      return ! is_xcf_save;

    case GIMP_FILE_PROCEDURE_GROUP_OPEN:
      /* No filter applied for Open */
      return TRUE;

    default:
    case GIMP_FILE_PROCEDURE_GROUP_ANY:
      return TRUE;
    }
}

static GimpPlugInProcedure *
file_proc_find (GSList  *procs,
                GFile   *file,
                GError **error)
{
  GimpPlugInProcedure *file_proc;
  GimpPlugInProcedure *size_matched_proc = NULL;
  gint                 size_match_count  = 0;

  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* First, check magicless prefixes/suffixes */
  file_proc = file_proc_find_by_name (procs, file, TRUE);

  if (file_proc)
    return file_proc;

  /* Then look for magics, but not on remote files */
  if (g_file_is_native (file))
    {
      GSList              *list;
      GInputStream        *input     = NULL;
      gboolean             opened    = FALSE;
      gsize                head_size = 0;
      guchar               head[256];
      FileMatchType        best_match_val = FILE_MATCH_NONE;
      GimpPlugInProcedure *best_file_proc = NULL;

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
                      GIMP_LOG (MAGIC_MATCH,
                                "magic match %d on %s\n",
                                match_val,
                                gimp_object_get_name (file_proc));

                      if (match_val > best_match_val)
                        {
                          best_match_val = match_val;
                          best_file_proc = file_proc;
                        }
                    }
                }
            }
        }

      if (input)
        g_object_unref (input);

      if (best_file_proc)
        {
          GIMP_LOG (MAGIC_MATCH,
                    "best magic match on %s\n",
                    gimp_object_get_name (best_file_proc));

          return best_file_proc;
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

static GimpPlugInProcedure *
file_proc_find_by_mime_type (GSList      *procs,
                             const gchar *mime_type)
{
  GSList *list;

  g_return_val_if_fail (mime_type != NULL, NULL);

  for (list = procs; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;
      GSList              *mime;

      for (mime = proc->mime_types_list; mime; mime = g_slist_next (mime))
        {
          if (! strcmp (mime_type, mime->data))
            return proc;
        }
    }

  return NULL;
}

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
                             gboolean  skip_magic)
{
  gchar *ext = gimp_file_get_extension (file);

  if (ext)
    {
      GSList *p;

      for (p = procs; p; p = g_slist_next (p))
        {
          GimpPlugInProcedure *proc = p->data;

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

  proc = file_proc_find_by_prefix (procs, file, skip_magic);

  if (! proc)
    proc = file_proc_find_by_extension (procs, file, skip_magic);

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
  gulong        num_operator_val;
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
            sscanf (num_operator_ptr+1, "%lu", &num_operator_val);
          else if (num_operator_ptr[2] == 'x') /* hexadecimal */
            sscanf (num_operator_ptr+3, "%lx", &num_operator_val);
          else                                 /* octal */
            sscanf (num_operator_ptr+2, "%lo", &num_operator_val);

          num_operator = *num_operator_ptr;
        }
    }

  if (numbytes > 0)
    {
      /* Numerical test */

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

      if (numbytes == 5)
        {
          /* Check for file size */

          GFileInfo *info = g_file_query_info (file,
                                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                               G_FILE_QUERY_INFO_NONE,
                                               NULL, NULL);
          if (! info)
            return FILE_MATCH_NONE;

          fileval = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
          g_object_unref (info);
        }
      else if (offs >= 0 &&
               (offs + numbytes <= headsize))
        {
           /* We have it in memory */

          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (glong) file_head[offs + k];
        }
      else
        {
          /* Read it from file */

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
        fileval &= num_operator_val;

      if (num_test == '<')
        {
          if (fileval < num_testval)
            found = numbytes;
        }
      else if (num_test == '>')
        {
          if (fileval > num_testval)
            found = numbytes;
        }
      else
        {
          if (fileval == num_testval)
            found = numbytes;
        }

      if (found && (numbytes == 5))
        found = FILE_MATCH_SIZE;
    }
  else if (numbytes == 0)
    {
       /* String test */

      gchar mem_testval[256];

      file_convert_string (value,
                           mem_testval, sizeof (mem_testval),
                           &numbytes);

      if (numbytes <= 0)
        return FILE_MATCH_NONE;

      if (offs >= 0 &&
          (offs + numbytes <= headsize))
        {
          /* We have it in memory */

          if (memcmp (mem_testval, file_head + offs, numbytes) == 0)
            found = numbytes;
        }
      else
        {
          /* Read it from file */

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

              if (byte != mem_testval[k])
                return FILE_MATCH_NONE;
            }

          found = numbytes;
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
  gboolean      and            = FALSE;
  gboolean      found          = FALSE;
  FileMatchType best_match_val = FILE_MATCH_NONE;
  FileMatchType match_val      = FILE_MATCH_NONE;

  for (; magics_list; magics_list = magics_list->next)
    {
      const gchar   *offset;
      const gchar   *type;
      const gchar   *value;
      FileMatchType  single_match_val = FILE_MATCH_NONE;

      if ((offset      = magics_list->data) == NULL) return FILE_MATCH_NONE;
      if ((magics_list = magics_list->next) == NULL) return FILE_MATCH_NONE;
      if ((type        = magics_list->data) == NULL) return FILE_MATCH_NONE;
      if ((magics_list = magics_list->next) == NULL) return FILE_MATCH_NONE;
      if ((value       = magics_list->data) == NULL) return FILE_MATCH_NONE;

      single_match_val = file_check_single_magic (offset, type, value,
                                                  head, headsize,
                                                  file, input);

      if (and)
        found = found && (single_match_val != FILE_MATCH_NONE);
      else
        found = (single_match_val != FILE_MATCH_NONE);

      if (found)
        {
          if (match_val == FILE_MATCH_NONE)
            {
              /* if we have no match yet, this is it in any case */

              match_val = single_match_val;
            }
          else if (single_match_val != FILE_MATCH_NONE)
            {
              /* else if we have a match on this one, combine it with the
               * existing return value
               */

              if (single_match_val == FILE_MATCH_SIZE)
                {
                  /* if we already have a magic match, simply increase
                   * that by one to indicate "better match", not perfect
                   * but better than losing the additional size match
                   * entirely
                   */
                  if (match_val != FILE_MATCH_SIZE)
                    match_val += 1;
                }
              else
                {
                  /* if we already have a magic match, simply add to its
                   * length; otherwise if we already have a size match,
                   * combine it with this match, see comment above
                   */
                  if (match_val != FILE_MATCH_SIZE)
                    match_val += single_match_val;
                  else
                    match_val = single_match_val + 1;
                }
            }
        }
      else
        {
          match_val = FILE_MATCH_NONE;
        }

      and = (strchr (offset, '&') != NULL);

      if (! and)
        {
          /* when done with this 'and' list, update best_match_val */

          if (best_match_val == FILE_MATCH_NONE)
            {
              /* if we have no best match yet, this is it */

              best_match_val = match_val;
            }
          else if (match_val != FILE_MATCH_NONE)
            {
              /* otherwise if this was a match, update the best match, note
               * that by using MAX we will not overwrite a magic match
               * with a size match
               */

              best_match_val = MAX (best_match_val, match_val);
            }

          match_val = FILE_MATCH_NONE;
        }
    }

  return best_match_val;
}
