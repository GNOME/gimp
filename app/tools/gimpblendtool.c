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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpprogress.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimpenummenu.h"

#include "gimpblendtool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


#define TARGET_SIZE 15


typedef struct _BlendOptions BlendOptions;

struct _BlendOptions
{
  GimpPaintOptions  paint_options;

  gdouble           offset;
  gdouble           offset_d;
  GtkObject        *offset_w;

  GimpBlendMode     blend_mode;
  GimpBlendMode     blend_mode_d;
  GtkWidget        *blend_mode_w;

  GimpGradientType  gradient_type;
  GimpGradientType  gradient_type_d;
  GtkWidget        *gradient_type_w;

  GimpRepeatMode    repeat;
  GimpRepeatMode    repeat_d;
  GtkWidget        *repeat_w;
   
  gint              supersample;
  gint              supersample_d;
  GtkWidget        *supersample_w;

  gint              max_depth;
  gint              max_depth_d;
  GtkObject        *max_depth_w;

  gdouble           threshold;
  gdouble           threshold_d;
  GtkObject        *threshold_w;
};


/*  local function prototypes  */

static void    gimp_blend_tool_class_init        (GimpBlendToolClass *klass);
static void    gimp_blend_tool_init              (GimpBlendTool      *blend_tool);

static void    gimp_blend_tool_button_press      (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_button_release    (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_motion            (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_cursor_update     (GimpTool        *tool,
                                                  GimpCoords      *coords,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);

static void    gimp_blend_tool_draw              (GimpDrawTool    *draw_tool);

static GimpToolOptions * blend_options_new       (GimpToolInfo    *tool_info);
static void              blend_options_reset     (GimpToolOptions *tool_options);

static void    gradient_type_callback            (GtkWidget       *widget,
                                                  gpointer         data);
static void    blend_options_drop_gradient       (GtkWidget       *widget,
						  GimpViewable    *viewable,
						  gpointer         data);
static void    blend_options_drop_tool           (GtkWidget       *widget,
						  GimpViewable    *viewable,
						  gpointer         data);


/*  private variables  */

static GimpDrawToolClass *parent_class = NULL;

static GtkTargetEntry blend_target_table[] =
{
  GIMP_TARGET_GRADIENT,  
  GIMP_TARGET_TOOL
};


/*  public functions  */

void
gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                          Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_BLEND_TOOL,
                blend_options_new,
                TRUE,
                "gimp-blend-tool",
                _("Blend"),
                _("Fill with a color gradient"),
                N_("/Tools/Paint Tools/Blend"), "L",
                NULL, "tools/blend.html",
                GIMP_STOCK_TOOL_BLEND,
                gimp);
}

GType
gimp_blend_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpBlendToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_blend_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBlendTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_blend_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpBlendTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_blend_tool_button_press;
  tool_class->button_release = gimp_blend_tool_button_release;
  tool_class->motion         = gimp_blend_tool_motion;
  tool_class->cursor_update  = gimp_blend_tool_cursor_update;

  draw_tool_class->draw      = gimp_blend_tool_draw;
}

static void
gimp_blend_tool_init (GimpBlendTool *blend_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (blend_tool);
 
  tool->control = gimp_tool_control_new  (TRUE,                       /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          TRUE,                       /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          GIMP_MOTION_MODE_HINT,      /* motion_mode */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_BLEND_TOOL_CURSOR,     /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);

}

static void
gimp_blend_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpBlendTool *blend_tool;
  gint           off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case GIMP_INDEXED_IMAGE: case GIMP_INDEXEDA_IMAGE:
      g_message (_("Blend: Invalid for indexed images."));
      return;

      break;
    default:
      break;
    }

  gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                         &off_x, &off_y);

  blend_tool->endx = blend_tool->startx = coords->x - off_x;
  blend_tool->endy = blend_tool->starty = coords->y - off_y;

  tool->gdisp = gdisp;

  gimp_tool_control_activate(tool->control);

  /* initialize the statusbar display */
  gimp_tool_push_status (tool, _("Blend: 0, 0"));

  /*  Start drawing the blend tool  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_blend_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpBlendTool *blend_tool;
  BlendOptions  *options;
  GimpImage     *gimage;
  GimpProgress  *progress;

  blend_tool = GIMP_BLEND_TOOL (tool);

  options = (BlendOptions *) tool->tool_info->tool_options;

  gimage = gdisp->gimage;

  gimp_tool_pop_status (tool);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt(tool->control); /* sets paused_count to 0 -- is this ok? */

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (state & GDK_BUTTON3_MASK) &&
      ((blend_tool->startx != blend_tool->endx) ||
       (blend_tool->starty != blend_tool->endy)))
    {
      progress = gimp_progress_start (gdisp, _("Blending..."), FALSE,
                                      NULL, NULL);

      gimp_drawable_blend (gimp_image_active_drawable (gimage),
                           options->blend_mode,
                           gimp_context_get_paint_mode (gimp_get_current_context (gimage->gimp)),
                           options->gradient_type,
                           gimp_context_get_opacity (gimp_get_current_context (gimage->gimp)),
                           options->offset,
                           options->repeat,
                           options->supersample,
                           options->max_depth,
                           options->threshold,
                           blend_tool->startx,
                           blend_tool->starty,
                           blend_tool->endx,
                           blend_tool->endy,
                           progress ? gimp_progress_update_and_flush : NULL, 
                           progress);

      if (progress)
	gimp_progress_end (progress);

      gdisplays_flush ();
    }
}

