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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdnd.h"

#include "app_procs.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimpprogress.h"
#include "undo.h"

#include "gimpblendtool.h"
#include "paint_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#define WANT_BLEND_BITS
#include "icons.h"


/*  target size  */
#define  TARGET_HEIGHT    15
#define  TARGET_WIDTH     15

#define  STATUSBAR_SIZE  128


typedef gdouble (* RepeatFunc) (gdouble);

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

typedef struct
{
  GimpGradient *gradient;
  gdouble       offset;
  gdouble       sx, sy;
  BlendMode     blend_mode;
  GradientType  gradient_type;
  GimpRGB       fg, bg;
  gdouble       dist;
  gdouble       vec[2];
  RepeatFunc    repeat_func;
} RenderBlendData;

typedef struct
{
  PixelRegion *PR;
  guchar      *row_data;
  gint         bytes;
  gint         width;
} PutPixelData;


/*  local function prototypes  */

static void    gimp_blend_tool_class_init        (GimpBlendToolClass *klass);
static void    gimp_blend_tool_init              (GimpBlendTool      *blend_tool);

static void    gimp_blend_tool_destroy           (GtkObject       *object);

static void    gradient_type_callback            (GtkWidget       *widget,
						  gpointer         data);

static void    gimp_blend_tool_button_press      (GimpTool        *tool,
						  GdkEventButton  *bevent,
						  GDisplay        *gdisp);
static void    gimp_blend_tool_button_release    (GimpTool        *tool,
						  GdkEventButton  *bevent,
						  GDisplay        *gdisp);
static void    gimp_blend_tool_motion            (GimpTool        *tool,
						  GdkEventMotion  *mevent,
						  GDisplay        *gdisp);
static void    gimp_blend_tool_cursor_update     (GimpTool        *tool,
						  GdkEventMotion  *mevent,
						  GDisplay        *gdisp);

static void    gimp_blend_tool_draw              (GimpDrawTool    *draw_tool);

static BlendOptions * blend_options_new          (void);

static void    blend_options_reset               (GimpToolOptions *tool_options);

static void    gradient_type_callback            (GtkWidget       *widget,
                                                  gpointer         data);
static void    blend_options_drop_gradient       (GtkWidget       *widget,
						  GimpViewable    *viewable,
						  gpointer         data);
static void    blend_options_drop_tool           (GtkWidget       *widget,
						  GimpViewable    *viewable,
						  gpointer         data);

