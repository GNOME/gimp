/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpForegroundSelectTool
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gegl/gimp-gegl-mask.h"

#include "core/gimp.h"
#include "core/gimpdrawable-foreground-extract.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"
#include "core/gimpscanconvert.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpforegroundselecttool.h"
#include "gimpforegroundselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


typedef struct
{
  gint         width;
  gboolean     background;
  gint         num_points;
  GimpVector2 *points;
} FgSelectStroke;


static void   gimp_foreground_select_tool_constructed    (GObject          *object);
static void   gimp_foreground_select_tool_finalize       (GObject          *object);

static void   gimp_foreground_select_tool_control        (GimpTool         *tool,
                                                          GimpToolAction    action,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_oper_update    (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          GdkModifierType   state,
                                                          gboolean          proximity,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_modifier_key   (GimpTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_cursor_update  (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static gboolean  gimp_foreground_select_tool_key_press   (GimpTool         *tool,
                                                          GdkEventKey      *kevent,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_button_press   (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpButtonPressType press_type,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_button_release (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpButtonReleaseType release_type,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_motion         (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);

static void   gimp_foreground_select_tool_draw           (GimpDrawTool     *draw_tool);

static void   gimp_foreground_select_tool_select         (GimpFreeSelectTool *free_sel,
                                                          GimpDisplay        *display);

static void   gimp_foreground_select_tool_set_trimap     (GimpForegroundSelectTool *fg_select,
                                                          GimpDisplay              *display);
static void   gimp_foreground_select_tool_set_preview    (GimpForegroundSelectTool *fg_select,
                                                          GimpDisplay              *display);
static void   gimp_foreground_select_tool_drop_masks     (GimpForegroundSelectTool *fg_select,
                                                          GimpDisplay              *display);

static void   gimp_foreground_select_tool_apply          (GimpForegroundSelectTool *fg_select,
                                                          GimpDisplay              *display);
static void   gimp_foreground_select_tool_preview        (GimpForegroundSelectTool *fg_select,
                                                          GimpDisplay              *display);

static void   gimp_foreground_select_options_notify      (GimpForegroundSelectOptions *options,
                                                          GParamSpec                  *pspec,
                                                          GimpForegroundSelectTool    *fg_select);

static void   gimp_foreground_select_tool_stroke_paint   (GimpForegroundSelectTool    *fg_select,
                                                          GimpDisplay                 *display,
                                                          GimpForegroundSelectOptions *options);


G_DEFINE_TYPE (GimpForegroundSelectTool, gimp_foreground_select_tool,
               GIMP_TYPE_FREE_SELECT_TOOL)

#define parent_class gimp_foreground_select_tool_parent_class


void
gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_FOREGROUND_SELECT_TOOL,
                GIMP_TYPE_FOREGROUND_SELECT_OPTIONS,
                gimp_foreground_select_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK | GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-foreground-select-tool",
                _("Foreground Select"),
                _("Foreground Select Tool: Select a region containing foreground objects"),
                N_("F_oreground Select"), NULL,
                NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                GIMP_STOCK_TOOL_FOREGROUND_SELECT,
                data);
}

static void
gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass)
{
  GObjectClass            *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass           *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass       *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpFreeSelectToolClass *free_select_tool_class;

  free_select_tool_class = GIMP_FREE_SELECT_TOOL_CLASS (klass);

  object_class->constructed      = gimp_foreground_select_tool_constructed;
  object_class->finalize         = gimp_foreground_select_tool_finalize;

  tool_class->control            = gimp_foreground_select_tool_control;
  tool_class->oper_update        = gimp_foreground_select_tool_oper_update;
  tool_class->modifier_key       = gimp_foreground_select_tool_modifier_key;
  tool_class->cursor_update      = gimp_foreground_select_tool_cursor_update;
  tool_class->key_press          = gimp_foreground_select_tool_key_press;
  tool_class->button_press       = gimp_foreground_select_tool_button_press;
  tool_class->button_release     = gimp_foreground_select_tool_button_release;
  tool_class->motion             = gimp_foreground_select_tool_motion;

  draw_tool_class->draw          = gimp_foreground_select_tool_draw;

  free_select_tool_class->select = gimp_foreground_select_tool_select;
}

static void
gimp_foreground_select_tool_init (GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  gimp_tool_control_set_scroll_lock (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_CENTER);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  gimp_tool_control_set_action_value_2 (tool->control,
                                        "tools/tools-foreground-select-brush-size-set");

  fg_select->stroke  = NULL;
  fg_select->mask    = NULL;
  fg_select->trimap  = NULL;
  fg_select->state   = MATTING_STATE_FREE_SELECT;
}

static void
gimp_foreground_select_tool_constructed (GObject *object)
{
  GimpToolOptions *options = GIMP_TOOL_GET_OPTIONS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (options, "notify",
                           G_CALLBACK (gimp_foreground_select_options_notify),
                           object, 0);
}

static void
gimp_foreground_select_tool_finalize (GObject *object)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (object);

  if (fg_select->stroke)
    g_warning ("%s: stroke should be NULL at this point", G_STRLOC);

  if (fg_select->mask)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  if (fg_select->trimap)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_foreground_select_tool_control (GimpTool       *tool,
                                     GimpToolAction  action,
                                     GimpDisplay    *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      {
        gimp_foreground_select_tool_drop_masks (fg_select, display);
        tool->display = NULL;
      }
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_foreground_select_tool_oper_update (GimpTool         *tool,
                                         const GimpCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         GimpDisplay      *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  const gchar              *status    = NULL;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP && display == tool->display)
    {
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

      gimp_draw_tool_pause (draw_tool);

      fg_select->last_coords = *coords;

      gimp_draw_tool_resume (draw_tool);
      status = _("Paint trimap, (background, foreground and unknown) or press Enter to preview");
    }
  else if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      if (GIMP_SELECTION_TOOL (tool)->function == SELECTION_SELECT)
        status = _("Roughly outline the object to extract");
    }
  else
    {
      status = _("Press escape for return to trimap or Enter to apply");
    }

  if (proximity && status)
    gimp_tool_replace_status (tool, display, "%s", status);
}

static void
gimp_foreground_select_tool_modifier_key (GimpTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
#if 0
  if (key == gimp_get_toggle_behavior_mask ())
    {
      GimpForegroundSelectOptions *options;

      options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

      g_object_set (options,
                    "background", ! options->background,
                    NULL);
    }
#endif
}

static void
gimp_foreground_select_tool_cursor_update (GimpTool         *tool,
                                           const GimpCoords *coords,
                                           GdkModifierType   state,
                                           GimpDisplay      *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      switch (GIMP_SELECTION_TOOL (tool)->function)
        {
        case SELECTION_MOVE_MASK:
        case SELECTION_MOVE:
        case SELECTION_MOVE_COPY:
        case SELECTION_ANCHOR:
          return;
        default:
          break;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_foreground_select_tool_key_press (GimpTool    *tool,
                                       GdkEventKey *kevent,
                                       GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (display != tool->display)
    return FALSE;

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_foreground_select_tool_preview (fg_select, display);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
          return TRUE;

        default:
          return FALSE;
        }
    }
  else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_foreground_select_tool_apply (fg_select, display);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_foreground_select_tool_set_trimap (fg_select, display);
          return TRUE;

        default:
          return FALSE;
        }
    }
  else
    {
      return GIMP_TOOL_CLASS (parent_class)->key_press (tool,
                                                        kevent,
                                                        display);
    }
}

static void
gimp_foreground_select_tool_button_press (GimpTool            *tool,
                                          const GimpCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          GimpButtonPressType  press_type,
                                          GimpDisplay         *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) && draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      gimp_tool_control_activate (tool->control);

      fg_select->last_coords = *coords;

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (GimpVector2));

      g_array_append_val (fg_select->stroke, point);

      if (!gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
  else if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
}

