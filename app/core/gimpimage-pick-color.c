/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gui/info-dialog.h"
#include "gui/palette-editor.h"

#include "gimpdrawtool.h"
#include "gimpcolorpickertool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "gdisplay.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"

#define WANT_COLOR_PICKER_BITS
#include "icons.h"


/*  maximum information buffer size  */
#define MAX_INFO_BUF 8


/*  the color picker structures  */

typedef struct _GimpColorPickerToolOptions GimpColorPickerToolOptions;

struct _GimpColorPickerToolOptions
{
  GimpToolOptions  tool_options;

  gboolean     sample_merged;
  gboolean     sample_merged_d;
  GtkWidget   *sample_merged_w;

  gboolean     sample_average;
  gboolean     sample_average_d;
  GtkWidget   *sample_average_w;

  gdouble      average_radius;
  gdouble      average_radius_d;
  GtkObject   *average_radius_w;

  gboolean     update_active;
  gboolean     update_active_d;
  GtkWidget   *update_active_w;
};


/*  local function prototypes  */

static void 	  gimp_color_picker_tool_class_init     (GimpColorPickerToolClass *klass);
static void       gimp_color_picker_tool_init           (GimpColorPickerTool *color_picker_tool);

static void       gimp_color_picker_tool_destroy        (GtkObject      *object);

static void       gimp_color_picker_tool_button_press   (GimpTool       *tool,
							 GdkEventButton *bevent,
							 GDisplay       *gdisp);
							
static void       gimp_color_picker_tool_button_release (GimpTool       *tool,
					        	 GdkEventButton *bevent,
					        	 GDisplay       *gdisp);
					        	
static void       gimp_color_picker_tool_motion         (GimpTool       *tool,
					       		 GdkEventMotion *mevent,
					       		 GDisplay       *gdisp);
					       		
static void       gimp_color_picker_tool_cursor_update  (GimpTool       *tool,
					       		 GdkEventMotion *mevent,
					       		 GDisplay       *gdisp);
					       		
static void       gimp_color_picker_tool_control        (GimpTool       *tool,
                                                         ToolAction      action,
					       		 GDisplay       *gdisp);

static void       gimp_color_picker_tool_draw           (GimpDrawTool   *draw_tool);



static GimpColorPickerToolOptions * gimp_color_picker_tool_options_new  (void);

static void       gimp_color_picker_tool_options_reset   	(GimpToolOptions *tool_options);
static void   gimp_color_picker_tool_info_window_close_callback (GtkWidget *widget,
                                                                 gpointer   data);
static void       gimp_color_picker_tool_info_update    	(GimpTool   *tool,
                                                                 gboolean   valid);

					      			
					      			
static gboolean   pick_color_do               (GimpImage      *gimage,
					       GimpDrawable   *drawable,
					       gint            x,
					       gint            y,
					       gboolean        sample_merged,
					       gboolean        sample_average,
					       gdouble         average_radius,
					       gboolean        update_active,
					       gint            final);


/*  the color picker tool options  */
static GimpColorPickerToolOptions * gimp_color_picker_tool_options = NULL;

/*  the color value  */
gint col_value[5] = { 0, 0, 0, 0, 0 };

/*  the color picker dialog  */
static gint           update_type;
static GimpImageType  sample_type;
static InfoDialog    *gimp_color_picker_tool_info = NULL;
static GtkWidget     *color_area        = NULL;
static gchar          red_buf  [MAX_INFO_BUF];
static gchar          green_buf[MAX_INFO_BUF];
static gchar          blue_buf [MAX_INFO_BUF];
static gchar          alpha_buf[MAX_INFO_BUF];
static gchar          index_buf[MAX_INFO_BUF];
static gchar          gray_buf [MAX_INFO_BUF];
static gchar          hex_buf  [MAX_INFO_BUF];

static GimpDrawToolClass *parent_class = NULL;


void
gimp_color_picker_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_COLOR_PICKER_TOOL,
                              FALSE,
			      "gimp:color_picker_tool",
			      _("Color Picker"),
			      _("Pick colors from the image"),
			      N_("/Tools/Color Picker"), "<shift>O",
			      NULL, "tools/color_picker.html",
			      (const gchar **) color_picker_bits);
}

