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
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage-mask.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpbycolorselecttool.h"
#include "selection_options.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define PREVIEW_WIDTH       256
#define PREVIEW_HEIGHT      256
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK

typedef struct _ByColorDialog ByColorDialog;

struct _ByColorDialog
{
  GtkWidget *shell;

  GtkWidget *preview;
  GtkWidget *gimage_name;

  GtkWidget *replace_button;
  GtkObject *threshold_adj;

  gint       threshold;  /*  threshold value for color select               */
  gint       operation;  /*  Add, Subtract, Replace                         */
  GimpImage *gimage;     /*  gimpimage which is currently under examination */
};


/* Local functions */
static void   gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass);
static void   gimp_by_color_select_tool_init       (GimpByColorSelectTool      *by_color_select);

static void   gimp_by_color_select_tool_initialize_by_image (GimpImage    *gimage);

static void   by_color_select_color_drop      (GtkWidget      *widget,
					       const GimpRGB  *color,
					       gpointer        data);

/*  by_color select action functions  */

static void   by_color_select_initialize      (GimpTool       *tool,
					       GDisplay       *gdisp);

static void   by_color_select_button_press    (GimpTool       *tool,
					       GdkEventButton *bevent,
					       GDisplay       *gdisp);
static void   by_color_select_button_release  (GimpTool       *tool,
					       GdkEventButton *bevent,
					       GDisplay       *gdisp);
static void   by_color_select_modifier_update (GimpTool       *tool,
					       GdkEventKey    *kevent,
					       GDisplay       *gdisp);
static void   by_color_select_cursor_update   (GimpTool       *tool,
					       GdkEventMotion *mevent,
					       GDisplay       *gdisp);
static void   by_color_select_oper_update     (GimpTool       *tool,
					       GdkEventMotion *mevent,
					       GDisplay       *gdisp);
static void   by_color_select_control         (GimpTool       *tool,
					       ToolAction      action,
					       GDisplay       *gdisp);
static void   by_color_select_mask_changed    (GimpImage      *gimage);

static ByColorDialog * by_color_select_dialog_new  (void);

static void   by_color_select_render               (ByColorDialog  *,
						    GimpImage      *);
static void   by_color_select_draw                 (ByColorDialog  *,
						    GimpImage      *);
static gint   by_color_select_preview_events       (GtkWidget      *,
						    GdkEventButton *,
						    ByColorDialog  *);
static void   by_color_select_invert_callback      (GtkWidget      *,
						    gpointer        );
static void   by_color_select_select_all_callback  (GtkWidget      *,
						    gpointer        );
static void   by_color_select_select_none_callback (GtkWidget      *,
						    gpointer        );
static void   by_color_select_reset_callback       (GtkWidget      *,
						    gpointer        );
static void   by_color_select_close_callback       (GtkWidget      *,
						    gpointer        );
static void   by_color_select_preview_button_press (ByColorDialog  *,
						    GdkEventButton *);

static gint   is_pixel_sufficiently_different      (guchar         *,
						    guchar         *,
						    gint            ,
						    gint            ,
						    gint            ,
						    gint            );
static GimpChannel * by_color_select_color         (GimpImage      *,
						    GimpDrawable   *,
						    guchar         *,
						    gint            ,
						    gint            ,
						    gint            );

/* The parent class (duh!) */
static GimpSelectionToolClass *parent_class = NULL;
/*  the by color selection tool options  */
static SelectionOptions * by_color_options = NULL;

/*  the by color selection dialog  */
static ByColorDialog    * by_color_dialog = NULL;

/*  dnd stuff  */
static GtkTargetEntry by_color_select_targets[] =
{
  GIMP_TARGET_COLOR
};
static guint n_by_color_select_targets = (sizeof (by_color_select_targets) /
					  sizeof (by_color_select_targets[0]));


/* public functions */

void
gimp_by_color_select_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_BY_COLOR_SELECT_TOOL,
			      FALSE,
                              "gimp:by_color_select_tool",
                              _("Select By Color"),
                              _("Select regions by color"),
                              _("/Tools/Selection Tools/By Color Select"), "C",
                              NULL, "tools/by_color_select.html",
                              GIMP_STOCK_TOOL_BY_COLOR_SELECT);
}

