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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimplut.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gdisplay.h"

#include "gimpdodgeburntool.h"
#include "gimppainttool.h"
#include "paint_options.h"
#include "gimptool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#define WANT_DODGE_BITS
#include "icons.h"


/*  Default values  */

#define DODGEBURN_DEFAULT_EXPOSURE 50.0
#define DODGEBURN_DEFAULT_TYPE     DODGE
#define DODGEBURN_DEFAULT_MODE     DODGEBURN_HIGHLIGHTS

/*  the dodgeburn structures  */

typedef struct _DodgeBurnOptions DodgeBurnOptions;

struct _DodgeBurnOptions
{
  PaintOptions   paint_options;

  DodgeBurnType  type;
  DodgeBurnType  type_d;
  GtkWidget     *type_w[2];

  DodgeBurnMode  mode;     /*highlights, midtones, shadows*/
  DodgeBurnMode  mode_d;
  GtkWidget     *mode_w[3];

  gdouble        exposure;
  gdouble        exposure_d;
  GtkObject     *exposure_w;

  GimpLut       *lut;
};


static void     gimp_dodgeburn_tool_class_init (GimpDodgeBurnToolClass *klass);
static void     gimp_dodgeburn_tool_init       (GimpDodgeBurnTool      *dodgeburn);

static void     gimp_dodgeburn_tool_make_luts     (GimpPaintTool  *paint_tool,
						   gdouble         db_exposure,
						   DodgeBurnType   type,
						   DodgeBurnMode   mode,
						   GimpLut        *lut,
						   GimpDrawable   *drawable);

static void     gimp_dodgeburn_tool_modifier_key  (GimpTool       *tool,
						   GdkEventKey    *kevent,
						   GDisplay       *gdisp);
static void     gimp_dodgeburn_tool_cursor_update (GimpTool       *tool,
						   GdkEventMotion *mevent,
						   GDisplay       *gdisp);

static void     gimp_dodgeburn_tool_paint         (GimpPaintTool  *paint_tool,
						   GimpDrawable   *drawable,
						   PaintState      state);

static void     gimp_dodgeburn_tool_motion        (GimpPaintTool        *paint_tool,
						   GimpDrawable         *drawable,
						   PaintPressureOptions *pressure_options,
						   gdouble               dodgeburn_exposure,
						   GimpLut              *lut);

static gfloat   gimp_dodgeburn_tool_highlights_lut_func (gpointer       user_data,
					                 gint           nchannels,
					                 gint           channel,
					                 gfloat         value);
static gfloat   gimp_dodgeburn_tool_midtones_lut_func   (gpointer       user_data,
					                 gint           nchannels,
					                 gint           channel,
					                 gfloat         value);
static gfloat   gimp_dodgeburn_tool_shadows_lut_func    (gpointer       user_data,
					                 gint           nchannels,
					                 gint           channel,
					                 gfloat         value);

static DodgeBurnOptions * gimp_dodgeburn_tool_options_new   (void);
static void               gimp_dodgeburn_tool_options_reset (ToolOptions *tool_options);


static gdouble  non_gui_exposure;
static GimpLut *non_gui_lut;

static DodgeBurnOptions * dodgeburn_options = NULL;

static GimpPaintToolClass *parent_class = NULL;


/* functions  */

void
gimp_dodgeburn_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_DODGEBURN_TOOL,
                              TRUE,
			      "gimp:dodgeburn_tool",
			      _("Dodge/Burn"),
			      _("Dodge or Burn strokes"),
			      N_("/Tools/Paint Tools/DodgeBurn"), "<shift>D",
			      NULL, "tools/dodgeburn.html",
			      (const gchar **) dodge_bits);
}

