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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpthumb/gimpthumb.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "plug-in/gimppluginprocedure.h"

#include "file-utils.h"

#include "gimp-intl.h"


typedef enum
{
  FILE_MATCH_NONE,
  FILE_MATCH_MAGIC,
  FILE_MATCH_SIZE
} FileMatchType;


/*  local function prototypes  */

static GimpPlugInProcedure * file_proc_find_by_prefix    (GSList       *procs,
                                                          const gchar  *uri,
                                                          gboolean      skip_magic);
static GimpPlugInProcedure * file_proc_find_by_extension (GSList       *procs,
                                                          const gchar  *uri,
                                                          gboolean      skip_magic);
static GimpPlugInProcedure * file_proc_find_by_name      (GSList       *procs,
                                                          const gchar  *uri,
                                                          gboolean      skip_magic);

static gchar *               file_utils_unescape_uri     (const gchar  *escaped,
                                                          gint          len,
                                                          const gchar  *illegal_escaped_characters,
                                                          gboolean      ascii_must_not_be_escaped);

static void                  file_convert_string         (const gchar  *instr,
                                                          gchar        *outmem,
                                                          gint          maxmem,
                                                          gint         *nmem);
static FileMatchType         file_check_single_magic     (const gchar  *offset,
                                                          const gchar  *type,
                                                          const gchar  *value,
                                                          const guchar *file_head,
                                                          gint          headsize,
                                                          FILE         *ifp);
