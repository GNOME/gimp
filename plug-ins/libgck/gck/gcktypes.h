/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

#ifndef __GCKTYPES_H__
#define __GCKTYPES_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GCK_CONSTRAIN_RGB      1<<0
#define GCK_CONSTRAIN_RGBA     1<<1
#define GCK_CONSTRAIN_GRAY     1<<2
#define GCK_CONSTRAIN_GRAYA    1<<3
#define GCK_CONSTRAIN_INDEXED  1<<4
#define GCK_CONSTRAIN_INDEXEDA 1<<5
#define GCK_CONSTRAIN_ALL      0xff

#define GCK_ALIGN_CENTERED 0.5
#define GCK_ALIGN_RIGHT 1.0
#define GCK_ALIGN_LEFT 0.0
#define GCK_ALIGN_TOP 0.0
#define GCK_ALIGN_BOTTOM 1.0

#define GCK_HSV_UNDEFINED -1.0
#define GCK_HSL_UNDEFINED -1.0

typedef enum
{
  DITHER_NONE,
  DITHER_FLOYD_STEINBERG,
  DITHER_ORDERED
} GckDitherType;

typedef struct
{
  double r,g,b,a;
} GckRGB;

typedef struct
{
  guchar r,g,b;
  const char *name;
} GckNamedRGB;

typedef struct
{
  GdkVisual    *visual;
  GdkColormap  *colormap;
  gulong        allocedpixels[256];
  guint32       colorcube[256];
  GdkColor      rgbpalette[256];
  guchar        map_r[256],map_g[256],map_b[256];
  guchar        indextab[7][7][7];
  guchar        invmap_r[256],invmap_g[256],invmap_b[256];
  int           shades_r,shades_g,shades_b,numcolors;
  GckDitherType dithermethod;
} GckVisualInfo;

typedef void (*GckRenderFunction)      (double, double, GckRGB *);
typedef void (*GckPutPixelFunction)    (int, int, GckRGB *);
typedef void (*GckProgressFunction)    (int, int, int);
typedef void (*GckColorUpdateFunction) (GckRGB *);
typedef gint (*GckEventFunction)       (GtkWidget *, GdkEvent *, gpointer);

typedef struct
{
  double size;
  double value;
  double lower;
  double upper;
  double step_inc;
  double page_inc;
  double page_size;
  GtkUpdateType update_type;
  gint   draw_value_flag;
} GckScaleValues;

typedef enum
{
  GCK_RIGHT,
  GCK_LEFT,
  GCK_TOP,
  GCK_BOTTOM
} GckPosition;

typedef struct
{
  GtkWidget *widget;
  GtkWidget *actionbox,*workbox;
  GtkWidget *okbutton;
  GtkWidget *cancelbutton;
  GtkWidget *helpbutton;
} GckDialogWindow;

typedef struct
{
  GtkWidget *widget;
  GtkStyle *style;
  GtkAccelGroup *accelerator_group;
  GckVisualInfo *visinfo;
} GckApplicationWindow;

typedef struct _GckMenuItem
{
  char *label;
  char accelerator_key;
  gint accelerator_mods;
  GtkSignalFunc item_selected_func;
  gpointer user_data;
  struct _GckMenuItem *subitems;
  GtkWidget *widget;
} GckMenuItem;

typedef struct
{
  GtkWidget *widget;
  GtkWidget *list;
  GckEventFunction event_handler;
  GdkEvent last_event;
  GList *itemlist;
  GList *current_selection;
  gint *selected_items,num_selected_items;
  gint width,height;
  gint num_items;
  gint disable_signals;
} GckListBox;

typedef struct
{
  char *label;
  GtkWidget *widget;
  gpointer user_data;
  GckListBox *listbox;
} GckListBoxItem;

typedef struct
{
  GtkWidget *widget;
  GtkWidget *tab_box;
  GtkWidget *workbox;
  GList *page_list;
  GtkPositionType tab_position;
  gint width;
  gint height;
  gint current_page;
  gint num_pages;
  gint button_down;
} GckNoteBook;

typedef struct
{
  char *label;
  GdkImage *image;
  GdkPixmap *pixmap;
  GdkRectangle area;
} GckNoteBookTab;

typedef struct
{
  gchar *label;
  gint position;
  gint active;
  GtkWidget *widget;
  GckNoteBookTab *tab;
  GtkWidget *labelwidget;
  GckNoteBook *notebook;
} GckNoteBookPage;

typedef struct {
  int x,y,w,h;
  GdkImage *buffer;
} _GckBackBuffer;

typedef struct
{
  double x,y;
} GckVector2;

typedef struct
{
  double x,y,z;
} GckVector3;

typedef struct
{
  double x,y,z,w;
} GckVector4;

#ifdef __cplusplus
}
#endif

#endif
