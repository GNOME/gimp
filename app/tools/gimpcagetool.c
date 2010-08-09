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
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimagemap.h"
#include "base/tile-manager.h"

#include "widgets/gimphelp-ids.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"

#include "gimp-intl.h"

#include <stdio.h>

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
static void         gimp_cage_tool_compute_coef   (GimpCageTool *ct,
                                                   GimpDisplay  *display);
static void         gimp_cage_tool_process        (GimpCageTool *ct,
                                                   GimpDisplay  *display);
static void         gimp_cage_tool_prepare_preview (GimpCageTool *ct,
                                                   GimpDisplay  *display);

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
  self->config = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  self->cursor_position.x = 0;
  self->cursor_position.y = 0;
  self->handle_moved = -1;
  self->cage_complete = FALSE;
  
  self->coef = NULL;
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

  g_object_unref (ct->config);
  
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
  
  //gimp_cage_tool_prepare_preview (ct, display);

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
  
  g_object_unref (ct->config);
  ct->config = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  ct->cursor_position.x = 0;
  ct->cursor_position.y = 0;
  ct->handle_moved = -1;
  ct->cage_complete = FALSE;
  
  gegl_buffer_destroy (ct->coef);
  ct->coef = NULL;
  
  //g_object_unref (ct->image_map);
  //ct->image_map = NULL;
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
  GimpCageConfig    *config   = ct->config;

  g_return_if_fail (GIMP_IS_CAGE_TOOL (ct));
  g_return_if_fail (GIMP_IS_CAGE_CONFIG (config));
  
  if (display != tool->display)
  {
    gimp_cage_tool_start  (ct,
                           display);
  }
   
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));
  
  
  if (ct->handle_moved < 0)
  {
    ct->handle_moved = gimp_cage_config_is_on_handle  (config,
                                                       options->cage_mode,
                                                       coords->x,
                                                       coords->y,
                                                       HANDLE_SIZE);
  }
  
  if (ct->handle_moved < 0 && !ct->cage_complete)
  {
    gimp_cage_config_add_cage_point (config,
                                    coords->x,
                                    coords->y);
  }
  
  // user is clicking on the first handle, we close the cage and switch to deform mode
  if (ct->handle_moved == 0 && config->cage_vertice_number > 2)
  {    
    ct->cage_complete = TRUE;
    gimp_cage_tool_switch_to_deform (ct);

    gimp_cage_config_reverse_cage_if_needed (config);
    gimp_cage_tool_compute_coef (ct, display);
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
      if (ct->cage_complete)
      gimp_cage_tool_process (ct, display); /*RUN IT BABY*/
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
  GimpCageConfig      *config     = ct->config;
  

  gimp_draw_tool_pause (draw_tool);

  
  if (ct->handle_moved >= 0)
  {
    gimp_cage_config_move_cage_point  (config,
                                      options->cage_mode,
                                      ct->handle_moved,
                                      coords->x,
                                      coords->y);
  }

/*  {
    GeglRectangle rect = {0, 0, 300, 300};
    gimp_image_map_apply (ct->image_map, &rect);
  }*/
  
  
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
  GimpCageConfig      *config   = ct->config;
  
  gint                 i = 0;
  gint                 on_handle = -1;
  GimpVector2         *vertices;
  

  if (config->cage_vertice_number <= 0)
  {
    return;
  }
  
  if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
  {
    vertices = config->cage_vertices;
  }
  else
  {
    vertices = config->cage_vertices_d;
  }
  
  
  gimp_draw_tool_draw_lines (draw_tool,
                             vertices,
                             config->cage_vertice_number,
                             FALSE, FALSE);
  
  if (ct->cage_complete)
  {
    gimp_draw_tool_draw_line (draw_tool,
                              vertices[config->cage_vertice_number - 1].x,
                              vertices[config->cage_vertice_number - 1].y,
                              vertices[0].x,
                              vertices[0].y,
                              FALSE);
  }
  else
  {
    gimp_draw_tool_draw_line (draw_tool,
                              vertices[config->cage_vertice_number - 1].x,
                              vertices[config->cage_vertice_number - 1].y,
                              ct->cursor_position.x,
                              ct->cursor_position.y,
                              FALSE);
  }
  
  on_handle = gimp_cage_config_is_on_handle (config,
                                            options->cage_mode,
                                            ct->cursor_position.x,
                                            ct->cursor_position.y,
                                            HANDLE_SIZE);
  
  for(i = 0; i < config->cage_vertice_number; i++)
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
  GimpCageConfig      *config   = ct->config;
  
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));
  gimp_cage_config_remove_last_cage_point (config);
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
}

