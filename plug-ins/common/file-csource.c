/* CSource - GIMP Plugin to dump image data in RGB(A) format for C source
 * Copyright (C) 1999 Tim Janik
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This plugin is heavily based on the header plugin by Spencer Kimball and
 * Peter Mattis.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-csource-save"
#define PLUG_IN_BINARY "file-csource"
#define PLUG_IN_ROLE   "gimp-file-csource"


typedef struct _Csource      Csource;
typedef struct _CsourceClass CsourceClass;

struct _Csource
{
  GimpPlugIn      parent_instance;
};

struct _CsourceClass
{
  GimpPlugInClass parent_class;
};


#define CSOURCE_TYPE  (csource_get_type ())
#define CSOURCE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CSOURCE_TYPE, Csource))

GType                   csource_get_type         (void) G_GNUC_CONST;

static GList          * csource_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * csource_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * csource_save             (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  gint                  n_drawables,
                                                  GimpDrawable        **drawables,
                                                  GFile                *file,
                                                  const GimpValueArray *args,
                                                  gpointer              run_data);

static gboolean         save_image               (GFile                *file,
                                                  GimpImage            *image,
                                                  GimpDrawable         *drawable,
                                                  GObject              *config,
                                                  GError              **error);
static gboolean         save_dialog              (GimpProcedure        *procedure,
                                                  GObject              *config);


G_DEFINE_TYPE (Csource, csource, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (CSOURCE_TYPE)


static void
csource_class_init (CsourceClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = csource_query_procedures;
  plug_in_class->create_procedure = csource_create_procedure;
}

static void
csource_init (Csource *csource)
{
}

static GList *
csource_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (SAVE_PROC));
}

static GimpProcedure *
csource_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           csource_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("C source code"));

      gimp_procedure_set_documentation (procedure,
                                        "Dump image data in RGB(A) format "
                                        "for C source",
                                        "CSource cannot be run non-interactively.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tim Janik",
                                      "Tim Janik",
                                      "1999");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-csrc");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "c");

      GIMP_PROC_AUX_ARG_STRING (procedure, "prefixed-name",
                                "Prefixed name",
                                "Prefixed name",
                                "gimp_image",
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_STRING (procedure, "comment",
                                "Comment",
                                "Comment",
                                gimp_get_default_comment (),
                                GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-comment",
                                 "Save comment",
                                 "Save comment",
                                 gimp_export_comment (),
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "glib-types",
                                 "GLib types",
                                 "Use GLib types",
                                 TRUE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-alpha",
                                 "Save alpha",
                                 "Save the alpha channel",
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "rgb565",
                                 "RGB565",
                                 "Use RGB565 encoding",
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-macros",
                                 "Use macros",
                                 "Use C macros",
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "use-rle",
                                 "Use RLE",
                                 "Use run-lenght-encoding",
                                 FALSE,
                                 GIMP_PARAM_READWRITE);

      GIMP_PROC_AUX_ARG_DOUBLE (procedure, "opacity",
                                "Opacity",
                                "Opacity",
                                0.0, 100.0, 100.0,
                                GIMP_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
csource_save (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              gint                  n_drawables,
              GimpDrawable        **drawables,
              GFile                *file,
              const GimpValueArray *args,
              gpointer              run_data)
{
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GimpExportReturn     export = GIMP_EXPORT_CANCEL;
  gchar               *prefixed_name;
  gchar               *comment;
  GError              *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_INTERACTIVE)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_export (config, image, run_mode, args, NULL);

  gimp_ui_init (PLUG_IN_BINARY);

  export = gimp_export_image (&image, &n_drawables, &drawables, "C Source",
                              GIMP_EXPORT_CAN_HANDLE_RGB |
                              GIMP_EXPORT_CAN_HANDLE_ALPHA);

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("C source does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  g_object_set (config,
                "save-alpha", gimp_drawable_has_alpha (drawables[0]),
                NULL);

  if (export == GIMP_EXPORT_CANCEL)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  if (! save_dialog (procedure, G_OBJECT (config)))
    status = GIMP_PDB_CANCEL;

  g_object_get (config,
                "prefixed-name", &prefixed_name,
                "comment",       &comment,
                NULL);

  if (! prefixed_name || ! prefixed_name[0])
    g_object_set (config,
                  "prefixed-name", "tmp",
                  NULL);

  if (comment && ! comment[0])
    g_object_set (config,
                  "comment", NULL,
                  NULL);

  g_free (prefixed_name);
  g_free (comment);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], G_OBJECT (config),
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  gimp_procedure_config_end_export (config, image, file, status);
  g_object_unref (config);

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
diff2_rgb565 (guint8 *ip)
{
  return ip[0] != ip[2] || ip[1] != ip[3];
}

static gboolean
diff2_rgb (guint8 *ip)
{
  return ip[0] != ip[3] || ip[1] != ip[4] || ip[2] != ip[5];
}

static gboolean
diff2_rgba (guint8 *ip)
{
  return ip[0] != ip[4] || ip[1] != ip[5] || ip[2] != ip[6] || ip[3] != ip[7];
}

static guint8 *
rl_encode_rgbx (guint8 *bp,
                guint8 *ip,
                guint8 *limit,
                guint   bpp)
{
  gboolean (*diff2_pix) (guint8 *);
  guint8 *ilimit = limit - bpp;

  switch (bpp)
    {
    case 2: diff2_pix = diff2_rgb565; break;
    case 3: diff2_pix = diff2_rgb; break;
    case 4: diff2_pix = diff2_rgba; break;
    default: g_assert_not_reached ();
    }

  while (ip < limit)
    {
      g_assert (ip < ilimit); /* paranoid */

      if (diff2_pix (ip))
        {
          guint8 *s_ip = ip;
          guint l = 1;

          ip += bpp;
          while (l < 127 && ip < ilimit && diff2_pix (ip))
            { ip += bpp; l += 1; }
          if (ip == ilimit && l < 127)
            { ip += bpp; l += 1; }
          *(bp++) = l;
          memcpy (bp, s_ip, l * bpp);
          bp += l * bpp;
        }
      else
        {
          guint l = 2;

          ip += bpp;
          while (l < 127 && ip < ilimit && !diff2_pix (ip))
            { ip += bpp; l += 1; }
          *(bp++) = l | 128;
          memcpy (bp, ip, bpp);
          ip += bpp;
          bp += bpp;
        }
      if (ip == ilimit)
        {
          *(bp++) = 1;
          memcpy (bp, ip, bpp);
          ip += bpp;
          bp += bpp;
        }
    }

  return bp;
}