GType
gimp_by_color_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpByColorSelectToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_by_color_select_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpByColorSelectTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_by_color_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
					  "GimpByColorSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

void
gimp_by_color_select_tool_select (GimpImage    *gimage,
				  GimpDrawable *drawable,
				  guchar       *color,
				  gint          threshold,
				  SelectOps     op,
				  gboolean      antialias,
				  gboolean      feather,
				  gdouble       feather_radius,
				  gboolean      sample_merged)
{
  GimpChannel *new_mask;
  gint         off_x, off_y;

  if (!drawable) 
    return;

  new_mask = by_color_select_color (gimage, drawable, color,
				    antialias, threshold, sample_merged);

  /*  if applicable, replace the current selection  */
  if (op == SELECTION_REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  if (sample_merged)
    {
      off_x = 0; off_y = 0;
    }
  else
    {
      gimp_drawable_offsets (drawable, &off_x, &off_y);
    }

  if (feather)
    gimp_channel_feather (new_mask, gimp_image_get_mask (gimage),
			  feather_radius,
			  feather_radius,
			  op, off_x, off_y);
  else
    gimp_channel_combine_mask (gimp_image_get_mask (gimage),
			       new_mask, op, off_x, off_y);

  g_object_unref (G_OBJECT (new_mask));
}

void
gimp_by_color_select_tool_initialize_by_image (GimpImage *gimage)
{

  /*  update the preview window  */
  if (by_color_dialog)
    {
      by_color_dialog->gimage = gimage;
      by_color_select_render (by_color_dialog, gimage);
      by_color_select_draw (by_color_dialog, gimage);
    }
}

/* private functions */

static void
gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->initialize     = by_color_select_initialize;
  tool_class->button_press   = by_color_select_button_press;
  tool_class->button_release = by_color_select_button_release;
  tool_class->cursor_update  = by_color_select_cursor_update;
  tool_class->modifier_key   = by_color_select_modifier_update;
  tool_class->oper_update    = by_color_select_oper_update;
  tool_class->control        = by_color_select_control;
}

static void
gimp_by_color_select_tool_init (GimpByColorSelectTool *by_color_select)
{
  GimpTool          *tool;
  GimpSelectionTool *select_tool;

  tool        = GIMP_TOOL (by_color_select);
  select_tool = GIMP_SELECTION_TOOL (by_color_select);

  if (! by_color_options)
    {
      by_color_options = selection_options_new (GIMP_TYPE_BY_COLOR_SELECT_TOOL,
						selection_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_BY_COLOR_SELECT_TOOL,
                                          (GimpToolOptions *) by_color_options);
    }
  /* Temporary until we find out how to make a new one */
  tool->tool_cursor = GIMP_RECT_SELECT_TOOL_CURSOR;
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  by_color_select->x = by_color_select->y = 0;
}

/*  by_color selection machinery  */

static gint
is_pixel_sufficiently_different (guchar *col1,
				 guchar *col2,
				 gint    antialias,
				 gint    threshold,
				 gint    bytes,
				 gint    has_alpha)
{
  gint diff;
  gint max;
  gint b;
  gint alpha;

  max = 0;
  alpha = (has_alpha) ? bytes - 1 : bytes;

  /*  if there is an alpha channel, never select transparent regions  */
  if (has_alpha && col2[alpha] == 0)
    return 0;

  for (b = 0; b < alpha; b++)
    {
      diff = col1[b] - col2[b];
      diff = abs (diff);
      if (diff > max)
	max = diff;
    }

  if (antialias && threshold > 0)
    {
      gfloat aa;

      aa = 1.5 - ((gfloat) max / threshold);
      if (aa <= 0)
	return 0;
      else if (aa < 0.5)
	return (guchar) (aa * 512);
      else
	return 255;
    }
  else
    {
      if (max > threshold)
	return 0;
      else
	return 255;
    }
}

