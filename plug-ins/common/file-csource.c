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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


typedef struct
{
  gchar    *prefixed_name;
  gchar    *comment;
  gboolean  use_comment;
  gboolean  glib_types;
  gboolean  alpha;
  gboolean  rgb565;
  gboolean  use_macros;
  gboolean  use_rle;
  gdouble   opacity;
} Config;


static void     query           (void);
static void     run             (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static gboolean save_image      (GFile            *file,
                                 Config           *config,
                                 gint32            image_ID,
                                 gint32            drawable_ID,
                                 GError          **error);
static gboolean run_save_dialog (Config           *config);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static Config config =
{
  "gimp_image", /* prefixed_name */
  NULL,         /* comment */
  FALSE,        /* use_comment */
  TRUE,         /* glib_types */
  FALSE,        /* alpha */
  FALSE,        /* rgb565 */
  FALSE,        /* use_macros */
  FALSE,        /* use_rle */
  100.0,        /* opacity */
};


MAIN ()


static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "Dump image data in RGB(A) format for C source",
                          "CSource cannot be run non-interactively.",
                          "Tim Janik",
                          "Tim Janik",
                          "1999",
                          N_("C source code"),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "text/x-csrc");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "c", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      strcmp (name, SAVE_PROC) == 0)
    {
      gint32         image_ID    = param[1].data.d_int32;
      gint32         drawable_ID = param[2].data.d_int32;
      GimpParasite  *parasite;
      gchar         *x;

      gimp_get_data (SAVE_PROC, &config);

      config.prefixed_name = "gimp_image";
      config.comment       = NULL;
      config.alpha         = gimp_drawable_has_alpha (drawable_ID);

      parasite = gimp_image_get_parasite (image_ID, "gimp-comment");
      if (parasite)
        {
          config.comment = g_strndup (gimp_parasite_data (parasite),
                                      gimp_parasite_data_size (parasite));
          gimp_parasite_free (parasite);
        }
      x = config.comment;

      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image_ID, &drawable_ID, "C Source",
                                  GIMP_EXPORT_CAN_HANDLE_RGB |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        {
          values[0].data.d_status = GIMP_PDB_CANCEL;
          return;
        }

      if (run_save_dialog (&config))
        {
          if (x != config.comment &&
              !(x && config.comment && strcmp (x, config.comment) == 0))
            {
              if (!config.comment || !config.comment[0])
                {
                  gimp_image_detach_parasite (image_ID, "gimp-comment");
                }
              else
                {
                  parasite = gimp_parasite_new ("gimp-comment",
                                                GIMP_PARASITE_PERSISTENT,
                                                strlen (config.comment) + 1,
                                                config.comment);
                  gimp_image_attach_parasite (image_ID, parasite);
                  gimp_parasite_free (parasite);
                }
            }

          if (! save_image (g_file_new_for_uri (param[3].data.d_string),
                            &config, image_ID, drawable_ID, &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;

              if (error)
                {
                  *nreturn_vals = 2;
                  values[1].type          = GIMP_PDB_STRING;
                  values[1].data.d_string = error->message;
                }
            }
          else
            {
              gimp_set_data (SAVE_PROC, &config, sizeof (config));
            }
        }
      else
        {
          status = GIMP_PDB_CANCEL;
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
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
                guint   n_ch)
{
  gboolean (*diff2_pix) (guint8 *) = n_ch > 3 ? diff2_rgba : diff2_rgb;
  guint8 *ilimit = limit - n_ch;

  while (ip < limit)
    {
      g_assert (ip < ilimit); /* paranoid */

      if (diff2_pix (ip))
        {
          guint8 *s_ip = ip;
          guint l = 1;

          ip += n_ch;
          while (l < 127 && ip < ilimit && diff2_pix (ip))
            { ip += n_ch; l += 1; }
          if (ip == ilimit && l < 127)
            { ip += n_ch; l += 1; }
          *(bp++) = l;
          memcpy (bp, s_ip, l * n_ch);
          bp += l * n_ch;
        }
      else
        {
          guint l = 2;

          ip += n_ch;
          while (l < 127 && ip < ilimit && !diff2_pix (ip))
            { ip += n_ch; l += 1; }
          *(bp++) = l | 128;
          memcpy (bp, ip, n_ch);
          ip += n_ch;
          bp += n_ch;
        }
      if (ip == ilimit)
        {
          *(bp++) = 1;
          memcpy (bp, ip, n_ch);
          ip += n_ch;
          bp += n_ch;
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
                  guint           n_ch,
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
           "  } else { /* RGB */ \\\n"
           "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
           s_uint) &&
    print (output, error,
           "      if (__l & 128) { __l = __l - 128; \\\n"
           "        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \\\n"
           "      } else { __l *= 3; memcpy (__ip, __rd, __l); \\\n"
           "               __ip += __l; __rd += __l; } } \\\n"
           "  } } while (0)\n");
}

static inline gboolean
save_uchar (GOutputStream  *output,
            guint          *c,
            guint8          d,
            Config         *config,
            GError        **error)
{
  static guint8 pad = 0;

  if (*c > 74)
    {
      if (! config->use_macros)
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
save_image (GFile   *file,
            Config  *config,
            gint32   image_ID,
            gint32   drawable_ID,
            GError **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  GimpImageType  drawable_type = gimp_drawable_type (drawable_ID);
  gchar         *s_uint_8, *s_uint, *s_char, *s_null;
  guint          c;
  gchar         *macro_name;
  guint8        *img_buffer, *img_buffer_end;
  gchar         *basename;
  guint8        *data, *p;
  gint           width;
  gint           height;
  gint           x, y, pad, n_bytes, bpp;
  gint           drawable_bpp;

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

  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  drawable_bpp = gimp_drawable_bpp (drawable_ID);

  bpp = config->rgb565 ? 2 : (config->alpha ? 4 : 3);
  n_bytes = width * height * bpp;
  pad = width * bpp;
  if (config->use_rle)
    pad = MAX (pad, 130 + n_bytes / 127);

  data = g_new (guint8, pad + n_bytes);
  p = data + pad;

  for (y = 0; y < height; y++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                       NULL, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (bpp == 2)
        {
          for (x = 0; x < width; x++)
            {
              guint8 *d = data + x * drawable_bpp;
              guint8 r, g, b;
              gushort rgb16;
              gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

              alpha *= config->opacity / 25500.0;
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
      else if (config->alpha)
        {
          for (x = 0; x < width; x++)
            {
              guint8 *d = data + x * drawable_bpp;
              gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

              alpha *= config->opacity / 100.0;
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

              alpha *= config->opacity / 25500.0;
              *(p++) = 0.5 + alpha * (gdouble) d[0];
              *(p++) = 0.5 + alpha * (gdouble) d[1];
              *(p++) = 0.5 + alpha * (gdouble) d[2];
            }
        }
    }

  img_buffer = data + pad;
  if (config->use_rle)
    {
      img_buffer_end = rl_encode_rgbx (data, img_buffer,
                                       img_buffer + n_bytes, bpp);
      img_buffer = data;
    }
  else
    {
      img_buffer_end = img_buffer + n_bytes;
    }

  if (!config->use_macros && config->glib_types)
    {
      s_uint_8 =  "guint8 ";
      s_uint  =   "guint  ";
      s_char =    "gchar  ";
      s_null =    "NULL";
    }
  else if (!config->use_macros)
    {
      s_uint_8 =  "unsigned char";
      s_uint =    "unsigned int ";
      s_char =    "char         ";
      s_null =    "(char*) 0";
    }
  else if (config->use_macros && config->glib_types)
    {
      s_uint_8 =  "guint8";
      s_uint  =   "guint";
      s_char =    "gchar";
      s_null =    "NULL";
    }
  else /* config->use_macros && !config->glib_types */
    {
      s_uint_8 =  "unsigned char";
      s_uint =    "unsigned int";
      s_char =    "char";
      s_null =    "(char*) 0";
    }

  macro_name = g_ascii_strup (config->prefixed_name, -1);

  basename = g_file_get_basename (file);

  if (! print (output, error,
               "/* GIMP %s C-Source image dump %s(%s) */\n\n",
               config->alpha ? "RGBA" : "RGB",
               config->use_rle ? "1-byte-run-length-encoded " : "",
               basename))
    goto fail;

  g_free (basename);

  if (config->use_rle && !config->use_macros)
    {
      if (! save_rle_decoder (output,
                              macro_name,
                              config->glib_types ? "guint" : "unsigned int",
                              config->glib_types ? "guint8" : "unsigned char",
                              config->alpha ? 4 : 3,
                              error))
        goto fail;
    }

  if (!config->use_macros)
    {
      if (! print (output, error,
                   "static const struct {\n"
                   "  %s\t width;\n"
                   "  %s\t height;\n"
                   "  %s\t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ \n",
                   s_uint, s_uint, s_uint))
        goto fail;

      if (config->use_comment)
        {
          if (! print (output, error, "  %s\t*comment;\n", s_char))
            goto fail;
        }

      if (! print (output, error,
                   "  %s\t %spixel_data[",
                   s_uint_8,
                   config->use_rle ? "rle_" : ""))
        goto fail;

      if (config->use_rle)
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
                       config->rgb565 ? 2 : (config->alpha ? 4 : 3)))
            goto fail;
        }

      if (! print (output, error, "} %s = {\n", config->prefixed_name))
        goto fail;

      if (! print (output, error,
                   "  %u, %u, %u,\n",
                   width,
                   height,
                   config->rgb565 ? 2 : (config->alpha ? 4 : 3)))
        goto fail;
    }
  else /* use macros */
    {
      if (! print (output, error,
                   "#define %s_WIDTH (%u)\n"
                   "#define %s_HEIGHT (%u)\n"
                   "#define %s_BYTES_PER_PIXEL (%u) /* 3:RGB, 4:RGBA */\n",
                   macro_name, width,
                   macro_name, height,
                   macro_name, config->alpha ? 4 : 3))
        {
          goto fail;
        }
    }

  if (config->use_comment && !config->comment)
    {
      if (! config->use_macros)
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
  else if (config->use_comment)
    {
      gchar *p = config->comment - 1;

      if (config->use_macros)
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
                             config->use_macros ? " \\" : "");
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

      if (! config->use_macros)
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

  if (config->use_macros)
    {
      if (! print (output, error,
                   "#define %s_%sPIXEL_DATA ((%s*) %s_%spixel_data)\n",
                   macro_name,
                   config->use_rle ? "RLE_" : "",
                   s_uint_8,
                   macro_name,
                   config->use_rle ? "rle_" : ""))
        goto fail;

      if (config->use_rle)
        {
          if (! save_rle_decoder (output,
                                  macro_name,
                                  s_uint,
                                  s_uint_8,
                                  config->alpha ? 4 : 3,
                                  error))
            goto fail;
        }

      if (! print (output, error,
                   "static const %s %s_%spixel_data[",
                   s_uint_8,
                   macro_name,
                   config->use_rle ? "rle_" : ""))
        goto fail;

      if (config->use_rle)
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
                       config->alpha ? 4 : 3))
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
          if (! save_uchar (output, &c, *(img_buffer++), config, error))
            goto fail;
        }
      while (img_buffer < img_buffer_end);
      break;

    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      goto fail;
    }

  if (! config->use_macros)
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

  g_object_unref (output);
  g_object_unref (buffer);

  return FALSE;
}

static void
rgb565_toggle_button_update (GtkWidget *toggle,
                             gpointer   data)
{
  GtkWidget *widget;
  gboolean   active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));

  gimp_toggle_button_update (toggle, data);

  widget = g_object_get_data (G_OBJECT (toggle), "set-insensitive-1");
  if (widget)
    gtk_widget_set_sensitive (widget, ! active);

  widget = g_object_get_data (G_OBJECT (toggle), "set-insensitive-2");
  if (widget)
    gtk_widget_set_sensitive (widget, ! active);
}

