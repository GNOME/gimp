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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpbucketfilltool.h"
#include "paint_options.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _BucketOptions BucketOptions;

struct _BucketOptions
{
  GimpPaintOptions    paint_options;

  gboolean            fill_transparent;
  gboolean            fill_transparent_d;
  GtkWidget          *fill_transparent_w;

  gboolean            sample_merged;
  gboolean            sample_merged_d;
  GtkWidget          *sample_merged_w;

  gdouble             threshold;
  GtkObject          *threshold_w;

  GimpBucketFillMode  fill_mode;
  GimpBucketFillMode  fill_mode_d;
  GtkWidget          *fill_mode_w;
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
gimp_bucket_fill_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data) 
{
  (* callback) (GIMP_TYPE_BUCKET_FILL_TOOL,
                bucket_options_new,
                TRUE,
                "gimp-bucket-fill-tool",
                _("Bucket Fill"),
                _("Fill with a color or pattern"),
                N_("/Tools/Paint Tools/Bucket Fill"), "<shift>B",
                NULL, "tools/bucket_fill.html",
                GIMP_STOCK_TOOL_BUCKET_FILL,
                data);
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

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
				     GIMP_BUCKET_FILL_TOOL_CURSOR);
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

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  options = (BucketOptions *) tool->tool_info->tool_options;

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

  tool->gdisp = gdisp;
  gimp_tool_control_activate (tool->control);
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

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  options = (BucketOptions *) tool->tool_info->tool_options;

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      GimpContext *context;

      context = gimp_get_current_context (gdisp->gimage->gimp);

      gimp_drawable_bucket_fill (gimp_image_active_drawable (gdisp->gimage),
                                 options->fill_mode,
                                 gimp_context_get_paint_mode (context),
                                 gimp_context_get_opacity (context),
                                 options->fill_transparent,
                                 options->threshold,
                                 options->sample_merged,
                                 bucket_tool->target_x,
                                 bucket_tool->target_y);

      gimp_image_flush (gdisp->gimage);
    }

  gimp_tool_control_halt (tool->control);    /* sets paused_count to 0 -- is this ok? */
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
        case GIMP_FG_BUCKET_FILL:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                                       GINT_TO_POINTER (GIMP_BG_BUCKET_FILL));
          break;
        case GIMP_BG_BUCKET_FILL:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                                       GINT_TO_POINTER (GIMP_FG_BUCKET_FILL));
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
		case GIMP_FG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
		  break;
		case GIMP_BG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
		  break;
		case GIMP_PATTERN_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_PATTERN;
		  break;
		}
	    }
	}
    }

  gimp_tool_control_set_cursor_modifier (tool->control, cmodifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static GimpToolOptions *
bucket_options_new (GimpToolInfo *tool_info)
{
  BucketOptions *options;
  GtkWidget     *vbox;
  GtkWidget     *vbox2;
  GtkWidget     *table;
  GtkWidget     *frame;
  gchar         *str;

  options = g_new0 (BucketOptions, 1);

  gimp_paint_options_init ((GimpPaintOptions *) options, tool_info->context);

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = bucket_options_reset;

  options->fill_transparent = options->fill_transparent_d = TRUE;
  options->sample_merged    = options->sample_merged_d    = FALSE;
  options->threshold        =
    GIMP_GUI_CONFIG (tool_info->gimp->config)->default_threshold;
  options->fill_mode        = options->fill_mode_d = GIMP_FG_BUCKET_FILL;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  str = g_strdup_printf (_("Fill Type  %s"), gimp_get_mod_name_control ());

  /*  fill type  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_BUCKET_FILL_MODE,
                                     gtk_label_new (str),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->fill_mode,
                                     &options->fill_mode_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                               GINT_TO_POINTER (options->fill_mode));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  frame = gtk_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  the fill transparent areas toggle  */
  options->fill_transparent_w =
    gtk_check_button_new_with_label (_("Fill Transparent Areas"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_transparent_w),
                                options->fill_transparent);
  gtk_box_pack_start (GTK_BOX (vbox2), options->fill_transparent_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->fill_transparent_w);

  gimp_help_set_help_data (options->fill_transparent_w,
                           _("Allow completely transparent regions "
                             "to be filled"), NULL);

  g_signal_connect (G_OBJECT (options->fill_transparent_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->fill_transparent);

  /*  the sample merged toggle  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_box_pack_start (GTK_BOX (vbox2), options->sample_merged_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  gimp_help_set_help_data (options->sample_merged_w,
			   _("Base filled area on all visible layers"), NULL);

  g_signal_connect (G_OBJECT (options->sample_merged_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->sample_merged);

  /*  the threshold scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Threshold:"), -1, -1,
					       options->threshold,
					       0.0, 255.0, 1.0, 16.0, 1,
					       TRUE, 0.0, 0.0,
					       _("Maximum color difference"),
					       NULL);

  g_signal_connect (G_OBJECT (options->threshold_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);

  bucket_options_reset ((GimpToolOptions *) options);

  return (GimpToolOptions *) options;
}

static void
bucket_options_reset (GimpToolOptions *tool_options)
{
  BucketOptions *options;

  options = (BucketOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_transparent_w),
				options->fill_transparent_d);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config)->default_threshold);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->fill_mode_w),
                               GINT_TO_POINTER (options->fill_mode_d));
}
