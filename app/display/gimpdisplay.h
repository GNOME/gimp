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
#ifndef __GDISPLAY_H__
#define __GDISPLAY_H__

#include "gimage.h"
#include "info_dialog.h"
#include "selection.h"

#include "gdisplayF.h"

/*
 *  Global variables
 *
 */

/*  some useful macros  */

/* unpacking the user scale level (char) */
#define  SCALESRC(g)    (g->scale & 0x00ff)
#define  SCALEDEST(g)   (g->scale >> 8)

/* finding the effective screen resolution (float) */
#define  SCREEN_XRES(g) (g->dot_for_dot? g->gimage->xresolution : monitor_xres)
#define  SCREEN_YRES(g) (g->dot_for_dot? g->gimage->yresolution : monitor_yres)

/* calculate scale factors (float) */
#define  SCALEFACTOR_X(g)  ((SCALEDEST(g) * SCREEN_XRES(g)) /          \
			    (SCALESRC(g) * g->gimage->xresolution))
#define  SCALEFACTOR_Y(g)  ((SCALEDEST(g) * SCREEN_YRES(g)) /          \
			    (SCALESRC(g) * g->gimage->yresolution))

/* scale values */
#define  SCALEX(g,x)    ((int)(x * SCALEFACTOR_X(g)))
#define  SCALEY(g,y)    ((int)(y * SCALEFACTOR_Y(g)))

/* unscale values */
#define  UNSCALEX(g,x)  ((int)(x / SCALEFACTOR_X(g)))
#define  UNSCALEY(g,y)  ((int)(y / SCALEFACTOR_Y(g)))
/* (and float-returning versions) */
#define  FUNSCALEX(g,x)  ((x / SCALEFACTOR_X(g)))
#define  FUNSCALEY(g,y)  ((y / SCALEFACTOR_Y(g)))




#define LOWPASS(x) ((x>0) ? x : 0)
/* #define HIGHPASS(x,y) ((x>y) ? y : x) */ /* unused - == MIN */


/* maximal width of the string holding the cursor-coordinates for
   the status line */
#define CURSOR_STR_LENGTH 256

/* maximal length of the format string for the cursor-coordinates */
#define CURSOR_FORMAT_LENGTH 32

typedef struct _IdleRenderStruct
{
  int width;
  int height;
  int x;
  int y;
  int basex;
  int basey;
  guint idleid;
  /*guint handlerid;*/
  gboolean active;
  GSList *update_areas;           /*  flushed update areas */

} IdleRenderStruct;


struct _GDisplay
{
  int ID;                         /*  unique identifier for this gdisplay     */

  GtkWidget *shell;               /*  shell widget for this gdisplay          */
  GtkWidget *canvas;              /*  canvas widget for this gdisplay         */
  GtkWidget *hsb, *vsb;           /*  widgets for scroll bars                 */
  GtkWidget *hrule, *vrule;       /*  widgets for rulers                      */
  GtkWidget *origin;              /*  widgets for rulers                      */
  GtkWidget *popup;               /*  widget for popup menu                   */
  GtkWidget *statusarea;          /*  hbox holding the statusbar and stuff    */
  GtkWidget *statusbar;           /*  widget for statusbar                    */
  GtkWidget *progressbar;         /*  widget for progressbar                  */
  GtkWidget *cursor_label;        /*  widget for cursor position              */
  char cursor_format_str [CURSOR_FORMAT_LENGTH]; /* we need a variable format
						  * string because different
						  * units have different number
						  * of decimals               */
  GtkWidget *cancelbutton;        /*  widget for cancel button                */
  guint progressid;               /*  id of statusbar message for progress    */

  InfoDialog *window_info_dialog; /*  dialog box for image information        */

  int color_type;                 /*  is this an RGB or GRAY colormap         */

  GtkAdjustment *hsbdata;         /*  horizontal data information             */
  GtkAdjustment *vsbdata;         /*  vertical data information               */

  GimpImage *gimage;	          /*  pointer to the associated gimage struct */
  int instance;                   /*  the instance # of this gdisplay as      */
                                  /*  taken from the gimage at creation       */

  int depth;   		          /*  depth of our drawables                  */
  int disp_width;                 /*  width of drawing area in the window     */
  int disp_height;                /*  height of drawing area in the window    */
  int disp_xoffset;
  int disp_yoffset;

