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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/curves.h"
#include "base/gimphistogram.h"
#include "base/gimplut.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpenummenu.h"

#include "display/gimpdisplay.h"

#include "gimpcurvestool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


#define GRAPH          0x1
#define XRANGE_TOP     0x2
#define XRANGE_BOTTOM  0x4
#define YRANGE         0x8
#define DRAW          0x10
#define ALL           0xFF

/*  NB: take care when changing these values: make sure the curve[] array in
 *  base/curves.h is large enough.
 */
#define GRAPH_WIDTH    256
#define GRAPH_HEIGHT   256
#define XRANGE_WIDTH   256
#define XRANGE_HEIGHT   16
#define YRANGE_WIDTH    16
#define YRANGE_HEIGHT  256
#define RADIUS           3
#define MIN_DISTANCE     8

#define GRAPH_MASK  GDK_EXPOSURE_MASK | \
		    GDK_POINTER_MOTION_MASK | \
		    GDK_POINTER_MOTION_HINT_MASK | \
                    GDK_ENTER_NOTIFY_MASK | \
		    GDK_BUTTON_PRESS_MASK | \
		    GDK_BUTTON_RELEASE_MASK | \
		    GDK_BUTTON1_MOTION_MASK


/*  local function prototypes  */

static void   gimp_curves_tool_class_init     (GimpCurvesToolClass *klass);
static void   gimp_curves_tool_init           (GimpCurvesTool      *c_tool);

static void   gimp_curves_tool_finalize       (GObject          *object);

static void   gimp_curves_tool_initialize     (GimpTool         *tool,
					       GimpDisplay      *gdisp);
static void   gimp_curves_tool_button_press   (GimpTool         *tool,
                                               GimpCoords       *coords,
                                               guint32           time,
					       GdkModifierType   state,
					       GimpDisplay      *gdisp);
static void   gimp_curves_tool_button_release (GimpTool         *tool,
                                               GimpCoords       *coords,
                                               guint32           time,
					       GdkModifierType   state,
					       GimpDisplay      *gdisp);
static void   gimp_curves_tool_motion         (GimpTool         *tool,
                                               GimpCoords       *coords,
                                               guint32           time,
					       GdkModifierType   state,
					       GimpDisplay      *gdisp);

static void   gimp_curves_tool_map            (GimpImageMapTool *image_map_tool);
static void   gimp_curves_tool_dialog         (GimpImageMapTool *image_map_tool);
static void   gimp_curves_tool_reset          (GimpImageMapTool *image_map_tool);

static void   curves_color_update             (GimpTool         *tool,
					       GimpDisplay      *gdisp,
					       GimpDrawable     *drawable,
					       gint              x,
					       gint              y);
static void   curves_add_point                (GimpCurvesTool   *c_tool,
					       gint              x,
					       gint              y,
					       gint              cchan);

static void   curves_update                   (GimpCurvesTool   *c_tool,
                                               gint              update);

static void   curves_channel_callback         (GtkWidget        *widget,
                                               gpointer          data);
static void   curves_channel_reset_callback   (GtkWidget        *widget,
                                               gpointer          data);

static gboolean curves_set_sensitive_callback (gpointer          item_data,
                                               GimpCurvesTool   *c_tool);
static void   curves_smooth_callback          (GtkWidget        *widget,
                                               gpointer          data);
static void   curves_free_callback            (GtkWidget        *widget,
                                               gpointer          data);

static void   curves_load_callback            (GtkWidget        *widget,
                                               gpointer          data);
static void   curves_save_callback            (GtkWidget        *widget,
                                               gpointer          data);
static gint   curves_graph_events             (GtkWidget        *widget,
                                               GdkEvent         *event,
                                               GimpCurvesTool   *c_tool);

static void       file_dialog_create          (GimpCurvesTool   *c_tool);
static void       file_dialog_ok_callback     (GimpCurvesTool   *c_tool);
static gboolean   file_dialog_cancel_callback (GimpCurvesTool   *c_tool);

static gboolean   curves_read_from_file       (GimpCurvesTool   *c_tool,
                                               FILE             *file);
static void       curves_write_to_file        (GimpCurvesTool   *c_tool,
                                               FILE             *file);


static GimpImageMapToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_curves_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_CURVES_TOOL,
                NULL,
                FALSE,
                "gimp-curves-tool",
                _("Curves"),
                _("Adjust color curves"),
                N_("/Layer/Colors/Curves..."), NULL,
                NULL, "tools/curves.html",
                GIMP_STOCK_TOOL_CURVES,
                data);
}

GType
gimp_curves_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpCurvesToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_curves_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCurvesTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_curves_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpCurvesTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_curves_tool_class_init (GimpCurvesToolClass *klass)
{
  GObjectClass          *object_class;
  GimpToolClass         *tool_class;
  GimpImageMapToolClass *image_map_tool_class;

  object_class         = G_OBJECT_CLASS (klass);
  tool_class           = GIMP_TOOL_CLASS (klass);
  image_map_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize       = gimp_curves_tool_finalize;

  tool_class->initialize       = gimp_curves_tool_initialize;
  tool_class->button_press     = gimp_curves_tool_button_press;
  tool_class->button_release   = gimp_curves_tool_button_release;
  tool_class->motion           = gimp_curves_tool_motion;

  image_map_tool_class->map    = gimp_curves_tool_map;
  image_map_tool_class->dialog = gimp_curves_tool_dialog;
  image_map_tool_class->reset  = gimp_curves_tool_reset;
}

