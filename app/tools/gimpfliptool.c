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
#include "transform_core.h"
#include "undo.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"

#include "tile_manager_pvt.h"		/* ick. */

#define FLIP 0

/*  the flip structures  */

typedef struct _FlipOptions FlipOptions;
struct _FlipOptions
{
  ToolOptions  tool_options;

  ToolType     type;
  ToolType     type_d;
  GtkWidget   *type_w[2];  /* 2 radio buttons */
};


/*  the flip tool options  */
static FlipOptions *flip_options = NULL;


/*  forward function declarations  */
static Tool        * tools_new_flip_horz  (void);
static Tool        * tools_new_flip_vert  (void);
static TileManager * flip_tool_flip_horz  (GImage *, GimpDrawable *,
					   TileManager *, int);
static TileManager * flip_tool_flip_vert  (GImage *, GimpDrawable *,
					   TileManager *, int);
static void          flip_change_type     (int);
static Argument    * flip_invoker         (Argument *);


/*  functions  */

static void
flip_type_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  flip_change_type ((long) client_data);
}

static void
flip_options_reset (void)
{
  FlipOptions *options = flip_options;

  gtk_toggle_button_set_active (((options->type_d == FLIP_HORZ) ?
				 GTK_TOGGLE_BUTTON (options->type_w[0]) : 
				 GTK_TOGGLE_BUTTON (options->type_w[1])),
				TRUE);
}

static FlipOptions *
flip_options_new (void)
{
  FlipOptions *options;

  GtkWidget *vbox;
  GSList    *group = NULL;
  GtkWidget *radio_box;

  /*  the new flip tool options structure  */
  options = (FlipOptions *) g_malloc (sizeof (FlipOptions));
  tool_options_init ((ToolOptions *) options,
		     _("Flip Tool Options"),
		     flip_options_reset);
  options->type = options->type_d = FLIP_HORZ;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  radio_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), radio_box, FALSE, FALSE, 0);

  /*  the radio buttons  */
  options->type_w[0] =
    gtk_radio_button_new_with_label (group, _("Horizontal"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (options->type_w[0]));
  gtk_box_pack_start (GTK_BOX (radio_box), options->type_w[0], FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->type_w[0]), "toggled",
		      (GtkSignalFunc) flip_type_callback,
		      (gpointer) ((long) (FLIP_HORZ)));
  gtk_widget_show (options->type_w[0]);

  options->type_w[1] = gtk_radio_button_new_with_label (group, _("Vertical"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (options->type_w[1]));
  gtk_box_pack_start (GTK_BOX (radio_box), options->type_w[1], FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->type_w[1]), "toggled",
		      (GtkSignalFunc) flip_type_callback,
		      (gpointer) ((long) (FLIP_VERT)));
  gtk_widget_show (options->type_w[1]);

  gtk_widget_show (radio_box);

  return options;
}

static void *
flip_tool_transform_horz (Tool     *tool,
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
      return flip_tool_flip_horz (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
				  transform_core->original, transform_core->trans_info[FLIP]);
      break;
    }

  return NULL;
}

static void *
flip_tool_transform_vert (Tool     *tool,
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
      /*transform_core->trans_info[FLIP] *= -1.0;*/
      return flip_tool_flip_vert (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
				  transform_core->original, transform_core->trans_info[FLIP]);
      break;
    }

  return NULL;
}

Tool *
tools_new_flip ()
{
  /*  The tool options  */
  if (! flip_options)
    {
      flip_options = flip_options_new ();
      tools_register (FLIP_HORZ, (ToolOptions *) flip_options);
      tools_register (FLIP_VERT, (ToolOptions *) flip_options);

      /*  press all default buttons  */
      flip_options_reset ();
    }

  switch (flip_options->type)
    {
    case FLIP_HORZ:
      return tools_new_flip_horz ();
      break;
    case FLIP_VERT:
      return tools_new_flip_vert ();
      break;
    default:
      return NULL;
      break;
    }
}

void
tools_free_flip_tool (Tool *tool)
{
  transform_core_free (tool);
}

