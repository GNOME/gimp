/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaseamlessclonetool.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h> /* gegl_operation_invalidate() */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h" /* playground */

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligmabuffer.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaclipboard.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmaseamlessclonetool.h"
#include "ligmaseamlesscloneoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


#define SC_DEBUG TRUE

#ifdef SC_DEBUG
#define sc_debug_fstart() g_debug ("%s::start", __FUNCTION__);
#define sc_debug_fend() g_debug ("%s::end", __FUNCTION__);
#else
#define sc_debug_fstart()
#define sc_debug_fend()
#endif

#define ligma_seamless_clone_tool_is_in_paste(sc,x0,y0)          \
  (   ((sc)->xoff <= (x0) && (x0) < (sc)->xoff + (sc)->width)   \
   && ((sc)->yoff <= (y0) && (y0) < (sc)->yoff + (sc)->height)) \

#define ligma_seamless_clone_tool_is_in_paste_c(sc,coords)       \
  ligma_seamless_clone_tool_is_in_paste((sc),(coords)->x,(coords)->y)


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


static void       ligma_seamless_clone_tool_control            (LigmaTool              *tool,
                                                               LigmaToolAction         action,
                                                               LigmaDisplay           *display);
static void       ligma_seamless_clone_tool_button_press       (LigmaTool              *tool,
                                                               const LigmaCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               LigmaButtonPressType    press_type,
                                                               LigmaDisplay           *display);

static void       ligma_seamless_clone_tool_button_release     (LigmaTool              *tool,
                                                               const LigmaCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               LigmaButtonReleaseType  release_type,
                                                               LigmaDisplay           *display);
static void       ligma_seamless_clone_tool_motion             (LigmaTool              *tool,
                                                               const LigmaCoords      *coords,
                                                               guint32                time,
                                                               GdkModifierType        state,
                                                               LigmaDisplay           *display);
static gboolean   ligma_seamless_clone_tool_key_press          (LigmaTool              *tool,
                                                               GdkEventKey           *kevent,
                                                               LigmaDisplay           *display);
static void       ligma_seamless_clone_tool_oper_update        (LigmaTool              *tool,
                                                               const LigmaCoords      *coords,
                                                               GdkModifierType        state,
                                                               gboolean               proximity,
                                                               LigmaDisplay           *display);
static void       ligma_seamless_clone_tool_cursor_update      (LigmaTool              *tool,
                                                               const LigmaCoords      *coords,
                                                               GdkModifierType        state,
                                                               LigmaDisplay           *display);
static void       ligma_seamless_clone_tool_options_notify     (LigmaTool              *tool,
                                                               LigmaToolOptions       *options,
                                                               const GParamSpec      *pspec);

static void       ligma_seamless_clone_tool_draw               (LigmaDrawTool          *draw_tool);

static void       ligma_seamless_clone_tool_start              (LigmaSeamlessCloneTool *sc,
                                                               LigmaDisplay           *display);

static void       ligma_seamless_clone_tool_stop               (LigmaSeamlessCloneTool *sc,
                                                               gboolean               display_change_only);

static void       ligma_seamless_clone_tool_commit             (LigmaSeamlessCloneTool *sc);

static void       ligma_seamless_clone_tool_create_render_node (LigmaSeamlessCloneTool *sc);
static gboolean   ligma_seamless_clone_tool_render_node_update (LigmaSeamlessCloneTool *sc);
static void       ligma_seamless_clone_tool_create_filter      (LigmaSeamlessCloneTool *sc,
                                                               LigmaDrawable          *drawable);
static void       ligma_seamless_clone_tool_filter_flush       (LigmaDrawableFilter     *filter,
                                                               LigmaTool              *tool);
static void       ligma_seamless_clone_tool_filter_update      (LigmaSeamlessCloneTool *sc);


