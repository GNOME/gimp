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

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpdnd.h"

#include "gimpblendtool.h"
#include "paint_options.h"

#include "gimpprogress.h"

#include "libgimp/gimpintl.h"


/*  target size  */
#define  TARGET_SIZE      15

#define  STATUSBAR_SIZE  128


typedef struct _BlendOptions BlendOptions;

struct _BlendOptions
{
  PaintOptions  paint_options;

  gdouble       offset;
  gdouble       offset_d;
  GtkObject    *offset_w;

  BlendMode     blend_mode;
  BlendMode     blend_mode_d;
  GtkWidget    *blend_mode_w;

  GradientType  gradient_type;
  GradientType  gradient_type_d;
  GtkWidget    *gradient_type_w;

  RepeatMode    repeat;
  RepeatMode    repeat_d;
  GtkWidget    *repeat_w;

  gint          supersample;
  gint          supersample_d;
  GtkWidget    *supersample_w;

  gint          max_depth;
  gint          max_depth_d;
  GtkObject    *max_depth_w;

  gdouble       threshold;
  gdouble       threshold_d;
  GtkObject    *threshold_w;
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
gimp_blend_tool_register (Gimp                     *gimp,
                          GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_BLEND_TOOL,
                blend_options_new,
                TRUE,
                "gimp:blend_tool",
                _("Blend"),
                _("Fill with a color gradient"),
                N_("/Tools/Paint Tools/Blend"), "L",
                NULL, "tools/blend.html",
                GIMP_STOCK_TOOL_BLEND);
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
 
  tool->tool_cursor = GIMP_BLEND_TOOL_CURSOR;
  tool->scroll_lock = TRUE;  /*  Disallow scrolling  */
}

static void
gimp_blend_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpBlendTool    *blend_tool;
  GimpDisplayShell *shell;
  gint              off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
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
  tool->state = ACTIVE;

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

  /* initialize the statusbar display */
  blend_tool->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar), "blend");
  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar),
		      blend_tool->context_id,
                      _("Blend: 0, 0"));

  /*  Start drawing the blend tool  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool),
                        shell->canvas->window);
}

static void
gimp_blend_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpBlendTool    *blend_tool;
  BlendOptions     *options;
  GimpDisplayShell *shell;
  GimpImage        *gimage;
#ifdef BLEND_UI_CALLS_VIA_PDB
  Argument         *return_vals;
  gint              nreturn_vals;
#else
  GimpProgress     *progress;
#endif

  blend_tool = GIMP_BLEND_TOOL (tool);

  options = (BlendOptions *) tool->tool_info->tool_options;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimage = gdisp->gimage;

  gdk_pointer_ungrab (time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar), blend_tool->context_id);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (state & GDK_BUTTON3_MASK) &&
      ((blend_tool->startx != blend_tool->endx) ||
       (blend_tool->starty != blend_tool->endy)))
    {
      /* we can't do callbacks easily with the PDB, so this UI/backend
       * separation (though good) is ignored for the moment */