static GimpChannel *
by_color_select_color (GimpImage    *gimage,
		       GimpDrawable *drawable,
		       guchar       *color,
		       gboolean      antialias,
		       gint          threshold,
		       gboolean      sample_merged)
{
  /*  Scan over the gimage's active layer, finding pixels within the specified
   *  threshold from the given R, G, & B values.  If antialiasing is on,
   *  use the same antialiasing scheme as in fuzzy_select.  Modify the gimage's
   *  mask to reflect the additional selection
   */
  GimpChannel *mask;
  PixelRegion  imagePR, maskPR;
  guchar      *image_data;
  guchar      *mask_data;
  guchar      *idata, *mdata;
  guchar       rgb[MAX_CHANNELS];
  gint         has_alpha, indexed;
  gint         width, height;
  gint         bytes, color_bytes, alpha;
  gint         i, j;
  gpointer     pr;
  gint         d_type;

  /*  Get the image information  */
  if (sample_merged)
    {
      bytes  = gimp_image_composite_bytes (gimage);
      d_type = gimp_image_composite_type (gimage);
      has_alpha = (d_type == RGBA_GIMAGE ||
		   d_type == GRAYA_GIMAGE ||
		   d_type == INDEXEDA_GIMAGE);
      indexed = d_type == INDEXEDA_GIMAGE || d_type == INDEXED_GIMAGE;
      width = gimage->width;
      height = gimage->height;
      pixel_region_init (&imagePR, gimp_image_composite (gimage),
			 0, 0, width, height, FALSE);
    }
  else
    {
      bytes     = gimp_drawable_bytes (drawable);
      d_type    = gimp_drawable_type (drawable);
      has_alpha = gimp_drawable_has_alpha (drawable);
      indexed   = gimp_drawable_is_indexed (drawable);
      width     = gimp_drawable_width (drawable);
      height    = gimp_drawable_height (drawable);

      pixel_region_init (&imagePR, gimp_drawable_data (drawable),
			 0, 0, width, height, FALSE);
    }

  if (indexed)
    {
      /* indexed colors are always RGB or RGBA */
      color_bytes = has_alpha ? 4 : 3;
    }
  else
    {
      /* RGB, RGBA, GRAY and GRAYA colors are shaped just like the image */
      color_bytes = bytes;
    }

  alpha = bytes - 1;
  mask = gimp_channel_new_mask (gimage, width, height);
  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
		     0, 0, width, height, TRUE);

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (2, &imagePR, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;
      mask_data = maskPR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;
	  mdata = mask_data;
	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimp_image_get_color (gimage, d_type, rgb, idata);

	      /*  Plug the alpha channel in there  */
	      if (has_alpha)
		rgb[color_bytes - 1] = idata[alpha];

	      /*  Find how closely the colors match  */
	      *mdata++ = is_pixel_sufficiently_different (color,
							  rgb,
							  antialias,
							  threshold,
							  color_bytes,
							  has_alpha);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	  mask_data += maskPR.rowstride;
	}
    }

  return mask;
}

/*  by_color select action functions  */

static void
by_color_select_button_press (GimpTool       *tool,
			      GdkEventButton *bevent,
			      GDisplay       *gdisp)
{
  GimpByColorSelectTool *by_color_sel;
  GimpDrawTool *draw_tool;

  draw_tool    = GIMP_DRAW_TOOL (tool);
  by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);

  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  if (!by_color_dialog)
    return;

  by_color_sel->x = bevent->x;
  by_color_sel->y = bevent->y;

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  /*  Make sure the "by color" select dialog is visible  */
  if (! GTK_WIDGET_VISIBLE (by_color_dialog->shell))
    gtk_widget_show (by_color_dialog->shell);

  /*  Update the by_color_dialog's active gdisp pointer  */
  /* if (by_color_dialog->gimage)
   *   by_color_dialog->gimage->by_color_select = FALSE;
   * Temporarily commented out. Do we need to do something to replace 
   * this?*/

  if (by_color_dialog->gimage != gdisp->gimage)
    {
      gdk_draw_rectangle
	(by_color_dialog->preview->window,
	 by_color_dialog->preview->style->bg_gc[GTK_STATE_NORMAL],
	 TRUE,
	 0, 0,
	 by_color_dialog->preview->allocation.width,
	 by_color_dialog->preview->allocation.width);
    }

  by_color_dialog->gimage = gdisp->gimage;
  /* gdisp->gimage->by_color_select = TRUE;
   * Temporarily commented out - do we need something to replace this? */

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
                    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
                    GDK_BUTTON_RELEASE_MASK,
                    NULL, NULL, bevent->time);
}