G_DEFINE_TYPE (LigmaSeamlessCloneTool, ligma_seamless_clone_tool,
               LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_seamless_clone_tool_parent_class


void
ligma_seamless_clone_tool_register (LigmaToolRegisterCallback  callback,
                                   gpointer                  data)
{
  /* we should not know that "data" is a Ligma*, but what the heck this
   * is experimental playground stuff
   */
  if (LIGMA_GUI_CONFIG (LIGMA (data)->config)->playground_seamless_clone_tool)
    (* callback) (LIGMA_TYPE_SEAMLESS_CLONE_TOOL,
                  LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS,
                  ligma_seamless_clone_options_gui,
                  0,
                  "ligma-seamless-clone-tool",
                  _("Seamless Clone"),
                  _("Seamless Clone: Seamlessly paste one image into another"),
                  N_("_Seamless Clone"), NULL,
                  NULL, LIGMA_HELP_TOOL_SEAMLESS_CLONE,
                  LIGMA_ICON_TOOL_SEAMLESS_CLONE,
                  data);
}

static void
ligma_seamless_clone_tool_class_init (LigmaSeamlessCloneToolClass *klass)
{
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->control        = ligma_seamless_clone_tool_control;
  tool_class->button_press   = ligma_seamless_clone_tool_button_press;
  tool_class->button_release = ligma_seamless_clone_tool_button_release;
  tool_class->motion         = ligma_seamless_clone_tool_motion;
  tool_class->key_press      = ligma_seamless_clone_tool_key_press;
  tool_class->oper_update    = ligma_seamless_clone_tool_oper_update;
  tool_class->cursor_update  = ligma_seamless_clone_tool_cursor_update;
  tool_class->options_notify = ligma_seamless_clone_tool_options_notify;

  draw_tool_class->draw      = ligma_seamless_clone_tool_draw;
}

static void
ligma_seamless_clone_tool_init (LigmaSeamlessCloneTool *self)
{
  LigmaTool *tool = LIGMA_TOOL (self);

  ligma_tool_control_set_dirty_mask  (tool->control,
                                     LIGMA_DIRTY_IMAGE           |
                                     LIGMA_DIRTY_IMAGE_STRUCTURE |
                                     LIGMA_DIRTY_DRAWABLE        |
                                     LIGMA_DIRTY_SELECTION);

  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_MOVE);

  self->tool_state = SC_STATE_INIT;
}

static void
ligma_seamless_clone_tool_control (LigmaTool       *tool,
                                  LigmaToolAction  action,
                                  LigmaDisplay    *display)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      if (tool->display)
        ligma_seamless_clone_tool_stop (sc, FALSE);

      /* TODO: If we have any tool options that should be reset, here is
       *       a good place to do so.
       */
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_seamless_clone_tool_commit (sc);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

/**
 * ligma_seamless_clone_tool_start:
 * @sc: The LigmaSeamlessCloneTool to initialize for usage on the given
 *      display
 * @display: The display to initialize the tool for
 *
 * A utility function to initialize a tool for working on a given
 * display. At the beginning of each function, we can check if the event's
 * display is the same as the tool's one, and if not call this. This is
 * not required by the ligmatool interface or anything like that, but
 * this is a convenient way to do all the initialization work in one
 * place, and this is how the base class (LigmaDrawTool) does that
 */
static void
ligma_seamless_clone_tool_start (LigmaSeamlessCloneTool *sc,
                                LigmaDisplay           *display)
{
  LigmaTool     *tool      = LIGMA_TOOL (sc);
  LigmaImage    *image     = ligma_display_get_image (display);
  GList        *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  /* First handle the paste - we need to make sure we have one in order
   * to do anything else.
   */
  if (sc->paste == NULL)
    {
      LigmaBuffer *buffer = ligma_clipboard_get_buffer (tool->tool_info->ligma);

      if (! buffer)
        {
          ligma_tool_push_status (tool, display,
                                 "%s",
                                 _("There is no image data in the clipboard to paste."));
          return;
        }

      sc->paste = ligma_gegl_buffer_dup (ligma_buffer_get_buffer (buffer));
      g_object_unref (buffer);

      sc->width  = gegl_buffer_get_width  (sc->paste);
      sc->height = gegl_buffer_get_height (sc->paste);
    }

  /* Free resources which are relevant only for the previous display */
  ligma_seamless_clone_tool_stop (sc, TRUE);

  tool->display = display;

  ligma_seamless_clone_tool_create_filter (sc, drawable);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (sc), display);

  sc->tool_state = SC_STATE_RENDER_WAIT;
}


/**
 * ligma_seamless_clone_tool_stop:
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
ligma_seamless_clone_tool_stop (LigmaSeamlessCloneTool *sc,
                               gboolean               display_change_only)
{
  /* See if we actually have any reason to stop */
  if (sc->tool_state == SC_STATE_INIT)
    return;

  if (! display_change_only)
    {
      sc->tool_state = SC_STATE_INIT;

      g_clear_object (&sc->paste);
      g_clear_object (&sc->render_node);
      sc->sc_node = NULL;
    }

  /* This should always happen, even when we just switch a display */
  if (sc->filter)
    {
      ligma_drawable_filter_abort (sc->filter);
      g_clear_object (&sc->filter);

      if (LIGMA_TOOL (sc)->display)
        ligma_image_flush (ligma_display_get_image (LIGMA_TOOL (sc)->display));
    }

  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (sc));
}

