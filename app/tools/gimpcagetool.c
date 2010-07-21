/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcagetool.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"
#include "tools/tools-enums.h"
#include "gimptoolcontrol.h"

#include "core/gimp.h"
#include "core/gimpprogress.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "display/gimpdisplay.h"
#include "core/gimp-transform-utils.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-operation.h"

#include "widgets/gimphelp-ids.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"

#include "gimp-intl.h"

/*static gboolean     gimp_cage_tool_initialize     (GimpTool    *tool,
                                                   GimpDisplay *display,
                                                   GError     **error);*/
static void         gimp_cage_tool_finalize       (GObject *object);
static void         gimp_cage_tool_start          (GimpCageTool *ct,
                                                   GimpDisplay        *display);
static void         gimp_cage_tool_halt           (GimpCageTool *ct);
static void         gimp_cage_tool_button_press   (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type,
                                                   GimpDisplay           *display);
static void         gimp_cage_tool_button_release (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static gboolean     gimp_cage_tool_key_press      (GimpTool    *tool,
                                                   GdkEventKey *kevent,
                                                   GimpDisplay *display);
static void         gimp_cage_tool_motion         (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   guint32           time,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);
static void         gimp_cage_tool_control        (GimpTool       *tool,
                                                   GimpToolAction  action,
                                                   GimpDisplay    *display);
static void         gimp_cage_tool_cursor_update  (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);
static void         gimp_cage_tool_oper_update    (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   GdkModifierType   state,
                                                   gboolean          proximity,
                                                   GimpDisplay      *display);
static void         gimp_cage_tool_draw           (GimpDrawTool *draw_tool);
static void         gimp_cage_tool_switch_to_deform 
                                                  (GimpCageTool *ct);
static void         gimp_cage_tool_remove_last_handle 
                                                  (GimpCageTool *ct);
static void         gimp_cage_tool_process        (GimpCageTool *ct,
                                                   GimpDisplay  *image);

G_DEFINE_TYPE (GimpCageTool, gimp_cage_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_cage_tool_parent_class

#define HANDLE_SIZE             14

void
gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_CAGE_TOOL, //Tool type
                GIMP_TYPE_CAGE_OPTIONS, //Tool options type
                gimp_cage_options_gui, //Tool opions gui
                0, //context_props
                "gimp-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>R",
                NULL, GIMP_HELP_TOOL_CAGE,
                GIMP_STOCK_TOOL_CAGE,
                data);
}

static void
gimp_cage_tool_class_init (GimpCageToolClass *klass)
{
  GObjectClass           *object_class         = G_OBJECT_CLASS (klass);
  GimpToolClass          *tool_class           = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass      *draw_tool_class      = GIMP_DRAW_TOOL_CLASS (klass);
  
  object_class->finalize          = gimp_cage_tool_finalize;
  
  /*tool_class->initialize          = gimp_cage_tool_initialize;*/
  tool_class->button_press        = gimp_cage_tool_button_press;
  tool_class->button_release      = gimp_cage_tool_button_release;
  tool_class->key_press           = gimp_cage_tool_key_press;
  tool_class->motion              = gimp_cage_tool_motion;
  tool_class->control             = gimp_cage_tool_control;
  tool_class->cursor_update       = gimp_cage_tool_cursor_update;
  tool_class->oper_update         = gimp_cage_tool_oper_update;
  
  draw_tool_class->draw           = gimp_cage_tool_draw;
}

static void
gimp_cage_tool_init (GimpCageTool *self)
{
  self->cage = g_object_new (GIMP_TYPE_CAGE, NULL);
  self->cursor_position.x = 0;
  self->cursor_position.y = 0;
  self->handle_moved = -1;
  self->cage_complete = FALSE;
}

/*static gboolean
gimp_cage_tool_initialize (GimpTool    *tool,
                           GimpDisplay *display,
                           GError     **error)
{
  GimpCageTool *cage_tool = GIMP_CAGE_TOOL (tool);

  return GIMP_TOOL_CLASS (parent_class)->initialize(tool, display, error);
}*/