GtkType
gimp_dodgeburn_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (!tool_type)
    {
      GtkTypeInfo tool_info =
	{
	  "GimpDodgeBurnTool",
	  sizeof(GimpDodgeBurnTool),
	  sizeof(GimpDodgeBurnToolClass),
	  (GtkClassInitFunc) gimp_dodgeburn_tool_class_init,
	  (GtkObjectInitFunc) gimp_dodgeburn_tool_init,
	  /* reserved_1 */ NULL,
	  /* reserved_2 */ NULL,
	  NULL
	};

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_dodgeburn_tool_class_init (GimpDodgeBurnToolClass *klass)
{
  GimpToolClass	     *tool_class;
  GimpPaintToolClass *paint_tool_class;

  tool_class	   = (GimpToolClass *) klass;
  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  tool_class->modifier_key  = gimp_dodgeburn_tool_modifier_key;
  tool_class->cursor_update = gimp_dodgeburn_tool_cursor_update;

  paint_tool_class->paint   = gimp_dodgeburn_tool_paint;
}

static void
gimp_dodgeburn_tool_init (GimpDodgeBurnTool *dodgeburn)
{
  GimpTool	*tool;
  GimpPaintTool *paint_tool;

  tool = GIMP_TOOL (dodgeburn);
  paint_tool = GIMP_PAINT_TOOL (dodgeburn);
  
  if (! dodgeburn_options)
    {
      dodgeburn_options = gimp_dodgeburn_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_DODGEBURN_TOOL, 
	                                  (ToolOptions *) dodgeburn_options);
    }

  tool->tool_cursor   = GIMP_DODGE_TOOL_CURSOR;
  tool->toggle_cursor = GIMP_BURN_TOOL_CURSOR;

  paint_tool->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}

static void 
gimp_dodgeburn_tool_make_luts (GimpPaintTool *paint_tool,
  		               gdouble        db_exposure,
		               DodgeBurnType  type,
		               DodgeBurnMode  mode,
		               GimpLut       *lut,
		               GimpDrawable  *drawable)
{
  GimpLutFunc   lut_func;
  gint          nchannels = gimp_drawable_bytes (drawable);
  static gfloat exposure;

  exposure = db_exposure / 100.0;

  /* make the exposure negative if burn for luts*/
  if (type == BURN)
    exposure = -exposure;

  switch (mode)
    {
    case DODGEBURN_HIGHLIGHTS:
      lut_func = gimp_dodgeburn_tool_highlights_lut_func; 
      break;
    case DODGEBURN_MIDTONES:
      lut_func = gimp_dodgeburn_tool_midtones_lut_func; 
      break;
    case DODGEBURN_SHADOWS:
      lut_func = gimp_dodgeburn_tool_shadows_lut_func; 
      break;
    default:
      lut_func = NULL; 
      break;
    }

  gimp_lut_setup_exact (lut,
			lut_func, (gpointer) &exposure,
			nchannels);
}

static void
gimp_dodgeburn_tool_modifier_key (GimpTool    *tool,
				  GdkEventKey *kevent,
				  GDisplay    *gdisp)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: 
    case GDK_Alt_R:
      break;

    case GDK_Shift_L: 
    case GDK_Shift_R:
      if (kevent->state & GDK_CONTROL_MASK)    /* reset tool toggle */
	{
	  switch (dodgeburn_options->type)
	    {
	    case BURN:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[DODGE]), TRUE);
	      break;
	    case DODGE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[BURN]), TRUE);
	      break;
	    default:
	      break;
	    }
	}
      break;

    case GDK_Control_L:
    case GDK_Control_R:
      if (! (kevent->state & GDK_SHIFT_MASK)) /* shift enables line draw mode */
	{
	  switch (dodgeburn_options->type)
	    {
	    case BURN:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[DODGE]), TRUE);
	      break;
	    case DODGE:
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (dodgeburn_options->type_w[BURN]), TRUE);
	      break;
	    default:
	      break;
	    }
	}
      break; 
    }

  tool->toggled = (dodgeburn_options->type == BURN);
}

static void
gimp_dodgeburn_tool_cursor_update (GimpTool       *tool,
				   GdkEventMotion *mevent,
				   GDisplay       *gdisp)
{
  tool->toggled = (dodgeburn_options->type == BURN);

  if (GIMP_TOOL_CLASS (parent_class)->cursor_update)
    GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, mevent, gdisp);
}

static void
gimp_dodgeburn_tool_paint (GimpPaintTool    *paint_tool,
		           GimpDrawable     *drawable,
		           PaintState        state)
{
  PaintPressureOptions *pressure_options;
  gdouble               exposure;
  GimpLut              *lut;

  if (dodgeburn_options)
    {
      pressure_options = dodgeburn_options->paint_options.pressure_options;
      exposure         = dodgeburn_options->exposure;
      lut              = dodgeburn_options->lut;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      exposure         = non_gui_exposure;
      lut              = non_gui_lut;
    }

  switch (state)
    {
    case INIT_PAINT:
      if (dodgeburn_options)
	{
	  dodgeburn_options->lut = gimp_lut_new ();
	  gimp_dodgeburn_tool_make_luts (paint_tool,
					 dodgeburn_options->exposure,
					 dodgeburn_options->type,
					 dodgeburn_options->mode,
					 dodgeburn_options->lut,
					 drawable);

	  lut = dodgeburn_options->lut;
	}
      break;

    case MOTION_PAINT:
      gimp_dodgeburn_tool_motion (paint_tool,
				  drawable,
			          pressure_options,
			          exposure, 
				  lut);
      break;

    case FINISH_PAINT:
      if (dodgeburn_options && dodgeburn_options->lut)
	{
	  gimp_lut_free (dodgeburn_options->lut);
	  dodgeburn_options->lut = NULL;
	}
      break;

    default:
      break;
    }
}

