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
#include <stdarg.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpselection.h"
#include "core/gimpundostack.h"

#include "vectors/gimpvectors.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimpeditselectiontool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


#define EDIT_SELECT_SCROLL_LOCK FALSE
#define ARROW_VELOCITY          25


#define GIMP_TYPE_EDIT_SELECTION_TOOL            (gimp_edit_selection_tool_get_type ())
#define GIMP_EDIT_SELECTION_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionTool))
#define GIMP_EDIT_SELECTION_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionToolClass))
#define GIMP_IS_EDIT_SELECTION_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL))
#define GIMP_IS_EDIT_SELECTION_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL))


typedef struct _GimpEditSelectionTool      GimpEditSelectionTool;
typedef struct _GimpEditSelectionToolClass GimpEditSelectionToolClass;

struct _GimpEditSelectionTool
{
  GimpDrawTool        parent_instance;

  gint                origx, origy;      /*  last x and y coords             */
  gint                cumlx, cumly;      /*  cumulative changes to x and yed */
  gint                x, y;              /*  current x and y coords          */
  gint                num_segs_in;       /* Num seg in selection boundary    */
  gint                num_segs_out;      /* Num seg in selection boundary    */
  BoundSeg           *segs_in;           /* Pointer to the channel sel. segs */
  BoundSeg           *segs_out;          /* Pointer to the channel sel. segs */

  gint                x1, y1;            /*  bounding box of selection mask  */
  gint                x2, y2;

  EditType            edit_type;         /*  translate the mask or layer?    */

  gboolean            first_move;        /*  we undo_freeze after the first  */
};

struct _GimpEditSelectionToolClass
{
  GimpDrawToolClass parent_class;
};


static GType   gimp_edit_selection_tool_get_type   (void) G_GNUC_CONST;

static void    gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass);
static void    gimp_edit_selection_tool_init       (GimpEditSelectionTool *edit_selection_tool);

static void    gimp_edit_selection_tool_button_release (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *gdisp);
static void    gimp_edit_selection_tool_motion         (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *gdisp);
void           gimp_edit_selection_tool_arrow_key      (GimpTool        *tool,
                                                        GdkEventKey     *kevent,
                                                        GimpDisplay     *gdisp);

static void    gimp_edit_selection_tool_draw           (GimpDrawTool    *tool);


static GimpDrawToolClass *parent_class = NULL;


static GType
gimp_edit_selection_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpEditSelectionToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_edit_selection_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpEditSelectionTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_edit_selection_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpEditSelectionTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_class;

  tool_class   = GIMP_TOOL_CLASS (klass);
  draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_release = gimp_edit_selection_tool_button_release;
  tool_class->motion         = gimp_edit_selection_tool_motion;

  draw_class->draw	     = gimp_edit_selection_tool_draw;
}

static void
gimp_edit_selection_tool_init (GimpEditSelectionTool *edit_selection_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (edit_selection_tool);

  gimp_tool_control_set_scroll_lock (tool->control, EDIT_SELECT_SCROLL_LOCK);
  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);

  edit_selection_tool->origx      = 0;
  edit_selection_tool->origy      = 0;

  edit_selection_tool->cumlx      = 0;
  edit_selection_tool->cumly      = 0;

  edit_selection_tool->first_move = TRUE;
}

static void
gimp_edit_selection_tool_calc_coords (GimpEditSelectionTool *edit_select,
                                      GimpDisplay           *gdisp,
                                      gdouble                x,
                                      gdouble                y)
{
  GimpDisplayShell *shell;
  gdouble           x1, y1;
  gdouble           dx, dy;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  dx = x - edit_select->origx;
  dy = y - edit_select->origy;

  x1 = edit_select->x1 + dx;
  y1 = edit_select->y1 + dy;

  edit_select->x = (gint) floor (x1) - (edit_select->x1 - edit_select->origx);
  edit_select->y = (gint) floor (y1) - (edit_select->y1 - edit_select->origy);
}