static void
gimp_curves_tool_init (GimpCurvesTool *c_tool)
{
  GimpImageMapTool *image_map_tool;
  gint              i;

  image_map_tool = GIMP_IMAGE_MAP_TOOL (c_tool);

  image_map_tool->shell_desc  = _("Adjust Color Curves");

  c_tool->curves              = g_new0 (Curves, 1);
  c_tool->lut                 = gimp_lut_new ();
  c_tool->channel             = GIMP_HISTOGRAM_VALUE;

  curves_init (c_tool->curves);

  for (i = 0;
       i < (sizeof (c_tool->col_value) /
            sizeof (c_tool->col_value[0]));
       i++)
    c_tool->col_value[i] = 0;
}

static void
gimp_curves_tool_finalize (GObject *object)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (object);

  if (c_tool->curves)
    {
      g_free(c_tool->curves);
      c_tool->curves = NULL;
    }

  if (c_tool->lut)
    {
      gimp_lut_free (c_tool->lut);
      c_tool->lut = NULL;
    }

  if (c_tool->pixmap)
    {
      g_object_unref (c_tool->pixmap);
      c_tool->pixmap = NULL;
    }

  if (c_tool->cursor_layout)
    {
      g_object_unref (c_tool->cursor_layout);
      c_tool->cursor_layout = NULL;
    }

  if (c_tool->xpos_layout)
    {
      g_object_unref (c_tool->xpos_layout);
      c_tool->xpos_layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curves_tool_initialize (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  GimpCurvesTool *c_tool;
  GimpDrawable   *drawable;

  c_tool = GIMP_CURVES_TOOL (tool);

  if (! gdisp)
    {
      GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);
      return;
    }

  if (gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Curves for indexed drawables cannot be adjusted."));
      return;
    }

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curves_init (c_tool->curves);

  c_tool->color   = gimp_drawable_is_rgb (drawable);
  c_tool->channel = GIMP_HISTOGRAM_VALUE;

  c_tool->grab_point = -1;
  c_tool->last       = 0;

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);

  /* set the sensitivity of the channel menu based on the drawable type */
  gimp_option_menu_set_sensitive (GTK_OPTION_MENU (c_tool->channel_menu),
                                  (GimpOptionMenuSensitivityCallback) curves_set_sensitive_callback,
                                  c_tool);

  /* set the current selection */
  gimp_option_menu_set_history (GTK_OPTION_MENU (c_tool->channel_menu),
                                GINT_TO_POINTER (c_tool->channel));

  curves_update (c_tool, ALL);
}

static void
gimp_curves_tool_button_press (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  GimpCurvesTool *c_tool;
  GimpDrawable   *drawable;

  c_tool = GIMP_CURVES_TOOL (tool);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  tool->gdisp = gdisp;

  g_assert (drawable == tool->drawable);

#if 0
  if (drawable != tool->drawable)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);
      gimp_image_map_abort (GIMP_IMAGE_MAP_TOOL (tool)->image_map);
      gimp_tool_control_set_preserve (tool->control, FALSE);

      tool->drawable = drawable;

      c_tool->color  = gimp_drawable_is_rgb (drawable);

      GIMP_IMAGE_MAP_TOOL (tool)->drawable  = drawable;
      GIMP_IMAGE_MAP_TOOL (tool)->image_map = gimp_image_map_new (TRUE, drawable);

      gimp_option_menu_set_sensitive 
        (GTK_OPTION_MENU (c_tool->channel_menu),
         (GimpOptionMenuSensitivityCallback) curves_set_sensitive_callback,
         c_tool);
    }
#endif

  gimp_tool_control_activate (tool->control);

  curves_color_update (tool, gdisp, drawable, coords->x, coords->y);
  curves_update (c_tool, GRAPH | DRAW);
}

static void
gimp_curves_tool_button_release (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
				 GdkModifierType  state,
				 GimpDisplay     *gdisp)
{
  GimpCurvesTool *c_tool;
  GimpDrawable   *drawable;

  c_tool = GIMP_CURVES_TOOL (tool);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curves_color_update (tool, gdisp, drawable, coords->x, coords->y);

  if (state & GDK_SHIFT_MASK)
    {
      curves_add_point (c_tool, coords->x, coords->y, c_tool->channel);
      curves_calculate_curve (c_tool->curves, c_tool->channel);
    }
  else if (state & GDK_CONTROL_MASK)
    {
      gint i;

      for (i = 0; i < 5; i++)
        {
          curves_add_point (c_tool, coords->x, coords->y, i);
          curves_calculate_curve (c_tool->curves, c_tool->channel);
        }
    }

  gimp_tool_control_halt (tool->control);

  curves_update (c_tool, GRAPH | DRAW);
}

