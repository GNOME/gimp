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

#ifndef __GIMP_PAINT_CORE_H__
#define __GIMP_PAINT_CORE_H__


#include "core/gimpobject.h"


#define PAINT_CORE_SUBSAMPLE        4
#define PAINT_CORE_SOLID_SUBSAMPLE  2

#define PRESSURE_SCALE              1.5


/* the different states that the painting function can be called with  */

typedef enum
{
  INIT_PAINT,       /* Setup PaintFunc internals */
  MOTION_PAINT,     /* PaintFunc performs motion-related rendering */
  PAUSE_PAINT,      /* Unused. Reserved */
  RESUME_PAINT,     /* Unused. Reserved */
  FINISH_PAINT,     /* Cleanup and/or reset PaintFunc operation */
  PRETRACE_PAINT,   /* PaintFunc performs window tracing activity prior to rendering */
  POSTTRACE_PAINT   /* PaintFunc performs window tracing activity following rendering */
} GimpPaintCoreState;

typedef enum
{
  /*  Set for tools that don't mind if
   *  the brush changes while painting.
   */
  CORE_HANDLES_CHANGING_BRUSH = 0x1 << 0,

  /*  Set for tools that perform
   *  temporary rendering directly to the
   *  window. These require sequencing with
   *  gdisplay_flush() routines.
   *  See gimpclone.c for example.
   */
  CORE_TRACES_ON_WINDOW       = 0x1 << 1
} GimpPaintCoreFlags;


#define GIMP_TYPE_PAINT_CORE            (gimp_paint_core_get_type ())
#define GIMP_PAINT_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_CORE, GimpPaintCore))
#define GIMP_PAINT_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_CORE, GimpPaintCoreClass))
#define GIMP_IS_PAINT_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_CORE))
#define GIMP_IS_PAINT_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_CORE))
#define GIMP_PAINT_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_CORE, GimpPaintCoreClass))


typedef struct _GimpPaintCoreClass GimpPaintCoreClass;

struct _GimpPaintCore
{
  GimpObject          parent_instance;

  gint                ID;            /*  unique instance ID               */

  GimpCoords          start_coords;  /*  starting coords (for undo only)  */

  GimpCoords          cur_coords;    /*  current coords                   */
  GimpCoords          last_coords;   /*  last coords                      */

  GimpVector2         last_paint;    /*  last point that was painted      */

  gdouble             distance;      /*  distance traveled by brush       */
  gdouble             pixel_dist;    /*  distance in pixels               */

  gint                x1, y1;        /*  undo extents in image coords     */
  gint                x2, y2;        /*  undo extents in image coords     */

  GimpPaintCoreFlags  flags;         /*  tool flags, see ToolFlags above  */
  gboolean            use_pressure;  /*  look at coords->pressure         */

  /*  undo blocks variables  */
  TileManager        *undo_tiles;
  TileManager        *canvas_tiles;

  /*  paint buffers variables  */
  TempBuf            *orig_buf;
  TempBuf            *canvas_buf;
};

struct _GimpPaintCoreClass
{
  GimpObjectClass  parent_class;

  /*  virtual functions  */
  gboolean  (* start)          (GimpPaintCore      *core,
                                GimpDrawable       *drawable,
                                GimpPaintOptions   *paint_options,
                                GimpCoords         *coords);

  gboolean  (* pre_paint)      (GimpPaintCore      *core,
                                GimpDrawable       *drawable,
                                GimpPaintOptions   *paint_options,
                                GimpPaintCoreState  paint_state);
  void      (* paint)          (GimpPaintCore      *core,
                                GimpDrawable       *drawable,
                                GimpPaintOptions   *paint_options,
                                GimpPaintCoreState  paint_state);

  void      (* interpolate)    (GimpPaintCore      *core,
                                GimpDrawable       *drawable,
                                GimpPaintOptions   *paint_options);

  TempBuf * (* get_paint_area) (GimpPaintCore      *core,
                                GimpDrawable       *drawable,
                                GimpPaintOptions   *paint_options);
};


GType     gimp_paint_core_get_type    (void) G_GNUC_CONST;

void      gimp_paint_core_paint       (GimpPaintCore       *core,
                                       GimpDrawable        *drawable,
                                       GimpPaintOptions    *paint_options,
                                       GimpPaintCoreState   state);

gboolean  gimp_paint_core_start       (GimpPaintCore       *core,
                                       GimpDrawable        *drawable,
                                       GimpPaintOptions    *paint_options,
                                       GimpCoords          *coords);
void      gimp_paint_core_finish      (GimpPaintCore       *core,
                                       GimpDrawable        *drawable);
void      gimp_paint_core_cancel      (GimpPaintCore       *core,
                                       GimpDrawable        *drawable);
void      gimp_paint_core_cleanup     (GimpPaintCore       *core);

void      gimp_paint_core_constrain   (GimpPaintCore       *core);

void      gimp_paint_core_interpolate (GimpPaintCore       *core,
                                       GimpDrawable        *drawable,
                                       GimpPaintOptions    *paint_options);


/*  protected functions  */

TempBuf * gimp_paint_core_get_paint_area (GimpPaintCore            *core,
                                          GimpDrawable             *drawable,
                                          GimpPaintOptions         *options);
TempBuf * gimp_paint_core_get_orig_image (GimpPaintCore            *core,
                                          GimpDrawable             *drawable,
                                          gint                      x1,
                                          gint                      y1,
                                          gint                      x2,
                                          gint                      y2);

void      gimp_paint_core_paste          (GimpPaintCore            *core,
                                          PixelRegion              *paint_maskPR,
                                          GimpDrawable	           *drawable,
                                          gdouble	            paint_opacity,
                                          gdouble	            image_opacity,
                                          GimpLayerModeEffects      paint_mode,
                                          GimpPaintApplicationMode  mode);
void      gimp_paint_core_replace        (GimpPaintCore            *core,
                                          PixelRegion              *paint_maskPR,
                                          GimpDrawable	           *drawable,
                                          gdouble	            paint_opacity,
                                          gdouble                   image_opacity,
                                          GimpPaintApplicationMode  mode);

void   gimp_paint_core_validate_undo_tiles   (GimpPaintCore *core,
                                              GimpDrawable  *drawable,
                                              gint           x,
                                              gint           y,
                                              gint           w,
                                              gint           h);
void   gimp_paint_core_validate_canvas_tiles (GimpPaintCore *core,
                                              gint           x,
                                              gint           y,
                                              gint           w,
                                              gint           h);


#endif  /*  __GIMP_PAINT_CORE_H__  */