static gboolean
run_save_dialog (Config *config)
{
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkWidget     *table;
  GtkWidget     *prefixed_name;
  GtkWidget     *centry;
  GtkWidget     *toggle;
  GtkWidget     *rle_toggle;
  GtkWidget     *alpha_toggle;
  GtkAdjustment *adj;
  gboolean       run;

  dialog = gimp_export_dialog_new (_("C-Source"), PLUG_IN_BINARY, SAVE_PROC);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Prefixed Name
   */
  prefixed_name = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Prefixed name:"), 0.0, 0.5,
                             prefixed_name, 1, FALSE);
  gtk_entry_set_text (GTK_ENTRY (prefixed_name),
                      config->prefixed_name ? config->prefixed_name : "");

  /* Comment Entry
   */
  centry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Co_mment:"), 0.0, 0.5,
                             centry, 1, FALSE);
  gtk_entry_set_text (GTK_ENTRY (centry),
                      config->comment ? config->comment : "");

  /* Use Comment
   */
  toggle = gtk_check_button_new_with_mnemonic (_("_Save comment to file"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->use_comment);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->use_comment);

  /* GLib types
   */
  toggle = gtk_check_button_new_with_mnemonic (_("_Use GLib types (guint8*)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->glib_types);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->glib_types);

  /* Use Macros
   */
  toggle =
    gtk_check_button_new_with_mnemonic (_("Us_e macros instead of struct"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->use_macros);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->use_macros);

  /* Use RLE
   */
  rle_toggle = toggle =
    gtk_check_button_new_with_mnemonic (_("Use _1 byte Run-Length-Encoding"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->use_rle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->use_rle);

  /* Alpha
   */
  alpha_toggle = toggle =
    gtk_check_button_new_with_mnemonic (_("Sa_ve alpha channel (RGBA/RGB)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->alpha);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->alpha);

  /* RGB-565
   */
  toggle = gtk_check_button_new_with_mnemonic (_("Save as _RGB565 (16-bit)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  /* RLE and alpha settings are not used with RGB-565 */
  g_object_set_data (G_OBJECT (toggle), "set-insensitive-1", rle_toggle);
  g_object_set_data (G_OBJECT (toggle), "set-insensitive-2", alpha_toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (rgb565_toggle_button_update),
                    &config->rgb565);
  gtk_widget_show (toggle);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                config->rgb565);

  /* Max Alpha Value
   */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Op_acity:"), 100, 0,
                              config->opacity, 0, 100, 1, 10, 1,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config->opacity);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      config->prefixed_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (prefixed_name)));
      config->comment = g_strdup (gtk_entry_get_text (GTK_ENTRY (centry)));
    }

  gtk_widget_destroy (dialog);

  if (!config->prefixed_name || !config->prefixed_name[0])
    config->prefixed_name = "tmp";

  if (config->comment && !config->comment[0])
    config->comment = NULL;

  return run;
}