static gdouble gradient_calc_conical_sym_factor  (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_conical_asym_factor (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_square_factor       (gdouble          dist,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_radial_factor   	 (gdouble          dist,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_linear_factor   	 (gdouble          dist,
						  gdouble         *vec,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_bilinear_factor 	 (gdouble          dist,
						  gdouble         *vec,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_spiral_factor       (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y,
						  gint             cwise);

static gdouble gradient_calc_shapeburst_angular_factor   (gdouble x,
							  gdouble y);
static gdouble gradient_calc_shapeburst_spherical_factor (gdouble x,
							  gdouble y);
static gdouble gradient_calc_shapeburst_dimpled_factor   (gdouble x,
							  gdouble y);

static gdouble gradient_repeat_none              (gdouble       val);
static gdouble gradient_repeat_sawtooth          (gdouble       val);
static gdouble gradient_repeat_triangular        (gdouble       val);

static void    gradient_precalc_shapeburst       (GimpImage    *gimage,
						  GimpDrawable *drawable, 
						  PixelRegion  *PR,
						  gdouble       dist);

static void    gradient_render_pixel             (gdouble       x,
						  gdouble       y,
						  GimpRGB      *color,
						  gpointer      render_data);
static void    gradient_put_pixel                (gint          x,
						  gint          y,
						  GimpRGB      *color,
						  gpointer      put_pixel_data);

static void    gradient_fill_region              (GimpImage    *gimage,
						  GimpDrawable *drawable,
						  PixelRegion  *PR,
						  gint          width,
						  gint          height,
						  BlendMode     blend_mode,
						  GradientType  gradient_type,
						  gdouble       offset,
						  RepeatMode    repeat,
						  gint          supersample,
						  gint          max_depth,
						  gdouble       threshold,
						  gdouble       sx,
						  gdouble       sy,
						  gdouble       ex,
						  gdouble       ey,
						  GimpProgressFunc progress_callback,
						  gpointer      progress_data);


static GimpDrawToolClass *parent_class = NULL;

static BlendOptions *blend_options = NULL;

/*  variables for the shapeburst algs  */
static PixelRegion distR =
{
  NULL,  /* data */
  NULL,  /* tiles */
  0,     /* rowstride */
  0, 0,  /* w, h */
  0, 0,  /* x, y */
  4,     /* bytes */
  0      /* process count */
};

/*  dnd stuff  */
static GtkTargetEntry blend_target_table[] =
{
  GIMP_TARGET_GRADIENT,  
  GIMP_TARGET_TOOL
};
static guint blend_n_targets = (sizeof (blend_target_table) /
				sizeof (blend_target_table[0]));


void
gimp_blend_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_BLEND_TOOL,
                              TRUE,
			      "gimp:blend_tool",
			      _("Blend"),
			      _("Fill with a color gradient"),
			      N_("/Tools/Paint Tools/Blend"), "L",
			      NULL, "tools/blend.html",
			      (const gchar **) blend_bits);
}

GtkType
gimp_blend_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
	"GimpBlendTool",
	sizeof (GimpBlendTool),
	sizeof (GimpBlendToolClass),
	(GtkClassInitFunc) gimp_blend_tool_class_init,
	(GtkObjectInitFunc) gimp_blend_tool_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_DRAW_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = (GtkObjectClass *) klass;
  tool_class      = (GimpToolClass *) klass;
  draw_tool_class = (GimpDrawToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAW_TOOL);

  object_class->destroy      = gimp_blend_tool_destroy;

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
 
  if (! blend_options)
    {
      blend_options = blend_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_BLEND_TOOL,
					  (GimpToolOptions *) blend_options);
    }

  tool->tool_cursor = GIMP_BLEND_TOOL_CURSOR;
  tool->scroll_lock = TRUE;  /*  Disallow scrolling  */
}

static void
gimp_blend_tool_destroy (GtkObject *object)
{
  /*  free the distance map data if it exists  */
  if (distR.tiles)
    tile_manager_destroy (distR.tiles);
  distR.tiles = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_blend_tool_button_press (GimpTool       *tool,
                              GdkEventButton *bevent,
                              GDisplay       *gdisp)
{
  GimpBlendTool *blend_tool;

  blend_tool = GIMP_BLEND_TOOL (tool);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      g_message (_("Blend: Invalid for indexed images."));
      return;

      break;
    default:
      break;
    }

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &blend_tool->startx, &blend_tool->starty,
			       FALSE, TRUE);

  blend_tool->endx = blend_tool->startx;
  blend_tool->endy = blend_tool->starty;

  /*  Make the tool active and set the gdisplay which owns it  */
  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->gdisp = gdisp;
  tool->state = ACTIVE;

  /* initialize the statusbar display */
  blend_tool->context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "blend");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      blend_tool->context_id, _("Blend: 0, 0"));

  /*  Start drawing the blend tool  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool),
                        gdisp->canvas->window);
}

static void
gimp_blend_tool_button_release (GimpTool       *tool,
                                GdkEventButton *bevent,
                                GDisplay       *gdisp)
{
  GimpImage     *gimage;
  GimpBlendTool *blend_tool;
#ifdef BLEND_UI_CALLS_VIA_PDB
  Argument      *return_vals;
  gint           nreturn_vals;
#else
  GimpProgress  *progress;
#endif

  blend_tool = GIMP_BLEND_TOOL (tool);

  gimage = gdisp->gimage;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK) &&
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
				PDB_INT32, (gint32) blend_options->blend_mode,
				PDB_INT32, (gint32) PAINT_OPTIONS_GET_PAINT_MODE (blend_options),
				PDB_INT32, (gint32) blend_options->gradient_type,
				PDB_FLOAT, (gdouble) PAINT_OPTIONS_GET_OPACITY (blend_options) * 100,
				PDB_FLOAT, (gdouble) blend_options->offset,
				PDB_INT32, (gint32) blend_options->repeat,
				PDB_INT32, (gint32) blend_options->supersample,
				PDB_INT32, (gint32) blend_options->max_depth,
				PDB_FLOAT, (gdouble) blend_options->threshold,
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

      blend (gimage,
	     gimp_image_active_drawable (gimage),
	     blend_options->blend_mode,
	     gimp_context_get_paint_mode (gimp_get_current_context (gimage->gimp)),
	     blend_options->gradient_type,
	     gimp_context_get_opacity (gimp_get_current_context (gimage->gimp)) * 100,
	     blend_options->offset,
	     blend_options->repeat,
	     blend_options->supersample,
	     blend_options->max_depth,
	     blend_options->threshold,
	     blend_tool->startx,
	     blend_tool->starty,
	     blend_tool->endx,
	     blend_tool->endy,
	     progress ? progress_update_and_flush : (GimpProgressFunc) NULL, 
	     progress);

      if (progress)
	progress_end (progress);

      gdisplays_flush ();
#endif /* ! BLEND_UI_CALLS_VIA_PDB */
    }
}