static void
by_color_select_button_release (GimpTool       *tool,
				GdkEventButton *bevent,
				GDisplay       *gdisp)
{
  GimpByColorSelectTool *by_color_sel;
  gint           x, y;
  GimpDrawable  *drawable;
  guchar        *color;
  gint           use_offsets;

  by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);
  drawable = gimp_image_active_drawable (gdisp->gimage);

  gdk_pointer_ungrab (bevent->time);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      use_offsets = (by_color_options->sample_merged) ? FALSE : TRUE;
      gdisplay_untransform_coords (gdisp, by_color_sel->x, by_color_sel->y,
				   &x, &y, FALSE, use_offsets);

      if( x >= 0 && x < gimp_drawable_width (drawable) && 
	  y >= 0 && y < gimp_drawable_height (drawable))
	{
	  /*  Get the start color  */
	  if (by_color_options->sample_merged)
	    {
	      if (!(color = gimp_image_get_color_at (gdisp->gimage, x, y)))
		return;
	    }
	  else
	    {
	      if (!(color = gimp_drawable_get_color_at (drawable, x, y)))
		return;
	    }

	  /*  select the area  */
	  gimp_by_color_select_tool_select (gdisp->gimage, drawable, color,
					    by_color_dialog->threshold,
					    by_color_sel->operation,
					    by_color_options->antialias,
					    by_color_options->feather,
					    by_color_options->feather_radius,
					    by_color_options->sample_merged);

	  g_free (color);

	  /*  show selection on all views  */
	  gdisplays_flush ();

	  /*  update the preview window  */
	  by_color_select_render (by_color_dialog, gdisp->gimage);
	  by_color_select_draw (by_color_dialog, gdisp->gimage);
	}
    }
}

static void
by_color_select_cursor_update (GimpTool       *tool,
			       GdkEventMotion *mevent,
			       GDisplay       *gdisp)
{
  GimpByColorSelectTool *by_col_sel;
  GimpLayer     *layer;
  gint           x, y;

  by_col_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if (by_color_options->sample_merged ||
      ((layer = gimp_image_pick_correlate_layer (gdisp->gimage, x, y)) &&
       layer == gdisp->gimage->active_layer))
    {
      switch (by_col_sel->operation)
	{
	case SELECTION_ADD:
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_TOOL_CURSOR_NONE,
					GIMP_CURSOR_MODIFIER_PLUS);
	  break;
	case SELECTION_SUB:
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_TOOL_CURSOR_NONE,
					GIMP_CURSOR_MODIFIER_MINUS);
	  break;
	case SELECTION_INTERSECT: 
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_TOOL_CURSOR_NONE,
					GIMP_CURSOR_MODIFIER_INTERSECT);
	  break;
	case SELECTION_REPLACE:
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_TOOL_CURSOR_NONE,
					GIMP_CURSOR_MODIFIER_NONE);
	  break;
	case SELECTION_MOVE_MASK:
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_RECT_SELECT_TOOL_CURSOR,
					GIMP_CURSOR_MODIFIER_MOVE);
	  break;
	case SELECTION_MOVE: 
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_MOVE_TOOL_CURSOR,
					GIMP_CURSOR_MODIFIER_NONE);
	  break;
	case SELECTION_ANCHOR:
	  gdisplay_install_tool_cursor (gdisp,
					GIMP_MOUSE_CURSOR,
					GIMP_RECT_SELECT_TOOL_CURSOR,
					GIMP_CURSOR_MODIFIER_ANCHOR);
	}

      return;
    }

  gdisplay_install_tool_cursor (gdisp,
				GIMP_BAD_CURSOR,
				GIMP_TOOL_CURSOR_NONE,
				GIMP_CURSOR_MODIFIER_NONE);
}

