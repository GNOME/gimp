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

#ifndef __GIMP_DISPLAY_SHELL_H__
#define __GIMP_DISPLAY_SHELL_H__


#include <gtk/gtkwindow.h>


/* FIXME remmove all gui/ stuff */
#include "gui/gui-types.h"


/* maximal width of the string holding the cursor-coordinates for
 * the status line
 */
#define CURSOR_STR_LENGTH 256

/* maximal length of the format string for the cursor-coordinates */
#define CURSOR_FORMAT_LENGTH 32


#define GIMP_TYPE_DISPLAY_SHELL            (gimp_display_shell_get_type ())
#define GIMP_DISPLAY_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShell))
#define GIMP_DISPLAY_SHELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShellClass))
#define GIMP_IS_DISPLAY_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY_SHELL))
#define GIMP_IS_DISPLAY_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY_SHELL))
#define GIMP_DISPLAY_SHELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY_SHELL, GimpDisplayShellClass))


typedef struct _GimpDisplayShellClass  GimpDisplayShellClass;

struct _GimpDisplayShell
{
  GtkWindow         parent_instance;

  GimpDisplay      *gdisp;

  GtkItemFactory   *ifactory;

  gint              offset_x;          /*  offset of display image into raw image  */
  gint              offset_y;

  gint              disp_width;        /*  width of drawing area   */
  gint              disp_height;       /*  height of drawing area  */
  gint              disp_xoffset;
  gint              disp_yoffset;

  gboolean          proximity;         /* is a device in proximity           */

  Selection        *select;            /*  Selection object    */

  GSList           *display_areas;     /*  display areas list  */

  GtkAdjustment    *hsbdata;           /*  adjustments         */
  GtkAdjustment    *vsbdata;

  GtkWidget        *canvas;            /*  canvas widget       */

  GtkWidget        *hsb;               /*  scroll bars         */
  GtkWidget        *vsb;
  GtkWidget        *qmaskoff;          /*  qmask buttons       */
  GtkWidget        *qmaskon;
  GtkWidget        *hrule;             /*  rulers              */
  GtkWidget        *vrule;
  GtkWidget        *origin;            /*  origin button       */
  GtkWidget        *statusarea;        /*  status area hbox    */
  GtkWidget        *statusbar;         /*  statusbar           */
  GtkWidget        *progressbar;       /*  progressbar         */
  guint             progressid;        /*  progress id         */
  GtkWidget        *cursor_label;      /*  cursor position     */
  gchar             cursor_format_str[CURSOR_FORMAT_LENGTH];
  GtkWidget        *cancelbutton;      /*  cancel button       */

  GdkGC            *scroll_gc;         /*  GC for scrolling    */

  GdkPixmap        *icon;              /*  Pixmap for the icon                */
  GdkBitmap        *iconmask;          /*  Bitmap for the icon mask           */
  guint             iconsize;          /*  size of the icon pixmap            */
  gboolean          icon_needs_update; /*  Do we need to render a new icon?   */
  guint             icon_timeout_id;   /*  ID of the timeout-function         */
  guint             icon_idle_id;      /*  ID of the idle-function            */

  GdkCursorType       current_cursor;  /*  Currently installed main cursor    */
  GimpToolCursorType  tool_cursor;     /*  Current Tool cursor                */
  GimpCursorModifier  cursor_modifier; /*  Cursor modifier (plus, minus, ...) */

  GdkCursorType     override_cursor;   /*  Overriding cursor                  */
  gboolean          using_override_cursor; /*  is the cursor overridden?      */

  gboolean          draw_cursor;       /* should we draw software cursor ?    */
  gboolean          have_cursor;       /* is cursor currently drawn ?         */
  gint              cursor_x;          /* software cursor X value             */
  gint              cursor_y;          /* software cursor Y value             */

  GtkWidget        *padding_button;    /* GimpColorPanel in the NE corner     */
  GimpRGB           padding_color;     /* color of the empty around the image */
  GdkGC            *padding_gc;        /* GC with padding_color as BG         */