GtkType
gimp_color_picker_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpColorPickerTool",
        sizeof (GimpColorPickerTool),
        sizeof (GimpColorPickerToolClass),
        (GtkClassInitFunc) gimp_color_picker_tool_class_init,
        (GtkObjectInitFunc) gimp_color_picker_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_DRAW_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_color_picker_tool_class_init (GimpColorPickerToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;
  draw_class   = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAW_TOOL);

  object_class->destroy = gimp_color_picker_tool_destroy;

  tool_class->control        = gimp_color_picker_tool_control;
  tool_class->button_press   = gimp_color_picker_tool_button_press;
  tool_class->button_release = gimp_color_picker_tool_button_release;
  tool_class->motion         = gimp_color_picker_tool_motion;
  tool_class->cursor_update  = gimp_color_picker_tool_cursor_update;

  draw_class->draw	     = gimp_color_picker_tool_draw;
}

static void
gimp_color_picker_tool_init (GimpColorPickerTool *color_picker_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (color_picker_tool);

  if (! gimp_color_picker_tool_options)
    {
      gimp_color_picker_tool_options = gimp_color_picker_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_COLOR_PICKER_TOOL,
					  (GimpToolOptions *) gimp_color_picker_tool_options);
    }

  tool->preserve = FALSE;  /*  Don't preserve on drawable change  */
}