void
init_edit_selection (GimpTool    *tool,
		     GimpDisplay *gdisp,
		     GimpCoords  *coords,
		     EditType     edit_type)
{
  GimpEditSelectionTool *edit_select;
  GimpDisplayShell      *shell;
  GimpItem              *active_item;
  gint                   off_x, off_y;
  const gchar           *undo_desc;

  edit_select = g_object_new (GIMP_TYPE_EDIT_SELECTION_TOOL, NULL);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  Make a check to see if it should be a floating selection translation  */
  if (edit_type == EDIT_MASK_TO_LAYER_TRANSLATE &&
      gimp_image_floating_sel (gdisp->gimage))
    {
      edit_type = EDIT_FLOATING_SEL_TRANSLATE;
    }

  if (edit_type == EDIT_LAYER_TRANSLATE)
    {
      GimpLayer *layer = gimp_image_get_active_layer (gdisp->gimage);

      if (gimp_layer_is_floating_sel (layer))
	edit_type = EDIT_FLOATING_SEL_TRANSLATE;
    }

  edit_select->edit_type = edit_type;

  switch (edit_select->edit_type)
    {
    case EDIT_VECTORS_TRANSLATE:
      undo_desc = _("Move Path");
      break;

    case EDIT_CHANNEL_TRANSLATE:
      undo_desc = _("Move Channel");
      break;

    case EDIT_LAYER_MASK_TRANSLATE:
      undo_desc = _("Move Layer Mask");
      break;

    case EDIT_MASK_TRANSLATE:
      undo_desc = _("Move Selection");
      break;

    case EDIT_LAYER_TRANSLATE:
      undo_desc = _("Move Layer");
      break;

    default:
      undo_desc = _("Move Floating Layer");
      break;
    }

  gimp_image_undo_group_start (gdisp->gimage,
                               edit_select->edit_type == EDIT_MASK_TRANSLATE ?
                               GIMP_UNDO_GROUP_MASK :
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               undo_desc);

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (gdisp->gimage));
  else
    active_item = GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage));

  gimp_item_offsets (active_item, &off_x, &off_y);

  edit_select->x = edit_select->origx = coords->x - off_x;
  edit_select->y = edit_select->origy = coords->y - off_y;

  switch (edit_select->edit_type)
    {
    case EDIT_CHANNEL_TRANSLATE:
    case EDIT_LAYER_MASK_TRANSLATE:
      gimp_channel_boundary (GIMP_CHANNEL (active_item),
                             (const BoundSeg **) &edit_select->segs_in,
                             (const BoundSeg **) &edit_select->segs_out,
                             &edit_select->num_segs_in,
                             &edit_select->num_segs_out,
                             0, 0, 0, 0);
      break;

    default:
      gimp_channel_boundary (gimp_image_get_mask (gdisp->gimage),
                             (const BoundSeg **) &edit_select->segs_in,
                             (const BoundSeg **) &edit_select->segs_out,
                             &edit_select->num_segs_in,
                             &edit_select->num_segs_out,
                             0, 0, 0, 0);
      break;
    }

  edit_select->segs_in  = g_memdup (edit_select->segs_in,
                                    edit_select->num_segs_in *
                                    sizeof (BoundSeg));
  edit_select->segs_out = g_memdup (edit_select->segs_out,
                                    edit_select->num_segs_out *
                                    sizeof (BoundSeg));

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE)
    {
      edit_select->x1 = 0;
      edit_select->y1 = 0;
      edit_select->x2 = gdisp->gimage->width;
      edit_select->y2 = gdisp->gimage->height;
    }
  else
    {
      /*  find the bounding box of the selection mask -
       *  this is used for the case of a EDIT_MASK_TO_LAYER_TRANSLATE,
       *  where the translation will result in floating the selection
       *  mask and translating the resulting layer
       */
      gimp_drawable_mask_bounds (GIMP_DRAWABLE (active_item),
                                 &edit_select->x1, &edit_select->y1,
                                 &edit_select->x2, &edit_select->y2);
    }

  gimp_edit_selection_tool_calc_coords (edit_select, gdisp,
                                        edit_select->origx,
                                        edit_select->origy);

  {
    gint x1, y1, x2, y2;

    switch (edit_select->edit_type)
      {
      case EDIT_CHANNEL_TRANSLATE:
        gimp_channel_bounds (GIMP_CHANNEL (active_item),
                             &x1, &y1, &x2, &y2);
        break;

      case EDIT_LAYER_MASK_TRANSLATE:
        gimp_channel_bounds (GIMP_CHANNEL (active_item),
                             &x1, &y1, &x2, &y2);
        x1 += off_x;
        y1 += off_y;
        x2 += off_x;
        y2 += off_y;
        break;

      case EDIT_MASK_TRANSLATE:
        gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                             &x1, &y1, &x2, &y2);
        break;

      case EDIT_MASK_TO_LAYER_TRANSLATE:
      case EDIT_MASK_COPY_TO_LAYER_TRANSLATE:
        x1 = edit_select->x1 + off_x;
        y1 = edit_select->y1 + off_y;
        x2 = edit_select->x2 + off_x;
        y2 = edit_select->y2 + off_y;
        break;

      case EDIT_LAYER_TRANSLATE:
      case EDIT_FLOATING_SEL_TRANSLATE:
        x1 = off_x;
        y1 = off_y;
        x2 = x1 + gimp_item_width  (active_item);
        y2 = y1 + gimp_item_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            GList *linked;
            GList *list;

            linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                GIMP_ITEM_LINKED_LAYERS);

            /*  Expand the rectangle to include all linked layers as well  */
            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_offsets (item, &x3, &y3);

                x4 = x3 + gimp_item_width  (item);
                y4 = y3 + gimp_item_height (item);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }
        break;

      case EDIT_VECTORS_TRANSLATE:
        {
          gdouble  xd1, yd1, xd2, yd2;

          gimp_vectors_bounds (GIMP_VECTORS (active_item),
                               &xd1, &yd1, &xd2, &yd2);

          if (gimp_item_get_linked (active_item))
            {
              /*  Expand the rectangle to include all linked layers as well  */

              GList *linked;
              GList *list;

              linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                  GIMP_ITEM_LINKED_VECTORS);

              for (list = linked; list; list = g_list_next (list))
                {
                  GimpItem *item = list->data;
                  gdouble   x3, y3;
                  gdouble   x4, y4;

                  gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                  if (x3 < xd1)
                    xd1 = x3;
                  if (y3 < yd1)
                    yd1 = y3;
                  if (x4 > xd2)
                    xd2 = x4;
                  if (y4 > yd2)
                    yd2 = y4;
                }
            }

          x1 = ROUND (xd1);
          y1 = ROUND (yd1);
          x2 = ROUND (xd2);
          y2 = ROUND (yd2);
        }
        break;
      }

    gimp_tool_control_set_snap_offsets (GIMP_TOOL (edit_select)->control,
                                        x1 - coords->x,
                                        y1 - coords->y,
                                        x2 - x1,
                                        y2 - y1);
  }

  gimp_tool_control_activate (GIMP_TOOL (edit_select)->control);
  GIMP_TOOL (edit_select)->gdisp = gdisp;

  tool_manager_push_tool (gdisp->gimage->gimp,
			  GIMP_TOOL (edit_select));

  /*  pause the current selection  */
  gimp_display_shell_selection_visibility (shell, GIMP_SELECTION_PAUSE);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (GIMP_TOOL (edit_select),
                                _("Move: "), 0, ", ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (edit_select), gdisp);
}