static gboolean print (GOutputStream  *stream,
                       GError        **error,
                       const gchar    *format,
                       ...) G_GNUC_PRINTF (3, 4);

static gboolean
print (GOutputStream  *stream,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (stream, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}

static inline gboolean
save_rle_decoder (GOutputStream  *output,
                  const gchar    *macro_name,
                  const gchar    *s_uint,
                  const gchar    *s_uint_8,
                  guint           bpp,
                  GError        **error)
{
  return
    print (output, error,
           "#define %s_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \\\n",
           macro_name) &&
    print (output, error,
           "{ %s __bpp; %s *__ip; const %s *__il, *__rd; \\\n",
           s_uint, s_uint_8, s_uint_8) &&
    print (output, error,
           "  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \\\n"
           "  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \\\n"
           "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
           s_uint) &&
    print (output, error,
           "      if (__l & 128) { __l = __l - 128; \\\n"
           "        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \\\n"
           "      } else { __l *= 4; memcpy (__ip, __rd, __l); \\\n"
           "               __ip += __l; __rd += __l; } } \\\n"
           "  } else if (__bpp == 3) { /* RGB */ \\\n"
           "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
           s_uint) &&
    print (output, error,
           "      if (__l & 128) { __l = __l - 128; \\\n"
           "        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \\\n"
           "      } else { __l *= 3; memcpy (__ip, __rd, __l); \\\n"
           "               __ip += __l; __rd += __l; } } \\\n"
           "  } else { /* RGB16 */ \\\n"
           "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
           s_uint) &&
    print (output, error,
           "      if (__l & 128) { __l = __l - 128; \\\n"
           "        do { memcpy (__ip, __rd, 2); __ip += 2; } while (--__l); __rd += 2; \\\n"
           "      } else { __l *= 2; memcpy (__ip, __rd, __l); \\\n"
           "               __ip += __l; __rd += __l; } } \\\n"
           "  } } while (0)\n");
}

static inline gboolean
save_uchar (GOutputStream  *output,
            guint          *c,
            guint8          d,
            gboolean        use_macros,
            GError        **error)
{
  static guint8 pad = 0;

  if (*c > 74)
    {
      if (! use_macros)
        {
          if (! print (output, error, "\"\n  \""))
            return FALSE;

          *c = 3;
        }
      else
        {
          if (! print (output, error, "\"\n \""))
            return FALSE;

          *c = 2;
        }
    }

  if (d < 33 || (d >= 48 && d <= 57)  || d > 126)
    {
      if (! print (output, error, "\\%03o", d))
        return FALSE;

      *c += 1 + 1 + (d > 7) + (d > 63);
      pad = d < 64;

      return TRUE;
    }

  if (d == '\\')
    {
      if (! print (output, error, "\\\\"))
        return FALSE;

      *c += 2;
    }
  else if (d == '"')
    {
      if (! print (output, error, "\\\""))
        return FALSE;

      *c += 2;
    }
  else if (pad && d >= '0' && d <= '9')
    {
      if (! print (output, error, "\"\"%c", d))
        return FALSE;

      *c += 3;
    }
  else
    {
      if (! print (output, error, "%c", d))
        return FALSE;

      *c += 1;
    }

  pad = 0;

  return TRUE;
}

static gboolean
save_image (GFile         *file,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GObject       *config,
            GError       **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  GCancellable  *cancellable;
  GimpImageType  drawable_type = gimp_drawable_type (drawable);
  gchar         *s_uint_8, *s_uint, *s_char, *s_null;
  guint          c;
  gchar         *macro_name;
  guint8        *img_buffer, *img_buffer_end;
  gchar         *basename;
  guint8        *data, *p;
  gint           width;
  gint           height;
  gint           x, y, pad, n_bytes, bpp;
  const Babl    *drawable_format;
  gint           drawable_bpp;
  gchar         *config_prefixed_name;
  gchar         *config_comment;
  gboolean       config_save_comment;
  gboolean       config_glib_types;
  gboolean       config_save_alpha;
  gboolean       config_rgb565;
  gboolean       config_use_macros;
  gboolean       config_use_rle;
  gdouble        config_opacity;

  g_object_get (config,
                "prefixed-name", &config_prefixed_name,
                "comment",       &config_comment,
                "save-comment",  &config_save_comment,
                "glib-types",    &config_glib_types,
                "save-alpha",    &config_save_alpha,
                "rgb565",        &config_rgb565,
                "use-macros",    &config_use_macros,
                "use-rle",       &config_use_rle,
                "opacity",       &config_opacity,
                NULL);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  if (gimp_drawable_has_alpha (drawable))
    drawable_format = babl_format ("R'G'B'A u8");
  else
    drawable_format = babl_format ("R'G'B' u8");

  drawable_bpp = babl_format_get_bytes_per_pixel (drawable_format);

  bpp = config_rgb565 ? 2 : (config_save_alpha ? 4 : 3);
  n_bytes = width * height * bpp;
  pad = width * drawable_bpp;
  if (config_use_rle)
    pad = MAX (pad, 130 + n_bytes / 127);

  data = g_new (guint8, pad + n_bytes);
  p = data + pad;

  for (y = 0; y < height; y++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                       drawable_format, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (bpp == 2)
        {
          for (x = 0; x < width; x++)
            {
              guint8 *d = data + x * drawable_bpp;
              guint8 r, g, b;
              gushort rgb16;
              gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

              alpha *= config_opacity / 25500.0;
              r = (0.5 + alpha * (gdouble) d[0]);
              g = (0.5 + alpha * (gdouble) d[1]);
              b = (0.5 + alpha * (gdouble) d[2]);
              r >>= 3;
              g >>= 2;
              b >>= 3;
              rgb16 = (r << 11) + (g << 5) + b;
              *(p++) = (guchar) rgb16;
              *(p++) = (guchar) (rgb16 >> 8);
            }
        }
      else if (config_save_alpha)
        {
          for (x = 0; x < width; x++)
            {
              guint8 *d = data + x * drawable_bpp;
              gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

              alpha *= config_opacity / 100.0;
              *(p++) = d[0];
              *(p++) = d[1];
              *(p++) = d[2];
              *(p++) = alpha + 0.5;
            }
        }
      else
        {
          for (x = 0; x < width; x++)
            {
              guint8 *d = data + x * drawable_bpp;
              gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

              alpha *= config_opacity / 25500.0;
              *(p++) = 0.5 + alpha * (gdouble) d[0];
              *(p++) = 0.5 + alpha * (gdouble) d[1];
              *(p++) = 0.5 + alpha * (gdouble) d[2];
            }
        }
    }

  img_buffer = data + pad;
  if (config_use_rle)
    {
      img_buffer_end = rl_encode_rgbx (data, img_buffer,
                                       img_buffer + n_bytes, bpp);
      img_buffer = data;
    }
  else
    {
      img_buffer_end = img_buffer + n_bytes;
    }

  if (! config_use_macros && config_glib_types)
    {
      s_uint_8 =  "guint8 ";
      s_uint  =   "guint  ";
      s_char =    "gchar  ";
      s_null =    "NULL";
    }
  else if (! config_use_macros)
    {
      s_uint_8 =  "unsigned char";
      s_uint =    "unsigned int ";
      s_char =    "char         ";
      s_null =    "(char*) 0";
    }
  else if (config_use_macros && config_glib_types)
    {
      s_uint_8 =  "guint8";
      s_uint  =   "guint";
      s_char =    "gchar";
      s_null =    "NULL";
    }
  else /* config_use_macros && ! config_glib_types */
    {
      s_uint_8 =  "unsigned char";
      s_uint =    "unsigned int";
      s_char =    "char";
      s_null =    "(char*) 0";
    }

  macro_name = g_ascii_strup (config_prefixed_name, -1);

  basename = g_file_get_basename (file);

  if (! print (output, error,
               "/* GIMP %s C-Source image dump %s(%s) */\n\n",
               config_save_alpha ? "RGBA" : "RGB",
               config_use_rle ? "1-byte-run-length-encoded " : "",
               basename))
    goto fail;

  g_free (basename);

  if (config_use_rle && !config_use_macros)
    {
      if (! save_rle_decoder (output,
                              macro_name,
                              config_glib_types ? "guint" : "unsigned int",
                              config_glib_types ? "guint8" : "unsigned char",
                              bpp,
                              error))
        goto fail;
    }

  if (!config_use_macros)
    {
      if (! print (output, error,
                   "static const struct {\n"
                   "  %s\t width;\n"
                   "  %s\t height;\n"
                   "  %s\t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ \n",
                   s_uint, s_uint, s_uint))
        goto fail;

      if (config_save_comment)
        {
          if (! print (output, error, "  %s\t*comment;\n", s_char))
            goto fail;
        }

      if (! print (output, error,
                   "  %s\t %spixel_data[",
                   s_uint_8,
                   config_use_rle ? "rle_" : ""))
        goto fail;

      if (config_use_rle)
        {
          if (! print (output, error,
                       "%u + 1];\n",
                       (guint) (img_buffer_end - img_buffer)))
            goto fail;
        }
      else
        {
          if (! print (output, error,
                       "%u * %u * %u + 1];\n",
                       width,
                       height,
                       bpp))
            goto fail;
        }

      if (! print (output, error, "} %s = {\n", config_prefixed_name))
        goto fail;

      if (! print (output, error,
                   "  %u, %u, %u,\n",
                   width,
                   height,
                   bpp))
        goto fail;
    }
  else /* use macros */
    {
      if (! print (output, error,
                   "#define %s_WIDTH (%u)\n"
                   "#define %s_HEIGHT (%u)\n"
                   "#define %s_BYTES_PER_PIXEL (%u) /* 2:RGB16, 3:RGB, 4:RGBA */\n",
                   macro_name, width,
                   macro_name, height,
                   macro_name, bpp))
        {
          goto fail;
        }
    }

  if (config_save_comment && ! config_comment)
    {
      if (! config_use_macros)
        {
          if (! print (output, error, "  %s,\n", s_null))
            goto fail;
        }
      else
        {
          if (! print (output, error,
                       "#define %s_COMMENT (%s)\n",
                       macro_name, s_null))
            goto fail;
        }
    }
  else if (config_save_comment)
    {
      gchar *p = config_comment - 1;

      if (config_use_macros)
        {
          if (! print (output, error, "#define %s_COMMENT \\\n", macro_name))
            goto fail;
        }

      if (! print (output, error, "  \""))
        goto fail;

      while (*(++p))
        {
          gboolean success = FALSE;

          if (*p == '\\')
            success = print (output, error, "\\\\");
          else if (*p == '"')
            success = print (output, error, "\\\"");
          else if (*p == '\n' && p[1])
            success = print (output, error,
                             "\\n\"%s\n  \"",
                             config_use_macros ? " \\" : "");
          else if (*p == '\n')
            success = print (output, error, "\\n");
          else if (*p == '\r')
            success = print (output, error, "\\r");
          else if (*p == '\b')
            success = print (output, error, "\\b");
          else if (*p == '\f')
            success = print (output, error, "\\f");
          else if (( *p >= 32 && *p <= 47 ) || (*p >= 58 && *p <= 126))
            success = print (output, error, "%c", *p);
          else
            success = print (output, error, "\\%03o", *p);

          if (! success)
            goto fail;
        }

      if (! config_use_macros)
        {
          if (! print (output, error, "\",\n"))
            goto fail;
        }
      else
        {
          if (! print (output, error, "\"\n"))
            goto fail;
        }
    }

  if (config_use_macros)
    {
      if (! print (output, error,
                   "#define %s_%sPIXEL_DATA ((%s*) %s_%spixel_data)\n",
                   macro_name,
                   config_use_rle ? "RLE_" : "",
                   s_uint_8,
                   macro_name,
                   config_use_rle ? "rle_" : ""))
        goto fail;

      if (config_use_rle)
        {
          if (! save_rle_decoder (output,
                                  macro_name,
                                  s_uint,
                                  s_uint_8,
                                  bpp,
                                  error))
            goto fail;
        }

      if (! print (output, error,
                   "static const %s %s_%spixel_data[",
                   s_uint_8,
                   macro_name,
                   config_use_rle ? "rle_" : ""))
        goto fail;

      if (config_use_rle)
        {
          if (! print (output, error,
                       "%u] =\n",
                       (guint) (img_buffer_end - img_buffer)))
            goto fail;
        }
      else
        {
          if (! print (output, error,
                       "%u * %u * %u + 1] =\n",
                       width,
                       height,
                       bpp))
            goto fail;
        }

      if (! print (output, error, "(\""))
        goto fail;

      c = 2;
    }
  else
    {
      if (! print (output, error, "  \""))
        goto fail;

      c = 3;
    }

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      do
        {
          if (! save_uchar (output, &c, *(img_buffer++), config_use_macros,
                            error))
            goto fail;
        }
      while (img_buffer < img_buffer_end);
      break;

    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      goto fail;
    }

  if (! config_use_macros)
    {
      if (! print (output, error, "\",\n};\n\n"))
        goto fail;
    }
  else
    {
      if (! print (output, error, "\");\n\n"))
        goto fail;
    }

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_object_unref (output);
  g_object_unref (buffer);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);
  g_object_unref (cancellable);

  g_object_unref (output);
  g_object_unref (buffer);

  return FALSE;
}

static gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *entry;
  GtkWidget *toggle;
  gboolean   run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Export Image as C-Source"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Prefixed Name
   */
  entry = gimp_prop_entry_new (config, "prefixed-name", -1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Prefixed name:"), 0.0, 0.5,
                            entry, 1);

  /* Comment Entry
   */
  entry = gimp_prop_entry_new (config, "comment", -1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Co_mment:"), 0.0, 0.5,
                            entry, 1);

  /* Use Comment
   */
  toggle = gimp_prop_check_button_new (config, "save-comment",
                                       _("_Save comment to file"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* GLib types
   */
  toggle = gimp_prop_check_button_new (config, "glib-types",
                                       _("_Use GLib types (guint8*)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* Use Macros
   */
  toggle = gimp_prop_check_button_new (config, "use-macros",
                                       _("Us_e macros instead of struct"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* Use RLE
   */
  toggle = gimp_prop_check_button_new (config, "use-rle",
                                       _("Use _1 byte Run-Length-Encoding"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* Alpha
   */
  toggle = gimp_prop_check_button_new (config, "save-alpha",
                                       _("Sa_ve alpha channel (RGBA/RGB)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  g_object_bind_property (config, "rgb565",
                          toggle, "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  /* RGB-565
   */
  toggle = gimp_prop_check_button_new (config, "rgb565",
                                       _("Save as _RGB565 (16-bit)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* Max Alpha Value
   */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  gimp_prop_scale_entry_new (config, "opacity",
                             GTK_GRID (grid), 0, 0,
                             _("Op_acity:"),
                             1, 10, 1,
                             FALSE, 0, 0);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
