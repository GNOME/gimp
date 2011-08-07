/* GIMP - The GNU Image Manipulation Program
 *
 * gimpseamlessclonetool.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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
#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpseamlessclonetool.h"
#include "gimpseamlesscloneoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

#define SC_DEBUG TRUE

#ifdef SC_DEBUG
#define sc_debug_fstart() g_debug ("%s::start", __FUNCTION__);
#define sc_debug_fend() g_debug ("%s::end", __FUNCTION__);
#else
#define sc_debug_fstart()
#define sc_debug_fend()
#endif


/*  init ----------> preprocess
 *    |                  |
 *    |                  |
 *    |                  |
 *    |                  v
 *    |                render(wait, motion)
 *    |               /  |
 *    |         _____/   |
 *    |   _____/         |
 *    v  v               v
 *  quit  <----------  commit
 *
 * Begin at INIT state
 *
 * INIT:       Wait for click on canvas
 *             have a paste ? -> PREPROCESS : -> QUIT
 * 
 * PREPROCESS: Do the preprocessing
 *             -> RENDER
 *
 * RENDER:     Interact and wait for quit signal
 *             commit quit ? -> COMMIT : -> QUIT
 *
 * COMMIT:     Commit the changes
 *             -> QUIT
 *
 * QUIT:       Invoked by sending a ACTION_HALT to the tool_control
 *             Free resources
 */
enum
{
  SC_STATE_INIT,
  SC_STATE_PREPROCESS,
  SC_STATE_RENDER_WAIT,
  SC_STATE_RENDER_MOTION,
  SC_STATE_COMMIT,
  SC_STATE_QUIT
};


static void       gimp_seamless_clone_tool_start              (GimpSeamlessCloneTool *sc,
                                                               GimpDisplay           *display);

static void       gimp_seamless_clone_tool_stop               (GimpSeamlessCloneTool *sc,
                                                               gboolean               display_change_only);

static void       gimp_seamless_clone_tool_options_notify     (GimpTool              *tool,
                                                               GimpToolOptions       *options,
                                                               const GParamSpec      *pspec);
                                                              
static void       gimp_seamless_clone_tool_button_press       (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpButtonPressType    press_type,
                                                               GimpDisplay           *display);
                                                               
static void       gimp_seamless_clone_tool_button_release     (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpButtonReleaseType  release_type,
                                                               GimpDisplay           *display);
                                                               
static gboolean   gimp_seamless_clone_tool_key_press          (GimpTool              *tool,
                                                               GdkEventKey           *kevent,
                                                               GimpDisplay           *display);
                                                               
static void       gimp_seamless_clone_tool_motion             (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               GimpDisplay           *display);
                                                               
static void       gimp_seamless_clone_tool_control            (GimpTool              *tool,
                                                               GimpToolAction         action,
                                                               GimpDisplay           *display);
                                                               
static void       gimp_seamless_clone_tool_cursor_update      (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               GdkModifierType        state,
                                                               GimpDisplay           *display);
                                                               
static void       gimp_seamless_clone_tool_oper_update        (GimpTool              *tool,
                                                               const GimpCoords      *coords,
                                                               GdkModifierType        state,
                                                               gboolean               proximity,
                                                               GimpDisplay           *display);

static void       gimp_seamless_clone_tool_draw               (GimpDrawTool          *draw_tool);

static void       gimp_seamless_clone_tool_create_image_map   (GimpSeamlessCloneTool *sc,
                                                               GimpDrawable          *drawable);
                                                               
static void       gimp_seamless_clone_tool_image_map_flush    (GimpImageMap          *image_map,
                                                               GimpTool              *tool);
                                                               
static void       gimp_seamless_clone_tool_image_map_update   (GimpSeamlessCloneTool *sc);

static void       gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool *sc);

static void       gimp_seamless_clone_tool_render_node_update (GimpSeamlessCloneTool *sc);