static void
gimp_dodgeburn_tool_motion (GimpPaintTool        *paint_tool,
			    GimpDrawable         *drawable,
 		            PaintPressureOptions *pressure_options,
		            double                dodgeburn_exposure,
		            GimpLut              *lut)
{
  GimpImage   *gimage;
  TempBuf     *area;
  TempBuf     *orig;
  PixelRegion  srcPR, destPR, tempPR;
  guchar      *temp_data;
  gint         opacity;
  gdouble      scale;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  If the image type is indexed, don't dodgeburn  */
  if ((gimp_drawable_type (drawable) == INDEXED_GIMAGE) ||
      (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE))
    return;

  if (pressure_options->size)
    scale = paint_tool->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  /* Constant painting --get a copy of the orig drawable (with no
   * paint from this stroke yet)
   */
  {
    gint x1, y1, x2, y2;

    x1 = CLAMP (area->x, 0, gimp_drawable_width (drawable));
    y1 = CLAMP (area->y, 0, gimp_drawable_height (drawable));
    x2 = CLAMP (area->x + area->width,  0, gimp_drawable_width (drawable));
    y2 = CLAMP (area->y + area->height, 0, gimp_drawable_height (drawable));

    if (!(x2 - x1) || !(y2 - y1))
      return;

    /*  get the original untouched image  */
    orig = gimp_paint_tool_get_orig_image (paint_tool, drawable, x1, y1, x2, y2);
    srcPR.bytes = orig->bytes;
    srcPR.x = 0; 
    srcPR.y = 0;
    srcPR.w = x2 - x1;
    srcPR.h = y2 - y1;
    srcPR.rowstride = srcPR.bytes * orig->width;
    srcPR.data = temp_buf_data (orig);
  }

  /* tempPR will hold the dodgeburned region*/
  tempPR.bytes = srcPR.bytes;
  tempPR.x = srcPR.x; 
  tempPR.y = srcPR.y;
  tempPR.w = srcPR.w;
  tempPR.h = srcPR.h;
  tempPR.rowstride = tempPR.bytes * tempPR.w;
  temp_data = g_malloc (tempPR.h * tempPR.rowstride);
  tempPR.data = temp_data;

  /*  DodgeBurn the region  */
  gimp_lut_process (lut, &srcPR, &tempPR);

  /* The dest is the paint area we got above (= canvas_buf) */ 
  destPR.bytes = area->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = area->width;
  destPR.h = area->height;
  destPR.rowstride = area->width * destPR.bytes;
  destPR.data = temp_buf_data (area);

  /* Now add an alpha to the dodgeburned region 
     and put this in area = canvas_buf */ 
  if (! gimp_drawable_has_alpha (drawable))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  opacity =
    255 * gimp_context_get_opacity (gimp_get_current_context (gimage->gimp));

  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->curpressure;

  /* Replace the newly dodgedburned area (canvas_buf) to the gimage*/   
  gimp_paint_tool_replace_canvas (paint_tool, drawable, 
			          MIN (opacity, 255),
		                  OPAQUE_OPACITY, 
			          pressure_options->pressure ? PRESSURE : SOFT,
			          scale, CONSTANT);
 
  g_free (temp_data);
}

static gfloat 
gimp_dodgeburn_tool_highlights_lut_func (gpointer  user_data, 
			                 gint      nchannels, 
			                 gint      channel, 
			                 gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  factor       = 1.0 + exposure * (.333333);

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  return factor * value;
}

static gfloat 
gimp_dodgeburn_tool_midtones_lut_func (gpointer  user_data, 
			               gint      nchannels, 
			               gint      channel, 
			               gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  factor;

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);

  return pow (value, factor); 
}