#ifdef BLEND_UI_CALLS_VIA_PDB
      return_vals = 
	procedural_db_run_proc ("gimp_blend",
				&nreturn_vals,
				PDB_DRAWABLE, drawable_ID (gimp_image_active_drawable (gimage)),
				PDB_INT32, (gint32) options->blend_mode,
				PDB_INT32, (gint32) PAINT_OPTIONS_GET_PAINT_MODE (options),
				PDB_INT32, (gint32) options->gradient_type,
				PDB_FLOAT, (gdouble) PAINT_OPTIONS_GET_OPACITY (options) * 100,
				PDB_FLOAT, (gdouble) options->offset,
				PDB_INT32, (gint32) options->repeat,
				PDB_INT32, (gint32) options->supersample,
				PDB_INT32, (gint32) options->max_depth,
				PDB_FLOAT, (gdouble) options->threshold,
				PDB_FLOAT, (gdouble) blend_tool->startx,
				PDB_FLOAT, (gdouble) blend_tool->starty,
				PDB_FLOAT, (gdouble) blend_tool->endx,
				PDB_FLOAT, (gdouble) blend_tool->endy,
				PDB_END);

      if (return_vals && return_vals[0].value.pdb_int == PDB_SUCCESS)
	gdisplays_flush ();
      else
	g_message (_("Blend operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);

#else /* ! BLEND_UI_CALLS_VIA_PDB */

      progress = progress_start (gdisp, _("Blending..."), FALSE, NULL, NULL);

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
                           progress ? progress_update_and_flush : NULL, 
                           progress);

      if (progress)
	progress_end (progress);

      gdisplays_flush ();
#endif /* ! BLEND_UI_CALLS_VIA_PDB */
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
  GimpDisplayShell *shell;
  gchar             vector[STATUSBAR_SIZE];
  gint              off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

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
	  dx = dx > 0 ? (cosinus[6-i] * radius) >> 8 : - ((cosinus[6-i] * radius) >> 8);
	  dy = dy > 0 ? (cosinus[i] * radius) >> 8 : - ((cosinus[i] * radius) >> 8);
	}
      blend_tool->endx = blend_tool->startx + dx;
      blend_tool->endy = blend_tool->starty + dy;
    }

  gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar), blend_tool->context_id);

  if (gdisp->dot_for_dot)
    {
      g_snprintf (vector, sizeof (vector), shell->cursor_format_str,
		  _("Blend: "),
		  blend_tool->endx - blend_tool->startx,
		  ", ",
		  blend_tool->endy - blend_tool->starty);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (vector, sizeof (vector), shell->cursor_format_str,
		  _("Blend: "),
		  blend_tool->endx - blend_tool->startx * unit_factor /
		  gdisp->gimage->xresolution,
		  ", ",
		  blend_tool->endy - blend_tool->starty * unit_factor /
		  gdisp->gimage->yresolution);
    }

  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar), blend_tool->context_id,
		      vector);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_blend_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE:
    case INDEXEDA_GIMAGE:
      gimp_display_shell_install_tool_cursor (shell,
                                              GIMP_BAD_CURSOR,
                                              GIMP_BLEND_TOOL_CURSOR,
                                              GIMP_CURSOR_MODIFIER_NONE);
      break;
    default:
      gimp_display_shell_install_tool_cursor (shell,
                                              GIMP_MOUSE_CURSOR,
                                              GIMP_BLEND_TOOL_CURSOR,
                                              GIMP_CURSOR_MODIFIER_NONE);
      break;
    }
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
  GtkWidget *scale;
  GtkWidget *frame;

  /*  the new blend tool options structure  */
  options = g_new0 (BlendOptions, 1);

  paint_options_init ((PaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = blend_options_reset;

  options->offset  	 = options->offset_d  	    = 0.0;
  options->blend_mode 	 = options->blend_mode_d    = FG_BG_RGB_MODE;
  options->gradient_type = options->gradient_type_d = LINEAR;
  options->repeat        = options->repeat_d        = REPEAT_NONE;
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
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  options->offset_w =
    gtk_adjustment_new (options->offset_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->offset_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Offset:"), 1.0, 1.0,
			     scale, 1, FALSE);
  g_signal_connect (G_OBJECT (options->offset_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->offset);

  /*  the blend mode menu  */
  options->blend_mode_w = gimp_option_menu_new2
    (FALSE,
     G_CALLBACK (gimp_menu_item_update),
     &options->blend_mode,
     GINT_TO_POINTER (options->blend_mode_d),

     _("FG to BG (RGB)"),    GINT_TO_POINTER (FG_BG_RGB_MODE), NULL,
     _("FG to BG (HSV)"),    GINT_TO_POINTER (FG_BG_HSV_MODE), NULL,
     _("FG to Transparent"), GINT_TO_POINTER (FG_TRANS_MODE), NULL,
     _("Custom Gradient"),   GINT_TO_POINTER (CUSTOM_MODE), NULL,

     NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Blend:"), 1.0, 0.5,
			     options->blend_mode_w, 1, TRUE);

  /*  the gradient type menu  */
  options->gradient_type_w = gimp_option_menu_new2
    (FALSE,
     G_CALLBACK (gradient_type_callback),
     options,
     GINT_TO_POINTER (options->gradient_type_d),

     _("Linear"),                 GINT_TO_POINTER (LINEAR), NULL,
     _("Bi-Linear"),              GINT_TO_POINTER (BILINEAR), NULL,
     _("Radial"),                 GINT_TO_POINTER (RADIAL), NULL,
     _("Square"),                 GINT_TO_POINTER (SQUARE), NULL,
     _("Conical (symmetric)"),    GINT_TO_POINTER (CONICAL_SYMMETRIC), NULL,
     _("Conical (asymmetric)"),   GINT_TO_POINTER (CONICAL_ASYMMETRIC), NULL,
     _("Shapeburst (angular)"),   GINT_TO_POINTER (SHAPEBURST_ANGULAR), NULL,
     _("Shapeburst (spherical)"), GINT_TO_POINTER (SHAPEBURST_SPHERICAL), NULL,
     _("Shapeburst (dimpled)"),   GINT_TO_POINTER (SHAPEBURST_DIMPLED), NULL,
     _("Spiral (clockwise)"),     GINT_TO_POINTER (SPIRAL_CLOCKWISE), NULL,
     _("Spiral (anticlockwise)"), GINT_TO_POINTER (SPIRAL_ANTICLOCKWISE), NULL,

     NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Gradient:"), 1.0, 0.5,
			     options->gradient_type_w, 1, TRUE);

  /*  the repeat option  */
  options->repeat_w = gimp_option_menu_new2
    (FALSE,
     G_CALLBACK (gimp_menu_item_update),
     &options->repeat,
     GINT_TO_POINTER (options->repeat_d),

     _("None"),            GINT_TO_POINTER (REPEAT_NONE), NULL,
     _("Sawtooth Wave"),   GINT_TO_POINTER (REPEAT_SAWTOOTH), NULL,
     _("Triangular Wave"), GINT_TO_POINTER (REPEAT_TRIANGULAR), NULL,

     NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Repeat:"), 1.0, 0.5,
			     options->repeat_w, 1, TRUE);

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
				options->supersample_d);
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->supersample_w);
  gtk_widget_show (options->supersample_w);

  g_signal_connect (G_OBJECT (options->supersample_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->supersample);

  /*  table for supersampling options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  automatically set the sensitive state of the table  */
  gtk_widget_set_sensitive (table, options->supersample_d);
  g_object_set_data (G_OBJECT (options->supersample_w), "set_sensitive",
                     table);

  /*  max depth scale  */
  options->max_depth_w =
    gtk_adjustment_new (options->max_depth_d, 1.0, 10.0, 1.0, 1.0, 1.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->max_depth_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Max Depth:"), 1.0, 1.0,
			     scale, 1, FALSE);

  g_signal_connect (G_OBJECT(options->max_depth_w), "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &options->max_depth);

  /*  threshold scale  */
  options->threshold_w =
    gtk_adjustment_new (options->threshold_d, 0.0, 4.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Threshold:"), 1.0, 1.0,
			     scale, 1, FALSE);

  g_signal_connect (G_OBJECT(options->threshold_w), "value_changed",
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
			       CUSTOM_MODE);
  options->blend_mode = CUSTOM_MODE;
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