static void
gimp_color_picker_tool_destroy (GtkObject *object)
{
  GimpTool        	*tool;
  GimpDrawTool          *draw_tool;

  tool              = GIMP_TOOL (object);
  draw_tool         = GIMP_DRAW_TOOL (object);


  if (tool->state == ACTIVE)
    gimp_draw_tool_stop (draw_tool);

  if (gimp_color_picker_tool_info)
    {
      info_dialog_free (gimp_color_picker_tool_info);
      gimp_color_picker_tool_info = NULL;

      color_area = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_color_picker_tool_options_reset (GimpToolOptions *tool_options)
{
  GimpColorPickerToolOptions *options;

  options = (GimpColorPickerToolOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->average_radius_w),
			    options->average_radius_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active_d);
}

static GimpColorPickerToolOptions *
gimp_color_picker_tool_options_new (void)
{
  GimpColorPickerToolOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;

  options = g_new0 (GimpColorPickerToolOptions, 1);
  tool_options_init ((GimpToolOptions *) options,
		     gimp_color_picker_tool_options_reset);

  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->sample_average = options->sample_average_d = FALSE;
  options->average_radius = options->average_radius_d = 1.0;
  options->update_active  = options->update_active_d  = TRUE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the sample merged toggle button  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->sample_merged);
  gtk_widget_show (options->sample_merged_w);

  /*  the sample average options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->sample_average_w =
    gtk_check_button_new_with_label (_("Sample Average"));
  gtk_table_attach (GTK_TABLE (table), options->sample_average_w, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average_d);
  gtk_signal_connect (GTK_OBJECT (options->sample_average_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->sample_average);
  gtk_widget_show (options->sample_average_w);

  label = gtk_label_new (_("Radius:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the feather radius scale  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 0, 2,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->average_radius_w =
    gtk_adjustment_new (options->average_radius_d, 1.0, 15.0, 2.0, 2.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->average_radius_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_widget_set_sensitive (scale, options->sample_average_d);
  gtk_object_set_data (GTK_OBJECT (options->sample_average_w), "set_sensitive",
		       scale);
  gtk_widget_set_sensitive (label, options->sample_average_d);
  gtk_object_set_data (GTK_OBJECT (scale), "set_sensitive",
		       label);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->average_radius_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->average_radius);
  gtk_widget_show (scale);
  gtk_widget_show (table);

  /*  the update active color toggle button  */
  options->update_active_w =
    gtk_check_button_new_with_label (_("Update Active Color"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->update_active_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->update_active_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->update_active);
  gtk_widget_show (options->update_active_w);

  return options;
}

static void
gimp_color_picker_tool_button_press (GimpTool       *tool,
				     GdkEventButton *bevent,
			   	     GDisplay       *gdisp)
{
  GimpColorPickerTool *cp_tool;
  gint                 x, y;

  cp_tool = GIMP_COLOR_PICKER_TOOL(tool);

  /*  Make the tool active and set it's gdisplay & drawable  */
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);
  tool->state    = ACTIVE;

  /*  create the info dialog if it doesn't exist  */
  if (! gimp_color_picker_tool_info)
    {
      GtkWidget *hbox;
      GtkWidget *frame;
      GimpRGB    color;

      gimp_color_picker_tool_info = info_dialog_new (_("Color Picker"),
					   tool_manager_help_func, NULL);

      /*  if the gdisplay is for a color image, the dialog must have RGB  */
      switch (gimp_drawable_type (tool->drawable))
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Red:"), red_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Green:"), green_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Blue:"), blue_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Hex Triplet:"), hex_buf);
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Index:"), index_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Red:"), red_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Green:"), green_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Blue:"), blue_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Hex Triplet"), hex_buf);
	  break;

	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Intensity:"), gray_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (gimp_color_picker_tool_info, _("Hex Triplet:"), hex_buf);
	  break;

	default :
	  break;
	}

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (gimp_color_picker_tool_info->vbox), hbox,
			  FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      gtk_widget_reparent (gimp_color_picker_tool_info->info_table, hbox);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

      gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);
      color_area =
	gimp_color_area_new (&color,
			     gimp_drawable_has_alpha (tool->drawable) ?
			     GIMP_COLOR_AREA_LARGE_CHECKS :
			     GIMP_COLOR_AREA_FLAT,
			     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
      gtk_widget_set_usize (color_area, 48, 64);
      gtk_drag_dest_unset (color_area);
      gtk_container_add (GTK_CONTAINER (frame), color_area);
      gtk_widget_show (color_area);
      gtk_widget_show (frame);

      /*  create the action area  */
      gimp_dialog_create_action_area
	(GTK_DIALOG (gimp_color_picker_tool_info->shell),

	 _("Close"), gimp_color_picker_tool_info_window_close_callback,
	 gimp_color_picker_tool_info, NULL, NULL, TRUE, FALSE,

	 NULL);
    }

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &cp_tool->centerx, &cp_tool->centery, FALSE, 1);

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    (GDK_POINTER_MOTION_HINT_MASK |
		     GDK_BUTTON1_MOTION_MASK |
		     GDK_BUTTON_RELEASE_MASK),
		    NULL, NULL, bevent->time);

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y,
			       FALSE, FALSE);

  /*  if the shift key is down, create a new color.
   *  otherwise, modify the current color.
   */
  if (bevent->state & GDK_SHIFT_MASK)
    {
      gimp_color_picker_tool_info_update
	(tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			      gimp_color_picker_tool_options->sample_merged,
			      gimp_color_picker_tool_options->sample_average,
			      gimp_color_picker_tool_options->average_radius,
			      gimp_color_picker_tool_options->update_active,
			      COLOR_NEW));
      update_type = COLOR_UPDATE_NEW;
    }
  else
    {
      gimp_color_picker_tool_info_update
	(tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			      gimp_color_picker_tool_options->sample_merged,
			      gimp_color_picker_tool_options->sample_average,
			      gimp_color_picker_tool_options->average_radius,
			      gimp_color_picker_tool_options->update_active,
			      COLOR_UPDATE));
      update_type = COLOR_UPDATE;
    }

  /*  Start drawing the colorpicker tool  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL(cp_tool), gdisp->canvas->window);
}

static void
gimp_color_picker_tool_button_release (GimpTool       *tool,
				       GdkEventButton *bevent,
				       GDisplay       *gdisp)
{
  GimpColorPickerTool *cp_tool;
  gint             x, y;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  cp_tool = GIMP_COLOR_PICKER_TOOL(tool);

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y,
			       FALSE, FALSE);

  gimp_color_picker_tool_info_update
    (tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			  gimp_color_picker_tool_options->sample_merged,
			  gimp_color_picker_tool_options->sample_average,
			  gimp_color_picker_tool_options->average_radius,
			  gimp_color_picker_tool_options->update_active,
			  update_type));

  gimp_draw_tool_stop (GIMP_DRAW_TOOL(cp_tool));
  tool->state = INACTIVE;
}

static void
gimp_color_picker_tool_motion (GimpTool       *tool,
		               GdkEventMotion *mevent,
		               GDisplay       *gdisp)
{
  GimpColorPickerTool *cp_tool;
  gint             x, y;

  cp_tool = GIMP_COLOR_PICKER_TOOL(tool);

  /*  undraw the current tool  */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL(cp_tool));

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y,
			       FALSE, FALSE);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &cp_tool->centerx, &cp_tool->centery,
			       FALSE, TRUE);

  gimp_color_picker_tool_info_update
    (tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			  gimp_color_picker_tool_options->sample_merged,
			  gimp_color_picker_tool_options->sample_average,
			  gimp_color_picker_tool_options->average_radius,
			  gimp_color_picker_tool_options->update_active,
			  update_type));

  /*  redraw the current tool  */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL(cp_tool));
}