static void
ligma_seamless_clone_tool_commit (LigmaSeamlessCloneTool *sc)
{
  LigmaTool *tool = LIGMA_TOOL (sc);

  if (sc->filter)
    {
      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_commit (sc->filter, LIGMA_PROGRESS (tool), FALSE);
      g_clear_object (&sc->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }
}

static void
ligma_seamless_clone_tool_button_press (LigmaTool            *tool,
                                       const LigmaCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       LigmaButtonPressType  press_type,
                                       LigmaDisplay         *display)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  if (display != tool->display)
    {
      ligma_seamless_clone_tool_start (sc, display);

      /* Center the paste on the mouse */
      sc->xoff = (gint) coords->x - sc->width / 2;
      sc->yoff = (gint) coords->y - sc->height / 2;
    }

  if (sc->tool_state == SC_STATE_RENDER_WAIT &&
      ligma_seamless_clone_tool_is_in_paste_c (sc, coords))
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (sc));

      /* Record previous location, in case the user cancels the
       * movement
       */
      sc->xoff_p = sc->xoff;
      sc->yoff_p = sc->yoff;

      /* Record the mouse location, so that the dragging offset can be
       * calculated
       */
      sc->xclick = coords->x;
      sc->yclick = coords->y;

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

      if (ligma_seamless_clone_tool_render_node_update (sc))
        {
          ligma_seamless_clone_tool_filter_update (sc);
        }

      sc->tool_state = SC_STATE_RENDER_MOTION;

      /* In order to receive motion events from the current click, we must
       * activate the tool control
       */
      ligma_tool_control_activate (tool->control);
    }
}

void
ligma_seamless_clone_tool_button_release (LigmaTool              *tool,
                                         const LigmaCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         LigmaButtonReleaseType  release_type,
                                         LigmaDisplay           *display)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  /* There is nothing to do, unless we were actually moving a paste */
  if (sc->tool_state == SC_STATE_RENDER_MOTION)
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (sc));

      if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        {
          sc->xoff = sc->xoff_p;
          sc->yoff = sc->yoff_p;
        }
      else
        {
          sc->xoff = sc->xoff_p + (gint) (coords->x - sc->xclick);
          sc->yoff = sc->yoff_p + (gint) (coords->y - sc->yclick);
        }

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

      if (ligma_seamless_clone_tool_render_node_update (sc))
        {
          ligma_seamless_clone_tool_filter_update (sc);
        }

      sc->tool_state = SC_STATE_RENDER_WAIT;
    }
}

static void
ligma_seamless_clone_tool_motion (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 LigmaDisplay      *display)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  if (sc->tool_state == SC_STATE_RENDER_MOTION)
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (sc));

      sc->xoff = sc->xoff_p + (gint) (coords->x - sc->xclick);
      sc->yoff = sc->yoff_p + (gint) (coords->y - sc->yclick);

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

      if (ligma_seamless_clone_tool_render_node_update (sc))
        {
          ligma_seamless_clone_tool_filter_update (sc);
        }
    }
}

static gboolean
ligma_seamless_clone_tool_key_press (LigmaTool    *tool,
                                    GdkEventKey *kevent,
                                    LigmaDisplay *display)
{
  LigmaSeamlessCloneTool *sct = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  if (sct->tool_state == SC_STATE_RENDER_MOTION ||
      sct->tool_state == SC_STATE_RENDER_WAIT)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          ligma_tool_control_set_preserve (tool->control, TRUE);

          /* TODO: there may be issues with committing the image map
           *       result after some changes were made and the preview
           *       was scrolled. We can fix these by either invalidating
           *       the area which is a union of the previous paste
           *       rectangle each time (in the update function) or by
           *       invalidating and re-rendering all now (expensive and
           *       perhaps useless */
          ligma_drawable_filter_commit (sct->filter, LIGMA_PROGRESS (tool), FALSE);
          g_clear_object (&sct->filter);

          ligma_tool_control_set_preserve (tool->control, FALSE);

          ligma_image_flush (ligma_display_get_image (display));

          ligma_seamless_clone_tool_control (tool, LIGMA_TOOL_ACTION_HALT,
                                            display);
          return TRUE;

        case GDK_KEY_Escape:
          ligma_seamless_clone_tool_control (tool, LIGMA_TOOL_ACTION_HALT,
                                            display);
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}