static void
by_color_select_update_op_state (GimpByColorSelectTool *by_col_sel,
				 gint                   state,  
				 GDisplay              *gdisp)
{
  if (tool_manager_get_active (gdisp->gimage->gimp)->state == ACTIVE)
    return;

  if ((state & GDK_SHIFT_MASK) &&
      !(state & GDK_CONTROL_MASK))
    by_col_sel->operation = SELECTION_ADD;   /* add to the selection */
  else if ((state & GDK_CONTROL_MASK) &&
           !(state & GDK_SHIFT_MASK))
    by_col_sel->operation = SELECTION_SUB;   /* subtract from the selection */
  else if ((state & GDK_CONTROL_MASK) &&
           (state & GDK_SHIFT_MASK))
    by_col_sel->operation = SELECTION_INTERSECT; /* intersect with selection */
  else
    if (by_color_dialog)
      {
	by_col_sel->operation = by_color_dialog->operation;
      }
    else
      {
      /* To be careful, set it to by_select_dialog's default */
	by_col_sel->operation = SELECTION_REPLACE;
      }
}

static void
by_color_select_modifier_update (GimpTool    *tool,
				 GdkEventKey *kevent,
				 GDisplay    *gdisp)
{
  GimpByColorSelectTool *by_col_sel;
  gint state;

  by_col_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);

  state = kevent->state;

  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      if (state & GDK_MOD1_MASK)
        state &= ~GDK_MOD1_MASK;
      else
        state |= GDK_MOD1_MASK;
      break;

    case GDK_Shift_L: case GDK_Shift_R:
      if (state & GDK_SHIFT_MASK)
        state &= ~GDK_SHIFT_MASK;
      else
        state |= GDK_SHIFT_MASK;
      break;

    case GDK_Control_L: case GDK_Control_R:
      if (state & GDK_CONTROL_MASK)
        state &= ~GDK_CONTROL_MASK;
      else
        state |= GDK_CONTROL_MASK;
      break;
    }

  by_color_select_update_op_state (by_col_sel, state, gdisp);
}

static void
by_color_select_oper_update (GimpTool       *tool,
			     GdkEventMotion *mevent,
			     GDisplay       *gdisp)
{
  GimpByColorSelectTool *by_col_sel;

  by_col_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);

  by_color_select_update_op_state (by_col_sel, mevent->state, gdisp);
}

static void
by_color_select_control (GimpTool   *tool,
			 ToolAction  action,
			 GDisplay   *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (by_color_dialog)
	by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
      break;

    default:
      break;
    }
}

void
by_color_select_initialize (GimpTool *tool,
			    GDisplay *gdisp)
{
  /*  The "by color" dialog  */
  if (!by_color_dialog)
    {
      by_color_dialog = by_color_select_dialog_new ();
      /* Catch the "mask_changed" signal and attach a handler that does 
       * stuff with it. Need to do this somewhere with the relevant 
       * GimpImage in context
       */
      g_signal_connect (G_OBJECT (gdisp->gimage), "mask_changed",
			G_CALLBACK (by_color_select_mask_changed),
			NULL);
    }
  else
    if (!GTK_WIDGET_VISIBLE (by_color_dialog->shell))
      gtk_widget_show (by_color_dialog->shell);
  
  gimp_by_color_select_tool_initialize_by_image (gdisp->gimage);
}

void by_color_select_mask_changed( GimpImage *gimage)
{
  if (by_color_dialog)
    gimp_by_color_select_tool_initialize_by_image (gimage );
}

/****************************/
/*  Select by Color dialog  */
/****************************/

