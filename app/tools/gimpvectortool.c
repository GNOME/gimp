/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"
#include "gui/gui-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpbezier.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gui/info-dialog.h"

#include "gimpvectortool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/*  definitions  */
#define  TARGET         8
#define  ARC_RADIUS     30
#define  STATUSBAR_SIZE 128

/*  maximum information buffer size  */
#define  MAX_INFO_BUF   16

/*  the vector tool options  */
typedef struct _VectorOptions VectorOptions;

struct _VectorOptions
{
  GimpToolOptions  tool_options;

  gboolean     use_info_window;
  gboolean     use_info_window_d;
  GtkWidget   *use_info_window_w;
};


/*  local function prototypes  */
static void   gimp_vector_tool_class_init      (GimpVectorToolClass *klass);
static void   gimp_vector_tool_init            (GimpVectorTool      *tool);

static void   gimp_vector_tool_control         (GimpTool        *tool,
                                                GimpToolAction   action,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_button_press    (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_button_release  (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_motion          (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_vector_tool_cursor_update   (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);

static void   gimp_vector_tool_draw            (GimpDrawTool    *draw_tool);



static GimpToolOptions * vector_tool_options_new     (GimpToolInfo    *tool_info);
static void              vector_tool_options_reset   (GimpToolOptions *tool_options);


static GimpDrawToolClass *parent_class = NULL;


void
gimp_vector_tool_register (Gimp                     *gimp,
                           GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_VECTOR_TOOL,
                vector_tool_options_new,
                FALSE,
                "gimp:vector_tool",
                _("Vector Tool"),
                _("Vector angles and lengths"),
                N_("/Tools/Vector"), NULL,
                NULL, "tools/vector.html",
                GIMP_STOCK_TOOL_PATH);
}

GType
gimp_vector_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpVectorToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_vector_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpVectorTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_vector_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpVectorTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_vector_tool_class_init (GimpVectorToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control        = gimp_vector_tool_control;
  tool_class->button_press   = gimp_vector_tool_button_press;
  tool_class->button_release = gimp_vector_tool_button_release;
  tool_class->motion         = gimp_vector_tool_motion;
  tool_class->cursor_update  = gimp_vector_tool_cursor_update;

  draw_tool_class->draw      = gimp_vector_tool_draw;
}

static void
gimp_vector_tool_init (GimpVectorTool *vector_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (vector_tool);

  tool->tool_cursor = GIMP_CURSOR_MODIFIER_NONE;

  tool->preserve    = TRUE;  /*  Preserve on drawable change  */
  
  vector_tool->vectors = g_object_new (GIMP_TYPE_BEZIER, 0);
}

static void
gimp_vector_tool_control (GimpTool       *tool,
                           GimpToolAction  action,
                           GimpDisplay    *gdisp)
{
  GimpVectorTool *vector_tool;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      gimp_tool_pop_status (tool);
      tool->state = INACTIVE;
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_vector_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpVectorTool  *vector_tool;
  VectorOptions   *options;
  GimpDisplayShell *shell;
  gint              i;
  GimpCoords        cur_point;
  GimpAnchor       *anchor;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  options = (VectorOptions *) tool->tool_info->tool_options;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  if we are changing displays, pop the statusbar of the old one  */ 
  if (tool->state == ACTIVE && gdisp != tool->gdisp)
    {
      gimp_tool_pop_status (tool);
    }
  
  vector_tool->function = VCREATING;

  if (tool->state == ACTIVE && gdisp == tool->gdisp)
    {
      /*  if the cursor is in one of the handles,
       *  the new function will be moving or adding a new point or guide
       */

      anchor = gimp_bezier_anchor_get (vector_tool->vectors, coords);

      if (anchor && gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), gdisp,
                                              coords->x,
                                              coords->y,
                                              GIMP_HANDLE_CIRCLE,
                                              anchor->position.x,
                                              anchor->position.y,
                                              TARGET, TARGET,
                                              GTK_ANCHOR_CENTER,
                                              FALSE))
        {
          vector_tool->function = VMOVING;
          vector_tool->cur_anchor = anchor;
        }

    }
  
  if (vector_tool->function == VCREATING)
    {
      if (tool->state == ACTIVE)
	{
	  /* reset everything */
	  gimp_draw_tool_stop (GIMP_DRAW_TOOL (vector_tool));
	}

      cur_point.x = coords->x;
      cur_point.y = coords->y;
      anchor = gimp_vectors_anchor_set (vector_tool->vectors, &cur_point, TRUE);
      vector_tool->cur_anchor = anchor;
      vector_tool->function = VMOVING;

      /*  set the gdisplay  */
      tool->gdisp = gdisp;

      if (tool->state == ACTIVE)
	{
	  gimp_tool_pop_status (tool);
	  gimp_tool_push_status (tool, "");
        }

      /*  start drawing the vector tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
    }

  tool->state = ACTIVE;
}

static void
gimp_vector_tool_button_release (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpVectorTool *vector_tool;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  vector_tool->function = VFINISHED;
}

static void
gimp_vector_tool_motion (GimpTool        *tool,
                         GimpCoords      *coords,
                         guint32          time,
                         GdkModifierType  state,
                         GimpDisplay     *gdisp)
{
  GimpVectorTool *vector_tool;
  VectorOptions  *options;
  gint            tmp;
  GimpAnchor     *anchor;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  options = (VectorOptions *) tool->tool_info->tool_options;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (vector_tool));

  switch (vector_tool->function)
    {
    case VMOVING:
      /*  if we are moving the start point and only have two, make it the end point  */
      anchor = vector_tool->cur_anchor;

      if (anchor)
        gimp_vectors_anchor_move_absolute (vector_tool->vectors,
                                           vector_tool->cur_anchor,
                                           coords, 0);

    default:
      break;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (vector_tool));
}

static void
gimp_vector_tool_cursor_update (GimpTool        *tool,
                                GimpCoords      *coords,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpVectorTool   *vector_tool;
  gboolean           in_handle = FALSE;
  GdkCursorType      ctype     = GIMP_MOUSE_CURSOR;
  GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gint               i;
  GimpAnchor        *anchor;

  vector_tool = GIMP_VECTOR_TOOL (tool);

  if (tool->state == ACTIVE && tool->gdisp == gdisp)
    {
      anchor = gimp_bezier_anchor_get (vector_tool->vectors, coords);

      if (anchor && gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), gdisp,
                                              coords->x,
                                              coords->y,
                                              GIMP_HANDLE_CIRCLE,
                                              anchor->position.x,
                                              anchor->position.y,
                                              TARGET, TARGET,
                                              GTK_ANCHOR_CENTER,
                                              FALSE))
	    {
	      in_handle = TRUE;
              ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
	    }
    }

  tool->cursor          = ctype;
  tool->cursor_modifier = cmodifier;

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_vector_tool_draw (GimpDrawTool *draw_tool)
{
  GimpVectorTool *vector_tool;
  GimpTool        *tool;
  gint             i;
  gint             angle1, angle2;
  gint             draw_arc = 0;
  GimpAnchor      *cur_anchor = NULL;
  GimpStroke      *cur_stroke = NULL;
  GimpVectors     *vectors;

  vector_tool = GIMP_VECTOR_TOOL (draw_tool);
  tool         = GIMP_TOOL (draw_tool);

  vectors = vector_tool->vectors;

  while ((cur_anchor = gimp_vectors_anchor_get_next (vectors, cur_anchor))) {
    gimp_draw_tool_draw_handle (draw_tool,
                                GIMP_HANDLE_CIRCLE,
                                cur_anchor->position.x,
                                cur_anchor->position.y,
                                TARGET,
                                TARGET,
                                GTK_ANCHOR_CENTER,
                                FALSE);
  }

  for (i = 0; i < vector_tool->num_points; i++)
    {
      if (i == 0 && vector_tool->num_points == 3)
	{
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CIRCLE,
                                      vector_tool->x[i],
                                      vector_tool->y[i],
                                      TARGET,
                                      TARGET,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
	}
      else
	{
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      vector_tool->x[i],
                                      vector_tool->y[i],
                                      TARGET * 2,
                                      TARGET * 2,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
	}

      if (i > 0)
	{
          gimp_draw_tool_draw_line (draw_tool,
                                    vector_tool->x[0],
                                    vector_tool->y[0],
                                    vector_tool->x[i],
                                    vector_tool->y[i],
                                    FALSE);

	  /*  only draw the arc if the lines are long enough  */
          if (gimp_draw_tool_calc_distance (draw_tool, tool->gdisp,
                                            vector_tool->x[0],
                                            vector_tool->y[0],
                                            vector_tool->x[i],
                                            vector_tool->y[i]) > ARC_RADIUS)
            {
              draw_arc++;
            }
	}
    }

  if (vector_tool->num_points > 1 && draw_arc == vector_tool->num_points - 1)
    {
      angle1 = vector_tool->angle2 * 64.0;
      angle2 = (vector_tool->angle1 - vector_tool->angle2) * 64.0;

      if (angle2 > 11520)
	  angle2 -= 23040;
      if (angle2 < -11520)
	  angle2 += 23040;

      if (angle2 != 0)
	{
          gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                             FALSE,
                                             vector_tool->x[0],
                                             vector_tool->y[0],
                                             ARC_RADIUS,
                                             ARC_RADIUS,
                                             angle1, angle2,
                                             GTK_ANCHOR_CENTER,
                                             FALSE);

	  if (vector_tool->num_points == 2)
            {
              gdouble target;
              gdouble arc_radius;

              target     = FUNSCALEX (tool->gdisp, (TARGET >> 1));
              arc_radius = FUNSCALEX (tool->gdisp, ARC_RADIUS);

              gimp_draw_tool_draw_line
                (draw_tool,
                 vector_tool->x[0],
                 vector_tool->y[0],
                 (vector_tool->x[1] >= vector_tool->x[0] ?
                  vector_tool->x[0] + arc_radius + target :
                  vector_tool->x[0] - arc_radius - target),
                 vector_tool->y[0],
                 FALSE);
            }
	}
    }
}


static GimpToolOptions *
vector_tool_options_new (GimpToolInfo *tool_info)
{
  VectorOptions *options;
  GtkWidget      *vbox;

  options = g_new0 (VectorOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = vector_tool_options_reset;

  options->use_info_window = options->use_info_window_d  = FALSE;

    /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the use_info_window toggle button  */
  options->use_info_window_w =
    gtk_check_button_new_with_label (_("Use Info Window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_info_window_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->use_info_window_w);

  g_signal_connect (G_OBJECT (options->use_info_window_w), "toggled",
		    G_CALLBACK (gimp_toggle_button_update),
		    &options->use_info_window);

  return (GimpToolOptions *) options;
}

static void
vector_tool_options_reset (GimpToolOptions *tool_options)
{
  VectorOptions *options;

  options = (VectorOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_info_window_w),
				options->use_info_window_d);
}