static void
gimp_color_picker_tool_cursor_update (GimpTool       *tool,
			              GdkEventMotion *mevent,
			              GDisplay       *gdisp)
{
  gint x, y;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y,
			       FALSE, FALSE);

  /*  We used to use the following code here:
   *
   *  if (gimp_image_pick_correlate_layer (gdisp->gimage, x, y)) { ... }
   */

  if (gdisp->gimage &&
      x > 0 && x < gdisp->gimage->width &&
      y > 0 && y < gdisp->gimage->height)
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_COLOR_PICKER_CURSOR,
				    GIMP_COLOR_PICKER_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_NONE);
    }
  else
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_BAD_CURSOR,
				    GIMP_COLOR_PICKER_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_NONE);
    }
}

static void
gimp_color_picker_tool_control (GimpTool    *tool,
				ToolAction  action,
		      		GDisplay   *gdisp)
{
  switch (action)
    {
    case PAUSE :
      break;

    case RESUME :
      break;

    case HALT :
      info_dialog_popdown (gimp_color_picker_tool_info);
      break;

    default:
      break;
    }

  if (GIMP_TOOL_CLASS (parent_class)->control)
    GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

typedef guchar * (*GetColorFunc) (GimpObject *object,
				  gint        x,
				  gint        y);


static gboolean
pick_color_do (GimpImage    *gimage,
	       GimpDrawable *drawable,
	       gint          x,
	       gint          y,
	       gboolean      sample_merged,
	       gboolean      sample_average,
	       gdouble       average_radius,
	       gboolean      update_active,
	       gint          final)
{
  guchar       *color;
  gint          offx, offy;
  gint          has_alpha;
  gint          is_indexed;
  GetColorFunc  get_color_func;
  GimpObject   *get_color_obj;


  if (!drawable && !sample_merged)
    return FALSE;

  if (!sample_merged)
    {
      gimp_drawable_offsets (drawable, &offx, &offy);
      x -= offx;
      y -= offy;

      sample_type = gimp_drawable_type (drawable);
      is_indexed = gimp_drawable_is_indexed (drawable);

      get_color_func = (GetColorFunc) gimp_drawable_get_color_at;
      get_color_obj = GIMP_OBJECT (drawable);
    }
  else
    {
      sample_type = gimp_image_composite_type (gimage);
      is_indexed = FALSE;

      get_color_func = (GetColorFunc) gimp_image_get_color_at;
      get_color_obj = GIMP_OBJECT (gimage);
    }

  has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (sample_type);

  if (!(color = (* get_color_func) (get_color_obj, x, y)))
    return FALSE;

  if (sample_average)
    {
      gint    i, j;
      gint    count = 0;
      gint    color_avg[4] = { 0, 0, 0, 0 };
      guchar *tmp_color;
      gint    radius = (gint) average_radius;

      for (i = x - radius; i <= x + radius; i++)
	for (j = y - radius; j <= y + radius; j++)
	  if ((tmp_color = (* get_color_func) (get_color_obj, i, j)))
	    {
	      count++;
	
	      color_avg[RED_PIX]   += tmp_color[RED_PIX];
	      color_avg[GREEN_PIX] += tmp_color[GREEN_PIX];
	      color_avg[BLUE_PIX]  += tmp_color[BLUE_PIX];
	      if (has_alpha)
		color_avg[ALPHA_PIX] += tmp_color[ALPHA_PIX];

	      g_free (tmp_color);
	    }

      color[RED_PIX]   = (guchar) (color_avg[RED_PIX] / count);
      color[GREEN_PIX] = (guchar) (color_avg[GREEN_PIX] / count);
      color[BLUE_PIX]  = (guchar) (color_avg[BLUE_PIX] / count);
      if (has_alpha)
	color[ALPHA_PIX] = (guchar) (color_avg[ALPHA_PIX] / count);

      is_indexed = FALSE;
    }

  col_value[RED_PIX]   = color[RED_PIX];
  col_value[GREEN_PIX] = color[GREEN_PIX];
  col_value[BLUE_PIX]  = color[BLUE_PIX];
  if (has_alpha)
    col_value[ALPHA_PIX] = color[ALPHA_PIX];
  if (is_indexed)
    col_value[4] = color[4];

  if (update_active)
    palette_set_active_color (col_value [RED_PIX],
			      col_value [GREEN_PIX],
			      col_value [BLUE_PIX],
			      final);

  g_free (color);

  return TRUE;
}

gboolean
pick_color (GimpImage    *gimage,
	    GimpDrawable *drawable,
	    gint          x,
	    gint          y,
	    gboolean      sample_merged,
	    gboolean      sample_average,
	    gdouble       average_radius,
	    gint          final)
{
  return pick_color_do (gimage, drawable,
			x, y,
			sample_merged,
			sample_average,
			average_radius,
			TRUE,
			final);
}

static void
gimp_color_picker_tool_draw (GimpDrawTool *draw_tool)
{
  GimpColorPickerTool *cp_tool;
  GimpTool            *tool;
  gint                 tx, ty;
  gint                 radiusx, radiusy;
  gint                 cx, cy;

  if (! gimp_color_picker_tool_options->sample_average)
    return;

  cp_tool = GIMP_COLOR_PICKER_TOOL (draw_tool);
  tool    = GIMP_TOOL (draw_tool);

  gdisplay_transform_coords (tool->gdisp, cp_tool->centerx, cp_tool->centery,
			     &tx, &ty, TRUE);

  radiusx = SCALEX (tool->gdisp, gimp_color_picker_tool_options->average_radius);
  radiusy = SCALEY (tool->gdisp, gimp_color_picker_tool_options->average_radius);
  cx      = SCALEX (tool->gdisp, 1);
  cy      = SCALEY (tool->gdisp, 1);

  /*  Draw the circle around the collecting area */
  gdk_draw_rectangle (draw_tool->win, draw_tool->gc, 0,
		      tx - radiusx,
		      ty - radiusy,
		      2 * radiusx + cx, 2 * radiusy + cy);

  if (radiusx > 1 && radiusy > 1)
    {
      gdk_draw_rectangle (draw_tool->win, draw_tool->gc, 0,
			  tx - radiusx + 2,
			  ty - radiusy + 2,
			  2 * radiusx + cx - 4, 2 * radiusy + cy - 4);
    }
}

static void
gimp_color_picker_tool_info_update (GimpTool  *tool,
				    gboolean  valid)
{
  if (!valid)
    {
      if (GTK_WIDGET_IS_SENSITIVE (color_area))
	gtk_widget_set_sensitive (color_area, FALSE);

      g_snprintf (red_buf,   MAX_INFO_BUF, _("N/A"));
      g_snprintf (green_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (blue_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (gray_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (hex_buf,   MAX_INFO_BUF, _("N/A"));
    }
  else
    {
      GimpRGB  color;
      guchar   r = 0;
      guchar   g = 0;
      guchar   b = 0;
      guchar   a = 0;

      if (! GTK_WIDGET_IS_SENSITIVE (color_area))
	gtk_widget_set_sensitive (color_area, TRUE);

      switch (sample_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == RGBA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);

	  r = col_value [RED_PIX];
	  g = col_value [GREEN_PIX];
	  b = col_value [BLUE_PIX];
	  if (sample_type == RGBA_GIMAGE)
	    a = col_value [ALPHA_PIX];
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  g_snprintf (index_buf, MAX_INFO_BUF, "%d", col_value [4]);
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == INDEXEDA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf,   MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);

	  r = col_value [RED_PIX];
	  g = col_value [GREEN_PIX];
	  b = col_value [BLUE_PIX];
	  if (sample_type == INDEXEDA_GIMAGE)
	    a = col_value [ALPHA_PIX];
	  break;

	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  g_snprintf (gray_buf, MAX_INFO_BUF, "%d", col_value [GRAY_PIX]);
	  if (sample_type == GRAYA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX]);

	  r = col_value [GRAY_PIX];
	  g = col_value [GRAY_PIX];
	  b = col_value [GRAY_PIX];
	  if (sample_type == GRAYA_GIMAGE)
	    a = col_value [ALPHA_PIX];
	  break;
	}

      gimp_rgba_set_uchar (&color, r, g, b, a);

      gimp_color_area_set_color (GIMP_COLOR_AREA (color_area), &color);
    }

  info_dialog_update (gimp_color_picker_tool_info);
  info_dialog_popup (gimp_color_picker_tool_info);
}

static void
gimp_color_picker_tool_info_window_close_callback (GtkWidget *widget,
						   gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}