static void
gimp_foreground_select_tool_button_release (GimpTool              *tool,
                                            const GimpCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            GimpButtonReleaseType  release_type,
                                            GimpDisplay           *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      GimpForegroundSelectOptions *options;

      options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_tool_control_halt (tool->control);

      gimp_foreground_select_tool_stroke_paint (fg_select, display, options);

      gimp_foreground_select_tool_set_trimap (fg_select, display);

      gimp_free_select_tool_select (GIMP_FREE_SELECT_TOOL (tool), display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool,
                                                      coords, time, state,
                                                      release_type,
                                                      display);
    }
}

static void
gimp_foreground_select_tool_motion (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    GimpDisplay      *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      GimpVector2 *last = &g_array_index (fg_select->stroke,
                                          GimpVector2,
                                          fg_select->stroke->len - 1);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      fg_select->last_coords = *coords;

      if (last->x != (gint) coords->x || last->y != (gint) coords->y)
        {
          GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

          g_array_append_val (fg_select->stroke, point);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool,
                                              coords, time, state, display);
    }
}

static void
gimp_foreground_select_tool_get_area (GeglBuffer *mask,
                                      gint       *x1,
                                      gint       *y1,
                                      gint       *x2,
                                      gint       *y2)
{
  gint width;
  gint height;

  gimp_gegl_mask_bounds (mask, x1, y1, x2, y2);

  width  = *x2 - *x1;
  height = *y2 - *y1;

  *x1 = MAX (*x1 - width  / 2, 0);
  *y1 = MAX (*y1 - height / 2, 0);
  *x2 = MIN (*x2 + width  / 2, gimp_item_get_width  (GIMP_ITEM (mask)));
  *y2 = MIN (*y2 + height / 2, gimp_item_get_height (GIMP_ITEM (mask)));
}

