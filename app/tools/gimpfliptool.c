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

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpfliptool.h"
#include "tool_options.h"

#include "path_transform.h"

#include "libgimp/gimpintl.h"


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

static void          gimp_flip_tool_modifier_key  (GimpTool          *tool,
                                                   GdkModifierType    key,
                                                   gboolean           press,
						   GdkModifierType    state,
						   GimpDisplay       *gdisp);
static void          gimp_flip_tool_cursor_update (GimpTool          *tool,
                                                   GimpCoords        *coords,
						   GdkModifierType    state,
						   GimpDisplay       *gdisp);

static TileManager * gimp_flip_tool_transform     (GimpTransformTool *tool,
						   GimpDisplay       *gdisp,
						   TransformState     state);

static GimpToolOptions * flip_options_new         (GimpToolInfo      *tool_info);
static void              flip_options_reset       (GimpToolOptions   *tool_options);


static GimpTransformToolClass *parent_class = NULL;


/*  public functions  */

void 
gimp_flip_tool_register (Gimp                     *gimp,
                         GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_FLIP_TOOL,
                flip_options_new,
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


/*  private functions  */

static void
gimp_flip_tool_class_init (GimpFlipToolClass *klass)
{
  GimpToolClass          *tool_class;
  GimpTransformToolClass *trans_class;

  tool_class   = GIMP_TOOL_CLASS (klass);
  trans_class  = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_flip_tool_modifier_key;
  tool_class->cursor_update = gimp_flip_tool_cursor_update;

  trans_class->transform    = gimp_flip_tool_transform;
}

static void
gimp_flip_tool_init (GimpFlipTool *flip_tool)
{
  GimpTool          *tool;
  GimpTransformTool *transform_tool;

  tool           = GIMP_TOOL (flip_tool);
  transform_tool = GIMP_TRANSFORM_TOOL (flip_tool);

  tool->tool_cursor   = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;
  tool->toggle_cursor = GIMP_FLIP_VERTICAL_TOOL_CURSOR;

  tool->auto_snap_to  = FALSE;  /*  Don't snap to guides  */

  transform_tool->use_grid = FALSE;
}

static void
gimp_flip_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  FlipOptions *options;

  options = (FlipOptions *) tool->tool_info->tool_options;

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->type)
        {
        case ORIENTATION_HORIZONTAL:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w[0]),
                                       GINT_TO_POINTER (ORIENTATION_VERTICAL));
          break;
        case ORIENTATION_VERTICAL:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w[0]),
                                       GINT_TO_POINTER (ORIENTATION_HORIZONTAL));
          break;
        default:
          break;
	}
    }
}

static void
gimp_flip_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpDisplayShell   *shell;
  GimpDrawable       *drawable;
  GdkCursorType       ctype       = GIMP_BAD_CURSOR;
  GimpToolCursorType  tool_cursor = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;
  FlipOptions        *options;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  options = (FlipOptions *) tool->tool_info->tool_options;

  if ((drawable = gimp_image_active_drawable (gdisp->gimage)))
    {
      gint off_x, off_y;

      gimp_drawable_offsets (drawable, &off_x, &off_y);

      if (coords->x >= off_x &&
          coords->y >= off_y &&
	  coords->x < (off_x + gimp_drawable_width (drawable)) &&
	  coords->y < (off_y + gimp_drawable_height (drawable)))
	{
	  /*  Is there a selected region? If so, is cursor inside? */
	  if (gimage_mask_is_empty (gdisp->gimage) ||
	      gimage_mask_value (gdisp->gimage, coords->x, coords->y))
	    {
	      if (options->type == ORIENTATION_HORIZONTAL)
		ctype = GDK_SB_H_DOUBLE_ARROW;
	      else
		ctype = GDK_SB_V_DOUBLE_ARROW;
	    }
	}
    }

  if (options->type == ORIENTATION_HORIZONTAL)
    tool_cursor = GIMP_FLIP_HORIZONTAL_TOOL_CURSOR;
  else
    tool_cursor = GIMP_FLIP_VERTICAL_TOOL_CURSOR;

  gimp_display_shell_install_tool_cursor (shell,
                                          ctype,
                                          tool_cursor,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static TileManager *
gimp_flip_tool_transform (GimpTransformTool *trans_tool,
			  GimpDisplay       *gdisp,
			  TransformState     state)
{
  FlipOptions *options;

  options = (FlipOptions *) GIMP_TOOL (trans_tool)->tool_info->tool_options;

  switch (state)
    {
    case TRANSFORM_INIT:
      break;

    case TRANSFORM_MOTION:
      break;

    case TRANSFORM_RECALC:
      break;

    case TRANSFORM_FINISH:
      return gimp_drawable_transform_tiles_flip (gimp_image_active_drawable (gdisp->gimage),
                                                 trans_tool->original, 
                                                 options->type);
      break;
    }

  return NULL;
}


/*  tool options stuff  */

static GimpToolOptions *
flip_options_new (GimpToolInfo *tool_info)
{
  FlipOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *frame;
 
  options = g_new0 (FlipOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = flip_options_reset;

  options->type = options->type_d = ORIENTATION_HORIZONTAL;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  tool toggle  */
  frame = gimp_radio_group_new2 (TRUE, _("Tool Toggle"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &options->type,
                                 GINT_TO_POINTER (options->type),

                                 _("Horizontal"),
                                 GINT_TO_POINTER (ORIENTATION_HORIZONTAL),
                                 &options->type_w[0],

                                 _("Vertical"),
                                 GINT_TO_POINTER (ORIENTATION_VERTICAL),
                                 &options->type_w[1],

                                 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return (GimpToolOptions *) options;
}

static void
flip_options_reset (GimpToolOptions *tool_options)
{
  FlipOptions *options;

  options = (FlipOptions *) tool_options;

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w[0]),
                               GINT_TO_POINTER (options->type_d));
}
