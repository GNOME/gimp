/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_DISPLAY_SHELL_H__
#define __LIGMA_DISPLAY_SHELL_H__


/* Apply to a float the same rounding mode used in the renderer */
#define  PROJ_ROUND(coord)   ((gint) RINT (coord))
#define  PROJ_ROUND64(coord) ((gint64) RINT (coord))

/* scale values */
#define  SCALEX(s,x)      PROJ_ROUND ((x) * (s)->scale_x)
#define  SCALEY(s,y)      PROJ_ROUND ((y) * (s)->scale_y)

/* unscale values */
#define  UNSCALEX(s,x)    ((gint) ((x) / (s)->scale_x))
#define  UNSCALEY(s,y)    ((gint) ((y) / (s)->scale_y))
/* (and float-returning versions) */
#define  FUNSCALEX(s,x)   ((x) / (s)->scale_x)
#define  FUNSCALEY(s,y)   ((y) / (s)->scale_y)


#define LIGMA_TYPE_DISPLAY_SHELL            (ligma_display_shell_get_type ())
#define LIGMA_DISPLAY_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY_SHELL, LigmaDisplayShell))
#define LIGMA_DISPLAY_SHELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY_SHELL, LigmaDisplayShellClass))
#define LIGMA_IS_DISPLAY_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY_SHELL))
#define LIGMA_IS_DISPLAY_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY_SHELL))
#define LIGMA_DISPLAY_SHELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DISPLAY_SHELL, LigmaDisplayShellClass))


typedef struct _LigmaDisplayShellClass  LigmaDisplayShellClass;

struct _LigmaDisplayShell
{
  GtkEventBox        parent_instance;

  LigmaDisplay       *display;

  LigmaUIManager     *popup_manager;
  GdkMonitor        *initial_monitor;

  LigmaDisplayOptions *options;
  LigmaDisplayOptions *fullscreen_options;
  LigmaDisplayOptions *no_image_options;

  LigmaUnit           unit;

  gint               offset_x;         /*  offset of display image            */
  gint               offset_y;

  gdouble            scale_x;          /*  horizontal scale factor            */
  gdouble            scale_y;          /*  vertical scale factor              */

  gboolean           flip_horizontally;
  gboolean           flip_vertically;
  gdouble            rotate_angle;
  cairo_matrix_t    *rotate_transform;
  cairo_matrix_t    *rotate_untransform;

  gdouble            monitor_xres;
  gdouble            monitor_yres;
  gboolean           dot_for_dot;      /*  ignore monitor resolution          */

  LigmaZoomModel     *zoom;

  gdouble            last_scale;       /*  scale used when reverting zoom     */
  guint              last_scale_time;  /*  time when last_scale was set       */
  gint               last_offset_x;    /*  offsets used when reverting zoom   */
  gint               last_offset_y;

  gdouble            other_scale;      /*  scale factor entered in Zoom->Other*/

  gint               disp_width;       /*  width of drawing area              */
  gint               disp_height;      /*  height of drawing area             */

  gboolean           proximity;        /*  is a device in proximity           */

  gboolean           show_image;       /*  whether to show the image          */

  gboolean           show_all;         /*  show the entire image              */

  Selection         *selection;        /*  Selection (marching ants)          */
  gint64             selection_update; /*  Last selection index update        */

  GList             *children;

  GtkWidget         *canvas;           /*  LigmaCanvas widget                  */
  GtkGesture        *zoom_gesture;     /*  Zoom gesture handler for the canvas*/
  GtkGesture        *rotate_gesture;   /*  Rotation gesture handler           */

  GtkAdjustment     *hsbdata;          /*  adjustments                        */
  GtkAdjustment     *vsbdata;
  GtkWidget         *hsb;              /*  scroll bars                        */
  GtkWidget         *vsb;

  GtkWidget         *hrule;            /*  rulers                             */
  GtkWidget         *vrule;

  GtkWidget         *origin;           /*  NW: origin                         */
  GtkWidget         *quick_mask_button;/*  SW: quick mask button              */
  GtkWidget         *zoom_button;      /*  NE: zoom toggle button             */
  GtkWidget         *nav_ebox;         /*  SE: navigation event box           */

  GtkWidget         *statusbar;        /*  statusbar                          */

