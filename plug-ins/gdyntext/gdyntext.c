/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999,2000 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
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
 *
 * $Id$
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

#include "gdyntext.h"
#include "font_selection.h"



static void gdt_query (void);
static void gdt_run   (gchar       *name, 
		       gint         nparams, 
		       GimpParam   *param, 
		       gint        *nreturn_vals, 
		       GimpParam  **return_vals);


GimpPlugInInfo PLUG_IN_INFO = 
{
  NULL,
  NULL,
  gdt_query,
  gdt_run
};

GdtVals gdtvals;


#ifndef DEBUG_UI
MAIN()
#endif


static void 
gdt_query (void)
{
  static GimpParamDef gdt_args[] = 
  {
    /* standard params */
    { GIMP_PDB_INT32,    "run_mode",        "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",           "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",        "Input drawable"               },
    /* gdyntext params */
    { GIMP_PDB_STRING,   "text",            "Text to render"               },
    { GIMP_PDB_INT32,    "antialias",       "Generate antialiased text"    },
    { GIMP_PDB_INT32,    "alignment",       "Text alignment: { LEFT = 0, CENTER = 1, RIGHT = 2 }" },
    { GIMP_PDB_INT32,    "rotation",        "Text rotation (degrees)"      },
    { GIMP_PDB_INT32,    "line_spacing",    "Line spacing"                 },
    { GIMP_PDB_COLOR,    "color",           "Text color"                   },
    { GIMP_PDB_INT32,    "layer_alignment", "Layer alignment { NONE = 0, BOTTOM_LEFT = 1, BOTTOM_CENTER = 2, BOTTOM_RIGHT = 3, MIDDLE_LEFT = 4, CENTER = 5, MIDDLE_RIGHT = 6, TOP_LEFT = 7, TOP_CENTER = 8, TOP_RIGHT = 9 }" },
    { GIMP_PDB_STRING,   "fontname",        "The fontname (conforming to the X Logical Font Description Conventions)" }
  };
  static GimpParamDef gdt_rets[] = 
  {
    { GIMP_PDB_LAYER,    "layer",           "The text layer"               }
  };
  static gint ngdt_args = sizeof (gdt_args) / sizeof (gdt_args[0]);
  static gint ngdt_rets = sizeof (gdt_rets) / sizeof (gdt_rets[0]);
    
  gimp_install_procedure ("plug_in_dynamic_text",
			  "GIMP Dynamic Text",
			  "",
			  "Marco Lamberto <lm@geocities.com>",
			  "Marco Lamberto",
			  "Jan 1999",
			  N_("<Image>/Filters/Render/Dynamic Text..."),
			  "RGB*,GRAY*,INDEXED*",
			  GIMP_PLUGIN,
			  ngdt_args, ngdt_rets,
			  gdt_args, gdt_rets);
}