static void
gimp_curves_tool_motion (GimpTool        *tool,
                         GimpCoords      *coords,
                         guint32          time,
			 GdkModifierType  state,
			 GimpDisplay     *gdisp)
{
  GimpCurvesTool *c_tool;
  GimpDrawable   *drawable;

  c_tool = GIMP_CURVES_TOOL (tool);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curves_color_update (tool, gdisp, drawable, coords->x, coords->y);
  curves_update (c_tool, GRAPH | DRAW);
}

static void
curves_color_update (GimpTool       *tool,
                     GimpDisplay    *gdisp,
                     GimpDrawable   *drawable,
                     gint            x,
                     gint            y)
{
  GimpCurvesTool *c_tool;
  guchar         *color;
  gint            offx;
  gint            offy;
  gint            maxval;
  gboolean        has_alpha;
  gboolean        is_indexed;
  GimpImageType   sample_type;

  c_tool = GIMP_CURVES_TOOL (tool);

  if (! (tool && gimp_tool_control_is_active (tool->control)))
    return;

  gimp_drawable_offsets (drawable, &offx, &offy);

  x -= offx;
  y -= offy;

  if (! (color = gimp_image_map_get_color_at (GIMP_IMAGE_MAP_TOOL (tool)->image_map,
                                              x, y)))
    return;

  sample_type = gimp_drawable_type (drawable);
  is_indexed  = gimp_drawable_is_indexed (drawable);
  has_alpha   = GIMP_IMAGE_TYPE_HAS_ALPHA (sample_type);

  c_tool->col_value[GIMP_HISTOGRAM_RED]   = color[RED_PIX];
  c_tool->col_value[GIMP_HISTOGRAM_GREEN] = color[GREEN_PIX];
  c_tool->col_value[GIMP_HISTOGRAM_BLUE]  = color[BLUE_PIX];

  if (has_alpha)
    c_tool->col_value[GIMP_HISTOGRAM_ALPHA] = color[3];

  if (is_indexed)
    c_tool->col_value[GIMP_HISTOGRAM_ALPHA] = color[4];

  maxval = MAX (color[RED_PIX], color[GREEN_PIX]);
  c_tool->col_value[GIMP_HISTOGRAM_VALUE] = MAX (maxval, color[BLUE_PIX]);

  g_free (color);
}

static void
curves_add_point (GimpCurvesTool *c_tool,
		  gint            x,
		  gint            y,
		  gint            cchan)
{
  /* Add point onto the curve */
  gint closest_point = 0;
  gint distance;
  gint curvex;
  gint i;

  switch (c_tool->curves->curve_type[cchan])
    {
    case CURVES_SMOOTH:
      curvex   = c_tool->col_value[cchan];
      distance = G_MAXINT;

      for (i = 0; i < 17; i++)
	{
	  if (c_tool->curves->points[cchan][i][0] != -1)
	    if (abs (curvex - c_tool->curves->points[cchan][i][0]) < distance)
	      {
		distance = abs (curvex - c_tool->curves->points[cchan][i][0]);
		closest_point = i;
	      }
	}
      
      if (distance > MIN_DISTANCE)
	closest_point = (curvex + 8) / 16;
      
      c_tool->curves->points[cchan][closest_point][0] = curvex;
      c_tool->curves->points[cchan][closest_point][1] = c_tool->curves->curve[cchan][curvex];
      break;
      
    case CURVES_FREE:
      c_tool->curves->curve[cchan][x] = 255 - y;
      break;
    }
}

static void
gimp_curves_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (image_map_tool);

  gimp_lut_setup (c_tool->lut, 
		  (GimpLutFunc) curves_lut_func,
                  c_tool->curves,
		  gimp_drawable_bytes (image_map_tool->drawable));
  gimp_image_map_apply (image_map_tool->image_map,
                        (GimpImageMapApplyFunc) gimp_lut_process_2,
                        c_tool->lut);
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
gimp_curves_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *c_tool;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *hbbox;
  GtkWidget      *frame;
  GtkWidget      *table;
  GtkWidget      *button;

  c_tool = GIMP_CURVES_TOOL (image_map_tool);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The option menu for selecting channels  */
  hbox = gtk_hbox_new (FALSE, 4);

  c_tool->channel_menu =
    gimp_enum_option_menu_new (GIMP_TYPE_HISTOGRAM_CHANNEL,
                               G_CALLBACK (curves_channel_callback),
                               c_tool);
  gtk_box_pack_start (GTK_BOX (hbox), c_tool->channel_menu, FALSE, FALSE, 0);
  gtk_widget_show (c_tool->channel_menu);

  button = gtk_button_new_with_label (_("Reset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (curves_channel_reset_callback),
                    c_tool);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Modify Curves for Channel:"), 1.0, 0.5,
                             hbox, 1, FALSE);

  /*  The option menu for selecting the drawing method  */
  c_tool->curve_type_menu =
    gimp_option_menu_new (FALSE,

                          _("Smooth"), curves_smooth_callback,
                          c_tool, GINT_TO_POINTER (CURVES_SMOOTH), NULL, TRUE,

                          _("Free"), curves_free_callback,
                          c_tool, GINT_TO_POINTER (CURVES_FREE), NULL, FALSE,

                          NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Curve Type:"), 1.0, 0.5,
                             c_tool->curve_type_menu, 1, TRUE);

  /*  The table for the yrange and the graph  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  c_tool->yrange = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (c_tool->yrange), YRANGE_WIDTH, YRANGE_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), c_tool->yrange);
  gtk_widget_show (c_tool->yrange);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_SHRINK | GTK_FILL,
		    GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  c_tool->graph = gtk_drawing_area_new ();
  gtk_widget_set_size_request (c_tool->graph,
                               GRAPH_WIDTH  + RADIUS * 2,
                               GRAPH_HEIGHT + RADIUS * 2);
  gtk_widget_set_events (c_tool->graph, GRAPH_MASK);
  gtk_container_add (GTK_CONTAINER (frame), c_tool->graph);
  gtk_widget_show (c_tool->graph);

  g_signal_connect (G_OBJECT (c_tool->graph), "event",
		    G_CALLBACK (curves_graph_events),
		    c_tool);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_widget_show (frame);

  c_tool->xrange = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (c_tool->xrange), XRANGE_WIDTH, XRANGE_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), c_tool->xrange);
  gtk_widget_show (c_tool->xrange);

  gtk_widget_show (table);

  /*  Horizontal button box for load / save  */
  frame = gtk_frame_new (_("All Channels"));
  gtk_box_pack_end (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbbox = gtk_hbutton_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (hbbox), 2);
  gtk_box_set_spacing (GTK_BOX (hbbox), 4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbbox), GTK_BUTTONBOX_SPREAD);
  gtk_container_add (GTK_CONTAINER (frame), hbbox);
  gtk_widget_show (hbbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Read curves settings from file"), NULL);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (curves_load_callback),
		    c_tool);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Save curves settings to file"), NULL);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (curves_save_callback),
		    c_tool);
}