static void
gimp_edit_selection_tool_button_release (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         guint32          time,
					 GdkModifierType  state,
					 GimpDisplay     *gdisp)
{
  GimpEditSelectionTool *edit_select;
  GimpDisplayShell      *shell;
  GimpItem              *active_item;

  edit_select = GIMP_EDIT_SELECTION_TOOL (tool);
  shell       = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  resume the current selection  */
  gimp_display_shell_selection_visibility (shell, GIMP_SELECTION_RESUME);

  gimp_tool_pop_status (tool);

  /*  Stop and free the selection core  */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (gdisp->gimage->gimp);

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (gdisp->gimage));
  else
    active_item = GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage));

  gimp_edit_selection_tool_calc_coords (edit_select, gdisp,
                                        coords->x,
                                        coords->y);

  /* EDIT_MASK_TRANSLATE is performed here at movement end, not 'live' like
   *  the other translation types.
   */
  if (edit_select->edit_type == EDIT_MASK_TRANSLATE)
    {
      /* move the selection -- whether there has been movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimp_item_translate (GIMP_ITEM (gimp_image_get_mask (gdisp->gimage)),
                           edit_select->cumlx,
                           edit_select->cumly,
                           TRUE);
    }

  if (edit_select->first_move)
    {
      gimp_image_undo_freeze (gdisp->gimage);
      edit_select->first_move = FALSE;
    }

  /* thaw the undo again */
  gimp_image_undo_thaw (gdisp->gimage);

  /*  EDIT_CHANNEL_TRANSLATE and EDIT_LAYER_MASK_TRANSLATE need to be
   *  preformed after thawing the undo.
   */
  if (edit_select->edit_type == EDIT_CHANNEL_TRANSLATE ||
      edit_select->edit_type == EDIT_LAYER_MASK_TRANSLATE)
    {
      /* move the channel -- whether there has been movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimp_item_translate (active_item,
                           edit_select->cumlx,
                           edit_select->cumly,
                           TRUE);
    }

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE ||
      edit_select->edit_type == EDIT_CHANNEL_TRANSLATE ||
      edit_select->edit_type == EDIT_LAYER_TRANSLATE)
    {
      if (! (state & GDK_BUTTON3_MASK) &&
          (edit_select->cumlx != 0 ||
           edit_select->cumly != 0))
        {
          if (gimp_item_get_linked (active_item))
            {
              /*  translate all linked channels as well  */

              GList *linked;
              GList *list;

              linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                  GIMP_ITEM_LINKED_CHANNELS);

              for (list = linked; list; list = g_list_next (list))
                gimp_item_translate (GIMP_ITEM (list->data),
                                     edit_select->cumlx,
                                     edit_select->cumly,
                                     TRUE);

              g_list_free (linked);
            }
        }
    }

  gimp_image_undo_group_end (gdisp->gimage);

  if (state & GDK_BUTTON3_MASK) /* OPERATION CANCELLED */
    {
      /* Operation cancelled - undo the undo-group! */
      gimp_image_undo (gdisp->gimage);
    }

  gimp_image_flush (gdisp->gimage);

  g_free (edit_select->segs_in);
  g_free (edit_select->segs_out);

  edit_select->segs_in      = NULL;
  edit_select->segs_out     = NULL;
  edit_select->num_segs_in  = 0;
  edit_select->num_segs_out = 0;

  g_object_unref (edit_select);
}


