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
#include <stdlib.h>
#include "appenv.h"
#include "cursorutil.h"
#include "drawable.h"
#include "flip_tool.h"
#include "gdisplay.h"
#include "temp_buf.h"
#include "tool_options_ui.h"
#include "transform_core.h"

#include "undo.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"

#include "tile_manager_pvt.h"		/* ick. */

#define FLIP_INFO 0

/*  the flip structures  */

typedef struct _FlipOptions FlipOptions;
struct _FlipOptions
{
  ToolOptions  tool_options;

  ToolType     type;
  ToolType     type_d;
  ToolOptionsRadioButtons type_toggle[3];
};

static FlipOptions *flip_options = NULL;


/*  functions  */

static void
flip_options_reset (void)
{
  FlipOptions *options = flip_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_toggle[options->type_d].widget), TRUE); 
}

static FlipOptions *
flip_options_new (void)
{
  FlipOptions *options;

  GtkWidget *vbox;
  GtkWidget *frame;

  /*  the new flip tool options structure  */
  options = (FlipOptions *) g_malloc (sizeof (FlipOptions));
  tool_options_init ((ToolOptions *) options,
		     _("Flip Tool Options"),
		     flip_options_reset);
  options->type_toggle[0].label = _("Horizontal");
  options->type_toggle[0].value = FLIP_HORZ;
  options->type_toggle[1].label = _("Vertical");
  options->type_toggle[1].value = FLIP_VERT;
  options->type_toggle[2].label = NULL;
  options->type = options->type_d = FLIP_HORZ;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  tool toggle  */
  frame = tool_options_radio_buttons_new (_("Tool Toggle"), options->type_toggle, &options->type);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_toggle[options->type_d].widget), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

static void
flip_toggle_key_func (Tool        *tool,
		      GdkEventKey *kevent,
		      gpointer     gdisp_ptr)
{
  if (flip_options->type == FLIP_HORZ)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (flip_options->type_toggle[1].widget), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (flip_options->type_toggle[0].widget), TRUE);
}

void *
flip_tool_transform (Tool     *tool,
                     gpointer  gdisp_ptr,
                     int       state)
{
  TransformCore * transform_core;
  GDisplay *gdisp;

  transform_core = (TransformCore *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  switch (state)
    {
    case INIT :
      transform_info = NULL;
      break;

    case MOTION :
      break;

    case RECALC :
      break;

    case FINISH :
      /*      transform_core->trans_info[FLIP] *= -1.0;*/
      return flip_tool_flip (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
			     transform_core->original, transform_core->trans_info[FLIP],
			     flip_options->type);
      break;
    }

  return NULL;
}

static void
flip_cursor_update (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;

  gdisp = (GDisplay *) gdisp_ptr;

  if (flip_options->type == FLIP_HORZ)
    ctype = GDK_SB_H_DOUBLE_ARROW;
  else
    ctype = GDK_SB_V_DOUBLE_ARROW;

  gdisplay_install_tool_cursor (gdisp, ctype);
}

Tool *
tools_new_flip ()
{

  Tool * tool;
  TransformCore * private;

  /*  The tool options  */
  if (! flip_options)
    {
      flip_options = flip_options_new ();
      tools_register (FLIP, (ToolOptions *) flip_options);
    }

  tool = transform_core_new (FLIP, NON_INTERACTIVE);
  private = tool->private;

  private->trans_func   = flip_tool_transform;
  tool->toggle_key_func = flip_toggle_key_func;
  tool->cursor_update_func = flip_cursor_update;
  private->trans_info[FLIP_INFO] = -1.0;

  return tool;
}

void
tools_free_flip_tool (Tool *tool)
{
  transform_core_free (tool);
}

TileManager *
flip_tool_flip (GImage      *gimage,
		GimpDrawable *drawable,
		TileManager *orig,
		int          flip,
		FlipType     type)
{
  TileManager *new;
  PixelRegion srcPR, destPR;
  int i;

  if (!orig)
    return NULL;

  if (flip > 0)
    {
      new = tile_manager_new (orig->width, orig->height, orig->bpp);
      pixel_region_init (&srcPR, orig, 0, 0, orig->width, orig->height, FALSE);
      pixel_region_init (&destPR, new, 0, 0, orig->width, orig->height, TRUE);

      copy_region (&srcPR, &destPR);
      new->x = orig->x;
      new->y = orig->y;
    }
  else
    {
      new = tile_manager_new (orig->width, orig->height, orig->bpp);
      new->x = orig->x;
      new->y = orig->y;

      if (type == FLIP_HORZ)
	for (i = 0; i < orig->width; i++)
	  {
	    pixel_region_init (&srcPR, orig, i, 0, 1, orig->height, FALSE);
	    pixel_region_init (&destPR, new, (orig->width - i - 1), 0, 1, orig->height, TRUE);
	    copy_region (&srcPR, &destPR); 
	  }
      else
	for (i = 0; i < orig->width; i++)
	  {
	    pixel_region_init (&srcPR, orig, 0, i, orig->width, 1, FALSE);
	    pixel_region_init (&destPR, new, 0, (orig->height - i - 1), orig->width, 1, TRUE);
	    copy_region (&srcPR, &destPR);
	  }
    }

  return new;
}