  LigmaCanvasItem    *canvas_item;      /*  items drawn on the canvas          */
  LigmaCanvasItem    *unrotated_item;   /*  unrotated items for e.g. cursor    */
  LigmaCanvasItem    *passe_partout;    /*  item for the highlight             */
  LigmaCanvasItem    *preview_items;    /*  item for previews                  */
  LigmaCanvasItem    *vectors;          /*  item proxy of vectors              */
  LigmaCanvasItem    *grid;             /*  item proxy of the grid             */
  LigmaCanvasItem    *guides;           /*  item proxies of guides             */
  LigmaCanvasItem    *sample_points;    /*  item proxies of sample points      */
  LigmaCanvasItem    *canvas_boundary;  /*  item for the cabvas boundary       */
  LigmaCanvasItem    *layer_boundary;   /*  item for the layer boundary        */
  LigmaCanvasItem    *tool_items;       /*  tools items, below the cursor      */
  LigmaCanvasItem    *cursor;           /*  item for the software cursor       */

  guint              title_idle_id;    /*  title update idle ID               */
  gchar             *title;            /*  current title                      */
  gchar             *status;           /*  current default statusbar content  */

  guint              fill_idle_id;     /*  display_shell_fill() idle ID       */

  LigmaHandedness     cursor_handedness;/*  Handedness for cursor display      */
  LigmaCursorType     current_cursor;   /*  Currently installed main cursor    */
  LigmaToolCursorType tool_cursor;      /*  Current Tool cursor                */
  LigmaCursorModifier cursor_modifier;  /*  Cursor modifier (plus, minus, ...) */

  LigmaCursorType     override_cursor;  /*  Overriding cursor                  */
  gboolean           using_override_cursor;
  gboolean           draw_cursor;      /* should we draw software cursor ?    */

  GtkWidget         *close_dialog;     /*  close dialog                       */
  GtkWidget         *scale_dialog;     /*  scale (zoom) dialog                */
  GtkWidget         *rotate_dialog;    /*  rotate dialog                      */
  GtkWidget         *nav_popup;        /*  navigation popup                   */

  LigmaColorConfig   *color_config;     /*  color management settings          */
  gboolean           color_config_set; /*  settings changed from defaults     */

  LigmaColorTransform *profile_transform;
  GeglBuffer         *profile_buffer;  /*  buffer for profile transform       */
  guchar             *profile_data;    /*  profile_buffer's pixels            */
  gint                profile_stride;  /*  profile_buffer's stride            */

  LigmaColorDisplayStack *filter_stack; /*  color display conversion stuff     */
  guint                  filter_idle_id;

  LigmaColorTransform *filter_transform;
  const Babl         *filter_format;   /*  filter_buffer's format             */
  LigmaColorProfile   *filter_profile;  /*  filter_format's profile            */
  GeglBuffer         *filter_buffer;   /*  buffer for display filters         */
  guchar             *filter_data;     /*  filter_buffer's pixels             */
  gint                filter_stride;   /*  filter_buffer's stride             */

  gint               render_scale;

  cairo_surface_t   *render_cache;
  cairo_region_t    *render_cache_valid;

  gint               render_buf_width;
  gint               render_buf_height;

  cairo_surface_t   *render_surface;   /*  buffer for rendering the mask      */
  cairo_surface_t   *mask_surface;     /*  buffer for rendering the mask      */
  cairo_pattern_t   *checkerboard;     /*  checkerboard pattern               */

  gint               paused_count;

  LigmaTreeHandler   *vectors_freeze_handler;
  LigmaTreeHandler   *vectors_thaw_handler;
  LigmaTreeHandler   *vectors_visible_handler;

  gboolean           zoom_on_resize;

  gboolean           size_allocate_from_configure_event;
  gboolean           size_allocate_center_image;

  /*  the state of ligma_display_shell_tool_events()  */
  GdkDevice         *grab_pointer;
  GdkDevice         *grab_pointer_source;
  guint32            grab_pointer_time;

  /*  the state of ligma_display_shell_zoom_gesture_*() */
  gdouble            last_zoom_scale;
  gboolean           zoom_gesture_active;

  /*  the state of ligma_display_shell_rotate_gesture_*() */
  guint              last_gesture_rotate_state;
  gdouble            initial_gesture_rotate_angle;
  gboolean           rotate_gesture_active;

  /* Two states are possible when the shell is grabbed: it can be
   * grabbed with space (or space+button1 which is the same state),
   * then if space is released but button1 was still pressed, we wait
   * for button1 to be released as well.
   */
  gboolean           space_release_pending;
  gboolean           button1_release_pending;
  const gchar       *space_shaded_tool;

  /* Modifier action currently ON. */
  LigmaModifierAction mod_action;
  gchar             *mod_action_desc;