static void
gimp_foreground_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpForegroundSelectTool    *fg_select = GIMP_FOREGROUND_SELECT_TOOL (draw_tool);
  GimpTool                    *tool      = GIMP_TOOL (draw_tool);
  GimpForegroundSelectOptions *options;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->stroke)
    {
      GimpDisplayShell  *shell = gimp_display_get_shell (draw_tool->display);

      gimp_draw_tool_add_pen (draw_tool,
                              (const GimpVector2 *) fg_select->stroke->data,
                              fg_select->stroke->len,
                              GIMP_CONTEXT (options),
                              GIMP_ACTIVE_COLOR_FOREGROUND,
                              options->stroke_width * shell->scale_y);
    }

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      gint    x = fg_select->last_coords.x;
      gint    y = fg_select->last_coords.y;
      gdouble radius;

      radius = options->stroke_width / 2;

      /*  warn if the user is drawing outside of the working area  */
      if (FALSE)
        {
          gint x1, y1;
          gint x2, y2;

          gimp_foreground_select_tool_get_area (fg_select->mask,
                                                &x1, &y1, &x2, &y2);

          if (x < x1 + radius || x > x2 - radius ||
              y < y1 + radius || y > y2 - radius)
            {
              gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                            x1, y1,
                                            x2 - x1, y2 - y1);
            }
        }

      gimp_draw_tool_add_arc (draw_tool, FALSE,
                              x - radius, y - radius,
                              2 * radius, 2 * radius,
                              0.0, 2.0 * G_PI);
    }
  else if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
    }
}

static void
gimp_foreground_select_tool_select (GimpFreeSelectTool *free_sel,
                                    GimpDisplay        *display)
{
  GimpForegroundSelectTool *fg_select;
  GimpImage                *image = gimp_display_get_image (display);
  GimpDrawable             *drawable;
  GimpScanConvert          *scan_convert;
  const GimpVector2        *points;
  gint                      n_points;

  drawable  = gimp_image_get_active_drawable (image);
  fg_select = GIMP_FOREGROUND_SELECT_TOOL (free_sel);

  if (! drawable)
    return;

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      scan_convert = gimp_scan_convert_new ();

      gimp_free_select_tool_get_points (free_sel,
                                        &points,
                                        &n_points);

      gimp_scan_convert_add_polyline (scan_convert,
                                      n_points,
                                      points,
                                      TRUE);

      fg_select->trimap = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                           gimp_image_get_width  (image),
                                                           gimp_image_get_height (image)),
                                           gimp_image_get_mask_format (image));

      gimp_scan_convert_render_value (scan_convert, fg_select->trimap,
                                      0, 0, 1.0);
      gimp_scan_convert_free (scan_convert);

      gimp_foreground_select_tool_set_trimap (fg_select, display);
    }
}