static void
gimp_edit_selection_tool_motion (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
				 GdkModifierType  state,
				 GimpDisplay     *gdisp)
{
  GimpEditSelectionTool *edit_select;
  GimpDisplayShell      *shell;
  GimpItem              *active_item;
  gint                   off_x, off_y;
  gdouble                motion_x, motion_y;

  edit_select = GIMP_EDIT_SELECTION_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gdk_flush ();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (gdisp->gimage));
  else
    active_item = GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage));

  gimp_item_offsets (active_item, &off_x, &off_y);

  motion_x = coords->x - off_x;
  motion_y = coords->y - off_y;

  /* now do the actual move. */

  gimp_edit_selection_tool_calc_coords (edit_select, gdisp,
                                        motion_x,
                                        motion_y);

  /******************************************* adam's live move *******/
  /********************************************************************/
  {
    gint x, y;

    x = edit_select->x;
    y = edit_select->y;

    /* if there has been movement, move the selection  */
    if (edit_select->origx != x || edit_select->origy != y)
      {
	gint xoffset, yoffset;

	xoffset = x - edit_select->origx;
	yoffset = y - edit_select->origy;

	edit_select->cumlx += xoffset;
	edit_select->cumly += yoffset;

	switch (edit_select->edit_type)
	  {
          case EDIT_LAYER_MASK_TRANSLATE:
	  case EDIT_MASK_TRANSLATE:
	    /*  we don't do the actual edit selection move here.  */
	    edit_select->origx = x;
	    edit_select->origy = y;
	    break;

          case EDIT_VECTORS_TRANSLATE:
          case EDIT_CHANNEL_TRANSLATE:
	    edit_select->origx = x;
	    edit_select->origy = y;

            /*  fallthru  */

	  case EDIT_LAYER_TRANSLATE:
            /*  for CHANNEL_TRANSLATE, only translate the linked layers
             *  and vectors on-the-fly, the channel is translated
             *  on button_release.
             */
            if (edit_select->edit_type != EDIT_CHANNEL_TRANSLATE)
              gimp_item_translate (active_item, xoffset, yoffset, TRUE);

            if (gimp_item_get_linked (active_item))
              {
                /*  translate all linked layers & vectors as well  */

                GList *linked;
                GList *list;

                linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                    GIMP_ITEM_LINKED_LAYERS |
                                                    GIMP_ITEM_LINKED_VECTORS);

                for (list = linked; list; list = g_list_next (list))
                  gimp_item_translate (GIMP_ITEM (list->data),
                                       xoffset, yoffset, TRUE);

                g_list_free (linked);
              }

            if (edit_select->first_move)
              {
                gimp_image_undo_freeze (gdisp->gimage);
                edit_select->first_move = FALSE;
              }
	    break;

	  case EDIT_MASK_TO_LAYER_TRANSLATE:
	  case EDIT_MASK_COPY_TO_LAYER_TRANSLATE:
	    if (! gimp_selection_float (gimp_image_get_mask (gdisp->gimage),
                                        GIMP_DRAWABLE (active_item),
                                        gimp_get_user_context (gdisp->gimage->gimp),
                                        edit_select->edit_type ==
                                        EDIT_MASK_TO_LAYER_TRANSLATE,
                                        0, 0))
	      {
		/* no region to float, abort safely */
		gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

		return;
	      }

	    /*  this is always the first move, since we switch to
             *  EDIT_FLOATING_SEL_TRANSLATE when finished here
             */
	    gimp_image_undo_freeze (gdisp->gimage);
	    edit_select->first_move = FALSE;

	    edit_select->origx -= edit_select->x1;
	    edit_select->origy -= edit_select->y1;
	    edit_select->x2    -= edit_select->x1;
	    edit_select->y2    -= edit_select->y1;
	    edit_select->x1     = 0;
	    edit_select->y1     = 0;

	    edit_select->edit_type = EDIT_FLOATING_SEL_TRANSLATE;

            active_item = GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage));

            /* fall through */

	  case EDIT_FLOATING_SEL_TRANSLATE:
            gimp_item_translate (active_item, xoffset, yoffset, TRUE);

            if (edit_select->first_move)
              {
                gimp_image_undo_freeze (gdisp->gimage);
                edit_select->first_move = FALSE;
              }
	    break;

	  default:
	    g_warning ("esm / BAD FALLTHROUGH");
	  }
      }

    gimp_display_flush (gdisp);
  }
  /********************************************************************/
  /********************************************************************/

  gimp_tool_pop_status (tool);

  gimp_tool_push_status_coords (tool,
                                _("Move: "),
                                edit_select->cumlx,
                                ", ",
                                edit_select->cumly);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_edit_selection_tool_draw (GimpDrawTool *draw_tool)
{
  GimpEditSelectionTool *edit_select;
  GimpDisplay           *gdisp;
  GimpItem              *active_item;

  edit_select = GIMP_EDIT_SELECTION_TOOL (draw_tool);
  gdisp       = GIMP_TOOL (draw_tool)->gdisp;

  if (edit_select->edit_type == EDIT_VECTORS_TRANSLATE)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (gdisp->gimage));
  else
    active_item = GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage));

  switch (edit_select->edit_type)
    {
    case EDIT_CHANNEL_TRANSLATE:
    case EDIT_LAYER_MASK_TRANSLATE:
    case EDIT_MASK_TRANSLATE:
      {
        gboolean floating_sel = FALSE;
        gint     off_x        = 0;
        gint     off_y        = 0;

        if (edit_select->edit_type == EDIT_MASK_TRANSLATE)
          {
            GimpLayer *layer = gimp_image_get_active_layer (gdisp->gimage);

            if (layer)
              floating_sel = gimp_layer_is_floating_sel (layer);
          }
        else
          {
            gimp_item_offsets (active_item, &off_x, &off_y);
          }

        if (! floating_sel)
          gimp_draw_tool_draw_boundary (draw_tool,
                                        edit_select->segs_in,
                                        edit_select->num_segs_in,
                                        edit_select->cumlx + off_x,
                                        edit_select->cumly + off_y,
                                        FALSE);

        if (edit_select->segs_out)
          {
            gimp_draw_tool_draw_boundary (draw_tool,
                                          edit_select->segs_out,
                                          edit_select->num_segs_out,
                                          edit_select->cumlx + off_x,
                                          edit_select->cumly + off_y,
                                          FALSE);
          }
        else if (edit_select->edit_type != EDIT_MASK_TRANSLATE)
          {
            gimp_draw_tool_draw_rectangle (draw_tool,
                                           FALSE,
                                           edit_select->cumlx + off_x,
                                           edit_select->cumly + off_y,
                                           active_item->width,
                                           active_item->height,
                                           FALSE);
          }
      }
      break;

    case EDIT_MASK_TO_LAYER_TRANSLATE:
    case EDIT_MASK_COPY_TO_LAYER_TRANSLATE:
      gimp_draw_tool_draw_rectangle (draw_tool,
                                     FALSE,
                                     edit_select->x1,
                                     edit_select->y1,
                                     edit_select->x2 - edit_select->x1,
                                     edit_select->y2 - edit_select->y1,
                                     TRUE);
      break;

    case EDIT_LAYER_TRANSLATE:
      {
        GimpItem *active_item;
        gint      x1, y1, x2, y2;

        active_item = GIMP_ITEM (gimp_image_get_active_layer (gdisp->gimage));

        gimp_item_offsets (active_item, &x1, &y1);

        x2 = x1 + gimp_item_width  (active_item);
        y2 = y1 + gimp_item_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked layers as well  */

            GList *linked;
            GList *list;

            linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                GIMP_ITEM_LINKED_LAYERS);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_offsets (item, &x3, &y3);

                x4 = x3 + gimp_item_width  (item);
                y4 = y3 + gimp_item_height (item);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }

        gimp_draw_tool_draw_rectangle (draw_tool,
                                       FALSE,
                                       x1, y1,
                                       x2 - x1, y2 - y1,
                                       FALSE);
      }
      break;

    case EDIT_VECTORS_TRANSLATE:
      {
        GimpItem *active_item;
        gdouble   x1, y1, x2, y2;

        active_item = GIMP_ITEM (gimp_image_get_active_vectors (gdisp->gimage));

        gimp_vectors_bounds (GIMP_VECTORS (active_item), &x1, &y1, &x2, &y2);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked vectors as well  */

            GList *linked;
            GList *list;

            linked = gimp_item_linked_get_list (gdisp->gimage, active_item,
                                                GIMP_ITEM_LINKED_VECTORS);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gdouble   x3, y3;
                gdouble   x4, y4;

                gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }

        gimp_draw_tool_draw_rectangle (draw_tool,
                                       FALSE,
                                       ROUND (x1), ROUND (y1),
                                       ROUND (x2 - x1), ROUND (y2 - y1),
                                       FALSE);
      }
      break;

    case EDIT_FLOATING_SEL_TRANSLATE:
      gimp_draw_tool_draw_boundary (draw_tool,
                                    edit_select->segs_in,
                                    edit_select->num_segs_in,
                                    edit_select->cumlx,
                                    edit_select->cumly,
                                    FALSE);
      break;
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