static void
gimp_curves_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpCurvesTool *c_tool;
  gint            i, j;

  c_tool = GIMP_CURVES_TOOL (image_map_tool);

  c_tool->grab_point = -1;

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 256; j++)
        c_tool->curves->curve[i][j] = j;

      curves_channel_reset (c_tool->curves, i);
    }

  curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);
}

static void
curve_print_loc (GimpCurvesTool *c_tool,
		 gint            xpos,
		 gint            ypos)
{
  gchar buf[32];

  if (! c_tool->cursor_layout)
    {
      c_tool->cursor_layout = gtk_widget_create_pango_layout (c_tool->graph,
                                                              "x:888 y:888");
      pango_layout_get_pixel_extents (c_tool->cursor_layout, 
                                      NULL, &c_tool->cursor_rect);
    }
  
  if (xpos >= 0 && xpos <= 255 && ypos >=0 && ypos <= 255)
    {
      gdk_draw_rectangle (c_tool->graph->window, 
			  c_tool->graph->style->bg_gc[GTK_STATE_ACTIVE],
			  TRUE, 
                          RADIUS * 2 + 2, 
                          RADIUS * 2 + 2, 
			  c_tool->cursor_rect.width  + 4,
			  c_tool->cursor_rect.height + 5);

      gdk_draw_rectangle (c_tool->graph->window, 
			  c_tool->graph->style->black_gc,
			  FALSE, 
                          RADIUS * 2 + 2, 
                          RADIUS * 2 + 2, 
			  c_tool->cursor_rect.width  + 3, 
			  c_tool->cursor_rect.height + 4);
      
      g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",xpos, ypos);
      pango_layout_set_text (c_tool->cursor_layout, buf, 11);
      gdk_draw_layout (c_tool->graph->window, 
                       c_tool->graph->style->black_gc,
		       RADIUS * 2 + 4 + c_tool->cursor_rect.x,
		       RADIUS * 2 + 5 + c_tool->cursor_rect.y,
		       c_tool->cursor_layout);
    }
}

