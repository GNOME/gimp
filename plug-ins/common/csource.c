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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "libgimp/gimp.h"
#include <gtk/gtk.h>


typedef struct {
  gchar   *file_name;
  gchar   *prefixed_name;
  gchar   *comment;
  gboolean use_comment;
  gboolean glib_types;
  gboolean alpha;
  gdouble  opacity;
} Config;


/* --- prototypes --- */
static void	query		(void);
static void	run		(gchar		*name,
				 gint		 nparams,
				 GParam		*param,
				 gint		*nreturn_vals,
				 GParam	       **return_vals);
static gint	save_image	(Config		*config,
				 gint32		 image_ID,
				 gint32		 drawable_ID);
static gboolean	run_save_dialog	(Config         *config);


/* --- variables --- */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* --- implement main (), provided by libgimp --- */
MAIN ();

/* --- functions --- */
static void
query (void)
{
  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);
  
  gimp_install_procedure ("file_csource_save",
                          "Dump image data in RGB(A) format for C source",
                          "FIXME: write help",
                          "Tim Janik",
                          "Tim Janik",
                          "1999",
                          "<Save>/C-Source",
			  "RGB*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);
  
  gimp_register_save_handler ("file_csource_save", "c", "");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  
  run_mode = param[0].data.d_int32;
  
  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  
  if (run_mode == RUN_INTERACTIVE &&
      strcmp (name, "file_csource_save") == 0)
    {
      gint32 image_ID = param[1].data.d_int32;
      gint32 drawable_ID = param[2].data.d_int32;
      GDrawableType drawable_type = gimp_drawable_type (drawable_ID);
      Config config = {
	param[3].data.d_string,
	"gimp_image",
	NULL,
	FALSE,
	TRUE,
	(drawable_type == RGBA_IMAGE ||
	 drawable_type == GRAYA_IMAGE ||
	 drawable_type == INDEXEDA_IMAGE),
	100.0,
      };
      Parasite *parasite;
      gchar *x;

      parasite = gimp_image_find_parasite (image_ID, "gimp-comment");
      if (parasite)
	{
	  config.comment = g_strdup (parasite->data);
	  parasite_free (parasite);
	}
      x = config.comment;
      
      if (run_save_dialog (&config))
	{
	  if (x != config.comment &&
	      !(x && config.comment && strcmp (x, config.comment) == 0))
	    {
	      if (!config.comment || !config.comment[0])
		gimp_image_detach_parasite (image_ID, "gimp-comment");
	      else
		{
		  parasite = parasite_new ("gimp-comment",
					   PARASITE_PERSISTENT,
					   strlen (config.comment) + 1,
					   config.comment);
		  gimp_image_attach_parasite (image_ID, parasite);
		  parasite_free (parasite);
		}
	    }

	  *nreturn_vals = 1;
	  if (save_image (&config, image_ID, drawable_ID))
	    values[0].data.d_status = STATUS_SUCCESS;
	  else
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
}

static inline guint
save_uchar (FILE  *fp,
	    guint  c,
	    guint8 d)
{
  if (c > 74)
    {
      fprintf (fp, "\n   ");
      c = 3;
    }
  fprintf (fp, " %u,", d);
  c += 1 + (d > 99) + (d > 9) + 1 + 1;

  return c;
}

static gint
save_image (Config *config,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GDrawable *drawable = gimp_drawable_get (drawable_ID);
  GDrawableType drawable_type = gimp_drawable_type (drawable_ID);
  GPixelRgn pixel_rgn;
  gchar *s_uint_8, *s_uint_32, *s_int, *s_uint, *s_char, *s_null;
  FILE *fp;
  guint c;
  
  fp = fopen (config->file_name, "w");
  if (!fp)
    return FALSE;
  
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  if (config->glib_types)
    {
      s_uint_8 =  "guint8 ";
      s_uint_32 = "guint32";
      s_uint  =   "guint  ";
      s_int  =    "gint   ";
      s_char =    "gchar  ";
      s_null =    "NULL";
    }
  else
    {
      s_uint_8 =  "unsigned char";
      s_uint_32 = "unsigned int ";
      s_uint =    "unsigned int ";
      s_int =     "int          ";
      s_char =    "char         ";
      s_null =    "(char*) 0";
    }
  
  fprintf (fp, "/* GIMP %s C-Source image dump (%s) */\n\n",
	   config->alpha ? "RGBA" : "RGB",
	   config->file_name);

  fprintf (fp, "static const struct {\n");
  fprintf (fp, "  %s\t width;\n", s_uint);
  fprintf (fp, "  %s\t height;\n", s_uint);
  fprintf (fp, "  %s\t bytes_per_pixel; /* 3:RGB, 4:RGBA */ \n", s_uint);
  if (config->use_comment)
    fprintf (fp, "  %s\t*comment;\n", s_char);
  fprintf (fp, "  %s\t pixel_data[%u * %u * %u];\n",
	   s_uint_8,
	   drawable->width,
	   drawable->height,
	   config->alpha ? 4 : 3);
  fprintf (fp, "} %s = {\n", config->prefixed_name);
  fprintf (fp, "  %u, %u, %u,\n",
	   drawable->width,
	   drawable->height,
	   config->alpha ? 4 : 3);
  if (config->use_comment && !config->comment)
    fprintf (fp, "  %s,\n", s_null);
  else if (config->use_comment)
    {
      gchar *p = config->comment - 1;
      
      fprintf (fp, "  \"");
      while (*(++p))
	if (*p == '\\')
	  fprintf (fp, "\\\\");
	else if (*p == '"')
	  fprintf (fp, "\\\"");
	else if (*p == '\n' && p[1])
	  fprintf (fp, "\\n\"\n  \"");
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
      fprintf (fp, "\",\n");
    }
  fprintf (fp, "  {");
  c = 3;
  switch (drawable_type)
    {
      guint8 *data;
      gint x, y;
      
    case RGB_IMAGE:
    case RGBA_IMAGE:
      data = g_new (guint8, drawable->width * drawable->bpp);
      
      for (y = 0; y < drawable->height; y++)
	{
	  gimp_pixel_rgn_get_row (&pixel_rgn, data, 0, y, drawable->width);
	  for (x = 0; x < drawable->width; x++)
	    {
	      guint8 *d = data + x * drawable->bpp;
	      
	      c = save_uchar (fp, c, d[0]);
	      c = save_uchar (fp, c, d[1]);
	      c = save_uchar (fp, c, d[2]);
	      if (config->alpha)
		{
		  gdouble alpha = drawable_type == RGBA_IMAGE ? d[3] : 0xff;

		  alpha *= config->opacity / 100.0;
		  c = save_uchar (fp, c, alpha + 0.5);
		}
	    }
	}
      
      g_free (data);
      break;
    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      return FALSE;
    }
  fprintf (fp, "\n  },\n};\n\n");
  
  fclose (fp);
  
  gimp_drawable_detach (drawable);
  
  return TRUE;
}

static void
cb_set_true (gboolean *bool_p)
{
  *bool_p = TRUE;
}

static gboolean
run_save_dialog	(Config *config)
{
  GtkWidget *dialog, *vbox, *hbox;
  GtkWidget *prefixed_name, *centry, *alpha, *gtype, *use_comment, *any;
  GtkObject *opacity;
  gboolean do_save = FALSE;
  
  gtk_init (NULL, NULL);
  g_set_prgname ("Save");
  gtk_rc_parse (gimp_gtkrc ());
  
  dialog = gtk_widget_new (GTK_TYPE_DIALOG,
			   "title", "Save as C-Source",
			   "window_position", GTK_WIN_POS_MOUSE,
			   "signal::destroy", gtk_main_quit, NULL,
			   NULL);
  vbox = GTK_DIALOG (dialog)->vbox;
  gtk_widget_set (vbox,
		  "border_width", 5,
		  "spacing", 5,
		  NULL);
  hbox = GTK_DIALOG (dialog)->action_area;
  gtk_widget_new (GTK_TYPE_BUTTON,
		  "visible", TRUE,
		  "label", "Ok",
		  "object_signal::clicked", cb_set_true, &do_save,
		  "object_signal::clicked", gtk_widget_destroy, dialog,
		  "parent", hbox,
		  "can_default", TRUE,
		  "has_default", TRUE,
		  NULL);
  gtk_widget_new (GTK_TYPE_BUTTON,
		  "visible", TRUE,
		  "label", "Cancel",
		  "object_signal::clicked", gtk_widget_destroy, dialog,
		  "parent", hbox,
		  "can_default", TRUE,
		  NULL);
  
  /* Prefixed Name
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  gtk_widget_new (GTK_TYPE_LABEL,
		  "label", "Prefixed Name: ",
		  "xalign", 0.0,
		  "visible", TRUE,
		  "parent", hbox,
		  NULL);
  prefixed_name = gtk_widget_new (GTK_TYPE_ENTRY,
				  "visible", TRUE,
				  "parent", hbox,
				  NULL);
  gtk_entry_set_text (GTK_ENTRY (prefixed_name),
		      config->prefixed_name ? config->prefixed_name : "");
  gtk_widget_ref (prefixed_name);
  
  /* Comment Entry
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  gtk_widget_new (GTK_TYPE_LABEL,
		  "label", "Comment: ",
		  "xalign", 0.0,
		  "visible", TRUE,
		  "parent", hbox,
		  NULL);
  centry = gtk_widget_new (GTK_TYPE_ENTRY,
			   "visible", TRUE,
			   "parent", hbox,
			   NULL);
  gtk_entry_set_text (GTK_ENTRY (centry),
		      config->comment ? config->comment : "");
  gtk_widget_ref (centry);

  /* Use Comment
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  use_comment = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
				"label", "Save comment to file?",
				"visible", TRUE,
				"parent", hbox,
				NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_comment), config->use_comment);
  gtk_widget_ref (use_comment);

  /* GLib types
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  gtype = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
			  "label", "Use GLib types (guint8*)",
			  "visible", TRUE,
			  "parent", hbox,
			  NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtype), config->glib_types);
  gtk_widget_ref (gtype);

  /* Alpha
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  alpha = gtk_widget_new (GTK_TYPE_CHECK_BUTTON,
			  "label", "Save RGBA instead of RGB",
			  "visible", TRUE,
			  "parent", hbox,
			  NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (alpha), config->alpha);
  gtk_widget_ref (alpha);
  
  /* Max Alpha Value
   */
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "parent", vbox,
			 NULL);
  gtk_widget_new (GTK_TYPE_LABEL,
		  "label", "Opacity: ",
		  "xalign", 0.0,
		  "visible", TRUE,
		  "parent", hbox,
		  NULL);
  opacity = gtk_adjustment_new (config->opacity,
				0,
				100,
				0.1,
				5.0,
				0);
  any = gtk_spin_button_new (GTK_ADJUSTMENT (opacity), 0, 1);
  gtk_widget_set (any,
		  "visible", TRUE,
		  "parent", hbox,
		  NULL);
  gtk_object_ref (opacity);
  
  gtk_widget_show (dialog);
  
  gtk_main ();
  
  config->prefixed_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (prefixed_name)));
  config->alpha = GTK_TOGGLE_BUTTON (alpha)->active;
  config->glib_types = GTK_TOGGLE_BUTTON (gtype)->active;
  config->comment = g_strdup (gtk_entry_get_text (GTK_ENTRY (centry)));
  config->use_comment = GTK_TOGGLE_BUTTON (use_comment)->active;
  config->opacity = GTK_ADJUSTMENT (opacity)->value;
  
  gtk_widget_unref (gtype);
  gtk_widget_unref (centry);
  gtk_widget_unref (alpha);
  gtk_widget_unref (use_comment);
  gtk_widget_unref (prefixed_name);
  gtk_object_unref (opacity);

  if (!config->prefixed_name || !config->prefixed_name[0])
    config->prefixed_name = "tmp";
  if (config->comment && !config->comment[0])
    config->comment = NULL;

  return do_save;
}