static ByColorDialog *
by_color_select_dialog_new (void)
{
  ByColorDialog *bcd;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *options_box;
  GtkWidget *label;
  GtkWidget *util_box;
  GtkWidget *slider;
  GtkWidget *table;
  GtkWidget *button;

  bcd = g_new (ByColorDialog, 1);
  bcd->gimage    = NULL;
  bcd->operation = SELECTION_REPLACE;
  bcd->threshold = gimprc.default_threshold;

  /*  The shell and main vbox  */
  bcd->shell = gimp_dialog_new (_("By Color Selection"), "by_color_selection",
			        NULL, NULL,
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Reset"), by_color_select_reset_callback,
				bcd, NULL, NULL, FALSE, FALSE,
				GTK_STOCK_CLOSE, by_color_select_close_callback,
				bcd, NULL, NULL, TRUE, TRUE,

				NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bcd->shell)->vbox), hbox);

  /*  The preview  */
  util_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), util_box, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (util_box), frame, FALSE, FALSE, 0);

  bcd->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (bcd->preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (bcd->preview, PREVIEW_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (frame), bcd->preview);

  g_signal_connect (G_OBJECT (bcd->preview), "button_press_event",
		    G_CALLBACK (by_color_select_preview_events),
		    bcd);

  /*  dnd colors to the image window  */
  gtk_drag_dest_set (bcd->preview, 
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP, 
                     by_color_select_targets,
                     n_by_color_select_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (bcd->preview, by_color_select_color_drop, bcd);

  gtk_widget_show (bcd->preview);
  gtk_widget_show (frame);
  gtk_widget_show (util_box);

  /*  options box  */
  options_box = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), options_box, FALSE, FALSE, 0);

  /*  Create the active image label  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);

  bcd->gimage_name = gtk_label_new (_("Inactive"));
  gtk_box_pack_start (GTK_BOX (util_box), bcd->gimage_name, FALSE, FALSE, 0);

  gtk_widget_show (bcd->gimage_name);
  gtk_widget_show (util_box);

  /*  Create the selection mode radio box  */
  frame = gimp_radio_group_new2
    (TRUE, _("Selection Mode"),
     G_CALLBACK (gimp_radio_button_update),
     &bcd->operation,
     GINT_TO_POINTER (bcd->operation),

     _("Replace"),   GINT_TO_POINTER (SELECTION_REPLACE),   &bcd->replace_button,
     _("Add"),       GINT_TO_POINTER (SELECTION_ADD),       NULL,
     _("Subtract"),  GINT_TO_POINTER (SELECTION_SUB),       NULL,
     _("Intersect"), GINT_TO_POINTER (SELECTION_INTERSECT), NULL,

     NULL);

  gtk_box_pack_start (GTK_BOX (options_box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Create the opacity scale widget  */
  util_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fuzziness Threshold"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (util_box);

  bcd->threshold_adj =
    gtk_adjustment_new (bcd->threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (bcd->threshold_adj));
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);

  g_signal_connect (G_OBJECT (bcd->threshold_adj), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &bcd->threshold);

  gtk_widget_show (slider);

  frame = gtk_frame_new (_("Selection"));
  gtk_box_pack_end (GTK_BOX (options_box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  button = gtk_button_new_with_label (_("Invert"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 2, 0, 1);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (by_color_select_invert_callback),
		    bcd);

  button = gtk_button_new_with_label (_("All"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 1, 2);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (by_color_select_select_all_callback),
		    bcd);

  button = gtk_button_new_with_label (_("None"));
  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (by_color_select_select_none_callback),
		    bcd);

  gtk_widget_show (options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (bcd->shell);

  g_signal_connect_swapped (G_OBJECT (bcd->shell), "unmap_event",
			    G_CALLBACK (gimp_dialog_hide), 
			    bcd->shell);

  return bcd;
}

static void
by_color_select_render (ByColorDialog *bcd,
			GimpImage     *gimage)
{
  GimpChannel *mask;
  MaskBuf     *scaled_buf = NULL;
  guchar      *buf;
  PixelRegion  srcPR, destPR;
  guchar      *src;
  gint         subsample;
  gint         width, height;
  gint         srcwidth;
  gint         i;
  gint         scale;

  mask = gimp_image_get_mask (gimage);
  if ((gimp_drawable_width (GIMP_DRAWABLE(mask)) > PREVIEW_WIDTH) ||
      (gimp_drawable_height (GIMP_DRAWABLE(mask)) > PREVIEW_HEIGHT))
    {
      if (((float) gimp_drawable_width (GIMP_DRAWABLE (mask)) / (float) PREVIEW_WIDTH) >
	  ((float) gimp_drawable_height (GIMP_DRAWABLE (mask)) / (float) PREVIEW_HEIGHT))
	{
	  width = PREVIEW_WIDTH;
	  height = ((gimp_drawable_height (GIMP_DRAWABLE (mask)) * PREVIEW_WIDTH) /
		    gimp_drawable_width (GIMP_DRAWABLE (mask)));
	}
      else
	{
	  width = ((gimp_drawable_width (GIMP_DRAWABLE (mask)) * PREVIEW_HEIGHT) /
		   gimp_drawable_height (GIMP_DRAWABLE (mask)));
	  height = PREVIEW_HEIGHT;
	}

      scale = TRUE;
    }
  else
    {
      width  = gimp_drawable_width (GIMP_DRAWABLE (mask));
      height = gimp_drawable_height (GIMP_DRAWABLE (mask));

      scale = FALSE;
    }

  if ((width != bcd->preview->requisition.width) ||
      (height != bcd->preview->requisition.height))
    gtk_preview_size (GTK_PREVIEW (bcd->preview), width, height);

  /*  clear the image buf  */
  buf = g_new0 (guchar, bcd->preview->requisition.width);
  for (i = 0; i < bcd->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (bcd->preview), buf,
			  0, i, bcd->preview->requisition.width);
  g_free (buf);

  /*  if the mask is empty, no need to scale and update again  */
  if (gimage_mask_is_empty (gimage))
    return;

  if (scale)
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      while ((width * (subsample + 1) * 2 < gimp_drawable_width (GIMP_DRAWABLE (mask))) &&
	     (height * (subsample + 1) * 2 < gimp_drawable_height (GIMP_DRAWABLE (mask))))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 gimp_drawable_width (GIMP_DRAWABLE (mask)), 
			 gimp_drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      subsample_region (&srcPR, &destPR, subsample);
    }
  else
    {
      pixel_region_init (&srcPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			 0, 0, 
			 gimp_drawable_width (GIMP_DRAWABLE (mask)), 
			 gimp_drawable_height (GIMP_DRAWABLE (mask)), FALSE);

      scaled_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = srcPR.bytes * width;
      destPR.data = mask_buf_data (scaled_buf);
      destPR.tiles = NULL;

      copy_region (&srcPR, &destPR);
    }

  src = mask_buf_data (scaled_buf);
  srcwidth = scaled_buf->width;
  for (i = 0; i < height; i++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (bcd->preview), src, 0, i, width);
      src += srcwidth;
    }

  mask_buf_free (scaled_buf);
}