static void
gimp_foreground_select_tool_set_trimap (GimpForegroundSelectTool *fg_select,
                                        GimpDisplay              *display)
{
  GimpTool                    *tool = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;
  GimpRGB                      color;

  g_return_if_fail (fg_select->trimap != NULL);

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  gimp_foreground_select_options_get_mask_color (options, &color);
  gimp_display_shell_set_mask (gimp_display_get_shell (display),
                               fg_select->trimap, &color);

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_tool_control_set_toggled (tool->control, FALSE);

  fg_select->state = MATTING_STATE_PAINT_TRIMAP;
}

static void
gimp_foreground_select_tool_set_preview (GimpForegroundSelectTool *fg_select,
                                         GimpDisplay              *display)
{

  GimpTool                    *tool = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;
  GimpRGB                      color;

  g_return_if_fail (fg_select->mask != NULL);

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  gimp_foreground_select_options_get_mask_color (options, &color);
  gimp_rgb_set_alpha (&color, 1.0);
  gimp_display_shell_set_mask (gimp_display_get_shell (display),
                               fg_select->mask, &color);

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_tool_control_set_toggled (tool->control, FALSE);

  fg_select->state = MATTING_STATE_PREVIEW_MASK;
}

static void
gimp_foreground_select_tool_drop_masks (GimpForegroundSelectTool *fg_select,
                                        GimpDisplay              *display)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  if (fg_select->trimap)
    {
      g_object_unref (fg_select->trimap);
      fg_select->trimap = NULL;
    }

  if (fg_select->mask)
    {
      g_object_unref (fg_select->mask);
      fg_select->mask = NULL;
    }

  if (GIMP_IS_DISPLAY (display))
    {
      gimp_display_shell_set_mask (gimp_display_get_shell (display),
                                   NULL, NULL);
    }

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_FREE_SELECT);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_FREE_SELECT);

  gimp_tool_control_set_toggled (tool->control, FALSE);
  fg_select->state = MATTING_STATE_FREE_SELECT;
}

static void
gimp_foreground_select_tool_preview (GimpForegroundSelectTool *fg_select,
                                     GimpDisplay              *display)
{
  GimpTool                    *tool     = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options  = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);
  GimpImage                   *image    = gimp_display_get_image (display);
  GimpDrawable                *drawable = gimp_image_get_active_drawable (image);
  GeglBuffer                  *trimap_buffer;
  GeglBuffer                  *drawable_buffer;
  GeglNode                    *gegl;
  GeglNode                    *matting_node;
  GeglNode                    *input_image;
  GeglNode                    *input_trimap;
  GeglNode                    *output_mask;
  GeglBuffer                  *buffer;
  GimpProgress                *progress;
  GeglProcessor               *processor;
  gdouble                     value;

  if (fg_select->mask)
    {
      g_object_unref (fg_select->mask);
      fg_select->mask = NULL;
    }

  progress = gimp_progress_start (GIMP_PROGRESS (fg_select),
                                  _("Computing alpha of unknown pixels"),
                                  FALSE);

  trimap_buffer   = fg_select->trimap;
  drawable_buffer = gimp_drawable_get_buffer (drawable);

  gegl = gegl_node_new ();

  input_trimap = gegl_node_new_child (gegl,
                                      "operation", "gegl:buffer-source",
                                      "buffer",    trimap_buffer,
                                      NULL);
  input_image = gegl_node_new_child (gegl,
                                     "operation", "gegl:buffer-source",
                                     "buffer",    drawable_buffer,
                                     NULL);
  output_mask = gegl_node_new_child (gegl,
                                     "operation", "gegl:buffer-sink",
                                     "buffer",    &buffer,
                                     "format",    NULL,
                                     NULL);

  if (options->engine == GIMP_MATTING_ENGINE_GLOBAL)
    {
      matting_node = gegl_node_new_child (gegl,
                                          "operation",  "gegl:matting-global",
                                          "iterations", options->iterations,
                                          NULL);
    }
  else
    {
      matting_node = gegl_node_new_child (gegl,
                                          "operation",     "gegl:matting-levin",
                                          "levels",        options->levels,
                                          "active_levels", options->active_levels,
                                          NULL);
    }

  gegl_node_connect_to (input_image,  "output",
                        matting_node, "input");
  gegl_node_connect_to (input_trimap, "output",
                        matting_node, "aux");
  gegl_node_connect_to (matting_node, "output",
                        output_mask,  "input");

  processor = gegl_node_new_processor (output_mask, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        gimp_progress_set_value (progress, value);
    }

  if (progress)
    gimp_progress_end (progress);

  g_object_unref (processor);

  fg_select->mask = buffer;

  gimp_foreground_select_tool_set_preview (fg_select, display);

  g_object_unref (gegl);
}