static void
gimp_blend_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  GimpBlendTool    *blend_tool;
  gint              off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                         &off_x, &off_y);

  /*  Get the current coordinates  */
  blend_tool->endx = coords->x - off_x;
  blend_tool->endy = coords->y - off_y;

  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (state & GDK_CONTROL_MASK)
    {
      gint tangens2[6] = {  34, 106, 196, 334, 618, 1944 };
      gint cosinus[7]  = { 256, 247, 222, 181, 128, 66, 0 };
      gint dx, dy, i, radius, frac;

      dx = blend_tool->endx - blend_tool->startx;
      dy = blend_tool->endy - blend_tool->starty;

      if (dy)
	{
	  radius = sqrt (SQR (dx) + SQR (dy));
	  frac = abs ((dx << 8) / dy);

	  for (i = 0; i < 6; i++)
	    {
	      if (frac < tangens2[i])
		break;  
	    }

	  dx = dx > 0 ? 
            (cosinus[6-i] * radius) >> 8 : - ((cosinus[6-i] * radius) >> 8);

	  dy = dy > 0 ? 
            (cosinus[i]   * radius) >> 8 : - ((cosinus[i]   * radius) >> 8);
	}

      blend_tool->endx = blend_tool->startx + dx;
      blend_tool->endy = blend_tool->starty + dy;
    }

  gimp_tool_pop_status (tool);

  gimp_tool_push_status_coords (tool,
                                _("Blend: "),
                                blend_tool->endx - blend_tool->startx,
                                ", ",
                                blend_tool->endy - blend_tool->starty);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_blend_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      gimp_tool_control_set_cursor(tool->control, GIMP_BAD_CURSOR);
      break;
    default:
      gimp_tool_control_set_cursor(tool->control, GIMP_MOUSE_CURSOR);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_blend_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBlendTool *blend_tool;

  blend_tool = GIMP_BLEND_TOOL (draw_tool);

  /*  Draw start target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->startx) + 0.5,
                              floor (blend_tool->starty) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              TRUE);

  /*  Draw end target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->endx) + 0.5,
                              floor (blend_tool->endy) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              TRUE);

  /*  Draw the line between the start and end coords  */
  gimp_draw_tool_draw_line (draw_tool,
                            floor (blend_tool->startx) + 0.5,
                            floor (blend_tool->starty) + 0.5,
                            floor (blend_tool->endx) + 0.5,
                            floor (blend_tool->endy) + 0.5,
                            TRUE);
}