G_DEFINE_TYPE (GimpSeamlessCloneTool, gimp_seamless_clone_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_seamless_clone_tool_parent_class


void
gimp_seamless_clone_tool_register (GimpToolRegisterCallback  callback,
                                   gpointer                  data)
{
  sc_debug_fstart ();
  
  (* callback) (GIMP_TYPE_SEAMLESS_CLONE_TOOL,
                GIMP_TYPE_SEAMLESS_CLONE_OPTIONS,
                gimp_seamless_clone_options_gui,
                0,
                "gimp-seamless-clone-tool",
                _("Seamless Clone"),
                _("Seamless Clone: Seamlessly paste one image into another"),
                N_("_Seamless Clone"), "<shift>L",
                NULL, GIMP_HELP_TOOL_SEAMLESS_CLONE,
                GIMP_STOCK_TOOL_MOVE,
                data);

  sc_debug_fend ();
}

static void
gimp_seamless_clone_tool_class_init (GimpSeamlessCloneToolClass *klass)
{
  sc_debug_fstart ();

  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->options_notify = gimp_seamless_clone_tool_options_notify;
  tool_class->button_press   = gimp_seamless_clone_tool_button_press;
  tool_class->button_release = gimp_seamless_clone_tool_button_release;
  tool_class->key_press      = gimp_seamless_clone_tool_key_press;
  tool_class->motion         = gimp_seamless_clone_tool_motion;
  tool_class->control        = gimp_seamless_clone_tool_control;
  tool_class->cursor_update  = gimp_seamless_clone_tool_cursor_update;
  tool_class->oper_update    = gimp_seamless_clone_tool_oper_update;

  draw_tool_class->draw      = gimp_seamless_clone_tool_draw;

  sc_debug_fend ();
}

/**
 * gimp_seamless_clone_tool_init:
 * @self: The instance of the tool to initialize
 *
 * Initialize all the pointers to NULL, initialize the state machine,
 * moust cursor and the tool control object. This function will be
 * called on the first time the tool is being switched to.
 */
static void
gimp_seamless_clone_tool_init (GimpSeamlessCloneTool *self)
{
  sc_debug_fstart ();

  GimpTool *tool = GIMP_TOOL (self);

  /* TODO: If we want our tool to be invalidated by something, enable
   * the following line. Note that the enum of the dirty properties is
   * GimpDirtyMask which is located under app/core/core-enums.h */

//  gimp_tool_control_set_dirty_mask  (tool->control,
//                                     GIMP_DIRTY_IMAGE           |
//                                     GIMP_DIRTY_IMAGE_STRUCTURE |
//                                     GIMP_DIRTY_DRAWABLE        |
//                                     GIMP_DIRTY_SELECTION);

  /* TODO: We do need click events, but so do all tools. What's this? */
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_MOVE);

  /* In order to achieve as much instant reponse as possible, only care
   * about the location of the mouse at present (while it's being moved)
   * and ignore motion events that happened while we were dealing with
   * previous motion events */
  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);

  self->paste           = NULL;
  
  self->tricache        = NULL;
  self->mesh_cache      = NULL;
  
  self->render_node     = NULL;
  self->translate_node  = NULL;

  self->tool_state      = SC_STATE_INIT;
  self->image_map       = NULL;

  sc_debug_fend ();
}

/* This function is called when the tool should pause and resume it's
 * action, and when it should halt (i.e. stop and free all resources).
 * We are interested in this mainly because we want to free resources
 * when the tool terminates, and nothing else...
 */
static void
gimp_seamless_clone_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display)
{
  sc_debug_fstart ();

  GimpSeamlessCloneTool *sc = GIMP_SEAMLESS_CLONE_TOOL (tool);

#if SC_DEBUG
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      g_debug ("sc_tool_control:: GIMP_TOOL_ACTION_PAUSE");
      break;
      
    case GIMP_TOOL_ACTION_RESUME:
      g_debug ("sc_tool_control:: GIMP_TOOL_ACTION_RESUME");
      break;

    case GIMP_TOOL_ACTION_HALT:
      g_debug ("sc_tool_control:: GIMP_TOOL_ACTION_HALT");
      break;
    }