static void
gimp_blend_tool_motion (GimpTool       *tool,
                        GdkEventMotion *mevent,
                        GDisplay       *gdisp)
{
  GimpBlendTool *blend_tool;
  gchar          vector[STATUSBAR_SIZE];

  blend_tool = GIMP_BLEND_TOOL (tool);

  /*  undraw the current tool  */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  Get the current coordinates  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &blend_tool->endx, &blend_tool->endy, FALSE, 1);


  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (mevent->state & GDK_CONTROL_MASK)
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

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id);

  if (gdisp->dot_for_dot)
    {
      g_snprintf (vector, sizeof (vector), gdisp->cursor_format_str,
		  _("Blend: "),
		  blend_tool->endx - blend_tool->startx,
		  ", ",
		  blend_tool->endy - blend_tool->starty);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (vector, sizeof (vector), gdisp->cursor_format_str,
		  _("Blend: "),
		  blend_tool->endx - blend_tool->startx * unit_factor /
		  gdisp->gimage->xresolution,
		  ", ",
		  blend_tool->endy - blend_tool->starty * unit_factor /
		  gdisp->gimage->yresolution);
    }

  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), blend_tool->context_id,
		      vector);

  /*  redraw the current tool  */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_blend_tool_cursor_update (GimpTool       *tool,
                               GdkEventMotion *mevent,
                               GDisplay       *gdisp)
{
  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case INDEXED_GIMAGE:
    case INDEXEDA_GIMAGE:
      gdisplay_install_tool_cursor (gdisp,
				    GIMP_BAD_CURSOR,
				    GIMP_BLEND_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_NONE);
      break;
    default:
      gdisplay_install_tool_cursor (gdisp,
				    GIMP_MOUSE_CURSOR,
				    GIMP_BLEND_TOOL_CURSOR,
				    GIMP_CURSOR_MODIFIER_NONE);
      break;
    }
}

