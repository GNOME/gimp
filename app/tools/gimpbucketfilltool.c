/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "pdb/procedural_db.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpbucketfilltool.h"
#include "paint_options.h"

#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _BucketOptions BucketOptions;

struct _BucketOptions
{
  PaintOptions    paint_options;

  gboolean        sample_merged;
  gboolean        sample_merged_d;
  GtkWidget      *sample_merged_w;

  gdouble         threshold;
  /* gdouble      threshold_d; (from gimprc) */
  GtkObject      *threshold_w;

  BucketFillMode  fill_mode;
  BucketFillMode  fill_mode_d;
  GtkWidget      *fill_mode_w[3];
};


static GimpToolClass *parent_class = NULL;


/*  local function prototypes  */

static void   gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass);
static void   gimp_bucket_fill_tool_init       (GimpBucketFillTool      *bucket_fill_tool);

static void   gimp_bucket_fill_tool_button_press    (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
						     GdkModifierType  state,
						     GimpDisplay     *gdisp);
static void   gimp_bucket_fill_tool_button_release  (GimpTool        *tool,
                                                     GimpCoords      *coords,
                                                     guint32          time,
						     GdkModifierType  state,
						     GimpDisplay     *gdisp);
static void   gimp_bucket_fill_tool_modifier_key    (GimpTool        *tool,
                                                     GdkModifierType  key,
                                                     gboolean         press,
						     GdkModifierType  state,
						     GimpDisplay     *gdisp);
static void   gimp_bucket_fill_tool_cursor_update   (GimpTool        *tool,
                                                     GimpCoords      *coords,
						     GdkModifierType  state,
						     GimpDisplay     *gdisp);

static GimpToolOptions * bucket_options_new         (GimpToolInfo    *tool_info);
static void              bucket_options_reset       (GimpToolOptions *tool_options);


/*  public functions  */

void
gimp_bucket_fill_tool_register (Gimp                     *gimp,
                                GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_BUCKET_FILL_TOOL,
                bucket_options_new,
                TRUE,
                "gimp:bucket_fill_tool",
                _("Bucket Fill"),
                _("Fill with a color or pattern"),
                N_("/Tools/Paint Tools/Bucket Fill"), "<shift>B",
                NULL, "tools/bucket_fill.html",
                GIMP_STOCK_TOOL_BUCKET_FILL);
}

GType
gimp_bucket_fill_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpBucketFillToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_bucket_fill_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBucketFillTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_bucket_fill_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpBucketFillTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_bucket_fill_tool_button_press;
  tool_class->button_release = gimp_bucket_fill_tool_button_release;
  tool_class->modifier_key   = gimp_bucket_fill_tool_modifier_key;
  tool_class->cursor_update  = gimp_bucket_fill_tool_cursor_update;
}

static void
gimp_bucket_fill_tool_init (GimpBucketFillTool *bucket_fill_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (bucket_fill_tool);

  tool->tool_cursor = GIMP_BUCKET_FILL_TOOL_CURSOR;
  tool->scroll_lock = TRUE;  /*  Disallow scrolling  */
}

static void
gimp_bucket_fill_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
				    GdkModifierType  state,
				    GimpDisplay     *gdisp)
{
  GimpBucketFillTool *bucket_tool;
  BucketOptions      *options;
  GimpDisplayShell   *shell;

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  options = (BucketOptions *) tool->tool_info->tool_options;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  bucket_tool->target_x = coords->x;
  bucket_tool->target_y = coords->y;

  if (! options->sample_merged)
    {
      gint off_x, off_y;

      gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                             &off_x, &off_y);

      bucket_tool->target_x -= off_x;
      bucket_tool->target_y -= off_y;
    }

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp = gdisp;
  tool->state = ACTIVE;
}