  int offset_x, offset_y;         /*  offset of display image into raw image  */
  int scale;        	          /*  scale factor from original raw image    */
  int dot_for_dot;		  /*  is monitor resolution being ignored?    */
  short draw_guides;              /*  should the guides be drawn?             */
  short snap_to_guides;           /*  should the guides be snapped to?        */

  Selection *select;              /*  Selection object                        */

  GdkGC *scroll_gc;               /*  GC for scrolling                        */

  GSList *update_areas;           /*  Update areas list                       */
  GSList *display_areas;          /*  Display areas list                      */

  int           current_cursor;   /*  Currently installed cursor              */

  GdkCursorType override_cursor;  /*  Overriding cursor (ie. hourglass)       */

  short draw_cursor;	          /* should we draw software cursor ?         */
  short using_override_cursor;    /* is the cursor overridden? (ie. hourglass)*/
  int cursor_x;			  /* software cursor X value                  */
  int cursor_y;			  /* software cursor Y value                  */
  short proximity;                /* is a device in proximity of gdisplay ?   */
  short have_cursor;		  /* is cursor currently drawn ?              */

  IdleRenderStruct idle_render;   /* state of this gdisplay's render thread   */
};



/* member function declarations */

GDisplay * gdisplay_new                    (GimpImage *, unsigned int);
void       gdisplay_reconnect              (GDisplay *, GimpImage *);
void       gdisplay_remove_and_delete      (GDisplay *);
int        gdisplay_mask_value             (GDisplay *, int, int);
int        gdisplay_mask_bounds            (GDisplay *, int *, int *, int *, int *);
void       gdisplay_transform_coords       (GDisplay *, int, int, int *, int *, int);
void       gdisplay_untransform_coords     (GDisplay *, int, int, int *,
					    int *, int, int);
void       gdisplay_transform_coords_f     (GDisplay *, double, double, double *,
					    double *, int);
void       gdisplay_untransform_coords_f   (GDisplay *, double, double, double *,
					    double *, int);
void       gdisplay_install_tool_cursor    (GDisplay *, GdkCursorType);
void       gdisplay_install_gimp_tool_cursor (GDisplay *, int);
void       gdisplay_remove_tool_cursor     (GDisplay *);
void       gdisplay_install_override_cursor(GDisplay *, GdkCursorType);
void       gdisplay_remove_override_cursor (GDisplay *);
void       gdisplay_set_menu_sensitivity   (GDisplay *);
void       gdisplay_expose_area            (GDisplay *, int, int, int, int);
void       gdisplay_expose_guide           (GDisplay *, Guide *);
void       gdisplay_expose_full            (GDisplay *);
void       gdisplay_flush                  (GDisplay *);
void       gdisplay_flush_now              (GDisplay *);
void       gdisplays_finish_draw           (void);
void       gdisplay_draw_guides            (GDisplay *);
void       gdisplay_draw_guide             (GDisplay *, Guide *, int);
Guide*     gdisplay_find_guide             (GDisplay *, int, int);
void       gdisplay_snap_point             (GDisplay *, double , double, double *, double *);
void       gdisplay_snap_rectangle         (GDisplay *, int, int, int, int, int *, int *);
void	   gdisplay_update_cursor	   (GDisplay *, int, int);
void	   gdisplay_set_dot_for_dot	   (GDisplay *, int);
void       gdisplay_resize_cursor_label    (GDisplay *);
GDisplay * gdisplays_check_valid           (GDisplay *, GimpImage *);

/*  function declarations  */

GDisplay * gdisplay_active                 (void);
GDisplay * gdisplay_get_ID                 (int);
void       gdisplay_update_title           (GDisplay*);
void       gdisplays_update_title          (GimpImage*);
void       gdisplays_resize_cursor_label   (GimpImage*);
void       gdisplays_update_area           (GimpImage*, int, int, int, int);
void       gdisplays_expose_guides         (GimpImage*);
void       gdisplays_expose_guide          (GimpImage*, Guide *);
void       gdisplays_update_full           (GimpImage*);
void       gdisplays_shrink_wrap           (GimpImage*);
void       gdisplays_expose_full           (void);
void       gdisplays_selection_visibility  (GimpImage*, SelectionControl);
int        gdisplays_dirty                 (void);
void       gdisplays_delete                (void);
void       gdisplays_flush                 (void);
void       gdisplays_flush_now             (void);
void       gdisplay_flush_displays_only (GDisplay *gdisp); /* no rerender! */



#endif /*  __GDISPLAY_H__  */