#endif

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      /* We only have what to stop if we are active on some display */
      if (tool->display != NULL)
        gimp_seamless_clone_tool_stop (sc, FALSE);

      /* Now, mark that we have no display, so any needed initialization
       * will happen on the next time a display is picked */
      tool->display = NULL;

      /* TODO: If we have any tool options that should be reset, here is
       *       a good place to do so.
       */
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);

  sc_debug_fend ();
}

/**
 * gimp_seamless_clone_tool_start:
 * @sc: The GimpSeamlessCloneTool to initialize for usage on the given
 *      display
 * @display: The display to initialize the tool for
 *
 * A utility function to initialize a tool for working on a given
 * display. At the begining of each function, we can check if the event's
 * display is the same as the tool's one, and if not call this. This is
 * not required by the gimptool interface or anything like that, but
 * this is a convinient way to do all the initialization work in one
 * place, and this is how the base class (GimpDrawTool) does that
 */
static void
gimp_seamless_clone_tool_start (GimpSeamlessCloneTool *sc,
                                GimpDisplay           *display)
{
  sc_debug_fstart ();

  GimpTool     *tool     = GIMP_TOOL (sc);
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  gint          off_x;
  gint          off_y;

  /* First handle the paste - we need to make sure we have one in order
   * to do anything else. */
  if (sc->paste == NULL)
    {
      /* TODO: Call to preprocessing should be done here, along with a
       * call to cache the paste. If there is no paste, prompt nicely
       * for a message requesting the user to actually copy something to
       * the clipboard */
    }
  
  /* Free resources which are relevant only for the previous display */
  gimp_seamless_clone_tool_stop (sc, TRUE);

  /* Set the new tool display */
  tool->display = display;

  sc->image_map = gimp_seamless_clone_tool_create_image_map (sc, drawable);

  /* Now call the start method of the draw tool */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (sc), display);

  sc_debug_fend ();
}


/**
 * gimp_seamless_clone_tool_stop:
 * @sc: The seamless clone tool whose resources should be freed
 * @display_change_only: Mark that the only reason for this call was a
 *                       switch of the working display.
 *
 * This function frees any resources associated with the seamless clone
 * tool, including caches, gegl graphs, and anything the tool created.
 * Afterwards, it initializes all the relevant pointers to some initial
 * value (usually NULL) like the init function does.
 *
 * Note that for seamless cloning, no change needs to be done when
 * switching to a different display, except for clearing the image map.
 * So for that, we provide a boolean parameter to specify that the only
 * change was one of the display
 */
static void
gimp_seamless_clone_tool_stop (GimpSeamlessCloneTool *sc,
                               gboolean               display_change_only)
{
  sc_debug_fstart ();

  if (! display_change_only)
    {
      sc->render_node     = NULL;
      sc->translate_node  = NULL;

      sc->tool_state      = SC_STATE_INIT;
      sc->image_map       = NULL;

      if (sc->paste)
        {
          gegl_buffer_destroy (sc->paste);
          sc->paste = NULL;
        }

      if (sc->tricache)
        {
          gegl_buffer_destroy (sc->tricache);
          sc->tricache = NULL;
        }

      if (sc->render_node)
        {
          /* When the parent render_node is unreffed completly, it will
           * also free all of it's child nodes */
          g_object_unref (sc->render_node);
          sc->render_node = NULL;
          sc->translate_node  = NULL;
        }
        
      /* TODO: free the mesh_cache object */
      if (sc->mesh_cache)
        {
        }
    }

  /* This should always happen, even when we just switch a display */
  if (sc->image_map)
    {
      /* This line makes sure the tool won't be aborted when the
       * active drawable is being switched.
       * Now, the question is why? We are currently handling an
       * abort signal, so unless somehow we can have two of these in
       * parallel, I don't see any reason to be afraid. Not that it
       * does any harm, but still.
       * TODO: check this?! */
//      gimp_tool_control_set_preserve (tool->control, TRUE);

      /* Stop the computation of the live preview, and mark it not
       * to be committed. Then free the image map object. */
      gimp_image_map_abort (sc->image_map);
      g_object_unref (sc->image_map);
      sc->image_map = NULL;

      /* Revert the previous set_preserve */
//      gimp_tool_control_set_preserve (tool->control, FALSE);

      if (GIMP_TOOL (sc)->display)
        gimp_image_flush (gimp_display_get_image (GIMP_TOOL (sc)->display));
    }

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (sc));

  sc_debug_fend ();

}