static void 
gdt_run (gchar       *name, 
	 gint         nparams, 
	 GimpParam   *param, 
	 gint        *nreturn_vals,
	 GimpParam  **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType run_mode;
  GdtVals oldvals;
  
 
  INIT_I18N_UI();

  gdtvals.valid		      = TRUE;
  gdtvals.change_layer_name   = FALSE;
  run_mode		      = param[0].data.d_int32;
  gdtvals.image_id	      = param[1].data.d_image;
  gdtvals.drawable_id	      = param[2].data.d_drawable;
  gdtvals.layer_id	      = param[2].data.d_layer;
  gdtvals.messages	      = NULL;
  gdtvals.preview	      = TRUE;
  
  *nreturn_vals = 2;
  *return_vals  = values;
	
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
  values[1].type          = GIMP_PDB_LAYER;
  values[1].data.d_int32  = -1;

  switch(run_mode) 
    {
    case GIMP_RUN_INTERACTIVE:
      memset (&oldvals, 0, sizeof(GdtVals));
      gimp_get_data ("plug_in_gdyntext", &oldvals);

      if (oldvals.valid) 
	{
	  strncpy (gdtvals.text, oldvals.text, sizeof(gdtvals.text));
	  strncpy (gdtvals.xlfd, oldvals.xlfd, sizeof(gdtvals.xlfd));
	  gdtvals.color	          = oldvals.color;
	  gdtvals.antialias	  = oldvals.antialias;
	  gdtvals.alignment       = oldvals.alignment;
	  gdtvals.rotation        = oldvals.rotation;
	  gdtvals.line_spacing	  = oldvals.line_spacing;
	  gdtvals.layer_alignment = oldvals.layer_alignment;
	  gdtvals.preview	  = oldvals.preview;
	} 
      else
	{
	  gdtvals.valid             = FALSE;
	}

      gdt_load (&gdtvals);
      if (!gdt_create_ui (&gdtvals))
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
#ifdef DEBUG
      g_print ("%d\n", nparams);
#endif
      if (nparams != 11) 
	{
	  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
	  return;
	} 
      else 
	{
	  gdtvals.new_layer	  = !gimp_drawable_has_alpha (gdtvals.drawable_id);
	  strncpy(gdtvals.text, param[3].data.d_string, sizeof(gdtvals.text));
	  gdtvals.antialias	  = param[4].data.d_int32;
	  gdtvals.alignment	  = param[5].data.d_int32;
	  gdtvals.rotation	  = param[6].data.d_int32;
	  gdtvals.line_spacing	  = param[7].data.d_int32;
	  gdtvals.color 	  = ((guint)param[8].data.d_color.red << 16) +
	                            ((guint)param[8].data.d_color.green << 8) +
	                             (guint)param[8].data.d_color.blue;
	  gdtvals.layer_alignment = param[9].data.d_int32;
	  strncpy(gdtvals.xlfd, param[10].data.d_string, sizeof(gdtvals.xlfd));
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_gdyntext", &gdtvals);
      gdtvals.image_id		 = param[1].data.d_image;
      gdtvals.drawable_id	 = param[2].data.d_drawable;
      gdtvals.layer_id		 = param[2].data.d_layer;
      gdtvals.new_layer		 = !gimp_drawable_has_alpha (gdtvals.drawable_id);
      break;
    }
  
  gdt_render_text (&gdtvals);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gdtvals.valid = TRUE;
      gimp_set_data ("plug_in_gdyntext", &gdtvals, sizeof(GdtVals));
    }

  values[1].data.d_int32 = gdtvals.layer_id; 
}


