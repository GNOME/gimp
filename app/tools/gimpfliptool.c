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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpfliptool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "undo.h"
#include "path_transform.h"

#include "libgimp/gimpintl.h"


/*  FIXME: Lame - 1 hacks abound since the code assumes certain values for
 *  the ORIENTATION_FOO constants.
 */

#define FLIP_INFO 0

typedef struct _FlipOptions FlipOptions;

struct _FlipOptions
{
  GimpToolOptions          tool_options;

  InternalOrientationType  type;
  InternalOrientationType  type_d;
  GtkWidget               *type_w[2];
};


/*  local function prototypes  */

static void          gimp_flip_tool_class_init    (GimpFlipToolClass *klass);
static void          gimp_flip_tool_init          (GimpFlipTool      *flip_tool);

static void          gimp_flip_tool_cursor_update (GimpTool          *tool,
						   GdkEventMotion    *mevent,
						   GimpDisplay       *gdisp);
static void          gimp_flip_tool_modifier_key  (GimpTool          *tool,
						   GdkEventKey       *kevent,
						   GimpDisplay       *gdisp);

static TileManager * gimp_flip_tool_transform     (GimpTransformTool *tool,
						   GimpDisplay       *gdisp,
						   TransformState     state);

static FlipOptions * flip_options_new             (void);
static void          flip_options_reset           (GimpToolOptions   *tool_options);


static FlipOptions *flip_options = NULL;

static GimpTransformToolClass *parent_class = NULL;


/*  public functions  */

void 
gimp_flip_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_FLIP_TOOL,
                              FALSE,
			      "gimp:flip_tool",
			      _("Flip Tool"),
			      _("Flip the layer or selection"),
			      N_("/Tools/Transform Tools/Flip"), "<shift>F",
			      NULL, "tools/flip.html",
			      GIMP_STOCK_TOOL_FLIP);
}

GType
gimp_flip_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpFlipToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_flip_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpFlipTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_flip_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
					  "GimpFlipTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

TileManager *
flip_tool_flip (GimpImage               *gimage,
		GimpDrawable            *drawable,
		TileManager             *orig,
		gint                     flip,
		InternalOrientationType  type)
{
  TileManager *new;
  PixelRegion  srcPR, destPR;
  gint         orig_width;
  gint         orig_height;
  gint         orig_bpp;
  gint         orig_x, orig_y;
  gint         i;

  if (! orig)
    return NULL;

  orig_width  = tile_manager_width (orig);
  orig_height = tile_manager_height (orig);
  orig_bpp    = tile_manager_bpp (orig);
  tile_manager_get_offsets (orig, &orig_x, &orig_y);

  if (flip > 0)
    {
      new = tile_manager_new (orig_width, orig_height, orig_bpp);
      pixel_region_init (&srcPR, orig,
			 0, 0, orig_width, orig_height, FALSE);
      pixel_region_init (&destPR, new,
			 0, 0, orig_width, orig_height, TRUE);

      copy_region (&srcPR, &destPR);
      tile_manager_set_offsets (new, orig_x, orig_y);
    }
  else
    {
      new = tile_manager_new (orig_width, orig_height, orig_bpp);
      tile_manager_set_offsets (new, orig_x, orig_y);

      if (type == ORIENTATION_HORIZONTAL)
	for (i = 0; i < orig_width; i++)
	  {
	    pixel_region_init (&srcPR, orig, i, 0, 1, orig_height, FALSE);
	    pixel_region_init (&destPR, new,
			       (orig_width - i - 1), 0, 1, orig_height, TRUE);
	    copy_region (&srcPR, &destPR); 
	  }
      else
	for (i = 0; i < orig_height; i++)
	  {
	    pixel_region_init (&srcPR, orig, 0, i, orig_width, 1, FALSE);
	    pixel_region_init (&destPR, new,
			       0, (orig_height - i - 1), orig_width, 1, TRUE);
	    copy_region (&srcPR, &destPR);
	  }

      /* flip locked paths */
      /* Note that the undo structures etc are setup before we enter this
       * function.
       */
      if (type == ORIENTATION_HORIZONTAL)
	path_transform_flip_horz (gimage);
      else
	path_transform_flip_vert (gimage);
    }

  return new;
}


/*  private functions  */

static void
gimp_flip_tool_class_init (GimpFlipToolClass *klass)
{
  GimpToolClass          *tool_class;
  GimpDrawToolClass      *draw_class;
  GimpTransformToolClass *trans_class;

  tool_class   = GIMP_TOOL_CLASS (klass);
  draw_class   = GIMP_DRAW_TOOL_CLASS (klass);
  trans_class  = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->cursor_update = gimp_flip_tool_cursor_update;
  tool_class->modifier_key  = gimp_flip_tool_modifier_key;

  draw_class->draw          = NULL;

  trans_class->transform    = gimp_flip_tool_transform;
}