  gint               scroll_start_x;
  gint               scroll_start_y;
  gint               scroll_last_x;
  gint               scroll_last_y;
  gdouble            rotate_drag_angle;
  gpointer           scroll_info;
  LigmaLayer         *picked_layer;

  GeglBuffer        *mask;
  gint               mask_offset_x;
  gint               mask_offset_y;
  LigmaRGB            mask_color;
  gboolean           mask_inverted;

  LigmaMotionBuffer  *motion_buffer;

  GdkPoint          *zoom_focus_point;

  gboolean           blink;
  guint              blink_timeout_id;
};

struct _LigmaDisplayShellClass
{
  GtkEventBoxClass  parent_class;

  void (* scaled)    (LigmaDisplayShell *shell);
  void (* scrolled)  (LigmaDisplayShell *shell);
  void (* rotated)   (LigmaDisplayShell *shell);
  void (* reconnect) (LigmaDisplayShell *shell);
};


GType             ligma_display_shell_get_type      (void) G_GNUC_CONST;

GtkWidget       * ligma_display_shell_new           (LigmaDisplay        *display,
                                                    LigmaUnit            unit,
                                                    gdouble             scale,
                                                    LigmaUIManager      *popup_manager,
                                                    GdkMonitor         *monitor);

void              ligma_display_shell_add_overlay   (LigmaDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    LigmaHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);
void              ligma_display_shell_move_overlay  (LigmaDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    LigmaHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);

LigmaImageWindow * ligma_display_shell_get_window    (LigmaDisplayShell   *shell);
LigmaStatusbar   * ligma_display_shell_get_statusbar (LigmaDisplayShell   *shell);

LigmaColorConfig * ligma_display_shell_get_color_config
                                                   (LigmaDisplayShell   *shell);

void              ligma_display_shell_present       (LigmaDisplayShell   *shell);

void              ligma_display_shell_reconnect     (LigmaDisplayShell   *shell);

void              ligma_display_shell_empty         (LigmaDisplayShell   *shell);
void              ligma_display_shell_fill          (LigmaDisplayShell   *shell,
                                                    LigmaImage          *image,
                                                    LigmaUnit            unit,
                                                    gdouble             scale);

void              ligma_display_shell_scaled        (LigmaDisplayShell   *shell);
void              ligma_display_shell_scrolled      (LigmaDisplayShell   *shell);
void              ligma_display_shell_rotated       (LigmaDisplayShell   *shell);

void              ligma_display_shell_set_unit      (LigmaDisplayShell   *shell,
                                                    LigmaUnit            unit);
LigmaUnit          ligma_display_shell_get_unit      (LigmaDisplayShell   *shell);

gboolean          ligma_display_shell_snap_coords   (LigmaDisplayShell   *shell,
                                                    LigmaCoords         *coords,
                                                    gint                snap_offset_x,
                                                    gint                snap_offset_y,
                                                    gint                snap_width,
                                                    gint                snap_height);

gboolean          ligma_display_shell_mask_bounds   (LigmaDisplayShell   *shell,
                                                    gint               *x,
                                                    gint               *y,
                                                    gint               *width,
                                                    gint               *height);

void              ligma_display_shell_set_show_image
                                                   (LigmaDisplayShell   *shell,
                                                    gboolean            show_image);

void              ligma_display_shell_set_show_all  (LigmaDisplayShell   *shell,
                                                    gboolean            show_all);

LigmaPickable    * ligma_display_shell_get_pickable  (LigmaDisplayShell   *shell);
LigmaPickable    * ligma_display_shell_get_canvas_pickable
                                                   (LigmaDisplayShell   *shell);
GeglRectangle     ligma_display_shell_get_bounding_box
                                                   (LigmaDisplayShell   *shell);
gboolean          ligma_display_shell_get_infinite_canvas
                                                   (LigmaDisplayShell   *shell);

void              ligma_display_shell_update_priority_rect
                                                   (LigmaDisplayShell *shell);

void              ligma_display_shell_flush         (LigmaDisplayShell   *shell);

void              ligma_display_shell_pause         (LigmaDisplayShell   *shell);
void              ligma_display_shell_resume        (LigmaDisplayShell   *shell);

void              ligma_display_shell_set_highlight (LigmaDisplayShell   *shell,
                                                    const GdkRectangle *highlight,
                                                    double              opacity);
void              ligma_display_shell_set_mask      (LigmaDisplayShell   *shell,
                                                    GeglBuffer         *mask,
                                                    gint                offset_x,
                                                    gint                offset_y,
                                                    const LigmaRGB      *color,
                                                    gboolean            inverted);


#endif /* __LIGMA_DISPLAY_SHELL_H__ */