/* could move this function to a more central location
 * so it can be used by other tools?
 */
static gint
process_event_queue_keys (GdkEventKey *kevent,
			  ... /* GdkKeyType, GdkModifierType, value ... 0 */)
{

#define FILTER_MAX_KEYS 50

  va_list          argp;
  GdkEvent        *event;
  GList           *event_list = NULL;
  GList           *list;
  guint            keys[FILTER_MAX_KEYS];
  GdkModifierType  modifiers[FILTER_MAX_KEYS];
  gint             values[FILTER_MAX_KEYS];
  gint             i      = 0;
  gint             n_keys = 0;
  gint             value  = 0;
  gboolean         done   = FALSE;
  GtkWidget       *orig_widget;

  va_start (argp, kevent);

  while (n_keys < FILTER_MAX_KEYS && (keys[n_keys] = va_arg (argp, guint)) != 0)
    {
      modifiers[n_keys] = va_arg (argp, GdkModifierType);
      values[n_keys]    = va_arg (argp, gint);
      n_keys++;
    }

  va_end (argp);

  for (i = 0; i < n_keys; i++)
    if (kevent->keyval                 == keys[i] &&
        (kevent->state & modifiers[i]) == modifiers[i])
      value += values[i];

  orig_widget = gtk_get_event_widget ((GdkEvent *) kevent);

  while (gdk_events_pending () > 0 && ! done)
    {
      gboolean discard_event = FALSE;

      event = gdk_event_get ();

      if (! event || orig_widget != gtk_get_event_widget (event))
        {
          done = TRUE;
        }
      else
        {
          if (event->any.type == GDK_KEY_PRESS)
            {
              for (i = 0; i < n_keys; i++)
                if (event->key.keyval                 == keys[i] &&
                    (event->key.state & modifiers[i]) == modifiers[i])
                  {
                    discard_event = TRUE;
                    value += values[i];
                  }

              if (! discard_event)
                done = TRUE;
            }
          /* should there be more types here? */
          else if (event->any.type != GDK_KEY_RELEASE &&
                   event->any.type != GDK_MOTION_NOTIFY &&
                   event->any.type != GDK_EXPOSE)
            done = FALSE;
        }

      if (! event)
        ; /* Do nothing */
      else if (! discard_event)
        event_list = g_list_prepend (event_list, event);
      else
        gdk_event_free (event);
    }

  event_list = g_list_reverse (event_list);

  /* unget the unused events and free the list */
  for (list = event_list; list; list = g_list_next (list))
    {
      gdk_event_put ((GdkEvent *) list->data);
      gdk_event_free ((GdkEvent *) list->data);
    }

  g_list_free (event_list);

  return value;

#undef FILTER_MAX_KEYS
}