static void
gimp_cage_tool_compute_coef (GimpCageTool *ct,
                             GimpDisplay  *display)
{
  GimpCageConfig    *config   = ct->config;
  
  Babl *format;
  GeglRectangle rect;
  GeglNode *gegl, *input, *output;
  GeglProcessor *processor;
  GimpProgress *progress;
  GeglBuffer *buffer;
  gdouble value;
  
  printf("compute the coef\n");
  if (ct->coef)
  {
    gegl_buffer_destroy (ct->coef);
    ct->coef = NULL;
  }
  
  format = babl_format_n(babl_type("float"), config->cage_vertice_number * 2);
  rect = gimp_cage_config_get_bounding_box (config);
  
  gegl = gegl_node_new ();
  
  input = gegl_node_new_child (gegl,
                                "operation", "gegl:cage_coef_calc",
                                "config", ct->config,
                                NULL);
  
  output = gegl_node_new_child (gegl,
                              "operation", "gegl:buffer-sink",
                              "buffer", &buffer,
                              "format", format,
                              NULL);
  
  gegl_node_connect_to (input, "output",
                          output, "input");
  
  gegl_node_process (output);
  /*progress = gimp_progress_start (GIMP_PROGRESS (display),
                                _("Coefficient computation"),
                                FALSE);

  processor = gegl_node_new_processor (output, &rect);
  
  while (gegl_processor_work (processor, &value))
  {
      gimp_progress_set_value (progress, value);
  }

  gimp_progress_end (progress);
  gegl_processor_destroy (processor);*/

  ct->coef = buffer;
  g_object_unref (gegl);
}

static void
gimp_cage_tool_process (GimpCageTool *ct,
                        GimpDisplay  *display)
{
  TileManager  *new_tiles;
  GeglRectangle rect;
  
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  GimpProgress *progress = gimp_progress_start (GIMP_PROGRESS (display),
                                                _("Blending"),
                                                FALSE);
  g_return_if_fail (ct->coef);

  printf("process the cage\n");
  
  if (GIMP_IS_LAYER (drawable))
  {
    GeglNode *gegl = gegl_node_new ();
    GeglNode *coef, *cage, *render, *input, *output;
    
    input  = gegl_node_new_child (gegl,
                                  "operation",    "gimp:tilemanager-source",
                                  "tile-manager", gimp_drawable_get_tiles (drawable),
                                  "linear",       TRUE,
                                  NULL);

    cage = gegl_node_new_child (gegl,
                                "operation", "gegl:cage_transform",
                                "config", ct->config,
                                NULL);
    
    coef = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-source",
                                "buffer",    ct->coef,
                                NULL);

    render = gegl_node_new_child (gegl,
                                  "operation", "gegl:render_mapping",
                                  NULL);

    new_tiles = gimp_drawable_get_shadow_tiles (drawable);
    output = gegl_node_new_child (gegl,
                                  "operation",    "gimp:tilemanager-sink",
                                  "tile-manager", new_tiles,
                                  "linear",       TRUE,
                                  NULL);

/*    render = gegl_node_new_child (gegl,
                                  "operation", "gegl:debugit",
                                  NULL);*/

    gegl_node_connect_to (input, "output",
                          cage, "input");

    gegl_node_connect_to (input, "output",
                          render, "input");

    gegl_node_connect_to (coef, "output",
                          cage, "aux");

    gegl_node_connect_to (cage, "output",
                          render, "aux");

    gegl_node_connect_to (render, "output",
                          output, "input");

/*    gimp_drawable_apply_operation (drawable, progress, _("Cage transform"),
                                   render, TRUE);*/
    

    gegl_node_process (output);
    
    rect.x      = 0;
    rect.y      = 0;
    rect.width  = tile_manager_width  (new_tiles);
    rect.height = tile_manager_height (new_tiles);
  
    gimp_drawable_merge_shadow_tiles (drawable, TRUE, _("Cage transform"));
    gimp_drawable_free_shadow_tiles (drawable);

    gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);
    
    g_object_unref (gegl);
    
    if (progress)
        gimp_progress_end (progress);
        
    gimp_image_flush (image);
    
    gimp_cage_tool_halt (ct);
  }
}


static void
gimp_cage_tool_prepare_preview (GimpCageTool *ct,
                                GimpDisplay  *display)
{
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  GimpImageMap *image_map;
  
  GeglNode *gegl = gegl_node_new ();
  GeglNode *coef, *cage;
  
  coef = gegl_node_new_child (gegl,
                              "operation", "gegl:buffer-source",
                              "buffer",    ct->coef,
                              NULL);

  cage = gegl_node_new_child (gegl,
                                "operation", "gegl:cage",
                                "config", ct->config,
                                NULL);

  gegl_node_connect_to (coef, "output",
                        cage, "aux");

  image_map = gimp_image_map_new (drawable,
                                  _("Cage transform"),
                                  cage,
                                  NULL,
                                  NULL);

  g_object_unref (gegl);
}