static void
by_color_select_draw (ByColorDialog *bcd,
		      GimpImage     *gimage)
{
  gchar *basename;

  /*  Draw the image buf to the preview window  */
  gtk_widget_draw (bcd->preview, NULL);

  /*  Update the gimage label to reflect the displayed gimage name  */
  basename = g_path_get_basename (gimp_image_filename (gimage));

  gtk_label_set_text (GTK_LABEL (bcd->gimage_name), basename);

  g_free (basename);
}

static gint
by_color_select_preview_events (GtkWidget      *widget,
				GdkEventButton *bevent,
				ByColorDialog  *bcd)
{
  switch (bevent->type)
    {
    case GDK_BUTTON_PRESS:
      by_color_select_preview_button_press (bcd, bevent);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
by_color_select_reset_callback (GtkWidget *widget,
				gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  gtk_widget_activate (bcd->replace_button);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (bcd->threshold_adj),
			    gimprc.default_threshold);
}

static void
by_color_select_invert_callback (GtkWidget *widget,
				 gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  if (!bcd->gimage)
    return;

  /*  check if the image associated to the mask still exists  */
  if (! gimp_drawable_gimage (GIMP_DRAWABLE (gimp_image_get_mask (bcd->gimage))))
    return;

  /*  invert the mask  */
  gimage_mask_invert (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_select_all_callback (GtkWidget *widget,
				     gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  if (!bcd->gimage)
    return;

  /*  check if the image associated to the mask still exists  */
  if (! gimp_drawable_gimage (GIMP_DRAWABLE (gimp_image_get_mask (bcd->gimage))))
    return;

  /*  fill the mask  */
  gimage_mask_all (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_select_none_callback (GtkWidget *widget,
				      gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;

  if (!bcd->gimage)
    return;

  /*  check if the image associated to the mask still exists  */
  if (! gimp_drawable_gimage (GIMP_DRAWABLE (gimp_image_get_mask (bcd->gimage))))
    return;

  /*  reset the mask  */
  gimage_mask_clear (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_close_callback (GtkWidget *widget,
				gpointer   data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;
  
  gimp_dialog_hide (bcd->shell);

  /* if (bcd->gimage && gimp_container_have (image_context,
   * 					  GIMP_OBJECT (bcd->gimage)))
   *  {
   *    bcd->gimage->by_color_select = FALSE;
   *  }
   * Temporarily commented out. */

  bcd->gimage = NULL;
}

static void
by_color_select_preview_button_press (ByColorDialog  *bcd,
				      GdkEventButton *bevent)
{
  gint          x, y;
  gboolean      replace;
  SelectOps     operation;
  GimpDrawable *drawable;
  Tile         *tile;
  guchar       *col;

  if (!bcd->gimage)
    return;

  drawable = gimp_image_active_drawable (bcd->gimage);

  /*  check if the gimage associated to the drawable still exists  */
  if (! gimp_drawable_gimage (drawable))
    return;

  /*  Defaults  */
  replace = FALSE;
  operation = SELECTION_REPLACE;

  /*  Based on modifiers, and the "by color" dialog's selection mode  */
  if ((bevent->state & GDK_SHIFT_MASK) &&
      !(bevent->state & GDK_CONTROL_MASK))
    operation = SELECTION_ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) &&
	   !(bevent->state & GDK_SHIFT_MASK))
    operation = SELECTION_SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) &&
	   (bevent->state & GDK_SHIFT_MASK))
    operation = SELECTION_INTERSECT;
  else
    operation = by_color_dialog->operation;

  /*  Find x, y and modify selection  */

  /*  Get the start color  */
  if (by_color_options->sample_merged)
    {
      x = bcd->gimage->width * bevent->x / bcd->preview->requisition.width;
      y = bcd->gimage->height * bevent->y / bcd->preview->requisition.height;
      if (x < 0 || y < 0 || x >= bcd->gimage->width || y >= bcd->gimage->height)
	return;
      tile = tile_manager_get_tile (gimp_image_composite (bcd->gimage),
				    x, y, TRUE, FALSE);
      col = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
    }
  else
    {
      gint offx, offy;

      gimp_drawable_offsets (drawable, &offx, &offy);
      x = gimp_drawable_width (drawable) * bevent->x / bcd->preview->requisition.width - offx;
      y = gimp_drawable_height (drawable) * bevent->y / bcd->preview->requisition.height - offy;
      if (x < 0 || y < 0 ||
	  x >= gimp_drawable_width (drawable) || y >= gimp_drawable_height (drawable))
	return;
      tile = tile_manager_get_tile (gimp_drawable_data (drawable),
				    x, y, TRUE, FALSE);
      col = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
    }

  gimp_by_color_select_tool_select (bcd->gimage, drawable, col,
				    bcd->threshold,
				    operation,
				    by_color_options->antialias,
				    by_color_options->feather,
				    by_color_options->feather_radius,
				    by_color_options->sample_merged);
  
  tile_release (tile, FALSE);

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}

static void
by_color_select_color_drop (GtkWidget     *widget,
                            const GimpRGB *color,
                            gpointer       data)

{
  GimpDrawable  *drawable;
  ByColorDialog *bcd;
  guchar         col[3];

  bcd = (ByColorDialog*) data;
  drawable = gimp_image_active_drawable (bcd->gimage);

  gimp_rgb_get_uchar (color, &col[0], &col[1], &col[2]);

  gimp_by_color_select_tool_select (bcd->gimage, 
				    drawable, 
				    col, 
				    bcd->threshold,
				    bcd->operation,
				    by_color_options->antialias,
				    by_color_options->feather,
				    by_color_options->feather_radius,
				    by_color_options->sample_merged);     

  /*  show selection on all views  */
  gdisplays_flush ();

  /*  update the preview window  */
  by_color_select_render (bcd, bcd->gimage);
  by_color_select_draw (bcd, bcd->gimage);
}