/* TODO: preview alpha channel stuff correctly.  -- austin, 20/May/99 */
static void
curves_update (GimpCurvesTool *c_tool,
	       gint            update)
{
  GimpHistogramChannel sel_channel;
  gint                 i, j;
  gchar                buf[32];
  gint                 offset;
  gint                 height;
  
  if (c_tool->color)
    {
      sel_channel = c_tool->channel;
    }
  else 
    {
      if (c_tool->channel == 2)
        sel_channel = GIMP_HISTOGRAM_ALPHA;
      else
        sel_channel = GIMP_HISTOGRAM_VALUE;
    }

  if (update & XRANGE_TOP)
    {
      guchar buf[XRANGE_WIDTH * 3];

      switch (sel_channel)
	{
	case GIMP_HISTOGRAM_VALUE:
	case GIMP_HISTOGRAM_ALPHA:
	  for (i = 0; i < XRANGE_HEIGHT / 2; i++)
	    {
	      for (j = 0; j < XRANGE_WIDTH ; j++)
		{
		  buf[j * 3 + 0] = c_tool->curves->curve[sel_channel][j];
		  buf[j * 3 + 1] = c_tool->curves->curve[sel_channel][j];
		  buf[j * 3 + 2] = c_tool->curves->curve[sel_channel][j];
		}
	      gtk_preview_draw_row (GTK_PREVIEW (c_tool->xrange),
				    buf, 0, i, XRANGE_WIDTH);
	    }
	  break;

	case GIMP_HISTOGRAM_RED:
	case GIMP_HISTOGRAM_GREEN:
	case GIMP_HISTOGRAM_BLUE:
	  {
	    for (i = 0; i < XRANGE_HEIGHT / 2; i++)
	      {
		for (j = 0; j < XRANGE_WIDTH; j++)
		  {
		    buf[j * 3 + 0] = c_tool->curves->curve[GIMP_HISTOGRAM_RED][j];
		    buf[j * 3 + 1] = c_tool->curves->curve[GIMP_HISTOGRAM_GREEN][j];
		    buf[j * 3 + 2] = c_tool->curves->curve[GIMP_HISTOGRAM_BLUE][j];
		  }
		gtk_preview_draw_row (GTK_PREVIEW (c_tool->xrange),
				      buf, 0, i, XRANGE_WIDTH);
	      }
	    break;
	  }

	default:
	  g_warning ("unknown channel type %d, can't happen!?!?",
		     c_tool->channel);
	  break;
	}

      if (update & DRAW)
        gtk_widget_queue_draw_area (c_tool->xrange,
                                    0, 0,
                                    XRANGE_WIDTH, XRANGE_HEIGHT / 2);
    }

  if (update & XRANGE_BOTTOM)
    {
      guchar buf[XRANGE_WIDTH * 3];

      for (i = 0; i < XRANGE_WIDTH; i++)
        {
          buf[i * 3 + 0] = i;
          buf[i * 3 + 1] = i;
          buf[i * 3 + 2] = i;
        }

      for (i = XRANGE_HEIGHT / 2; i < XRANGE_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (c_tool->xrange),
                              buf, 0, i, XRANGE_WIDTH);

      if (update & DRAW)
        gtk_widget_queue_draw_area (c_tool->xrange,
                                    0, XRANGE_HEIGHT / 2,
                                    XRANGE_WIDTH, XRANGE_HEIGHT / 2);
    }

  if (update & YRANGE)
    {
      guchar buf[YRANGE_WIDTH * 3];
      guchar pix[3];

      for (i = 0; i < YRANGE_HEIGHT; i++)
	{
	  switch (sel_channel)
	    {
	    case GIMP_HISTOGRAM_VALUE:
	    case GIMP_HISTOGRAM_ALPHA:
	      pix[0] = pix[1] = pix[2] = (255 - i);
	      break;

	    case GIMP_HISTOGRAM_RED:
	    case GIMP_HISTOGRAM_GREEN:
	    case GIMP_HISTOGRAM_BLUE:
	      pix[0] = pix[1] = pix[2] = 0;
	      pix[sel_channel - 1] = (255 - i);
	      break;

	    default:
	      g_warning ("unknown channel type %d, can't happen!?!?",
			 c_tool->channel);
	      break;
	    }

	  for (j = 0; j < YRANGE_WIDTH * 3; j++)
	    buf[j] = pix[j%3];

	  gtk_preview_draw_row (GTK_PREVIEW (c_tool->yrange),
				buf, 0, i, YRANGE_WIDTH);
	}

      if (update & DRAW)
	gtk_widget_queue_draw (c_tool->yrange);
    }

  if ((update & GRAPH) && (update & DRAW) && c_tool->pixmap)
    {
      GdkPoint points[256];

      /*  Clear the pixmap  */
      gdk_draw_rectangle (c_tool->pixmap,
                          c_tool->graph->style->bg_gc[GTK_STATE_NORMAL],
			  TRUE, 0, 0,
			  GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);

      /*  Draw the grid lines  */
      for (i = 0; i < 5; i++)
	{
	  gdk_draw_line (c_tool->pixmap,
                         c_tool->graph->style->dark_gc[GTK_STATE_NORMAL],
			 RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS,
			 GRAPH_WIDTH + RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);
	  gdk_draw_line (c_tool->pixmap,
                         c_tool->graph->style->dark_gc[GTK_STATE_NORMAL],
			 i * (GRAPH_WIDTH / 4) + RADIUS, RADIUS,
			 i * (GRAPH_WIDTH / 4) + RADIUS, GRAPH_HEIGHT + RADIUS);
	}

      /*  Draw the curve  */
      for (i = 0; i < 256; i++)
	{
	  points[i].x = i + RADIUS;
	  points[i].y = 255 - c_tool->curves->curve[c_tool->channel][i] + RADIUS;
	}

      if (c_tool->curves->curve_type[c_tool->channel] == CURVES_FREE)
        {
          gdk_draw_points (c_tool->pixmap,
                           c_tool->graph->style->black_gc,
                           points, 256);
        }
      else
        {
          gdk_draw_lines (c_tool->pixmap,
                          c_tool->graph->style->black_gc,
                          points, 256);

          /*  Draw the points  */
          for (i = 0; i < 17; i++)
            {
              if (c_tool->curves->points[c_tool->channel][i][0] != -1)
                gdk_draw_arc (c_tool->pixmap,
                              c_tool->graph->style->black_gc,
                              TRUE,
                              c_tool->curves->points[c_tool->channel][i][0],
                              255 - c_tool->curves->points[c_tool->channel][i][1],
                              RADIUS * 2, RADIUS * 2, 0, 23040);
            }
        }

      /* draw the color line */
      gdk_draw_line (c_tool->pixmap,
                     c_tool->graph->style->black_gc,
		     c_tool->col_value[sel_channel] + RADIUS,
                     RADIUS,
		     c_tool->col_value[sel_channel] + RADIUS,
                     GRAPH_HEIGHT + RADIUS);

      /* and xpos indicator */
      g_snprintf (buf, sizeof (buf), "x:%d", c_tool->col_value[sel_channel]);

      if (! c_tool->xpos_layout)
        c_tool->xpos_layout = gtk_widget_create_pango_layout (c_tool->graph,
                                                              buf);
      else
        pango_layout_set_text (c_tool->xpos_layout, buf, -1);

      if ((c_tool->col_value[sel_channel] + RADIUS) < 127)
	{
	  offset = RADIUS + 4;
	}
      else
	{
          pango_layout_get_pixel_size (c_tool->xpos_layout, &offset, &height);
          offset = - (offset + 2);
	}

      gdk_draw_layout (c_tool->pixmap,
		       c_tool->graph->style->black_gc,
		       c_tool->col_value[sel_channel] + offset,
		       GRAPH_HEIGHT - height - 2,
		       c_tool->xpos_layout);

      gdk_draw_drawable (c_tool->graph->window, 
                         c_tool->graph->style->black_gc, 
                         c_tool->pixmap,
			 0, 0, 
                         0, 0, 
                         GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);
    }
}