static void
gimp_cage_tool_finalize (GObject *object)
{
  GimpCageTool        *ct  = GIMP_CAGE_TOOL (object);

  g_object_unref (ct->cage);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static void
gimp_cage_tool_start (GimpCageTool       *ct,
                      GimpDisplay        *display)
{
  GimpTool                  *tool      = GIMP_TOOL (ct);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_tool_control_activate (tool->control);

  tool->display = display;

  gimp_draw_tool_start (draw_tool, display);
}

static void
gimp_cage_tool_halt (GimpCageTool *ct)
{
  GimpTool                  *tool      = GIMP_TOOL (ct);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (ct);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  tool->display              = NULL;
  
  g_object_unref (ct->cage);
  ct->cage = g_object_new (GIMP_TYPE_CAGE, NULL);
  ct->cursor_position.x = 0;
  ct->cursor_position.y = 0;
  ct->handle_moved = -1;
  ct->cage_complete = FALSE;
}

static void 
gimp_cage_tool_button_press (GimpTool              *tool,
                             const GimpCoords      *coords,
                             guint32                time,
                             GdkModifierType        state,
                             GimpButtonPressType    press_type,
                             GimpDisplay           *display)
{
  GimpCageTool      *ct       = GIMP_CAGE_TOOL (tool);
  GimpCageOptions   *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCage          *cage     = ct->cage;

  g_return_if_fail (GIMP_IS_CAGE_TOOL (ct));
  g_return_if_fail (GIMP_IS_CAGE (cage));
  
  if (display != tool->display)
  {
    gimp_cage_tool_start  (ct,
                           display);
  }
   
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));
  
  
  if (ct->handle_moved < 0)
  {
    if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    {
      ct->handle_moved = gimp_cage_is_on_handle (cage,
                                                 coords->x,
                                                 coords->y,
                                                 HANDLE_SIZE);
    }
    else
    {
      ct->handle_moved = gimp_cage_is_on_handle_d (cage,
                                                   coords->x,
                                                   coords->y,
                                                   HANDLE_SIZE);
    }
  }
  
  if (ct->handle_moved < 0)
  {
    gimp_cage_add_cage_point (cage,
                              coords->x,
                              coords->y);
  }
  
  // user is clicking on the first handle, we close the cage and switch to deform mode
  if (ct->handle_moved == 0)
  {    
    ct->cage_complete = TRUE;
    gimp_cage_tool_switch_to_deform (ct);
    
    gimp_cage_tool_process (ct, display);
  }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
  
}

void 
gimp_cage_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{                                         
  GimpCageTool        *ct         = GIMP_CAGE_TOOL (tool);
  
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));
  ct->handle_moved = -1;
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
}


static gboolean
gimp_cage_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpCageTool *ct    = GIMP_CAGE_TOOL (tool);
  
  switch (kevent->keyval)
    {
    case GDK_BackSpace:
      gimp_cage_tool_remove_last_handle (ct);
      return TRUE;

    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      gimp_cage_tool_switch_to_deform (ct);
      return TRUE;

    case GDK_Escape:
      gimp_cage_tool_halt (ct);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_cage_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpCageTool        *ct         = GIMP_CAGE_TOOL (tool);
  GimpDrawTool        *draw_tool  = GIMP_DRAW_TOOL (tool);
  GimpCageOptions     *options    = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCage            *cage       = ct->cage;
  

  gimp_draw_tool_pause (draw_tool);

  
  if (ct->handle_moved >= 0)
  {
    if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    {
      gimp_cage_move_cage_point  (cage,
                                  ct->handle_moved,
                                  coords->x,
                                  coords->y);
    }
    else
    {
      gimp_cage_move_cage_point_d  (cage,
                                    ct->handle_moved,
                                    coords->x,
                                    coords->y);
    }
  }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_cage_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_cage_tool_halt (GIMP_CAGE_TOOL (tool));
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_cage_tool_cursor_update  (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpCageTool    *ct       = GIMP_CAGE_TOOL (tool);
  GimpCageOptions *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);

  if (tool->display == NULL)
  {
    GIMP_TOOL_CLASS (parent_class)->cursor_update (tool,
                                                   coords,
                                                   state,
                                                   display);
  }
  else
  {
    GimpCursorModifier modifier;

    if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    {
      modifier = GIMP_CURSOR_MODIFIER_ANCHOR;
    }
    else
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

    gimp_tool_set_cursor (tool, display,
                          gimp_tool_control_get_cursor (tool->control),
                          gimp_tool_control_get_tool_cursor (tool->control),
                          modifier);
  }
}