static FileMatchType         file_check_magic_list       (GSList       *magics_list,
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
  else if (strstr (filename, "://"))
    {
      gchar *scheme;
      gchar *canon;

      scheme = g_strndup (filename, (strstr (filename, "://") - filename));
      canon  = g_strdup (scheme);

      g_strcanon (canon, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+-.", '-');

      if (! strcmp (scheme, canon) && g_ascii_isgraph (canon[0]))
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("URI scheme '%s:' is not supported"), scheme);

          g_free (scheme);
          g_free (canon);

          return NULL;
        }

      g_free (scheme);
      g_free (canon);
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

GimpPlugInProcedure *
file_utils_find_proc (GSList       *procs,
                      const gchar  *uri,
                      GError      **error)
{
  GimpPlugInProcedure *file_proc;
  GSList              *all_procs = procs;
  gchar               *filename;

  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* First, check magicless prefixes/suffixes */
  file_proc = file_proc_find_by_name (all_procs, uri, TRUE);

  if (file_proc)
    return file_proc;

  filename = file_utils_filename_from_uri (uri);

  /* Then look for magics */
  if (filename)
    {
      GimpPlugInProcedure *size_matched_proc = NULL;
      FILE                *ifp               = NULL;
      gint                 head_size         = -2;
      gint                 size_match_count  = 0;
      FileMatchType        match_val;
      guchar               head[256];

      while (procs)
        {
          file_proc = procs->data;
          procs = procs->next;

          if (file_proc->magics_list)
            {
              if (head_size == -2)
                {
                  head_size = 0;

                  if ((ifp = g_fopen (filename, "rb")) != NULL)
                    {
                      head_size = fread ((gchar *) head, 1, sizeof (head), ifp);
                    }
                  else
                    {
                      g_set_error (error,
                                   G_FILE_ERROR,
                                   g_file_error_from_errno (errno),
                                   g_strerror (errno));
                    }
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
  file_proc = file_proc_find_by_name (all_procs, uri, FALSE);

  if (file_proc)
    {
      /* we found a procedure, clear error that might have been set */
      g_clear_error (error);
    }
  else
    {
      /* set an error message unless one was already set */
      if (error && *error == NULL)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Unknown file type"));
    }

  return file_proc;
}

GimpPlugInProcedure *
file_utils_find_proc_by_extension (GSList      *procs,
                                   const gchar *uri)
{
  g_return_val_if_fail (procs != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  return file_proc_find_by_extension (procs, uri, FALSE);
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

static gchar *
file_utils_uri_to_utf8_basename (const gchar *uri)
{
  gchar *filename;
  gchar *basename = NULL;

  g_return_val_if_fail (uri != NULL, NULL);

  filename = file_utils_uri_to_utf8_filename (uri);

  if (strstr (filename, G_DIR_SEPARATOR_S))
    {
      basename = g_path_get_basename (filename);
    }
  else if (strstr (filename, "://"))
    {
      basename = strrchr (uri, '/');

      if (basename)
        basename = g_strdup (basename + 1);
    }

  if (basename)
    {
      g_free (filename);
      return basename;
    }

  return filename;
}

gchar *
file_utils_uri_display_basename (const gchar *uri)
{
  gchar *basename = NULL;

  g_return_val_if_fail (uri != NULL, NULL);

  if (g_str_has_prefix (uri, "file:"))
    {
      gchar *filename = file_utils_filename_from_uri (uri);

      if (filename)
        {
          basename = g_filename_display_basename (filename);
          g_free (filename);
        }
    }
  else
    {
      gchar *name = file_utils_uri_display_name (uri);

      basename = strrchr (name, '/');
      if (basename)
        basename = g_strdup (basename + 1);

      g_free (name);
    }

  return basename ? basename : file_utils_uri_to_utf8_basename (uri);
}

gchar *
file_utils_uri_display_name (const gchar *uri)
{
  gchar *name = NULL;

  g_return_val_if_fail (uri != NULL, NULL);

  if (g_str_has_prefix (uri, "file:"))
    {
      gchar *filename = file_utils_filename_from_uri (uri);

      if (filename)
        {
          name = g_filename_display_name (filename);
          g_free (filename);
        }
    }
  else
    {
      name = file_utils_unescape_uri (uri, -1, "/", FALSE);
    }

  return name ? name : g_strdup (uri);
}

GdkPixbuf *
file_utils_load_thumbnail (const gchar *filename)
{
  GimpThumbnail *thumbnail = NULL;
  GdkPixbuf     *pixbuf    = NULL;
  gchar         *uri;

  g_return_val_if_fail (filename != NULL, NULL);

  uri = g_filename_to_uri (filename, NULL, NULL);

  if (uri)
    {
      thumbnail = gimp_thumbnail_new ();
      gimp_thumbnail_set_uri (thumbnail, uri);

      pixbuf = gimp_thumbnail_load_thumb (thumbnail,
                                          GIMP_THUMBNAIL_SIZE_NORMAL,
                                          NULL);
    }

  g_free (uri);

  if (pixbuf)
    {
      gint width  = gdk_pixbuf_get_width (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

      if (gdk_pixbuf_get_n_channels (pixbuf) != 3)
        {
          GdkPixbuf *tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                                           width, height);

          gdk_pixbuf_composite_color (pixbuf, tmp,
                                      0, 0, width, height, 0, 0, 1.0, 1.0,
                                      GDK_INTERP_NEAREST, 255,
                                      0, 0, GIMP_CHECK_SIZE_SM,
                                      0x66666666, 0x99999999);

          g_object_unref (pixbuf);
          pixbuf = tmp;
        }
    }

  return pixbuf;
}

gboolean
file_utils_save_thumbnail (GimpImage   *image,
                           const gchar *filename)
{
  const gchar *image_uri;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  image_uri = gimp_object_get_name (GIMP_OBJECT (image));

  if (image_uri)
    {
      gchar *uri = g_filename_to_uri (filename, NULL, NULL);

      if (uri)
        {
          if ( ! strcmp (uri, image_uri))
            {
              GimpImagefile *imagefile;

              imagefile = gimp_imagefile_new (image->gimp, uri);
              success = gimp_imagefile_save_thumbnail (imagefile, NULL,
                                                       image);
              g_object_unref (imagefile);
            }

          g_free (uri);
        }
    }

  return success;
}


/*  private functions  */

static GimpPlugInProcedure *
file_proc_find_by_prefix (GSList      *procs,
                          const gchar *uri,
                          gboolean     skip_magic)
{
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
            return proc;
        }
     }

  return NULL;
}

static GimpPlugInProcedure *
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
      GimpPlugInProcedure *proc = p->data;
      GSList              *extensions;

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

static GimpPlugInProcedure *
file_proc_find_by_name (GSList      *procs,
                        const gchar *uri,
                        gboolean     skip_magic)
{
  GimpPlugInProcedure *proc;

  proc = file_proc_find_by_prefix (procs, uri, skip_magic);

  if (! proc)
    proc = file_proc_find_by_extension (procs, uri, skip_magic);

  return proc;
}


/* the following two functions are copied from glib/gconvert.c */

static gint
unescape_character (const gchar *scanner)
{
  gint first_digit;
  gint second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
file_utils_unescape_uri (const gchar *escaped,
                         gint         len,
                         const gchar *illegal_escaped_characters,
                         gboolean     ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  gint c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
	{
	  /* catch partial escape sequences past the end of the substring */
	  if (in + 3 > in_end)
	    break;

	  c = unescape_character (in + 1);

	  /* catch bad escape sequences and NUL characters */
	  if (c <= 0)
	    break;

	  /* catch escaped ASCII */
	  if (ascii_must_not_be_escaped && c <= 0x7F)
	    break;

	  /* catch other illegal escaped characters */
	  if (strchr (illegal_escaped_characters, c) != NULL)
	    break;

	  in += 2;
	}

      *out++ = c;
    }

  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end)
    {
      g_free (result);
      return NULL;
    }

  return result;
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
                         FILE         *ifp)

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

  if (offs < 0)
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
          struct stat buf;

          if (fstat (fileno (ifp), &buf) < 0)
            return FILE_MATCH_NONE;

          fileval = buf.st_size;
        }
      else if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (glong) file_head[offs+k];
        }
      else   /* Read it from file */
        {
          gint c = 0;

          if (fseek (ifp, offs, SEEK_SET) < 0)
            return FILE_MATCH_NONE;

          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (c = getc (ifp));

          if (c == EOF)
            return FILE_MATCH_NONE;
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

      if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          found = (memcmp (mem_testval, file_head + offs, numbytes) == 0);
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0)
            return FILE_MATCH_NONE;

          found = FILE_MATCH_MAGIC;

          for (k = 0; found && (k < numbytes); k++)
            {
              gint c = getc (ifp);

              found = (c != EOF) && (c == (gint) mem_testval[k]);
            }
        }
    }

  return found;
}

static FileMatchType
file_check_magic_list (GSList       *magics_list,
                       const guchar *head,
                       gint          headsize,
                       FILE         *ifp)

{
  const gchar   *offset;
  const gchar   *type;
  const gchar   *value;
  gboolean       and   = FALSE;
  FileMatchType  found = FILE_MATCH_NONE;
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
                                           ifp);
      if (and)
        found = found && match_val;
      else
        found = match_val;

      and = (strchr (offset, '&') != NULL);

      if ((! and) && found)
        return match_val;
    }

  return FILE_MATCH_NONE;
}
