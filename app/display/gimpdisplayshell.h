/* GIMP - The GNU Image Manipulation Program
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

#pragma once


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


#define GIMP_TYPE_DISPLAY_SHELL            (gimp_display_shell_get_type ())
#define GIMP_DISPLAY_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShell))
#define GIMP_DISPLAY_SHELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShellClass))
#define GIMP_IS_DISPLAY_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY_SHELL))
#define GIMP_IS_DISPLAY_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY_SHELL))
#define GIMP_DISPLAY_SHELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShellClass))


typedef struct _GimpDisplayShellClass   GimpDisplayShellClass;
typedef struct _GimpDisplayShellPrivate GimpDisplayShellPrivate;

struct _GimpDisplayShell
{
  GtkEventBox        parent_instance;

  GimpDisplay       *display;

  GBytes            *window_handle;

  GimpUIManager     *popup_manager;

  GimpDisplayOptions *options;
  GimpDisplayOptions *fullscreen_options;
  GimpDisplayOptions *no_image_options;

  GimpUnit           *unit;

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

  GimpZoomModel     *zoom;

  gdouble            last_scale;       /*  scale used when reverting zoom     */
  guint              last_scale_time;  /*  time when last_scale was set       */
  gint               last_offset_x;    /*  offsets used when reverting zoom   */
  gint               last_offset_y;

  gint               disp_width;       /*  width of drawing area              */
  gint               disp_height;      /*  height of drawing area             */

  gboolean           proximity;        /*  is a device in proximity           */

  gboolean           show_image;       /*  whether to show the image          */

  gboolean           show_all;         /*  show the entire image              */

  Selection         *selection;        /*  Selection (marching ants)          */
  gint64             selection_update; /*  Last selection index update        */

  GList             *children;

  GtkWidget         *canvas;           /*  GimpCanvas widget                  */
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

  GimpCanvasItem    *canvas_item;      /*  items drawn on the canvas          */
  GimpCanvasItem    *unrotated_item;   /*  unrotated items for e.g. cursor    */
  GimpCanvasItem    *passe_partout;    /*  item for the highlight             */
  GimpCanvasItem    *preview_items;    /*  item for previews                  */
  GimpCanvasItem    *paths;            /*  item proxy of paths                */
  GimpCanvasItem    *grid;             /*  item proxy of the grid             */
  GimpCanvasItem    *guides;           /*  item proxies of guides             */
  GimpCanvasItem    *sample_points;    /*  item proxies of sample points      */
  GimpCanvasItem    *canvas_boundary;  /*  item for the cabvas boundary       */
  GimpCanvasItem    *layer_boundary;   /*  item for the layer boundary        */
  GimpCanvasItem    *tool_items;       /*  tools items, below the cursor      */
  GimpCanvasItem    *cursor;           /*  item for the software cursor       */

  guint              title_idle_id;    /*  title update idle ID               */
  gchar             *title;            /*  current title                      */
  gchar             *status;           /*  current default statusbar content  */

  guint              fill_idle_id;     /*  display_shell_fill() idle ID       */

  GimpHandedness     cursor_handedness;/*  Handedness for cursor display      */
  GimpCursorType     current_cursor;   /*  Currently installed main cursor    */
  GimpToolCursorType tool_cursor;      /*  Current Tool cursor                */
  GimpCursorModifier cursor_modifier;  /*  Cursor modifier (plus, minus, ...) */

  GimpCursorType     override_cursor;  /*  Overriding cursor                  */
  gboolean           using_override_cursor;
  gboolean           draw_cursor;      /* should we draw software cursor ?    */

  GtkWidget         *close_dialog;     /*  close dialog                       */
  GtkWidget         *scale_dialog;     /*  scale (zoom) dialog                */
  GtkWidget         *rotate_dialog;    /*  rotate dialog                      */
  GtkWidget         *nav_popup;        /*  navigation popup                   */

  GimpColorConfig   *color_config;     /*  color management settings          */
  gboolean           color_config_set; /*  settings changed from defaults     */

  GimpColorTransform *profile_transform;
  GeglBuffer         *profile_buffer;  /*  buffer for profile transform       */
  guchar             *profile_data;    /*  profile_buffer's pixels            */
  gint                profile_stride;  /*  profile_buffer's stride            */

  GimpColorDisplayStack *filter_stack; /*  color display conversion stuff     */
  guint                  filter_idle_id;

  GimpColorTransform *filter_transform;
  const Babl         *filter_format;   /*  filter_buffer's format             */
  GimpColorProfile   *filter_profile;  /*  filter_format's profile            */
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

  GimpTreeHandler   *path_freeze_handler;
  GimpTreeHandler   *path_thaw_handler;
  GimpTreeHandler   *path_visible_handler;

  gboolean           zoom_on_resize;

  gboolean           size_allocate_from_configure_event;
  gboolean           size_allocate_center_image;

  /*  the state of gimp_display_shell_tool_events()  */
  GdkSeat           *grab_seat;
  GdkDevice         *grab_pointer;
  GdkDevice         *grab_pointer_source;

  /*  the state of gimp_display_shell_zoom_gesture_*() */
  gdouble            last_zoom_scale;
  gboolean           zoom_gesture_active;

  /*  the state of gimp_display_shell_rotate_gesture_*() */
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
  GimpModifierAction mod_action;
  gchar             *mod_action_desc;

  gint               scroll_start_x;
  gint               scroll_start_y;
  gint               scroll_last_x;
  gint               scroll_last_y;
  gdouble            rotate_drag_angle;
  gpointer           scroll_info;
  GimpLayer         *picked_layer;

  GeglBuffer        *mask;
  gint               mask_offset_x;
  gint               mask_offset_y;
  GeglColor         *mask_color;
  gboolean           mask_inverted;

  GimpMotionBuffer  *motion_buffer;

  GdkPoint          *zoom_focus_point;

  gboolean           blink;
  guint              blink_timeout_id;

  GimpAlignmentType  snapped_side_horizontal; /* if GIMP_ARRANGE_HFILL then no snapping happen */
  GimpLayer         *snapped_layer_horizontal;

  GimpAlignmentType  snapped_side_vertical; /* if GIMP_ARRANGE_HFILL then no snapping happen */
  GimpLayer         *snapped_layer_vertical;

  GimpAlignmentType  equidistance_side_horizontal;
  GimpLayer         *near_layer_horizontal1;
  GimpLayer         *near_layer_horizontal2;

  GimpAlignmentType  equidistance_side_vertical;
  GimpLayer         *near_layer_vertical1;
  GimpLayer         *near_layer_vertical2;

  gboolean           drawn;

  GimpDisplayShellPrivate *priv;
};