void
gimp_edit_selection_tool_arrow_key (GimpTool    *tool,
				    GdkEventKey *kevent,
				    GimpDisplay *gdisp)
{
  gint          inc_x     = 0;
  gint          inc_y     = 0;
  GimpUndo     *undo;
  gboolean      push_undo = TRUE;
  GimpItem     *item      = NULL;
  EditType      edit_type = EDIT_MASK_TRANSLATE;
  GimpUndoType  undo_type = GIMP_UNDO_GROUP_MASK;
  const gchar  *undo_desc = NULL;

  /*  check for mask translation first because the translate_layer
   *  modifiers match the translate_mask ones...
   */
  inc_x =
    process_event_queue_keys (kevent,
			      GDK_Left, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
                              -1 * ARROW_VELOCITY,

			      GDK_Left, GDK_MOD1_MASK,
                              -1,

			      GDK_Right, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
                              1 * ARROW_VELOCITY,

			      GDK_Right, GDK_MOD1_MASK,
                              1,

			      0);

  inc_y =
    process_event_queue_keys (kevent,
			      GDK_Up, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
                              -1 * ARROW_VELOCITY,

			      GDK_Up, GDK_MOD1_MASK,
                              -1,

			      GDK_Down, (GDK_MOD1_MASK | GDK_SHIFT_MASK),
                              1 * ARROW_VELOCITY,

			      GDK_Down, GDK_MOD1_MASK,
                              1,

			      0);

  if (inc_x != 0 || inc_y != 0)
    {
      item = GIMP_ITEM (gimp_image_get_mask (gdisp->gimage));

      edit_type = EDIT_MASK_TRANSLATE;
      undo_type = GIMP_UNDO_GROUP_MASK;
      undo_desc = _("Move Selection");
    }
  else
    {
      inc_x = process_event_queue_keys (kevent,
                                        GDK_Left, (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                        -1 * ARROW_VELOCITY,

                                        GDK_Left, GDK_CONTROL_MASK,
                                        -1,

                                        GDK_Right, (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                        1 * ARROW_VELOCITY,

                                        GDK_Right, GDK_CONTROL_MASK,
                                        1,

                                        0);

      inc_y = process_event_queue_keys (kevent,
                                        GDK_Up, (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                        -1 * ARROW_VELOCITY,

                                        GDK_Up, GDK_CONTROL_MASK,
                                        -1,

                                        GDK_Down, (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                        1 * ARROW_VELOCITY,

                                        GDK_Down, GDK_CONTROL_MASK,
                                        1,

                                        0);

      if (inc_x != 0 || inc_y != 0)
        {
          item = (GimpItem *) gimp_image_get_active_vectors (gdisp->gimage);

          edit_type = EDIT_VECTORS_TRANSLATE;
          undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;
          undo_desc = _("Move Path");
        }
      else
        {
          inc_x = process_event_queue_keys (kevent,
                                            GDK_Left, GDK_SHIFT_MASK,
                                            -1 * ARROW_VELOCITY,

                                            GDK_Left, 0,
                                            -1,

                                            GDK_Right, GDK_SHIFT_MASK,
                                            1 * ARROW_VELOCITY,

                                            GDK_Right, 0,
                                            1,

                                            0);

          inc_y = process_event_queue_keys (kevent,
                                            GDK_Up, GDK_SHIFT_MASK,
                                            -1 * ARROW_VELOCITY,

                                            GDK_Up, 0,
                                            -1,

                                            GDK_Down, GDK_SHIFT_MASK,
                                            1 * ARROW_VELOCITY,

                                            GDK_Down, 0,
                                            1,

                                            0);

          if (inc_x != 0 || inc_y != 0)
            {
              item = (GimpItem *) gimp_image_active_drawable (gdisp->gimage);

              if (item)
                {
                  if (GIMP_IS_LAYER_MASK (item))
                    {
                      edit_type = EDIT_LAYER_MASK_TRANSLATE;
                      undo_desc = _("Move Layer Mask");
                    }
                  else if (GIMP_IS_CHANNEL (item))
                    {
                      edit_type = EDIT_CHANNEL_TRANSLATE;
                      undo_desc = _("Move Channel");
                    }
                  else if (gimp_layer_is_floating_sel (GIMP_LAYER (item)))
                    {
                      edit_type = EDIT_FLOATING_SEL_TRANSLATE;
                      undo_desc = _("Move Floating Layer");
                    }
                  else
                    {
                      edit_type = EDIT_LAYER_TRANSLATE;
                      undo_desc = _("Move Layer");
                    }

                  undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;
                }
            }
        }
    }

  if (! item)
    return;

  undo = gimp_undo_stack_peek (gdisp->gimage->undo_stack);

  /* compress undo */
  if (! gimp_undo_stack_peek (gdisp->gimage->redo_stack) &&
      GIMP_IS_UNDO_STACK (undo) && undo->undo_type == undo_type)
    {
      if (g_object_get_data (G_OBJECT (undo), "edit-selection-tool") ==
          (gpointer) tool                                                &&
          g_object_get_data (G_OBJECT (undo), "edit-selection-item") ==
          (gpointer) item                                                &&
          g_object_get_data (G_OBJECT (undo), "edit-selection-type") ==
          GINT_TO_POINTER (edit_type))
        {
          push_undo = FALSE;
        }
    }

  if (push_undo)
    {
      if (gimp_image_undo_group_start (gdisp->gimage, undo_type, undo_desc))
        {
          undo = gimp_undo_stack_peek (gdisp->gimage->undo_stack);

          if (GIMP_IS_UNDO_STACK (undo) && undo->undo_type == undo_type)
            {
              g_object_set_data (G_OBJECT (undo), "edit-selection-tool",
                                 tool);
              g_object_set_data (G_OBJECT (undo), "edit-selection-item",
                                 item);
              g_object_set_data (G_OBJECT (undo), "edit-selection-type",
                                 GINT_TO_POINTER (edit_type));
            }
        }
    }

  switch (edit_type)
    {
    case EDIT_LAYER_MASK_TRANSLATE:
    case EDIT_MASK_TRANSLATE:
      gimp_item_translate (item, inc_x, inc_y, push_undo);
      break;

    case EDIT_MASK_TO_LAYER_TRANSLATE:
    case EDIT_MASK_COPY_TO_LAYER_TRANSLATE:
      /*  this won't happen  */
      break;

    case EDIT_VECTORS_TRANSLATE:
    case EDIT_CHANNEL_TRANSLATE:
    case EDIT_LAYER_TRANSLATE:
      gimp_item_translate (item, inc_x, inc_y, push_undo);

      /*  translate all linked items as well  */
      if (gimp_item_get_linked (item))
        gimp_item_linked_translate (item, inc_x, inc_y, push_undo);
      break;

    case EDIT_FLOATING_SEL_TRANSLATE:
      gimp_item_translate (item, inc_x, inc_y, push_undo);
      break;
    }

  if (push_undo)
    gimp_image_undo_group_end (gdisp->gimage);
  else
    gimp_undo_refresh_preview (undo);

  gimp_image_flush (gdisp->gimage);
}