static void
gimp_seamless_clone_tool_options_notify (GimpTool         *tool,
                                         GimpToolOptions  *options,
                                         const GParamSpec *pspec)
{
  sc_debug_fstart ();

  GimpSeamlessCloneTool *sc = GIMP_SEAMLESS_CLONE_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  /* Pause the tool before modifying the tool data, so that drawing
   * won't be done using data in intermidiate states */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* TODO: Modify data here */

  /* Done modifying data? If so, now restore the tool drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  sc_debug_fend ();
}

static gboolean
gimp_seamless_clone_tool_key_press (GimpTool    *tool,
                                    GdkEventKey *kevent,
                                    GimpDisplay *display)
{
  sc_debug_fstart ();

  /* TODO: After checking the key code, return TRUE if the event was
   * handled, or FALSE if not */

  return FALSE;

  sc_debug_fend ();
}

static void
gimp_seamless_clone_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  sc_debug_fstart ();

  GimpSeamlessCloneTool    *sc       = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpSeamlessCloneOptions *options  = GIMP_SEAMLESS_CLONE_TOOL_GET_OPTIONS (sc);

  /* Pause the tool before modifying the tool data, so that drawing
   * won't be done using data in intermidiate states */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* There is nothing to do, unless we were actually moving a paste */
  if (sc->tool_state == SC_STATE_RENDER_MOTION)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (sc));

      sc->xoff = sc->xoff_p + (gint) (coords->x - sc->xclick);
      sc->yoff = sc->yoff_p + (gint) (coords->y - sc->yclick);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  /* Done modifying data? If so, now restore the tool drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  sc_debug_fend ();
}

static void
gimp_seamless_clone_tool_oper_update (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      GimpDisplay      *display)
{
  sc_debug_fstart ();

  /* Pause the tool before modifying the tool data, so that drawing
   * won't be done using data in intermidiate states */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* TODO: Modify data here */

  /* Done modifying data? If so, now restore the tool drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  sc_debug_fend ();

}

static void
gimp_seamless_clone_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display)
{
  sc_debug_fstart ();

  GimpSeamlessCloneTool *sc = GIMP_SEAMLESS_CLONE_TOOL (tool);
  
  if (display != tool->display)
    gimp_seamless_clone_tool_start (sc, display);

  if (sc->tool_state == SC_STATE_RENDER_WAIT
      && gimp_seamless_clone_tool_is_in_paste_c (sc, coords))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (sc));

      /* Record previous location, in case the user cancel's the
       * movement */
      sc->xoff_p = sc->xoff;
      sc->yoff_p = sc->yoff;

      /* Record the mouse location, so that the dragging offset can be
       * calculated */
      sc->xclick = coords->x;
      sc->yclick = coords->y;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  sc_debug_fend ();
}

void
gimp_seamless_clone_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  sc_debug_fstart ();

  GimpSeamlessCloneTool    *sc      = GIMP_SEAMLESS_CLONE_TOOL (tool);

  /* There is nothing to do, unless we were actually moving a paste */
  if (sc->tool_state == SC_STATE_RENDER_MOTION)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (sc));

      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          sc->xoff = sc->xoff_p;
          sc->yoff = sc->yoff_p;
        }
      else
        {
          sc->xoff = sc->xoff_p + (gint) (coords->x - sc->xclick);
          sc->yoff = sc->yoff_p + (gint) (coords->y - sc->yclick);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  sc_debug_fend ();

}

/* Mouse cursor policy:
 * - Always use the move cursor
 * - While dragging the paste, use a move modified
 * - Else, While hovering above it, display no modifier
 * - Else, display a "bad" modifier
 */