static void
ligma_seamless_clone_tool_oper_update (LigmaTool         *tool,
                                      const LigmaCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      LigmaDisplay      *display)
{
  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  /* TODO: Modify data here */

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

/* Mouse cursor policy:
 * - Always use the move cursor
 * - While dragging the paste, use a move modified
 * - Else, While hovering above it, display no modifier
 * - Else, display a "bad" modifier
 */
static void
ligma_seamless_clone_tool_cursor_update (LigmaTool         *tool,
                                        const LigmaCoords *coords,
                                        GdkModifierType   state,
                                        LigmaDisplay      *display)
{
  LigmaSeamlessCloneTool *sc       = LIGMA_SEAMLESS_CLONE_TOOL (tool);
  LigmaCursorModifier     modifier = LIGMA_CURSOR_MODIFIER_BAD;

  /* Only update if the tool is actually active on some display */
  if (tool->display)
    {
      if (sc->tool_state == SC_STATE_RENDER_MOTION)
        {
          modifier = LIGMA_CURSOR_MODIFIER_MOVE;
        }
      else if (sc->tool_state == SC_STATE_RENDER_WAIT &&
               ligma_seamless_clone_tool_is_in_paste_c (sc, coords))
        {
          modifier = LIGMA_CURSOR_MODIFIER_NONE;
        }

      ligma_tool_control_set_cursor_modifier (tool->control, modifier);
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_seamless_clone_tool_options_notify (LigmaTool         *tool,
                                         LigmaToolOptions  *options,
                                         const GParamSpec *pspec)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (tool);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if (! strcmp (pspec->name, "max-refine-scale"))
    {
      if (ligma_seamless_clone_tool_render_node_update (sc))
        {
          ligma_seamless_clone_tool_filter_update (sc);
        }
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_seamless_clone_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaSeamlessCloneTool *sc = LIGMA_SEAMLESS_CLONE_TOOL (draw_tool);

  if (sc->tool_state == SC_STATE_RENDER_WAIT ||
      sc->tool_state == SC_STATE_RENDER_MOTION)
    {
      ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                    sc->xoff, sc->yoff, sc->width, sc->height);
    }
}

/**
 * ligma_seamless_clone_tool_create_render_node:
 * @sc: The LigmaSeamlessCloneTool to initialize
 *
 * This function creates a Gegl node graph of the composition which is
 * needed to render the drawable. The graph should have an "input" pad
 * which will receive the drawable on which the preview is applied, and
 * it should also have an "output" pad to which the final result will be
 * rendered
 */
static void
ligma_seamless_clone_tool_create_render_node (LigmaSeamlessCloneTool *sc)
{
  /* Here is a textual description of the graph we are going to create:
   *
   * <input>  <- drawable
   * +--+--------------------------+
   * |  |output                    |
   * |  |                          |
   * |  | <buffer-source> <- paste |
   * |  |       |output            |
   * |  |       |                  |
   * |  |input  |aux               |
   * |<seamless-paste-render>      |
   * |    |output                  |
   * |    |                        |
   * |    |input                   |
   * +----+------------------------+
   *   <output>
   */
  LigmaSeamlessCloneOptions *options = LIGMA_SEAMLESS_CLONE_TOOL_GET_OPTIONS (sc);
  GeglNode *node;
  GeglNode *op, *paste, *overlay;
  GeglNode *input, *output;

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  paste = gegl_node_new_child (node,
                               "operation", "gegl:buffer-source",
                               "buffer",    sc->paste,
                               NULL);

  op = gegl_node_new_child (node,
                            "operation",         "gegl:seamless-clone",
                            "max-refine-scale",  options->max_refine_scale,
                            NULL);

  overlay = gegl_node_new_child (node,
                                 "operation", "svg:dst-over",
                                 NULL);

  gegl_node_connect_to (input,   "output",
                        op,      "input");

  gegl_node_connect_to (paste,   "output",
                        op,      "aux");

  gegl_node_connect_to (op,      "output",
                        overlay, "input");

  gegl_node_connect_to (input,   "output",
                        overlay, "aux");

  gegl_node_connect_to (overlay, "output",
                        output,  "input");

  sc->render_node = node;
  sc->sc_node     = op;
}

/* ligma_seamless_clone_tool_render_node_update:
 * sc: the Seamless Clone tool whose render has to be updated.
 *
 * Returns: TRUE if any property changed.
 */
static gboolean
ligma_seamless_clone_tool_render_node_update (LigmaSeamlessCloneTool *sc)
{
  static gint rendered__max_refine_scale = -1;
  static gint rendered_xoff              = G_MAXINT;
  static gint rendered_yoff              = G_MAXINT;

  LigmaSeamlessCloneOptions *options = LIGMA_SEAMLESS_CLONE_TOOL_GET_OPTIONS (sc);
  LigmaDrawable *bg = LIGMA_TOOL (sc)->drawables->data;
  gint          off_x, off_y;

  /* All properties stay the same. No need to update. */
  if (rendered__max_refine_scale == options->max_refine_scale &&
      rendered_xoff == sc->xoff                               &&
      rendered_yoff == sc->yoff)
    return FALSE;

  ligma_item_get_offset (LIGMA_ITEM (bg), &off_x, &off_y);

  gegl_node_set (sc->sc_node,
                 "xoff", (gint) sc->xoff - off_x,
                 "yoff", (gint) sc->yoff - off_y,
                 "max-refine-scale", (gint) options->max_refine_scale,
                 NULL);

  rendered__max_refine_scale = options->max_refine_scale;
  rendered_xoff              = sc->xoff;
  rendered_yoff              = sc->yoff;

  return TRUE;
}

static void
ligma_seamless_clone_tool_create_filter (LigmaSeamlessCloneTool *sc,
                                        LigmaDrawable          *drawable)
{
  if (! sc->render_node)
    ligma_seamless_clone_tool_create_render_node (sc);

  sc->filter = ligma_drawable_filter_new (drawable,
                                         _("Seamless Clone"),
                                         sc->render_node,
                                         LIGMA_ICON_TOOL_SEAMLESS_CLONE);

  ligma_drawable_filter_set_region (sc->filter, LIGMA_FILTER_REGION_DRAWABLE);

  g_signal_connect (sc->filter, "flush",
                    G_CALLBACK (ligma_seamless_clone_tool_filter_flush),
                    sc);
}

static void
ligma_seamless_clone_tool_filter_flush (LigmaDrawableFilter *filter,
                                       LigmaTool           *tool)
{
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_seamless_clone_tool_filter_update (LigmaSeamlessCloneTool *sc)
{
  LigmaTool         *tool  = LIGMA_TOOL (sc);
  LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);
  LigmaItem         *item  = LIGMA_ITEM (tool->drawables->data);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;
  GeglOperation    *op = NULL;

  LigmaProgress     *progress;
  GeglNode         *output;
  GeglProcessor    *processor;
  gdouble           value;

  progress = ligma_progress_start (LIGMA_PROGRESS (sc), FALSE,
                                  _("Cloning the foreground object"));

  /* Find out at which x,y is the top left corner of the currently
   * displayed part */
  ligma_display_shell_untransform_viewport (shell, ! shell->show_all,
                                           &x, &y, &w, &h);

  /* Find out where is our drawable positioned */
  ligma_item_get_offset (item, &off_x, &off_y);

  /* Create a rectangle from the intersection of the currently displayed
   * part with the drawable */
  ligma_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            ligma_item_get_width  (item),
                            ligma_item_get_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  /* Since the filter_apply function receives a rectangle describing
   * where it should update the preview, and since that rectangle should
   * be relative to the drawable's location, we now offset back by the
   * drawable's offsetts. */
  visible.x -= off_x;
  visible.y -= off_y;

  g_object_get (sc->sc_node, "gegl-operation", &op, NULL);
  /* If any cache of the visible area was present, clear it!
   * We need to clear the cache in the sc_node, since that is
   * where the previous paste was located
   */
  gegl_operation_invalidate (op, &visible, TRUE);
  g_object_unref (op);

  /* Now update the image map and show this area */
  ligma_drawable_filter_apply (sc->filter, NULL);

  /* Show update progress. */
  output = gegl_node_get_output_proxy (sc->render_node, "output");
  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        ligma_progress_set_value (progress, value);
    }

  if (progress)
    ligma_progress_end (progress);

  g_object_unref (processor);
}