void 
gdt_load (GdtVals *data)
{
  gchar  *gdtparams  = NULL;
  gchar  *gdtparams0 = NULL;
  gchar **params     = NULL;
  GimpParamColor  col;
  GimpParasite   *parasite = NULL;  
	
  if (gdt_compat_load (data))
    return;
  
  if ((parasite = gimp_drawable_parasite_find (data->drawable_id, GDYNTEXT_PARASITE)) != NULL)
    {
      gdtparams = g_strdup (gimp_parasite_data (parasite));
      gimp_parasite_free (parasite);
    }
  
  if (gdtparams == NULL)
    gdtparams = gimp_layer_get_name (data->layer_id);
  
  if (!gimp_drawable_has_alpha (data->drawable_id) ||
      strncmp (gdtparams, "GDT", 3) != 0)
    {
      data->messages = g_list_append (data->messages, _(" Current layer isn't a GDynText layer or it has no alpha channel."
							" Forcing new layer creation."));
      data->new_layer = TRUE;
      if (!data->valid)   /* setup default values if it wasn't already done */
	{		
	  strcpy (data->text, "");
	  strcpy (data->xlfd, "");

	  gimp_palette_get_foreground (&col.red, &col.green, &col.blue);
	  data->color = (col.red << 16) + (col.green << 8) + col.blue;

	  data->antialias         = TRUE;
	  data->alignment 	  = LEFT;
	  data->rotation	  = 0;
	  data->line_spacing	  = 0;
	  data->layer_alignment	  = LA_NONE;
	  data->change_layer_name = FALSE;
	  data->messages	  = NULL;
	  data->preview		  = TRUE;
	}

      return;
    }

#ifdef DEBUG
  g_print ("'%s'\n", gdtparams);
#endif
  {
    gchar *st;
    gint len;
    
    st = strchr (gdtparams, '{') + 1;
    len = strrchr (st, '}') - st;
    gdtparams0 = g_strndup (st, len);
  }
#ifdef DEBUG
  g_print ("'%s'\n", gdtparams0);
#endif
  params = g_strsplit (gdtparams0, "}{", -1);
  g_free (gdtparams0);
  
  data->new_layer       = FALSE;
  data->antialias       = atoi (params[ANTIALIAS]);
  data->alignment       = atoi (params[ALIGNMENT]);
  data->rotation        = atoi (params[ROTATION]);
  data->line_spacing	= atoi (params[LINE_SPACING]);
  data->layer_alignment	= atoi (params[LAYER_ALIGNMENT]);
  data->color		= strtol (params[COLOR], (gchar **)NULL, 16);

  strncpy (data->xlfd, params[XLFD], sizeof(data->xlfd));
  strncpy (data->text, params[TEXT], sizeof(data->text));
  {
    gchar *text = gimp_strcompress (data->text);
    g_snprintf (data->text, sizeof(data->text), "%s", text);
    g_free (text);
  }
  g_free (gdtparams);
  g_strfreev (params);
}


void 
gdt_save (GdtVals *data)
{
  gchar *lname, *text;
  GimpParasite *parasite;
  
  text = gimp_strescape (data->text, NULL);
  lname = g_strdup_printf (GDYNTEXT_MAGIC
			   "{%s}{%d}{%d}{%d}{%d}{%06X}{%d}{%s}",
			   text,
			   data->antialias, data->alignment, data->rotation, data->line_spacing,
			   data->color, data->layer_alignment, data->xlfd);
  g_free(text);
  
  parasite = gimp_parasite_new (GDYNTEXT_PARASITE,
				GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE, strlen(lname), lname);
  gimp_drawable_parasite_attach (data->drawable_id, parasite);
  gimp_parasite_free (parasite);
  
  if (!data->change_layer_name) 
    {
      gimp_layer_set_name (data->layer_id, _("GDynText Layer"));
      gimp_displays_flush ();
      g_free (lname);

      return;
    }

  /* change the layer name as in GIMP 1.x */
  gimp_layer_set_name (data->layer_id, lname);
  gimp_displays_flush ();
  g_free (lname);
}


void 
gdt_render_text (GdtVals *data)
{
  gdt_render_text_p (data, TRUE);
}