static void
gimp_foreground_select_tool_apply (GimpForegroundSelectTool *fg_select,
                                   GimpDisplay              *display)
{
  GimpTool      *tool    = GIMP_TOOL (fg_select);
  GimpImage     *image   = gimp_display_get_image (display);
  GimpLayer     *layer   = gimp_image_get_active_layer (image);
  GimpLayerMask *layer_mask;
  GimpRGB        color   = { 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE };

  g_return_if_fail (fg_select->mask != NULL);

  layer_mask = gimp_layer_mask_new_from_buffer (fg_select->mask, image,
                                                "mask", &color);

  gimp_layer_add_mask (layer, layer_mask, TRUE, NULL);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  gimp_image_flush (image);
}

static void
gimp_foreground_select_tool_stroke_paint (GimpForegroundSelectTool    *fg_select,
                                          GimpDisplay                 *display,
                                          GimpForegroundSelectOptions *options)
{
  GimpScanConvert *scan_convert;
  gint             width;

  g_return_if_fail (fg_select->stroke != NULL);

  scan_convert = gimp_scan_convert_new ();

  if (fg_select->stroke->len == 1)
    {
      GimpVector2 points[2];

      points[0] = points[1] = ((GimpVector2 *) fg_select->stroke->data)[0];

      points[1].x += 0.01;
      points[1].y += 0.01;

      gimp_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
    }
  else
    {
      gimp_scan_convert_add_polyline (scan_convert,
                                      fg_select->stroke->len,
                                      (GimpVector2 *) fg_select->stroke->data,
                                      FALSE);
    }

  width = ROUND ((gdouble) options->stroke_width);

  gimp_scan_convert_stroke (scan_convert,
                            width,
                            GIMP_JOIN_ROUND, GIMP_CAP_ROUND, 10.0,
                            0.0, NULL);

  gimp_scan_convert_compose_value (scan_convert, fg_select->trimap,
                                   0, 0,
                                   gimp_foreground_select_options_get_opacity (options));

  gimp_scan_convert_free (scan_convert);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;

}

static void
gimp_foreground_select_options_notify (GimpForegroundSelectOptions *options,
                                       GParamSpec                  *pspec,
                                       GimpForegroundSelectTool    *fg_select)
{
  if (g_str_has_prefix (pspec->name, "mask-color"))
    {
      GimpTool *tool = GIMP_TOOL (fg_select);

      if (tool->display)
        {
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            {
              gimp_foreground_select_tool_set_trimap (fg_select, tool->display);
            }

          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            {
              gimp_foreground_select_tool_set_preview (fg_select, tool->display);
            }
        }
    }
}