static void
gimp_cage_tool_oper_update  (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpCageTool *ct          = GIMP_CAGE_TOOL (tool);
  GimpDrawTool *draw_tool   = GIMP_DRAW_TOOL (tool);
  
  gimp_draw_tool_pause (draw_tool);
  
  ct->cursor_position.x = coords->x;
  ct->cursor_position.y = coords->y;
  
  gimp_draw_tool_resume (draw_tool);
}

/**
 * gimp_cage_tool_draw:
 * @draw_tool: 
 * 
 * Draw the tool on the canvas. 
 */
static void
gimp_cage_tool_draw (GimpDrawTool *draw_tool)
{
  GimpCageTool        *ct       = GIMP_CAGE_TOOL (draw_tool);
  GimpCageOptions     *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCage            *cage     = ct->cage;
  
  gint                 i = 0;
  gint                 n = 0;
  gint                 on_handle = -1;
  GimpVector2         *vertices;
  gint               (*is_on_handle)     (GimpCage    *cage,
                                          gdouble      x,
                                          gdouble      y,
                                          gint         handle_size);
  

  if (cage->cage_vertice_number <= 0)
  {
    return;
  }
  
  if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
  {
    is_on_handle = gimp_cage_is_on_handle;
    vertices = cage->cage_vertices;
  }
  else
  {
    is_on_handle = gimp_cage_is_on_handle_d;
    vertices = cage->cage_vertices_d;
  }
  
  
  gimp_draw_tool_draw_lines (draw_tool,
                             vertices,
                             cage->cage_vertice_number,
                             FALSE, FALSE);
  
  if (ct->cage_complete)
  {
    gimp_draw_tool_draw_line (draw_tool,
                              vertices[cage->cage_vertice_number - 1].x,
                              vertices[cage->cage_vertice_number - 1].y,
                              vertices[0].x,
                              vertices[0].y,
                              FALSE);
  }
  else
  {
    gimp_draw_tool_draw_line (draw_tool,
                              vertices[cage->cage_vertice_number - 1].x,
                              vertices[cage->cage_vertice_number - 1].y,
                              ct->cursor_position.x,
                              ct->cursor_position.y,
                              FALSE);
  }
  
  n = cage->cage_vertice_number;
  
  on_handle = is_on_handle (cage,
                            ct->cursor_position.x,
                            ct->cursor_position.y,
                            HANDLE_SIZE);
  
  for(i = 0; i < n; i++)
  {
    GimpVector2 point = vertices[i];
    
    GimpHandleType handle = GIMP_HANDLE_CIRCLE;
    
    if (i == on_handle)
    {
      handle = GIMP_HANDLE_FILLED_CIRCLE;
    }
    
    gimp_draw_tool_draw_handle (draw_tool, handle,
                                point.x,
                                point.y,
                                HANDLE_SIZE, HANDLE_SIZE,
                                GTK_ANCHOR_CENTER, FALSE);
  }
}

static void
gimp_cage_tool_switch_to_deform (GimpCageTool *ct)
{
  GimpCageOptions *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  
  g_object_set (options, "cage-mode", GIMP_CAGE_MODE_DEFORM, NULL);
}

static void
gimp_cage_tool_remove_last_handle (GimpCageTool *ct)
{
  GimpCage           *cage    = ct->cage;
  
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));
  gimp_cage_remove_last_cage_point (cage);
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
}

static void
gimp_cage_tool_process (GimpCageTool *ct,
                        GimpDisplay  *display)
{  
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  GimpProgress *progress = gimp_progress_start (GIMP_PROGRESS (display),
                                                _("Blending"),
                                                FALSE);


  if (GIMP_IS_LAYER (drawable))
  {
    GeglNode *node;

    node = g_object_new (GEGL_TYPE_NODE,
                         "operation", "gegl:cage",
                         NULL);

    gimp_drawable_apply_operation (drawable, progress, _("Cage transform"),
                                   node, TRUE);
    g_object_unref (node);
  }
}
