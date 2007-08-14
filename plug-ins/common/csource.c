/* CSource - GIMP Plugin to dump image data in RGB(A) format for C source
 * Copyright (C) 1999 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * This plugin is heavily based on the header plugin by Spencer Kimball and
 * Peter Mattis.
 */

#include "config.h"

#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-csource-save"
#define PLUG_IN_BINARY "csource"


typedef struct
{
  gchar    *file_name;
  gchar    *prefixed_name;
  gchar    *comment;
  gboolean  use_comment;
  gboolean  glib_types;
  gboolean  alpha;
  gboolean  use_macros;
  gboolean  use_rle;
  gdouble   opacity;
} Config;


/* --- prototypes --- */
static void	query		(void);
static void	run		(const gchar      *name,
				 gint              nparams,
				 const GimpParam  *param,
				 gint             *nreturn_vals,
				 GimpParam       **return_vals);

static gint	save_image	(Config           *config,
				 gint32            image_ID,
				 gint32            drawable_ID);
static gboolean	run_save_dialog	(Config           *config);


/* --- variables --- */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

Config config =
{
  NULL,         /* file_name */
  "gimp_image", /* prefixed_name */
  NULL,         /* comment */
  FALSE,        /* use_comment */
  TRUE,         /* glib_types */
  FALSE,        /* alpha */
  FALSE,        /* use_macros */
  FALSE,        /* use_rle */
  100.0,        /* opacity */
};

/* --- implement main (), provided by libgimp --- */
MAIN ()

/* --- functions --- */
static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive" },
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
  gimp_register_save_handler (SAVE_PROC, "c", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      strcmp (name, SAVE_PROC) == 0)
    {
      gint32         image_ID    = param[1].data.d_int32;
      gint32         drawable_ID = param[2].data.d_int32;
      GimpParasite  *parasite;
      gchar         *x;
      GimpImageType  drawable_type = gimp_drawable_type (drawable_ID);

      gimp_get_data (SAVE_PROC, &config);
      config.prefixed_name = "gimp_image";
      config.comment       = NULL;

      config.file_name = param[3].data.d_string;
      config.alpha = (drawable_type == GIMP_RGBA_IMAGE ||
		      drawable_type == GIMP_GRAYA_IMAGE ||
		      drawable_type == GIMP_INDEXEDA_IMAGE);

      parasite = gimp_image_parasite_find (image_ID, "gimp-comment");
      if (parasite)
	{
	  config.comment = g_strndup (gimp_parasite_data (parasite),
                                      gimp_parasite_data_size (parasite));
	  gimp_parasite_free (parasite);
	}
      x = config.comment;

      gimp_ui_init (PLUG_IN_BINARY, FALSE);
      export = gimp_export_image (&image_ID, &drawable_ID, "C Source",
				  (GIMP_EXPORT_CAN_HANDLE_RGB |
				   GIMP_EXPORT_CAN_HANDLE_ALPHA ));
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
		gimp_image_parasite_detach (image_ID, "gimp-comment");
	      else
		{
		  parasite = gimp_parasite_new ("gimp-comment",
						GIMP_PARASITE_PERSISTENT,
						strlen (config.comment) + 1,
						config.comment);
		  gimp_image_parasite_attach (image_ID, parasite);
		  gimp_parasite_free (parasite);
		}
	    }

	  if (! save_image (&config, image_ID, drawable_ID))
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
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

static inline void
save_rle_decoder (FILE        *fp,
		  const gchar *macro_name,
		  const gchar *s_uint,
		  const gchar *s_uint_8,
		  guint        n_ch)
{
  fprintf (fp, "#define %s_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \\\n",
	   macro_name);
  fprintf (fp, "{ %s __bpp; %s *__ip; const %s *__il, *__rd; \\\n", s_uint, s_uint_8, s_uint_8);
  fprintf (fp, "  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \\\n");

  fprintf (fp, "  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \\\n");

  fprintf (fp, "    while (__ip < __il) { %s __l = *(__rd++); \\\n", s_uint);
  fprintf (fp, "      if (__l & 128) { __l = __l - 128; \\\n");
  fprintf (fp, "        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \\\n");
  fprintf (fp, "      } else { __l *= 4; memcpy (__ip, __rd, __l); \\\n");
  fprintf (fp, "               __ip += __l; __rd += __l; } } \\\n");

  fprintf (fp, "  } else { /* RGB */ \\\n");

  fprintf (fp, "    while (__ip < __il) { %s __l = *(__rd++); \\\n", s_uint);
  fprintf (fp, "      if (__l & 128) { __l = __l - 128; \\\n");
  fprintf (fp, "        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \\\n");
  fprintf (fp, "      } else { __l *= 3; memcpy (__ip, __rd, __l); \\\n");
  fprintf (fp, "               __ip += __l; __rd += __l; } } \\\n");

  fprintf (fp, "  } } while (0)\n");
}