static void
curves_channel_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (data);

  gimp_menu_item_update (widget, &c_tool->channel);

  if (! c_tool->color)
    {
      if (c_tool->channel > 1)
        c_tool->channel = 2;
      else 
        c_tool->channel = 1;
    }

  gimp_option_menu_set_history (GTK_OPTION_MENU (c_tool->curve_type_menu),
                                GINT_TO_POINTER (c_tool->curves->curve_type[c_tool->channel]));

  curves_update (c_tool, XRANGE_TOP | YRANGE | GRAPH | DRAW);
}

static void
curves_channel_reset_callback (GtkWidget *widget,
                               gpointer   data)
{
  GimpCurvesTool *c_tool;
  gint            j;

  c_tool = GIMP_CURVES_TOOL (data);

  c_tool->grab_point = -1;

  for (j = 0; j < 256; j++)
    c_tool->curves->curve[c_tool->channel][j] = j;

  curves_channel_reset (c_tool->curves, c_tool->channel);

  curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (c_tool));
}

static gboolean
curves_set_sensitive_callback (gpointer        item_data,
                               GimpCurvesTool *c_tool)
{
  GimpHistogramChannel  channel = GPOINTER_TO_INT (item_data);

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;
    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return c_tool->color;
    case GIMP_HISTOGRAM_ALPHA:
      return gimp_drawable_has_alpha (GIMP_IMAGE_MAP_TOOL (c_tool)->drawable);
    }

  return FALSE;
}

static void
curves_smooth_callback (GtkWidget *widget,
			gpointer   data)
{
  GimpCurvesTool *c_tool;
  gint            i;
  gint32          index;

  c_tool = GIMP_CURVES_TOOL (data);

  if (c_tool->curves->curve_type[c_tool->channel] != CURVES_SMOOTH)
    {
      c_tool->curves->curve_type[c_tool->channel] = CURVES_SMOOTH;

      /*  pick representative points from the curve
       *  and make them control points
       */
      for (i = 0; i <= 8; i++)
	{
	  index = CLAMP0255 (i * 32);
	  c_tool->curves->points[c_tool->channel][i * 2][0] = index;
	  c_tool->curves->points[c_tool->channel][i * 2][1] = c_tool->curves->curve[c_tool->channel][index];
	}

      curves_calculate_curve (c_tool->curves, c_tool->channel);

      curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (c_tool));
    }
}

static void
curves_free_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (data);

  if (c_tool->curves->curve_type[c_tool->channel] != CURVES_FREE)
    {
      c_tool->curves->curve_type[c_tool->channel] = CURVES_FREE;

      curves_calculate_curve (c_tool->curves, c_tool->channel);

      curves_update (c_tool, GRAPH | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (c_tool));
    }
}