static Tool *
tools_new_flip_horz ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (FLIP_HORZ, NON_INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = flip_tool_transform_horz;
  private->trans_info[FLIP] = -1.0;

  return tool;
}

static Tool *
tools_new_flip_vert ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (FLIP_VERT, NON_INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = flip_tool_transform_vert;
  private->trans_info[FLIP] = -1.0;

  return tool;
}

static TileManager *
flip_tool_flip_horz (GImage      *gimage,
		     GimpDrawable *drawable,
		     TileManager *orig,
		     int          flip)
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

      for (i = 0; i < orig->width; i++)
	{
	  pixel_region_init (&srcPR, orig, i, 0, 1, orig->height, FALSE);
	  pixel_region_init (&destPR, new, (orig->width - i - 1), 0, 1, orig->height, TRUE);

	  copy_region (&srcPR, &destPR);
	}
    }

  return new;
}

static TileManager *
flip_tool_flip_vert (GImage      *gimage,
		     GimpDrawable *drawable,
		     TileManager *orig,
		     int          flip)
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

      for (i = 0; i < orig->height; i++)
	{
	  pixel_region_init (&srcPR, orig, 0, i, orig->width, 1, FALSE);
	  pixel_region_init (&destPR, new, 0, (orig->height - i - 1), orig->width, 1, TRUE);

	  copy_region (&srcPR, &destPR);
	}
    }

  return new;
}

static void
flip_change_type (int new_type)
{
  if (flip_options->type != new_type)
    {
      /*  change the type, free the old tool, create the new tool  */
      flip_options->type = new_type;

      tools_select (flip_options->type);
    }
}

/*  The flip procedure definition  */
ProcArg flip_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the affected drawable"
  },
  { PDB_INT32,
    "flip_type",
    "Type of flip: { HORIZONTAL (0), VERTICAL (1) }"
  }
};

ProcArg flip_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the flipped drawable"
  }
};

ProcRecord flip_proc =
{
  "gimp_flip",
  "Flip the specified drawable about its center either vertically or horizontally",
  "This tool flips the specified drawable if no selection exists.  If a selection exists, the portion of the drawable which lies under the selection is cut from the drawable and made into a floating selection which is then flipd by the specified amount.  The return value is the ID of the flipped drawable.  If there was no selection, this will be equal to the drawable ID supplied as input.  Otherwise, this will be the newly created and flipd drawable.  The flip type parameter indicates whether the flip will be applied horizontally or vertically.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  flip_args,

  /*  Output arguments  */
  1,
  flip_out_args,

  /*  Exec method  */
  { { flip_invoker } },
};

static Argument *
flip_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int flip_type;
  int int_value;
  TileManager *float_tiles;
  TileManager *new_tiles;
  int new_layer;
  Layer *layer;
  Argument *return_args;

  drawable = NULL;
  flip_type   = 0;
  new_tiles   = NULL;
  layer       = NULL;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  flip type */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      switch (int_value)
	{
	case 0: flip_type = 0; break;
	case 1: flip_type = 1; break;
	default: success = FALSE;
	}
    }

  /*  call the flip procedure  */
  if (success)
    {
      /*  Start a transform undo group  */
      undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

      /*  Cut/Copy from the specified drawable  */
      float_tiles = transform_core_cut (gimage, drawable, &new_layer);

      /*  flip the buffer  */
      switch (flip_type)
	{
	case 0: /* horz */
	  new_tiles = flip_tool_flip_horz (gimage, drawable, float_tiles, -1);
	  break;
	case 1: /* vert */
	  new_tiles = flip_tool_flip_vert (gimage, drawable, float_tiles, -1);
	  break;
	}

      /*  free the cut/copied buffer  */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
	success = (layer = transform_core_paste (gimage, drawable, new_tiles, new_layer)) != NULL;
      else
	success = FALSE;

      /*  push the undo group end  */
      undo_push_group_end (gimage);
    }

  return_args = procedural_db_return_args (&flip_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}