  GtkWidget        *warning_dialog;    /*  close warning dialog               */
  InfoDialog       *info_dialog;       /*  image information dialog           */
  NavigationDialog *nav_dialog;        /*  image navigation dialog            */
  NavigationDialog *nav_popup;         /*  navigation popup                   */

#ifdef DISPLAY_FILTERS
  GList            *cd_list;           /* color display conversion stuff      */
  GtkWidget        *cd_ui;             /* color display filter dialog         */
#endif /* DISPLAY_FILTERS */
};

struct _GimpDisplayShellClass
{
  GtkWindowClass  parent_class;
};


GType       gimp_display_shell_get_type             (void) G_GNUC_CONST;

GtkWidget * gimp_display_shell_new                  (GimpDisplay *gdisp);

void        gimp_display_shell_close                (GimpDisplayShell *shell,
                                                     gboolean          kill_it);

void        gimp_display_shell_reconnect            (GimpDisplayShell *shell);

void        gimp_display_shell_transform_coords     (GimpDisplayShell *shell,
                                                     GimpCoords       *image_coords,
                                                     GimpCoords       *display_coords);
void        gimp_display_shell_untransform_coords   (GimpDisplayShell *shell,
                                                     GimpCoords       *display_coords,
                                                     GimpCoords       *image_coords);

void        gimp_display_shell_set_menu_sensitivity (GimpDisplayShell *shell);

GimpGuide * gimp_display_shell_find_guide           (GimpDisplayShell *shell,
                                                     gdouble           x,
                                                     gdouble           y);
gboolean    gimp_display_shell_snap_point           (GimpDisplayShell *shell,
                                                     gdouble           x,
                                                     gdouble           y,
                                                     gdouble          *tx,
                                                     gdouble          *ty);
gboolean    gimp_display_shell_snap_rectangle       (GimpDisplayShell *shell,
                                                     gdouble           x1,
                                                     gdouble           y1,
                                                     gdouble           x2,
                                                     gdouble           y2,
                                                     gdouble          *tx1,
                                                     gdouble          *ty1);

gint        gimp_display_shell_mask_value           (GimpDisplayShell *shell,
                                                     gint              x,
                                                     gint              y);
gboolean    gimp_display_shell_mask_bounds          (GimpDisplayShell *shell,
                                                     gint             *x1,
                                                     gint             *y1,
                                                     gint             *x2,
                                                     gint             *y2);

void        gimp_display_shell_add_expose_area      (GimpDisplayShell *shell,
                                                     gint              x,
                                                     gint              y,
                                                     gint              w,
                                                     gint              h);
void        gimp_display_shell_expose_guide         (GimpDisplayShell *shell,
                                                     GimpGuide        *guide);
void        gimp_display_shell_expose_full          (GimpDisplayShell *shell);

void        gimp_display_shell_flush                (GimpDisplayShell *shell);

void    gimp_display_shell_real_install_tool_cursor (GimpDisplayShell *shell,
                                                     GdkCursorType     cursor_type,
                                                     GimpToolCursorType  tool_cursor,
                                                     GimpCursorModifier  modifier,
                                                     gboolean          always_install);
void        gimp_display_shell_install_tool_cursor  (GimpDisplayShell *shell,
                                                     GdkCursorType     cursor_type,
                                                     GimpToolCursorType  tool_cursor,
                                                     GimpCursorModifier  modifier);
void        gimp_display_shell_remove_tool_cursor   (GimpDisplayShell *shell);
void     gimp_display_shell_install_override_cursor (GimpDisplayShell *shell,
                                                     GdkCursorType     cursor_type);
void      gimp_display_shell_remove_override_cursor (GimpDisplayShell *shell);

void	    gimp_display_shell_update_cursor	    (GimpDisplayShell *shell,
                                                     gint              x,
                                                     gint              y);
void        gimp_display_shell_resize_cursor_label  (GimpDisplayShell *shell);
void        gimp_display_shell_update_title         (GimpDisplayShell *shell);

void        gimp_display_shell_draw_guide           (GimpDisplayShell *shell,
                                                     GimpGuide        *guide,
                                                     gboolean          active);
void        gimp_display_shell_draw_guides          (GimpDisplayShell *shell);

void        gimp_display_shell_shrink_wrap          (GimpDisplayShell *shell);

void        gimp_display_shell_selection_visibility (GimpDisplayShell *shell,
                                                     GimpSelectionControl  control);


#endif /* __GIMP_DISPLAY_SHELL_H__ */