static inline guint
save_uchar (FILE   *fp,
	    guint   c,
	    guint8  d,
	    Config *config)
{
  static guint8 pad = 0;

  if (c > 74)
    {
      if (!config->use_macros)
	{
	  fprintf (fp, "\"\n  \"");
	  c = 3;
	}
      else
	{
	  fprintf (fp, "\"\n \"");
	  c = 2;
	}
    }
  if (d < 33 || d > 126)
    {
      fprintf (fp, "\\%o", d);
      c += 1 + 1 + (d > 7) + (d > 63);
      pad = d < 64;

      return c;
    }

  if (d == '\\')
    {
      fputs ("\\\\", fp);
      c += 2;
    }
  else if (d == '"')
    {
      fputs ("\\\"", fp);
      c += 2;
    }
  else if (pad && d >= '0' && d <= '9')
    {
      fputs ("\"\"", fp);
      fputc (d, fp);
      c += 3;
    }
  else
    {
      fputc (d, fp);
      c += 1;
    }
  pad = 0;

  return c;
}

static gint
save_image (Config *config,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GimpDrawable *drawable      = gimp_drawable_get (drawable_ID);
  GimpImageType drawable_type = gimp_drawable_type (drawable_ID);
  GimpPixelRgn pixel_rgn;
  gchar *s_uint_8, *s_uint_32, *s_uint, *s_char, *s_null;
  FILE *fp;
  guint c;
  gchar *macro_name;
  guint8 *img_buffer, *img_buffer_end;
  gchar *basename;

  fp = g_fopen (config->file_name, "w");
  if (!fp)
    return FALSE;

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  if (1)
    {
      guint8 *data, *p;
      gint x, y, pad, n_bytes, bpp;

      bpp = config->alpha ? 4 : 3;
      n_bytes = drawable->width * drawable->height * bpp;
      pad = drawable->width * drawable->bpp;
      if (config->use_rle)
	pad = MAX (pad, 130 + n_bytes / 127);
      data = g_new (guint8, pad + n_bytes);
      p = data + pad;
      for (y = 0; y < drawable->height; y++)
	{
	  gimp_pixel_rgn_get_row (&pixel_rgn, data, 0, y, drawable->width);
	  if (config->alpha)
	    for (x = 0; x < drawable->width; x++)
	      {
		guint8 *d = data + x * drawable->bpp;
		gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

		alpha *= config->opacity / 100.0;
		*(p++) = d[0];
		*(p++) = d[1];
		*(p++) = d[2];
		*(p++) = alpha + 0.5;
	      }
	  else
	    for (x = 0; x < drawable->width; x++)
	      {
		guint8 *d = data + x * drawable->bpp;
		gdouble alpha = drawable_type == GIMP_RGBA_IMAGE ? d[3] : 0xff;

		alpha *= config->opacity / 25500.0;
		*(p++) = 0.5 + alpha * (gdouble) d[0];
		*(p++) = 0.5 + alpha * (gdouble) d[1];
		*(p++) = 0.5 + alpha * (gdouble) d[2];
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
	img_buffer_end = img_buffer + n_bytes;
    }

  if (!config->use_macros && config->glib_types)
    {
      s_uint_8 =  "guint8 ";
      s_uint_32 = "guint32";
      s_uint  =   "guint  ";
      s_char =    "gchar  ";
      s_null =    "NULL";
    }
  else if (!config->use_macros)
    {
      s_uint_8 =  "unsigned char";
      s_uint_32 = "unsigned int ";
      s_uint =    "unsigned int ";
      s_char =    "char         ";
      s_null =    "(char*) 0";
    }
  else if (config->use_macros && config->glib_types)
    {
      s_uint_8 =  "guint8";
      s_uint_32 = "guint32";
      s_uint  =   "guint";
      s_char =    "gchar";
      s_null =    "NULL";
    }
  else /* config->use_macros && !config->glib_types */
    {
      s_uint_8 =  "unsigned char";
      s_uint_32 = "unsigned int";
      s_uint =    "unsigned int";
      s_char =    "char";
      s_null =    "(char*) 0";
    }

  macro_name = g_ascii_strup (config->prefixed_name, -1);

  basename = g_path_get_basename (config->file_name);

  fprintf (fp, "/* GIMP %s C-Source image dump %s(%s) */\n\n",
	   config->alpha ? "RGBA" : "RGB",
	   config->use_rle ? "1-byte-run-length-encoded " : "",
	   basename);

  g_free (basename);

  if (config->use_rle && !config->use_macros)
    save_rle_decoder (fp,
		      macro_name,
		      config->glib_types ? "guint" : "unsigned int",
		      config->glib_types ? "guint8" : "unsigned char",
		      config->alpha ? 4 : 3);

  if (!config->use_macros)
    {
      fprintf (fp, "static const struct {\n");
      fprintf (fp, "  %s\t width;\n", s_uint);
      fprintf (fp, "  %s\t height;\n", s_uint);
      fprintf (fp, "  %s\t bytes_per_pixel; /* 3:RGB, 4:RGBA */ \n", s_uint);
      if (config->use_comment)
	fprintf (fp, "  %s\t*comment;\n", s_char);
      fprintf (fp, "  %s\t %spixel_data[",
	       s_uint_8,
	       config->use_rle ? "rle_" : "");
      if (config->use_rle)
	fprintf (fp, "%u + 1];\n", (guint) (img_buffer_end - img_buffer));
      else
	fprintf (fp, "%u * %u * %u + 1];\n",
		 drawable->width,
		 drawable->height,
		 config->alpha ? 4 : 3);
      fprintf (fp, "} %s = {\n", config->prefixed_name);
      fprintf (fp, "  %u, %u, %u,\n",
	       drawable->width,
	       drawable->height,
	       config->alpha ? 4 : 3);
    }
  else /* use macros */
    {
      fprintf (fp, "#define %s_WIDTH (%u)\n",
	       macro_name, drawable->width);
      fprintf (fp, "#define %s_HEIGHT (%u)\n",
	       macro_name, drawable->height);
      fprintf (fp, "#define %s_BYTES_PER_PIXEL (%u) /* 3:RGB, 4:RGBA */\n",
	       macro_name, config->alpha ? 4 : 3);
    }
  if (config->use_comment && !config->comment)
    {
      if (!config->use_macros)
	fprintf (fp, "  %s,\n", s_null);
      else /* use macros */
	fprintf (fp, "#define %s_COMMENT (%s)\n", macro_name, s_null);
    }
  else if (config->use_comment)
    {
      gchar *p = config->comment - 1;

      if (config->use_macros)
	fprintf (fp, "#define %s_COMMENT \\\n", macro_name);
      fprintf (fp, "  \"");
      while (*(++p))
	if (*p == '\\')
	  fprintf (fp, "\\\\");
	else if (*p == '"')
	  fprintf (fp, "\\\"");
	else if (*p == '\n' && p[1])
	  fprintf (fp, "\\n\"%s\n  \"",
		   config->use_macros ? " \\" : "");
	else if (*p == '\n')
	  fprintf (fp, "\\n");
	else if (*p == '\r')
	  fprintf (fp, "\\r");
	else if (*p == '\b')
	  fprintf (fp, "\\b");
	else if (*p == '\f')
	  fprintf (fp, "\\f");
	else if (*p >= 32 && *p <= 126)
	  fprintf (fp, "%c", *p);
	else
	  fprintf (fp, "\\%03o", *p);
      if (!config->use_macros)
	fprintf (fp, "\",\n");
      else /* use macros */
	fprintf (fp, "\"\n");
    }
  if (config->use_macros)
    {
      fprintf (fp, "#define %s_%sPIXEL_DATA ((%s*) %s_%spixel_data)\n",
	       macro_name,
	       config->use_rle ? "RLE_" : "",
	       s_uint_8,
	       macro_name,
	       config->use_rle ? "rle_" : "");
      if (config->use_rle)
	save_rle_decoder (fp,
			  macro_name,
			  s_uint,
			  s_uint_8,
			  config->alpha ? 4 : 3);
      fprintf (fp, "static const %s %s_%spixel_data[",
	       s_uint_8,
	       macro_name,
	       config->use_rle ? "rle_" : "");
      if (config->use_rle)
	fprintf (fp, "%u] =\n", (guint) (img_buffer_end - img_buffer));
      else
	fprintf (fp, "%u * %u * %u + 1] =\n",
		 drawable->width,
		 drawable->height,
		 config->alpha ? 4 : 3);
      fprintf (fp, "(\"");
      c = 2;
    }
  else
    {
      fprintf (fp, "  \"");
      c = 3;
    }
  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      do
	c = save_uchar (fp, c, *(img_buffer++), config);
      while (img_buffer < img_buffer_end);
      break;
    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      return FALSE;
    }
  if (!config->use_macros)
    fprintf (fp, "\",\n};\n\n");
  else /* use macros */
    fprintf (fp, "\");\n\n");

  fclose (fp);

  gimp_drawable_detach (drawable);

  return TRUE;
}

static GtkWidget *prefixed_name;
static GtkWidget *centry;

static gboolean
run_save_dialog	(Config *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *adj;
  gboolean   run;

  dialog = gimp_dialog_new (_("Save as C-Source"), PLUG_IN_BINARY,
                            NULL, 0,
			    gimp_standard_help_func, SAVE_PROC,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
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
  toggle = gtk_check_button_new_with_mnemonic (_("Us_e macros instead of struct"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				config->use_macros);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->use_macros);

  /* Use RLE
   */
  toggle = gtk_check_button_new_with_mnemonic (_("Use _1 byte Run-Length-Encoding"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				config->use_rle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->use_rle);

  /* Alpha
   */
  toggle = gtk_check_button_new_with_mnemonic (_("Sa_ve alpha channel (RGBA/RGB)"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				config->alpha);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &config->alpha);

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