static void
gimp_bucket_fill_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
				      GdkModifierType  state,
				      GimpDisplay     *gdisp)
{
  GimpBucketFillTool *bucket_tool;
  BucketOptions      *options;
  Argument           *return_vals;
  gint                nreturn_vals;

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  options = (BucketOptions *) tool->tool_info->tool_options;

  gdk_pointer_ungrab (time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      GimpContext *context;

      context = gimp_get_current_context (gdisp->gimage->gimp);

      return_vals =
	procedural_db_run_proc (gdisp->gimage->gimp,
				"gimp_bucket_fill",
				&nreturn_vals,
				GIMP_PDB_DRAWABLE, gimp_drawable_get_ID (gimp_image_active_drawable (gdisp->gimage)),
				GIMP_PDB_INT32, (gint32) options->fill_mode,
				GIMP_PDB_INT32, (gint32) gimp_context_get_paint_mode (context),
				GIMP_PDB_FLOAT, (gdouble) gimp_context_get_opacity (context) * 100,
				GIMP_PDB_FLOAT, (gdouble) options->threshold,
				GIMP_PDB_INT32, (gint32) options->sample_merged,
				GIMP_PDB_FLOAT, (gdouble) bucket_tool->target_x,
				GIMP_PDB_FLOAT, (gdouble) bucket_tool->target_y,
				GIMP_PDB_END);

      if (return_vals && return_vals[0].value.pdb_int == GIMP_PDB_SUCCESS)
	gdisplays_flush ();
      else
	g_message (_("Bucket Fill operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  tool->state = INACTIVE;
}

static void
gimp_bucket_fill_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  BucketOptions *options;

  options = (BucketOptions *) tool->tool_info->tool_options;

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->fill_mode)
        {
        case FG_BUCKET_FILL:
          gimp_radio_group_set_active
            (GTK_RADIO_BUTTON (options->fill_mode_w[0]),
             GINT_TO_POINTER (BG_BUCKET_FILL));
          break;
        case BG_BUCKET_FILL:
          gimp_radio_group_set_active
            (GTK_RADIO_BUTTON (options->fill_mode_w[0]),
             GINT_TO_POINTER (FG_BUCKET_FILL));
          break;
        default:
          break;
        }
    }
}

static void
gimp_bucket_fill_tool_cursor_update (GimpTool        *tool,
                                     GimpCoords      *coords,
				     GdkModifierType  state,
				     GimpDisplay     *gdisp)
{
  BucketOptions      *options;
  GimpLayer          *layer;
  GimpCursorModifier  cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gint                off_x, off_y;

  options = (BucketOptions *) tool->tool_info->tool_options;

  if ((layer = gimp_image_get_active_layer (gdisp->gimage))) 
    {
      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (coords->x >= off_x &&
          coords->y >= off_y &&
	  coords->x < (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) &&
	  coords->y < (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimp_image_mask_is_empty (gdisp->gimage) ||
	      gimp_image_mask_value (gdisp->gimage, coords->x, coords->y))
	    {
	      switch (options->fill_mode)
		{
		case FG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
		  break;
		case BG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
		  break;
		case PATTERN_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_PATTERN;
		  break;
		}
	    }
	}
    }

  tool->cursor_modifier = cmodifier;

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static GimpToolOptions *
bucket_options_new (GimpToolInfo *tool_info)
{
  BucketOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  options = g_new0 (BucketOptions, 1);

  paint_options_init ((PaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = bucket_options_reset;

  options->sample_merged = options->sample_merged_d = FALSE;
  options->threshold                                = gimprc.default_threshold;
  options->fill_mode     = options->fill_mode_d     = FG_BUCKET_FILL;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /*  the sample merged toggle  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  g_signal_connect (G_OBJECT (options->sample_merged_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  /*  the threshold scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->threshold_w =
    gtk_adjustment_new (gimprc.default_threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (options->threshold_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);
  gtk_widget_show (scale);

  gtk_widget_show (hbox);

  /*  fill type  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Fill Type (<Ctrl>)"),
			   G_CALLBACK (gimp_radio_button_update),
			   &options->fill_mode,
			   GINT_TO_POINTER (options->fill_mode),

			   _("FG Color Fill"),
			   GINT_TO_POINTER (FG_BUCKET_FILL),
			   &options->fill_mode_w[0],
			   _("BG Color Fill"),
			   GINT_TO_POINTER (BG_BUCKET_FILL),
			   &options->fill_mode_w[1],
			   _("Pattern Fill"),
			   GINT_TO_POINTER (PATTERN_BUCKET_FILL),
			   &options->fill_mode_w[2],

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  bucket_options_reset ((GimpToolOptions *) options);

  return (GimpToolOptions *) options;
}

static void
bucket_options_reset (GimpToolOptions *tool_options)
{
  BucketOptions *options;

  options = (BucketOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    gimprc.default_threshold);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w[0]),
                               GINT_TO_POINTER (options->fill_mode_d));
}