static gboolean
curves_graph_events (GtkWidget      *widget,
		     GdkEvent       *event,
		     GimpCurvesTool *c_tool)
{
  static GdkCursorType cursor_type = GDK_TOP_LEFT_ARROW;

  GdkCursorType   new_type;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            i;
  gint            tx, ty;
  gint            x, y;
  gint            closest_point;
  gint            distance;
  gint            x1, x2, y1, y2;

  new_type      = GDK_X_CURSOR;
  closest_point = 0;

  /*  get the pointer position  */
  gdk_window_get_pointer (c_tool->graph->window, &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, 255);
  y = CLAMP ((ty - RADIUS), 0, 255);

  distance = G_MAXINT;
  for (i = 0; i < 17; i++)
    {
      if (c_tool->curves->points[c_tool->channel][i][0] != -1)
	if (abs (x - c_tool->curves->points[c_tool->channel][i][0]) < distance)
	  {
	    distance = abs (x - c_tool->curves->points[c_tool->channel][i][0]);
	    closest_point = i;
	  }
    }
  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (c_tool->pixmap == NULL)
	c_tool->pixmap = gdk_pixmap_new (c_tool->graph->window,
                                         GRAPH_WIDTH + RADIUS * 2,
                                         GRAPH_HEIGHT + RADIUS * 2, -1);

      curves_update (c_tool, GRAPH | DRAW);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      new_type = GDK_TCROSS;

      switch (c_tool->curves->curve_type[c_tool->channel])
	{
	case CURVES_SMOOTH:
	  /*  determine the leftmost and rightmost points  */
	  c_tool->leftmost = -1;
	  for (i = closest_point - 1; i >= 0; i--)
	    if (c_tool->curves->points[c_tool->channel][i][0] != -1)
	      {
		c_tool->leftmost = c_tool->curves->points[c_tool->channel][i][0];
		break;
	      }
	  c_tool->rightmost = 256;
	  for (i = closest_point + 1; i < 17; i++)
	    if (c_tool->curves->points[c_tool->channel][i][0] != -1)
	      {
		c_tool->rightmost = c_tool->curves->points[c_tool->channel][i][0];
		break;
	      }

	  c_tool->grab_point = closest_point;
	  c_tool->curves->points[c_tool->channel][c_tool->grab_point][0] = x;
	  c_tool->curves->points[c_tool->channel][c_tool->grab_point][1] = 255 - y;
	  break;

	case CURVES_FREE:
	  c_tool->curves->curve[c_tool->channel][x] = 255 - y;
	  c_tool->grab_point = x;
	  c_tool->last = y;
	  break;
	}
      
      curves_calculate_curve (c_tool->curves, c_tool->channel);

      curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);
      gtk_grab_add (widget);
      break;

    case GDK_BUTTON_RELEASE:
      new_type = GDK_FLEUR;
      c_tool->grab_point = -1;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (c_tool));

      gtk_grab_remove (widget);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if (mevent->is_hint)
	{
	  mevent->x = tx;
	  mevent->y = ty;
	}

      switch (c_tool->curves->curve_type[c_tool->channel])
	{
	case CURVES_SMOOTH:
	  /*  If no point is grabbed...  */
	  if (c_tool->grab_point == -1)
	    {
	      if (c_tool->curves->points[c_tool->channel][closest_point][0] != -1)
		new_type = GDK_FLEUR;
	      else
		new_type = GDK_TCROSS;
	    }
	  /*  Else, drag the grabbed point  */
	  else
	    {
	      new_type = GDK_TCROSS;

	      c_tool->curves->points[c_tool->channel][c_tool->grab_point][0] = -1;

	      if (x > c_tool->leftmost && x < c_tool->rightmost)
		{
		  closest_point = (x + 8) / 16;
		  if (c_tool->curves->points[c_tool->channel][closest_point][0] == -1)
		    c_tool->grab_point = closest_point;

		  c_tool->curves->points[c_tool->channel][c_tool->grab_point][0] = x;
		  c_tool->curves->points[c_tool->channel][c_tool->grab_point][1] = 255 - y;
		}

	      curves_calculate_curve (c_tool->curves, c_tool->channel);

	      curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);
	    }
	  break;

	case CURVES_FREE:
	  if (c_tool->grab_point != -1)
	    {
	      if (c_tool->grab_point > x)
		{
		  x1 = x;
		  x2 = c_tool->grab_point;
		  y1 = y;
		  y2 = c_tool->last;
		}
	      else
		{
		  x1 = c_tool->grab_point;
		  x2 = x;
		  y1 = c_tool->last;
		  y2 = y;
		}

	      if (x2 != x1)
		for (i = x1; i <= x2; i++)
		  c_tool->curves->curve[c_tool->channel][i] = 255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
	      else
		c_tool->curves->curve[c_tool->channel][x] = 255 - y;

	      c_tool->grab_point = x;
	      c_tool->last = y;

	      curves_update (c_tool, GRAPH | XRANGE_TOP | DRAW);
	    }

	  if (mevent->state & GDK_BUTTON1_MASK)
	    new_type = GDK_TCROSS;
	  else
	    new_type = GDK_PENCIL;
	  break;
	}

      if (new_type != cursor_type)
	{
	  GdkCursor *cursor;

	  cursor_type = new_type;

	  cursor = gimp_cursor_new (cursor_type,
				    GIMP_TOOL_CURSOR_NONE,
				    GIMP_CURSOR_MODIFIER_NONE);
	  gdk_window_set_cursor (c_tool->graph->window, cursor);
	  gdk_cursor_unref (cursor);
	}

      curve_print_loc (c_tool, x, 255 - y);
      break;

    default:
      break;
    }

  return TRUE;
}

static void
curves_load_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (data);

  if (! c_tool->file_dialog)
    file_dialog_create (c_tool);
  else if (GTK_WIDGET_VISIBLE (c_tool->file_dialog))
    return;

  c_tool->is_save = FALSE;

  gtk_window_set_title (GTK_WINDOW (c_tool->file_dialog), _("Load Curves"));
  gtk_widget_show (c_tool->file_dialog);
}