static void
gimp_blend_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool      *tool;
  GimpBlendTool *blend_tool;
  gint           tx1, ty1, tx2, ty2;

  tool       = GIMP_TOOL (draw_tool);
  blend_tool = GIMP_BLEND_TOOL (draw_tool);

  gdisplay_transform_coords (tool->gdisp,
			     blend_tool->startx, blend_tool->starty,
			     &tx1, &ty1, 1);
  gdisplay_transform_coords (tool->gdisp,
			     blend_tool->endx, blend_tool->endy,
			     &tx2, &ty2, 1);

  /*  Draw start target  */
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 tx1 - (TARGET_WIDTH >> 1), ty1,
		 tx1 + (TARGET_WIDTH >> 1), ty1);
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 tx1, ty1 - (TARGET_HEIGHT >> 1),
		 tx1, ty1 + (TARGET_HEIGHT >> 1));

  /*  Draw end target  */
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 tx2 - (TARGET_WIDTH >> 1), ty2,
		 tx2 + (TARGET_WIDTH >> 1), ty2);
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 tx2, ty2 - (TARGET_HEIGHT >> 1),
		 tx2, ty2 + (TARGET_HEIGHT >> 1));

  /*  Draw the line between the start and end coords  */
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 tx1, ty1, tx2, ty2);
}