static gfloat 
gimp_dodgeburn_tool_shadows_lut_func (gpointer  user_data, 
			              gint      nchannels, 
			              gint      channel, 
			              gfloat    value)
{
  gfloat *exposure_ptr = (gfloat *) user_data;
  gfloat  exposure     = *exposure_ptr;
  gfloat  new_value;
  gfloat  factor;

  if ((nchannels == 2 && channel == 1) ||
      (nchannels == 4 && channel == 3))
    return value;

  if (exposure >= 0)
    {
      factor = 0.333333 * exposure;
      new_value =  factor + value - factor * value; 
    }
  else /* exposure < 0 */ 
    {
      factor = -0.333333 * exposure;
      if (value < factor)
	new_value = 0;
      else /*factor <= value <=1*/
	new_value = (value - factor)/(1 - factor);
    }

  return new_value; 
}


/*  non-gui stuff  */

gboolean
gimp_dodgeburn_tool_non_gui_default (GimpDrawable *drawable,
			             gint          num_strokes,
			             gdouble      *stroke_array)
{
  gdouble           exposure = DODGEBURN_DEFAULT_EXPOSURE;
  DodgeBurnType     type     = DODGEBURN_DEFAULT_TYPE;
  DodgeBurnMode     mode     = DODGEBURN_DEFAULT_MODE;
  DodgeBurnOptions *options  = dodgeburn_options;

  if (options)
    {
      exposure = dodgeburn_options->exposure;
      type     = dodgeburn_options->type;
      mode     = dodgeburn_options->mode;
    }

  return gimp_dodgeburn_tool_non_gui (drawable, exposure, type, mode,
			              num_strokes, stroke_array);
}

gboolean
gimp_dodgeburn_tool_non_gui (GimpDrawable  *drawable,
		             gdouble        exposure,
		             DodgeBurnType  type,
		             DodgeBurnMode  mode,
		             gint           num_strokes,
		             gdouble       *stroke_array)
{
  static GimpDodgeBurnTool *non_gui_dodgeburn = NULL;

  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_dodgeburn)
    {
      non_gui_dodgeburn = gtk_type_new (GIMP_TYPE_DODGEBURN_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_dodgeburn);
  
  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0], 
			     stroke_array[1]))
    {
      non_gui_exposure = exposure;
      non_gui_lut      = gimp_lut_new ();
      gimp_dodgeburn_tool_make_luts (paint_tool, 
			             exposure,
			             type,
			             mode,
			             non_gui_lut,
			             drawable);

      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      gimp_dodgeburn_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->curx = stroke_array[i * 2 + 0];
	  paint_tool->cury = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->lastx = paint_tool->curx;
	  paint_tool->lasty = paint_tool->cury;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      gimp_lut_free (non_gui_lut);
      non_gui_lut = NULL;

      return TRUE;
    }

  return FALSE;
}


/*  tool options stuff  */

static DodgeBurnOptions *
gimp_dodgeburn_tool_options_new (void)
{
  DodgeBurnOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  options = g_new0 (DodgeBurnOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_DODGEBURN_TOOL,
		      gimp_dodgeburn_tool_options_reset);

  options->type     = options->type_d     = DODGEBURN_DEFAULT_TYPE;
  options->exposure = options->exposure_d = DODGEBURN_DEFAULT_EXPOSURE;
  options->mode     = options->mode_d     = DODGEBURN_DEFAULT_MODE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the exposure scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Exposure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->exposure_w =
    gtk_adjustment_new (options->exposure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->exposure_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (options->exposure_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->exposure);

  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  /* the type (dodge or burn) */
  frame = gimp_radio_group_new2 (TRUE, _("Type"),
				 gimp_radio_button_update,
				 &options->type, (gpointer) options->type,

				 _("Dodge"), (gpointer) DODGE,
				 &options->type_w[0],
				 _("Burn"), (gpointer) BURN,
				 &options->type_w[1],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  mode (highlights, midtones, or shadows)  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Mode"),
			   gimp_radio_button_update,
			   &options->mode, (gpointer) options->mode,

			   _("Highlights"), (gpointer) DODGEBURN_HIGHLIGHTS, 
			   &options->mode_w[0],
			   _("Midtones"), (gpointer) DODGEBURN_MIDTONES,
			   &options->mode_w[1],
			   _("Shadows"), (gpointer) DODGEBURN_SHADOWS,
			   &options->mode_w[2],

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

static void
gimp_dodgeburn_tool_options_reset (ToolOptions *tool_options)
{
  DodgeBurnOptions *options;

  options = (DodgeBurnOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->exposure_w),
			    options->exposure_d);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->mode_w[options->mode_d]), TRUE);
}
