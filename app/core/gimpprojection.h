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

#ifndef __GIMP_DISPLAY_H__
#define __GIMP_DISPLAY_H__


#include "core/gimpobject.h"


/* FIXME move all GTK stuff to gimpdisplayshell */
#include "gui/gui-types.h"


/*  some useful macros  */

/* unpacking the user scale level (char) */
#define  SCALESRC(g)      (g->scale & 0x00ff)
#define  SCALEDEST(g)     (g->scale >> 8)

/* finding the effective screen resolution (double) */
#define  SCREEN_XRES(g)   (g->dot_for_dot ? \
                           g->gimage->xresolution : gimprc.monitor_xres)
#define  SCREEN_YRES(g)   (g->dot_for_dot ? \
                           g->gimage->yresolution : gimprc.monitor_yres)

/* calculate scale factors (double) */
#define  SCALEFACTOR_X(g) ((SCALEDEST(g) * SCREEN_XRES(g)) / \
			   (SCALESRC(g) * g->gimage->xresolution))
#define  SCALEFACTOR_Y(g) ((SCALEDEST(g) * SCREEN_YRES(g)) / \
			   (SCALESRC(g) * g->gimage->yresolution))

/* scale values */
#define  SCALEX(g,x)      ((gint) (x * SCALEFACTOR_X(g)))
#define  SCALEY(g,y)      ((gint) (y * SCALEFACTOR_Y(g)))

/* unscale values */
#define  UNSCALEX(g,x)    ((gint) (x / SCALEFACTOR_X(g)))
#define  UNSCALEY(g,y)    ((gint) (y / SCALEFACTOR_Y(g)))
/* (and float-returning versions) */
#define  FUNSCALEX(g,x)   (x / SCALEFACTOR_X(g))
#define  FUNSCALEY(g,y)   (y / SCALEFACTOR_Y(g))


typedef struct _IdleRenderStruct IdleRenderStruct;

struct _IdleRenderStruct
{
  gint      width;
  gint      height;
  gint      x;
  gint      y;
  gint      basex;
  gint      basey;
  guint     idleid;
  gboolean  active;
  GSList   *update_areas;   /*  flushed update areas */
};


extern GSList *display_list; /* EEK */


#define GIMP_TYPE_DISPLAY            (gimp_display_get_type ())
#define GIMP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY, GimpDisplay))
#define GIMP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY, GimpDisplayClass))
#define GIMP_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY))
#define GIMP_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY))
#define GIMP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY, GimpDisplayClass))


typedef struct _GimpDisplayClass GimpDisplayClass;

struct _GimpDisplay
{
  GimpObject  parent_instance;

  gint        ID;               /*  unique identifier for this gdisplay     */

  GimpImage  *gimage;	        /*  pointer to the associated gimage        */
  gint        instance;         /*  the instance # of this gdisplay as      */
                                /*  taken from the gimage at creation       */

  GtkWidget  *shell;            /*  shell widget for this gdisplay          */

  gint        scale;            /*  scale factor from original raw image    */
  gboolean    dot_for_dot;      /*  is monitor resolution being ignored?    */
  gboolean    draw_guides;      /*  should the guides be drawn?             */
  gboolean    snap_to_guides;   /*  should the guides be snapped to?        */

  Selection  *select;           /*  Selection object                        */

  GSList     *update_areas;     /*  Update areas list                       */

  IdleRenderStruct idle_render; /*  state of this gdisplay's render thread  */
};

struct _GimpDisplayClass
{
  GimpObjectClass  parent_class;
};


/* member function declarations */

GType         gimp_display_get_type             (void);

GimpDisplay * gdisplay_new                      (GimpImage            *gimage,
                                                 guint                 scale);
void          gdisplay_delete                   (GimpDisplay          *gdisp);

GimpDisplay * gdisplay_get_by_ID                (Gimp                 *gimp,
                                                 gint                  ID);

void          gdisplay_reconnect                (GimpDisplay          *gdisp,
                                                 GimpImage            *gimage);

void          gdisplay_add_update_area          (GimpDisplay          *gdisp,
                                                 gint                  x,
                                                 gint                  y,
                                                 gint                  w,
                                                 gint                  h);

void          gdisplay_transform_coords         (GimpDisplay          *gdisp,
                                                 gint                  x,
                                                 gint                  y,
                                                 gint                 *nx,
                                                 gint                 *ny,
                                                 gboolean              use_offsets);
void          gdisplay_untransform_coords       (GimpDisplay          *gdisp,
                                                 gint                  x,
                                                 gint                  y,
                                                 gint                 *nx,
                                                 gint                 *ny,
                                                 gboolean              round,
                                                 gboolean              use_offsets);
void          gdisplay_transform_coords_f       (GimpDisplay          *gdisp,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble              *nx,
                                                 gdouble              *ny,
                                                 gboolean              use_offsets);
void          gdisplay_untransform_coords_f     (GimpDisplay          *gdisp,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble              *nx,
                                                 gdouble              *ny,
                                                 gboolean              use_offsets);

void          gdisplay_selection_visibility     (GimpDisplay          *gdisp,
                                                 GimpSelectionControl  control);

void          gdisplay_flush                    (GimpDisplay          *gdisp);
void          gdisplay_flush_now                (GimpDisplay          *gdisp);

void          gdisplays_finish_draw             (void);


#endif /*  __GIMP_DISPLAY_H__  */