static BlendOptions *
blend_options_new (void)
{
  BlendOptions *options;

  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *scale;
  GtkWidget *frame;

  /*  the new blend tool options structure  */
  options = g_new0 (BlendOptions, 1);

  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_BLEND_TOOL,
		      blend_options_reset);

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
		     blend_target_table, blend_n_targets,
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
  gtk_signal_connect (GTK_OBJECT (options->offset_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
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
     &options->gradient_type,
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
  gtk_signal_connect (GTK_OBJECT (options->supersample_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->supersample);
  gtk_box_pack_start (GTK_BOX (vbox), options->supersample_w, FALSE, FALSE, 0);
  gtk_widget_show (options->supersample_w);

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
  gtk_signal_connect (GTK_OBJECT(options->max_depth_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
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
  gtk_signal_connect (GTK_OBJECT(options->threshold_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->threshold);

  /*  show the table  */
  gtk_widget_show (table);

  return options;
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
  gimp_menu_item_update (widget, data);

  gtk_widget_set_sensitive (blend_options->repeat_w, 
			    (blend_options->gradient_type < 6));
}

static void
blend_options_drop_gradient (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  BlendOptions *options;

  options = (BlendOptions *) data;

  gimp_context_set_gradient (gimp_get_user_context (the_gimp),
			     GIMP_GRADIENT (viewable));

  gtk_option_menu_set_history (GTK_OPTION_MENU (options->blend_mode_w), 
			       CUSTOM_MODE);
  options->blend_mode = CUSTOM_MODE;
}

static void
blend_options_drop_tool (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  gimp_context_set_tool (gimp_get_user_context (the_gimp),
			 GIMP_TOOL_INFO (viewable));
}


/*  The actual blending procedure  */

void
blend (GimpImage        *gimage,
       GimpDrawable     *drawable,
       BlendMode         blend_mode,
       int               paint_mode,
       GradientType      gradient_type,
       double            opacity,
       double            offset,
       RepeatMode        repeat,
       int               supersample,
       int               max_depth,
       double            threshold,
       double            startx,
       double            starty,
       double            endx,
       double            endy,
       GimpProgressFunc  progress_callback,
       gpointer          progress_data)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         has_alpha;
  gint         has_selection;
  gint         bytes;
  gint         x1, y1, x2, y2;

  gimp_set_busy (gimage->gimp);

  has_selection = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  has_alpha = gimp_drawable_has_alpha (drawable);
  bytes = gimp_drawable_bytes (drawable);

  /*  Always create an alpha temp buf (for generality) */
  if (! has_alpha)
    {
      has_alpha = TRUE;
      bytes += 1;
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  gradient_fill_region (gimage, drawable,
  			&bufPR, (x2 - x1), (y2 - y1),
			blend_mode, gradient_type, offset, repeat,
			supersample, max_depth, threshold,
			(startx - x1), (starty - y1),
			(endx - x1), (endy - y1),
			progress_callback, progress_data);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE,
			  (opacity * 255) / 100, paint_mode, NULL, x1, y1);

  /*  update the image  */
  gimp_drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);

  gimp_unset_busy (gimage->gimp);
}

static gdouble
gradient_calc_conical_sym_factor (gdouble  dist,
				  gdouble *axis,
				  gdouble  offset,
				  gdouble  x,
				  gdouble  y)
{
  gdouble vec[2];
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else if ((x != 0) || (y != 0))
    {
      /* Calculate offset from the start in pixels */

      r = sqrt (x * x + y * y);

      vec[0] = x / r;
      vec[1] = y / r;

      rat = axis[0] * vec[0] + axis[1] * vec[1]; /* Dot product */

      if (rat > 1.0)
	rat = 1.0;
      else if (rat < -1.0)
	rat = -1.0;

      /* This cool idea is courtesy Josh MacDonald,
       * Ali Rahimi --- two more XCF losers.  */

      rat = acos (rat) / G_PI;
      rat = pow (rat, (offset / 10) + 1);

      rat = CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      rat = 0.5;
    }

  return rat;
}

static gdouble
gradient_calc_conical_asym_factor (gdouble  dist,
				   gdouble *axis,
				   gdouble  offset,
				   gdouble  x,
				   gdouble  y)
{
  gdouble ang0, ang1;
  gdouble ang;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      if ((x != 0) || (y != 0))
	{
	  ang0 = atan2(axis[0], axis[1]) + G_PI;
	  ang1 = atan2(x, y) + G_PI;

	  ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * G_PI);

	  rat = ang / (2.0 * G_PI);
	  rat = pow(rat, (offset / 10) + 1);

	  rat = CLAMP(rat, 0.0, 1.0);
	}
      else
	{
	  rat = 0.5; /* We are on middle point */
	}
    }

  return rat;
}

static gdouble
gradient_calc_square_factor (gdouble dist,
			     gdouble offset,
			     gdouble x,
			     gdouble y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = MAX (abs (x), abs (y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_radial_factor (gdouble dist,
			     gdouble offset,
			     gdouble x,
			     gdouble y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate radial offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = sqrt(SQR(x) + SQR(y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_linear_factor (gdouble  dist,
			     gdouble *vec,
			     gdouble  offset,
			     gdouble  x,
			     gdouble  y)
{
  double r;
  double rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_bilinear_factor (gdouble  dist,
			       gdouble *vec,
			       gdouble  offset,
			       gdouble  x,
			       gdouble  y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate linear offset from the start line outward */

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (fabs(rat) < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (fabs(rat) - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_spiral_factor (gdouble  dist,
			     gdouble *axis,
			     gdouble  offset,
			     gdouble  x,
			     gdouble  y,
			     gint     cwise)
{
  gdouble ang0, ang1;
  gdouble ang, r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      if (x != 0.0 || y != 0.0)
	{
	  ang0 = atan2 (axis[0], axis[1]) + G_PI;
	  ang1 = atan2 (x, y) + G_PI;
	  if(!cwise)
	    ang = ang0 - ang1;
	  else
	    ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * G_PI);

	  r = sqrt (x * x + y * y) / dist;
	  rat = ang / (2.0 * G_PI) + r + offset;
	  rat = fmod (rat, 1.0);
	}
      else
	rat = 0.5 ; /* We are on the middle point */
    }

  return rat;
}

static gdouble
gradient_calc_shapeburst_angular_factor (gdouble x,
					 gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = 1.0 - *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  tile_release (tile, FALSE);

  return value;
}


static gdouble
gradient_calc_shapeburst_spherical_factor (gdouble x,
					   gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((gfloat *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  value = 1.0 - sin (0.5 * G_PI * value);
  tile_release (tile, FALSE);

  return value;
}


static gdouble
gradient_calc_shapeburst_dimpled_factor (gdouble x,
					 gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  value = cos (0.5 * G_PI * value);
  tile_release (tile, FALSE);

  return value;
}

static gdouble
gradient_repeat_none (gdouble val)
{
  return CLAMP (val, 0.0, 1.0);
}

static gdouble
gradient_repeat_sawtooth (gdouble val)
{
  if (val >= 0.0)
    return fmod (val, 1.0);
  else
    return 1.0 - fmod (-val, 1.0);
}

static gdouble
gradient_repeat_triangular (gdouble val)
{
  gint ival;

  if (val < 0.0)
    val = -val;

  ival = (int) val;

  if (ival & 1)
    return 1.0 - fmod (val, 1.0);
  else
    return fmod (val, 1.0);
}

/*****/
static void
gradient_precalc_shapeburst (GimpImage    *gimage,
			     GimpDrawable *drawable,
			     PixelRegion  *PR,
			     gdouble       dist)
{
  GimpChannel *mask;
  PixelRegion  tempR;
  gfloat       max_iteration;
  gfloat      *distp;
  gint         size;
  gpointer     pr;
  guchar       white[1] = { OPAQUE_OPACITY };

  /*  allocate the distance map  */
  if (distR.tiles)
    tile_manager_destroy (distR.tiles);
  distR.tiles = tile_manager_new (PR->w, PR->h, sizeof (gfloat));

  /*  allocate the selection mask copy  */
  tempR.tiles = tile_manager_new (PR->w, PR->h, 1);
  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);

  /*  If the gimage mask is not empty, use it as the shape burst source  */
  if (! gimage_mask_is_empty (gimage))
    {
      PixelRegion maskR;
      gint        x1, y1, x2, y2;
      gint        offx, offy;

      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      gimp_drawable_offsets (drawable, &offx, &offy);

      /*  the selection mask  */
      mask = gimp_image_get_mask (gimage);
      pixel_region_init (&maskR, gimp_drawable_data (GIMP_DRAWABLE (mask)),
			 x1 + offx, y1 + offy, (x2 - x1), (y2 - y1), FALSE);

      /*  copy the mask to the temp mask  */
      copy_region (&maskR, &tempR);
    }
  /*  otherwise...  */
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (gimp_drawable_has_alpha (drawable))
	{
	  PixelRegion drawableR;

	  pixel_region_init (&drawableR, gimp_drawable_data (drawable),
			     PR->x, PR->y, PR->w, PR->h, FALSE);

	  extract_alpha_region (&drawableR, NULL, &tempR);
	}
      else
	{
	  /*  Otherwise, just fill the shapeburst to white  */
	  color_region (&tempR, white);
	}
    }

  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);
  pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);
  max_iteration = shapeburst_region (&tempR, &distR);

  /*  normalize the shapeburst with the max iteration  */
  if (max_iteration > 0)
    {
      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);

      for (pr = pixel_regions_register (1, &distR);
	   pr != NULL;
	   pr = pixel_regions_process (pr))
	{
	  distp = (gfloat *) distR.data;
	  size  = distR.w * distR.h;

	  while (size--)
	    *distp++ /= max_iteration;
	}

      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, FALSE);
    }

  tile_manager_destroy (tempR.tiles);
}


static void
gradient_render_pixel (double    x, 
		       double    y, 
		       GimpRGB  *color, 
		       gpointer  render_data)
{
  RenderBlendData *rbd;
  gdouble          factor;

  rbd = render_data;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case RADIAL:
      factor = gradient_calc_radial_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor (rbd->dist, rbd->vec, rbd->offset,
						 x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor (rbd->dist, rbd->vec, rbd->offset,
						  x - rbd->sx, y - rbd->sy);
      break;

    case SQUARE:
      factor = gradient_calc_square_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case LINEAR:
      factor = gradient_calc_linear_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case BILINEAR:
      factor = gradient_calc_bilinear_factor (rbd->dist, rbd->vec, rbd->offset,
					      x - rbd->sx, y - rbd->sy);
      break;

    case SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor (x, y);
      break;

    case SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor (x, y);
      break;

    case SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor (x, y);
      break;

    case SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,TRUE);
      break;

    case SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,FALSE);
      break;

    default:
      gimp_fatal_error ("gradient_render_pixel(): Unknown gradient type %d",
			(int) rbd->gradient_type);
      return;
    }

  /* Adjust for repeat */

  factor = (*rbd->repeat_func)(factor);

  /* Blend the colors */

  if (rbd->blend_mode == CUSTOM_MODE)
    {
      gimp_gradient_get_color_at (rbd->gradient, factor, color);
    }
  else
    {
      /* Blend values */

      color->r = rbd->fg.r + (rbd->bg.r - rbd->fg.r) * factor;
      color->g = rbd->fg.g + (rbd->bg.g - rbd->fg.g) * factor;
      color->b = rbd->fg.b + (rbd->bg.b - rbd->fg.b) * factor;
      color->a = rbd->fg.a + (rbd->bg.a - rbd->fg.a) * factor;

      if (rbd->blend_mode == FG_BG_HSV_MODE)
	gimp_hsv_to_rgb_double (&color->r, &color->g, &color->b);
    }
}

static void
gradient_put_pixel (int      x, 
		    int      y, 
		    GimpRGB *color, 
		    void    *put_pixel_data)
{
  PutPixelData  *ppd;
  guchar        *data;

  ppd = put_pixel_data;

  /* Paint */

  data = ppd->row_data + ppd->bytes * x;

  if (ppd->bytes >= 3)
    {
      *data++ = color->r * 255.0;
      *data++ = color->g * 255.0;
      *data++ = color->b * 255.0;
      *data++ = color->a * 255.0;
    }
  else
    {
      /* Convert to grayscale */

      *data++ = 255.0 * INTENSITY (color->r, color->g, color->b);
      *data++ = color->a * 255.0;
    }

  /* Paint whole row if we are on the rightmost pixel */

  if (x == (ppd->width - 1))
    pixel_region_set_row(ppd->PR, 0, y, ppd->width, ppd->row_data);
}

static void
gradient_fill_region (GimpImage        *gimage,
		      GimpDrawable     *drawable,
		      PixelRegion      *PR,
		      gint              width,
		      gint              height,
		      BlendMode         blend_mode,
		      GradientType      gradient_type,
		      gdouble           offset,
		      RepeatMode        repeat,
		      gint              supersample,
		      gint              max_depth,
		      gdouble           threshold,
		      gdouble           sx,
		      gdouble           sy,
		      gdouble           ex,
		      gdouble           ey,
		      GimpProgressFunc  progress_callback,
		      gpointer          progress_data)
{
  RenderBlendData  rbd;
  PutPixelData     ppd;
  gint             x, y;
  gint             endx, endy;
  gpointer        *pr;
  guchar          *data;
  GimpRGB          color;
  GimpContext     *context;

  context = gimp_get_current_context (gimage->gimp);

  rbd.gradient = gimp_context_get_gradient (context);

  /* Get foreground and background colors, normalized */

  gimp_context_get_foreground (context, &rbd.fg);

  /* rbd.fg.a = 1.0; */ /* Foreground is always opaque */

  gimp_context_get_background (context, &rbd.bg);

  /* rbd.bg.a = 1.0; */ /* opaque, for now */

  switch (blend_mode)
    {
    case FG_BG_RGB_MODE:
      break;

    case FG_BG_HSV_MODE:
      /* Convert to HSV */

      gimp_rgb_to_hsv_double (&rbd.fg.r, &rbd.fg.g, &rbd.fg.b);
      gimp_rgb_to_hsv_double (&rbd.bg.r, &rbd.bg.g, &rbd.bg.b);

      break;

    case FG_TRANS_MODE:
      /* Color does not change, just the opacity */

      rbd.bg   = rbd.fg;
      rbd.bg.a = 0.0; /* transparent */

      break;

    case CUSTOM_MODE:
      break;

    default:
      gimp_fatal_error ("gradient_fill_region(): Unknown blend mode %d",
			(gint) blend_mode);
      break;
    }

  /* Calculate type-specific parameters */

  switch (gradient_type)
    {
    case RADIAL:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      break;

    case SQUARE:
      rbd.dist = MAX (fabs (ex - sx), fabs (ey - sy));
      break;

    case CONICAL_SYMMETRIC:
    case CONICAL_ASYMMETRIC:
    case SPIRAL_CLOCKWISE:
    case SPIRAL_ANTICLOCKWISE:
    case LINEAR:
    case BILINEAR:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));

      if (rbd.dist > 0.0)
	{
	  rbd.vec[0] = (ex - sx) / rbd.dist;
	  rbd.vec[1] = (ey - sy) / rbd.dist;
	}

      break;

    case SHAPEBURST_ANGULAR:
    case SHAPEBURST_SPHERICAL:
    case SHAPEBURST_DIMPLED:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      gradient_precalc_shapeburst (gimage, drawable, PR, rbd.dist);
      break;

    default:
      gimp_fatal_error ("gradient_fill_region(): Unknown gradient type %d",
			(gint) gradient_type);
      break;
    }

  /* Set repeat function */

  switch (repeat)
    {
    case REPEAT_NONE:
      rbd.repeat_func = gradient_repeat_none;
      break;

    case REPEAT_SAWTOOTH:
      rbd.repeat_func = gradient_repeat_sawtooth;
      break;

    case REPEAT_TRIANGULAR:
      rbd.repeat_func = gradient_repeat_triangular;
      break;

    default:
      gimp_fatal_error ("gradient_fill_region(): Unknown repeat mode %d",
			(gint) repeat);
      break;
    }

  /* Initialize render data */

  rbd.offset        = offset;
  rbd.sx            = sx;
  rbd.sy            = sy;
  rbd.blend_mode    = blend_mode;
  rbd.gradient_type = gradient_type;

  /* Render the gradient! */

  if (supersample)
    {
      /* Initialize put pixel data */

      ppd.PR       = PR;
      ppd.row_data = g_malloc (width * PR->bytes);
      ppd.bytes    = PR->bytes;
      ppd.width    = width;

      /* Render! */

      gimp_adaptive_supersample_area (0, 0, (width - 1), (height - 1),
				      max_depth, threshold,
				      gradient_render_pixel, &rbd,
				      gradient_put_pixel, &ppd,
				      progress_callback, progress_data);

      /* Clean up */

      g_free (ppd.row_data);
    }
  else
    {
      gint max_progress = PR->w * PR->h;
      gint progress = 0;

      for (pr = pixel_regions_register(1, PR);
	   pr != NULL;
	   pr = pixel_regions_process(pr))
	{
	  data = PR->data;
	  endx = PR->x + PR->w;
	  endy = PR->y + PR->h;

	  for (y = PR->y; y < endy; y++)
	    for (x = PR->x; x < endx; x++)
	      {
		gradient_render_pixel(x, y, &color, &rbd);

		if (PR->bytes >= 3)
		  {
		    *data++ = color.r * 255.0;
		    *data++ = color.g * 255.0;
		    *data++ = color.b * 255.0;
		    *data++ = color.a * 255.0;
		  }
		else
		  {
		    /* Convert to grayscale */

		    *data++ = 255.0 * INTENSITY (color.r, color.g, color.b);
		    *data++ = color.a * 255.0;
		  }
	      }

	  progress += PR->w * PR->h;
	  if (progress_callback)
	    (* progress_callback) (0, max_progress, progress, progress_data);
	}
    }
}