static void
gimp_flip_tool_init (GimpFlipTool *flip_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;

  tool    = GIMP_TOOL (flip_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (flip_tool);

  /*  The tool options  */
  if (! flip_options)
    {
      flip_options = flip_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_FLIP_TOOL,
					  (GimpToolOptions *) flip_options);
    }

  tool->tool_cursor   = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;
  tool->toggle_cursor = GIMP_FLIP_VERTICAL_TOOL_CURSOR;

  tool->auto_snap_to = FALSE;  /*  Don't snap to guides  */

  tr_tool->trans_info[FLIP_INFO] = -1.0;
}

static void
gimp_flip_tool_cursor_update (GimpTool       *tool,
			      GdkEventMotion *mevent,
			      GimpDisplay    *gdisp)
{
  GimpDisplayShell   *shell;
  GimpDrawable       *drawable;
  GdkCursorType       ctype       = GIMP_BAD_CURSOR;
  GimpToolCursorType  tool_cursor = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if ((drawable = gimp_image_active_drawable (gdisp->gimage)))
    {
      gint x, y;
      gint off_x, off_y;

      gimp_drawable_offsets (drawable, &off_x, &off_y);
      gdisplay_untransform_coords (gdisp, 
				   (double) mevent->x, (double) mevent->y,
				   &x, &y, TRUE, FALSE);

      if (x >= off_x && y >= off_y &&
	  x < (off_x + gimp_drawable_width (drawable)) &&
	  y < (off_y + gimp_drawable_height (drawable)))
	{
	  /*  Is there a selected region? If so, is cursor inside? */
	  if (gimage_mask_is_empty (gdisp->gimage) ||
	      gimage_mask_value (gdisp->gimage, x, y))
	    {
	      if (flip_options->type == ORIENTATION_HORIZONTAL)
		ctype = GDK_SB_H_DOUBLE_ARROW;
	      else
		ctype = GDK_SB_V_DOUBLE_ARROW;
	    }
	}
    }

  if (flip_options->type == ORIENTATION_HORIZONTAL)
    tool_cursor = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;
  else
    tool_cursor = GIMP_FLIP_VERTICAL_TOOL_CURSOR;

  gimp_display_shell_install_tool_cursor (shell,
                                          ctype,
                                          tool_cursor,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_flip_tool_modifier_key (GimpTool    *tool,
			     GdkEventKey *kevent,
			     GimpDisplay *gdisp)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
      if (flip_options->type == ORIENTATION_HORIZONTAL)
	{
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (flip_options->type_w[ORIENTATION_VERTICAL - 1]), TRUE);
	}
      else
	{
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (flip_options->type_w[ORIENTATION_HORIZONTAL - 1]), TRUE);
	}
      break;
    }
}

static TileManager *
gimp_flip_tool_transform (GimpTransformTool *trans_tool,
			  GimpDisplay       *gdisp,
			  TransformState     state)
{
  GimpTool *tool;

  tool = GIMP_TOOL (trans_tool);

  switch (state)
    {
    case TRANSFORM_INIT:
      transform_info = NULL;
      break;

    case TRANSFORM_MOTION:
      break;

    case TRANSFORM_RECALC:
      break;

    case TRANSFORM_FINISH:
      return flip_tool_flip (gdisp->gimage,
	  		     gimp_image_active_drawable (gdisp->gimage),
			     trans_tool->original, 
			     (gint) trans_tool->trans_info[FLIP_INFO],
			     flip_options->type);
      break;
    }

  return NULL;
}

static FlipOptions *
flip_options_new (void)
{
  FlipOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *frame;
 
  options = g_new0 (FlipOptions, 1);
  tool_options_init ((GimpToolOptions *) options,
		     flip_options_reset);

  options->type = options->type_d = ORIENTATION_HORIZONTAL;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  tool toggle  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Tool Toggle"),
			   G_CALLBACK (gimp_radio_button_update),
			   &options->type,
			   (gpointer) options->type,

			   _("Horizontal"), (gpointer) ORIENTATION_HORIZONTAL,
			   &options->type_w[0],
			   _("Vertical"), (gpointer) ORIENTATION_VERTICAL,
			   &options->type_w[1],

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

static void
flip_options_reset (GimpToolOptions *tool_options)
{
  FlipOptions *options;

  options = (FlipOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d - 1]), TRUE); 
}