void 
gdt_render_text_p (GdtVals  *data, 
		   gboolean  show_progress)
{
  gint layer_ox, layer_oy, i, xoffs;
  gint32 layer_f, selection_empty, selection_channel = -1;
  gint32 text_width, text_height = 0;
  gint32 text_ascent, text_descent;
  gint32 layer_width, layer_height;
  gint32 image_width, image_height;
  gint32 space_width;
  gfloat font_size;
  gint32 font_size_type;
  gchar **text_xlfd, **text_lines;
  gint32 *text_lines_w;
  GimpParamColor  old_color, text_color;
  
  if (show_progress)
    gimp_progress_init (_("GIMP Dynamic Text"));
  
  /* undo start */
  gimp_undo_push_group_start (gdtvals.image_id);
  
  /* save and remove current selection */
  selection_empty = gimp_selection_is_empty (data->image_id);
  if (!selection_empty) 
    {
      /* there is an active selection to save */
      selection_channel = gimp_selection_save (data->image_id);
      gimp_selection_none (data->image_id);
    }

  text_xlfd = g_strsplit (data->xlfd, "-", -1);
  if (!strlen (text_xlfd[XLFD_PIXEL_SIZE]) ||
      !strcmp (text_xlfd[XLFD_PIXEL_SIZE], "*"))
    {
      font_size = atof (text_xlfd[XLFD_POINT_SIZE]);
      font_size_type = FONT_METRIC_POINTS;
    } 
  else 
    {
      font_size = atof (text_xlfd[XLFD_PIXEL_SIZE]);
      font_size_type = FONT_METRIC_PIXELS;
    }
  g_strfreev (text_xlfd);

  /* retrieve space char width */
  gimp_text_get_extents_fontname ("AA", 
				  font_size, 
				  font_size_type, 
				  data->xlfd,
				  &text_width,
				  &text_height,
				  &text_ascent,
				  &text_descent);
  space_width = text_width;
  gimp_text_get_extents_fontname ("AA", 
				  font_size, 
				  font_size_type, 
				  data->xlfd,
				  &text_width,
				  &text_height,
				  &text_ascent,
				  &text_descent);
  space_width -= text_width;
#ifdef DEBUG
  g_print ("GDT: space width = %d\n", space_width);
#endif
  
  /* setup text and compute layer size */
  text_lines = g_strsplit (data->text, "\n", -1);
  for (i = 0; text_lines[i]; i++);
  text_lines_w = g_new0 (gint32, i);
  layer_width = layer_height = 0;

  for (i = 0; text_lines[i]; i++) 
    {
      gimp_text_get_extents_fontname (strlen(text_lines[i]) > 0 ? text_lines[i] : " ",
				      font_size,
				      font_size_type,
				      data->xlfd,
				      &text_width,
				      &text_height,
				      &text_ascent,
				      &text_descent);
#ifdef DEBUG
      g_print ("GDT: %4dx%4d A:%3d D:%3d [%s]\n", text_width, text_height, text_ascent, text_descent, text_lines[i]);
#endif
      text_lines_w[i] = text_width;
      if (layer_width < text_width)
	layer_width = text_width;
      layer_height += text_height + data->line_spacing;
    }
  layer_height -= data->line_spacing;
  
  if (layer_height == 0) 
    layer_height = 10;
  if (layer_width == 0) 
    layer_width = 10;

  if (!data->new_layer) 
    {
      /* resize the old layer */
      gimp_layer_resize (data->layer_id, layer_width, layer_height, 0, 0);
    } 
  else 
    {
      /* create a new layer */
      data->layer_id = data->drawable_id = gimp_layer_new (data->image_id,
							   _("GDynText Layer"), layer_width, layer_height,
							   (GimpImageType)(gimp_image_base_type (data->image_id) * 2 + 1),
							   100.0, GIMP_NORMAL_MODE);
      gimp_layer_add_alpha (data->layer_id);
      gimp_image_add_layer (data->image_id, data->layer_id, 0);
      gimp_image_set_active_layer (data->image_id, data->layer_id);
    }
  
  /* clear layer */
  gimp_layer_set_preserve_transparency (data->layer_id, 0);
  gimp_edit_clear (data->drawable_id);
  
  /* get layer offsets */
  gimp_drawable_offsets (data->layer_id, &layer_ox, &layer_oy);
  
  /* get foreground color */
  gimp_palette_get_foreground (&old_color.red, 
			       &old_color.green, 
			       &old_color.blue);
  
  /* set foreground color to the wanted text color */
  text_color.red   = (data->color & 0xff0000) >> 16;
  text_color.green = (data->color & 0xff00) >> 8;
  text_color.blue  = data->color & 0xff;
  gimp_palette_set_foreground (text_color.red, 
			       text_color.green, 
			       text_color.blue);
  
  /* write text */
  for (i = 0; text_lines[i]; i++) 
    {
      switch (data->alignment) 
	{
	case LEFT:
	  xoffs = 0;
	  break;
	case RIGHT:
	  xoffs = layer_width - text_lines_w[i];
	  break;
	case CENTER:
	  xoffs = (layer_width - text_lines_w[i]) / 2;
	  break;
	default:
	  xoffs = 0;
	}
      layer_f = gimp_text_fontname (data->image_id,
				    data->drawable_id,
				    (gdouble)layer_ox +
				    strspn (text_lines[i], " ") * space_width + xoffs,	        /* x */
				    (gdouble)layer_oy + i * (text_height + data->line_spacing), /* y */
				    text_lines[i],
				    0,  /* border */
				    data->antialias,
				    font_size,
				    font_size_type,
				    data->xlfd);
      
      /* FIXME: ascent/descent stuff, use gimp_layer_translate */
#ifdef DEBUG
      g_print ("GDT: MH:%d LH:%d\n", text_height, gimp_drawable_height (layer_f));
#endif
      
      gimp_floating_sel_anchor (layer_f);
      
      if (show_progress)
	gimp_progress_update ((gdouble)(i + 2) * 100.0 * (gdouble)text_height / (gdouble)layer_height);
    }
  g_strfreev (text_lines);
  g_free (text_lines_w);
  
  /* set foreground color to the old one */
  gimp_palette_set_foreground (old_color.red, 
			       old_color.green, 
			       old_color.blue);
  
  /* apply rotation */
  if (data->rotation != 0 && abs(data->rotation) != 360) 
    {
      gimp_rotate (data->drawable_id, 
		   TRUE,
		   (gdouble)data->rotation * G_PI / 180.0);
      gimp_layer_set_offsets (data->layer_id, layer_ox, layer_oy);
    }

  /* sets the layer alignment */
  layer_width  = gimp_drawable_width (data->drawable_id);
  layer_height = gimp_drawable_height (data->drawable_id);
  image_width  = gimp_image_width (data->image_id);
  image_height = gimp_image_height (data->image_id);

  switch (data->layer_alignment) 
    {
    case LA_TOP_LEFT:
      gimp_layer_set_offsets(data->layer_id, 0, 0);
      break;
    case LA_TOP_CENTER:
      gimp_layer_set_offsets(data->layer_id, (image_width - layer_width) >> 1, 0);
      break;
    case LA_TOP_RIGHT:
      gimp_layer_set_offsets(data->layer_id, image_width - layer_width, 0);
      break;
    case LA_MIDDLE_LEFT:
      gimp_layer_set_offsets(data->layer_id, 0,
			     (image_height - layer_height) >> 1);
      break;
    case LA_CENTER:
      gimp_layer_set_offsets(data->layer_id, (image_width - layer_width) >> 1,
			     (image_height - layer_height) >> 1);
      break;
    case LA_MIDDLE_RIGHT:
      gimp_layer_set_offsets(data->layer_id, image_width - layer_width,
			     (image_height - layer_height) >> 1);
      break;
    case LA_BOTTOM_LEFT:
      gimp_layer_set_offsets(data->layer_id, 0, image_height - layer_height);
      break;
    case LA_BOTTOM_CENTER:
      gimp_layer_set_offsets(data->layer_id, (image_width - layer_width) >> 1,
			     image_height - layer_height);
      break;
    case LA_BOTTOM_RIGHT:
      gimp_layer_set_offsets(data->layer_id, image_width - layer_width,
			     image_height - layer_height);
      break;
    case LA_NONE:
    default:
    }
  
  gimp_layer_set_preserve_transparency (data->layer_id, 1);

  /* restore old selection if any */
  if (selection_empty == FALSE) 
    {
      gimp_selection_load (selection_channel);
      gimp_image_remove_channel (data->image_id, selection_channel);
    }

  gdt_save (data);
  
  /* undo end */
  gimp_undo_push_group_end (gdtvals.image_id);
  
  if (show_progress)
    gimp_progress_update (100.0);
  
  gimp_displays_flush ();
}