static GimpToolOptions *
blend_options_new (GimpToolInfo *tool_info)
{
  BlendOptions *options;

  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;

  /*  the new blend tool options structure  */
  options = g_new0 (BlendOptions, 1);

  gimp_paint_options_init ((GimpPaintOptions *) options);

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = blend_options_reset;

  options->offset  	 = options->offset_d  	    = 0.0;
  options->blend_mode 	 = options->blend_mode_d    = GIMP_FG_BG_RGB_MODE;
  options->gradient_type = options->gradient_type_d = GIMP_LINEAR;
  options->repeat        = options->repeat_d        = GIMP_REPEAT_NONE;
  options->supersample   = options->supersample_d   = FALSE;
  options->max_depth     = options->max_depth_d     = 3;
  options->threshold     = options->threshold_d     = 0.2;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /*  dnd stuff  */
  gtk_drag_dest_set (vbox,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     blend_target_table,
                     G_N_ELEMENTS (blend_target_table),
		     GDK_ACTION_COPY); 
  gimp_dnd_viewable_dest_set (vbox,
                              GIMP_TYPE_GRADIENT,
                              blend_options_drop_gradient,
			      options);
  gimp_dnd_viewable_dest_set (vbox,
                              GIMP_TYPE_TOOL_INFO,
                              blend_options_drop_tool,
			      options);

  /*  the offset scale  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->offset_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					    _("Offset:"), -1, 50,
					    options->offset,
					    0.0, 100.0, 1.0, 10.0, 1,
					    TRUE, 0.0, 0.0,
					    NULL, NULL);

  g_signal_connect (G_OBJECT (options->offset_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->offset);

  /*  the blend mode menu  */
  options->blend_mode_w =
    gimp_enum_option_menu_new (GIMP_TYPE_BLEND_MODE,
                               G_CALLBACK (gimp_menu_item_update),
                               &options->blend_mode);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->blend_mode_w),
                                GINT_TO_POINTER (options->blend_mode));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Blend:"), 1.0, 0.5,
			     options->blend_mode_w, 2, TRUE);

  /*  the gradient type menu  */
  options->gradient_type_w =
    gimp_enum_option_menu_new (GIMP_TYPE_GRADIENT_TYPE,
                               G_CALLBACK (gradient_type_callback),
                               options);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w),
                                GINT_TO_POINTER (options->gradient_type));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Gradient:"), 1.0, 0.5,
			     options->gradient_type_w, 2, TRUE);

  /*  the repeat option  */
  options->repeat_w = 
    gimp_enum_option_menu_new (GIMP_TYPE_REPEAT_MODE,
                               G_CALLBACK (gimp_menu_item_update),
                               &options->repeat);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->repeat_w),
                                GINT_TO_POINTER (options->repeat));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Repeat:"), 1.0, 0.5,
			     options->repeat_w, 2, TRUE);

  /*  show the table  */
  gtk_widget_show (table);

  /*  frame for supersampling options  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* vbox for the supersampling stuff */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  supersampling toggle  */
  options->supersample_w =
    gtk_check_button_new_with_label (_("Adaptive Supersampling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->supersample_w),
				options->supersample);
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->supersample_w);
  gtk_widget_show (options->supersample_w);

  g_signal_connect (G_OBJECT (options->supersample_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->supersample);

  /*  table for supersampling options  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  automatically set the sensitive state of the table  */
  gtk_widget_set_sensitive (table, options->supersample);
  g_object_set_data (G_OBJECT (options->supersample_w), "set_sensitive",
                     table);

  /*  max depth scale  */
  options->max_depth_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Max Depth:"), -1, 50,
					       options->max_depth,
					       1.0, 10.0, 1.0, 1.0, 0,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (G_OBJECT(options->max_depth_w), "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &options->max_depth);

  /*  threshold scale  */
  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					       _("Threshold:"), -1, 50,
					       options->threshold,
					       0.0, 4.0, 0.01, 0.1, 2,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (G_OBJECT (options->threshold_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);

  /*  show the table  */
  gtk_widget_show (table);

  return (GimpToolOptions *) options;
}

static void
blend_options_reset (GimpToolOptions *tool_options)
{
  BlendOptions *options;

  options = (BlendOptions *) tool_options;

  paint_options_reset (tool_options);

  options->blend_mode    = options->blend_mode_d;
  options->gradient_type = options->gradient_type_d;
  options->repeat        = options->repeat_d;

  gtk_option_menu_set_history (GTK_OPTION_MENU (options->blend_mode_w),
			       options->blend_mode_d);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w),
			       options->gradient_type_d);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->repeat_w),
			       options->repeat_d);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->offset_w),
			    options->offset_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->supersample_w),
				options->supersample_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->max_depth_w),
			    options->max_depth_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
}

static void
gradient_type_callback (GtkWidget *widget,
			gpointer   data)
{
  BlendOptions *options;

  options = (BlendOptions *) data;

  gimp_menu_item_update (widget, &options->gradient_type);

  gtk_widget_set_sensitive (options->repeat_w, 
			    (options->gradient_type < 6));
}

static void
blend_options_drop_gradient (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  BlendOptions    *options;
  GimpToolOptions *tool_options;
  GimpContext     *context;

  options      = (BlendOptions *) data;
  tool_options = (GimpToolOptions *) data;

  context = gimp_get_user_context (tool_options->tool_info->gimp);

  gimp_context_set_gradient (context, GIMP_GRADIENT (viewable));

  gtk_option_menu_set_history (GTK_OPTION_MENU (options->blend_mode_w), 
			       GIMP_CUSTOM_MODE);
  options->blend_mode = GIMP_CUSTOM_MODE;
}

static void
blend_options_drop_tool (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  GimpToolOptions *tool_options;
  GimpContext     *context;

  tool_options = (GimpToolOptions *) data;

  context = gimp_get_user_context (tool_options->tool_info->gimp);

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}