struct _GimpDisplayShellClass
{
  GtkEventBoxClass  parent_class;

  void (* scaled)    (GimpDisplayShell *shell);
  void (* scrolled)  (GimpDisplayShell *shell);
  void (* rotated)   (GimpDisplayShell *shell);
  void (* reconnect) (GimpDisplayShell *shell);
};


GType             gimp_display_shell_get_type      (void) G_GNUC_CONST;

GtkWidget       * gimp_display_shell_new           (GimpDisplay        *display,
                                                    GimpUnit           *unit,
                                                    gdouble             scale,
                                                    GimpUIManager      *popup_manager,
                                                    GdkMonitor         *monitor);

void              gimp_display_shell_add_overlay   (GimpDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    GimpHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);
void              gimp_display_shell_move_overlay  (GimpDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    GimpHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);

GimpImageWindow * gimp_display_shell_get_window    (GimpDisplayShell   *shell);
GimpStatusbar   * gimp_display_shell_get_statusbar (GimpDisplayShell   *shell);

GimpColorConfig * gimp_display_shell_get_color_config
                                                   (GimpDisplayShell   *shell);

void              gimp_display_shell_present       (GimpDisplayShell   *shell);

void              gimp_display_shell_reconnect     (GimpDisplayShell   *shell);

void              gimp_display_shell_empty         (GimpDisplayShell   *shell);
void              gimp_display_shell_fill          (GimpDisplayShell   *shell,
                                                    GimpImage          *image,
                                                    GimpUnit           *unit,
                                                    gdouble             scale);

void              gimp_display_shell_scaled        (GimpDisplayShell   *shell);
void              gimp_display_shell_scrolled      (GimpDisplayShell   *shell);
void              gimp_display_shell_rotated       (GimpDisplayShell   *shell);

void              gimp_display_shell_set_unit      (GimpDisplayShell   *shell,
                                                    GimpUnit           *unit);
GimpUnit        * gimp_display_shell_get_unit      (GimpDisplayShell   *shell);

gboolean          gimp_display_shell_snap_coords   (GimpDisplayShell   *shell,
                                                    GimpCoords         *coords,
                                                    gint                snap_offset_x,
                                                    gint                snap_offset_y,
                                                    gint                snap_width,
                                                    gint                snap_height);

gboolean          gimp_display_shell_mask_bounds   (GimpDisplayShell   *shell,
                                                    gint               *x,
                                                    gint               *y,
                                                    gint               *width,
                                                    gint               *height);

void              gimp_display_shell_set_show_image
                                                   (GimpDisplayShell   *shell,
                                                    gboolean            show_image);

void              gimp_display_shell_set_show_all  (GimpDisplayShell   *shell,
                                                    gboolean            show_all);

GimpPickable    * gimp_display_shell_get_pickable  (GimpDisplayShell   *shell);
GimpPickable    * gimp_display_shell_get_canvas_pickable
                                                   (GimpDisplayShell   *shell);
GeglRectangle     gimp_display_shell_get_bounding_box
                                                   (GimpDisplayShell   *shell);
gboolean          gimp_display_shell_get_infinite_canvas
                                                   (GimpDisplayShell   *shell);

void              gimp_display_shell_update_priority_rect
                                                   (GimpDisplayShell *shell);

void              gimp_display_shell_flush         (GimpDisplayShell   *shell);

void              gimp_display_shell_pause         (GimpDisplayShell   *shell);
void              gimp_display_shell_resume        (GimpDisplayShell   *shell);

void              gimp_display_shell_set_highlight (GimpDisplayShell   *shell,
                                                    const GdkRectangle *highlight,
                                                    double              opacity);
void              gimp_display_shell_set_mask      (GimpDisplayShell   *shell,
                                                    GeglBuffer         *mask,
                                                    gint                offset_x,
                                                    gint                offset_y,
                                                    GeglColor          *color,
                                                    gboolean            inverted);

gboolean          gimp_display_shell_is_drawn      (GimpDisplayShell   *shell);