static void
gimp_seamless_clone_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display)
{
  sc_debug_fstart ();
  GimpSeamlessCloneTool *sc       = GIMP_SEAMLESS_CLONE_TOOL (tool);
  GimpCursorModifier     modifier = GIMP_CURSOR_MODIFIER_BAD;

  /* Only update if the tool is actually active on some display */
  if (tool->display)
    {
      if (sc->tool_state == SC_STATE_RENDER_MOTION)
        modifier = GIMP_CURSOR_MODIFIER_MOVE;
        
      else if (sc->tool_state == SC_STATE_RENDER_WAIT
               && gimp_seamless_clone_tool_is_in_paste_c (sc, coords))
        modified = GIMP_CURSOR_MODIFIER_NONE;

      gimp_tool_control_set_cursor_modifier (tool->control, modifier);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);

  sc_debug_fend ();

}

/* Draw any required on canvas interaction. For now, this is kept empty.
 * It's a place holder for the future of this tool, for some more
 * advanced features that may be added later */
static void
gimp_seamless_clone_tool_draw (GimpDrawTool *draw_tool)
{
  sc_debug_fstart ();

  sc_debug_fend ();

}

/* This function creates a Gegl node graph of the composition which is
 * needed to render the drawable. The graph should have an "input" pad
 * which will receive the drawable on which the preview is applied, and
 * it should also have an "output" pad to which the final result will be
 * rendered */
static void
gimp_seamless_clone_tool_create_render_node (GimpSeamlessCloneTool *sc)
{
  sc_debug_fstart ();

  sc_debug_fend ();

}

static void
gimp_seamless_clone_tool_render_node_update (GimpSeamlessCloneTool *sc)
{
  sc_debug_fstart ();

  sc_debug_fend ();

}

/* Create an image map to render the operation of the current tool with
 * a preview on the given drawable */
static void
gimp_seamless_clone_tool_create_image_map (GimpSeamlessCloneTool *sc,
                                           GimpDrawable          *drawable)
{
  sc_debug_fstart ();

  /* FIrst, make sure we actually have the GEGL graph needed to render
   * the operation's result */
  if (!sc->render_node)
    gimp_seamless_clone_tool_create_render_node (sc);

  sc->image_map = gimp_image_map_new (drawable,
                                      _("Seamless Clone"),
                                      sc->render_node,
                                      NULL,
                                      NULL);

  g_signal_connect (sc->image_map, "flush",
                    G_CALLBACK (gimp_seamless_clone_tool_image_map_flush),
                    sc);

  sc_debug_fend ();

}

/* Flush (Refresh) the image map preview */
static void
gimp_seamless_clone_tool_image_map_flush (GimpImageMap *image_map,
                                          GimpTool     *tool)
{
  sc_debug_fstart ();

  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (tool->display);

  sc_debug_fend ();

}

/**
 * gimp_seamless_clone_tool_image_map_update:
 * @sc: A GimpSeamlessCloneTool to update
 *
 * This functions updates the image map preview displayed by a given
 * GimpSeamlessCloneTool. The image_map must be initialized prior to the
 * call to this function!
 */
static void
gimp_seamless_clone_tool_image_map_update (GimpSeamlessCloneTool *sc)
{
  sc_debug_fstart ();

  GimpTool         *tool  = GIMP_TOOL (sc);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GimpItem         *item  = GIMP_ITEM (tool->drawable);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;

  /* Find out at which x,y is the top left corner of the currently
   * displayed part */
  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  /* Find out where is our drawable positioned */
  gimp_item_get_offset (item, &off_x, &off_y);

  /* Create a rectangle from the intersection of the currently displayed
   * part with the drawable */
  gimp_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  /* Since the image_map_apply function receives a rectangle describing
   * where it should update the preview, and since that rectangle should
   * be relative to the drawable's location, we now offset back by the
   * drawable's offsetts. */
  visible.x -= off_x;
  visible.y -= off_y;

  /* Now update the image map and show this area */
  gimp_image_map_apply (sc->image_map, &visible);

  sc_debug_fend ();

}