static void
curves_save_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpCurvesTool *c_tool;

  c_tool = GIMP_CURVES_TOOL (data);

  if (! c_tool->file_dialog)
    file_dialog_create (c_tool);
  else if (GTK_WIDGET_VISIBLE (c_tool->file_dialog))
    return;

  c_tool->is_save = TRUE;

  gtk_window_set_title (GTK_WINDOW (c_tool->file_dialog), _("Save Curves"));
  gtk_widget_show (c_tool->file_dialog);
}

static void
file_dialog_create (GimpCurvesTool *c_tool)
{
  GtkFileSelection *file_dlg;
  gchar            *temp;

  c_tool->file_dialog = gtk_file_selection_new ("");

  file_dlg = GTK_FILE_SELECTION (c_tool->file_dialog);

  gtk_window_set_wmclass (GTK_WINDOW (file_dlg), "load_save_curves", "Gimp");
  gtk_window_set_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (file_dlg), 2);
  gtk_container_set_border_width (GTK_CONTAINER (file_dlg->button_area), 2);

  g_signal_connect_swapped (G_OBJECT (file_dlg->cancel_button), "clicked",
                            G_CALLBACK (file_dialog_cancel_callback),
                            c_tool);
  g_signal_connect_swapped (G_OBJECT (file_dlg->ok_button), "clicked",
                            G_CALLBACK (file_dialog_ok_callback),
                            c_tool);

  g_signal_connect_swapped (G_OBJECT (file_dlg), "delete_event",
                            G_CALLBACK (file_dialog_cancel_callback),
                            c_tool);
  g_signal_connect_swapped (G_OBJECT (GIMP_IMAGE_MAP_TOOL (c_tool)->shell),
                            "unmap",
                            G_CALLBACK (file_dialog_cancel_callback),
                            c_tool);

  temp = g_build_filename (gimp_directory (), "curves", NULL);
  gtk_file_selection_set_filename (file_dlg, temp);
  g_free (temp);

  gimp_help_connect (c_tool->file_dialog, tool_manager_help_func, NULL);
}

static void
file_dialog_ok_callback (GimpCurvesTool *c_tool)
{
  FILE        *file = NULL;
  const gchar *filename;

  filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (c_tool->file_dialog));

  if (! c_tool->is_save)
    {
      file = fopen (filename, "rt");

      if (! file)
	{
	  g_message (_("Failed to open file: '%s': %s"),
                     filename, g_strerror (errno));
	  return;
	}

      if (! curves_read_from_file (c_tool, file))
	{
	  g_message (("Error in reading file '%s'."), filename);
	}
    }
  else
    {
      file = fopen (filename, "wt");

      if (! file)
	{
	  g_message (_("Failed to open file: '%s': %s"),
                     filename, g_strerror (errno));
	  return;
	}

      curves_write_to_file (c_tool, file);
    }

  if (file)
    fclose (file);

  file_dialog_cancel_callback (c_tool);
}

static gboolean
file_dialog_cancel_callback (GimpCurvesTool *c_tool)
{
  if (c_tool->file_dialog)
    {
      gtk_widget_destroy (c_tool->file_dialog);
      c_tool->file_dialog = NULL;
    }

  return TRUE;
}

static gboolean
curves_read_from_file (GimpCurvesTool *c_tool,
                       FILE           *file)
{
  gint  i, j;
  gint  fields;
  gchar buf[50];
  gint  index[5][17];
  gint  value[5][17];
  
  if (! fgets (buf, 50, file))
    return FALSE;

  if (strcmp (buf, "# GIMP Curves File\n") != 0)
    return FALSE;

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 17; j++)
	{
	  fields = fscanf (file, "%d %d ", &index[i][j], &value[i][j]);
	  if (fields != 2)
	    {
	      g_print ("fields != 2");
	      return FALSE;
	    }
	}
    }

  for (i = 0; i < 5; i++)
    {
      c_tool->curves->curve_type[i] = CURVES_SMOOTH;

      for (j = 0; j < 17; j++)
	{
	  c_tool->curves->points[i][j][0] = index[i][j];
	  c_tool->curves->points[i][j][1] = value[i][j];
	}
    }

  for (i = 0; i < 5; i++)
    curves_calculate_curve (c_tool->curves, i);

  curves_update (c_tool, ALL);

  gimp_option_menu_set_history (GTK_OPTION_MENU (c_tool->curve_type_menu),
                                GINT_TO_POINTER (CURVES_SMOOTH));

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (c_tool));

  return TRUE;
}

static void
curves_write_to_file (GimpCurvesTool *c_tool,
                      FILE           *file)
{
  gint   i, j;
  gint32 index;

  for (i = 0; i < 5; i++)
    if (c_tool->curves->curve_type[i] == CURVES_FREE)
      {  
	/*  pick representative points from the curve and make them control points  */
	for (j = 0; j <= 8; j++)
	  {
	    index = CLAMP0255 (j * 32);
	    c_tool->curves->points[i][j * 2][0] = index;
	    c_tool->curves->points[i][j * 2][1] = c_tool->curves->curve[i][index];
	  }      
      }

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 17; j++)
	fprintf (file, "%d %d ",
                 c_tool->curves->points[i][j][0], 
                 c_tool->curves->points[i][j][1]);
      
      fprintf (file, "\n");
    }
}
