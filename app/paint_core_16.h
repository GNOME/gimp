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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PAINT_CORE_16_H__
#define __PAINT_CORE_16_H__


/* Forward declarations */
struct _Canvas;
struct _DrawCore;
struct _tool;
struct _GimpDrawable;


/* the different states that the painting function can be called with  */
typedef enum _PaintCoreState PaintCoreState;
enum _PaintCoreState
{
  INIT_PAINT,
  MOTION_PAINT,
  PAUSE_PAINT,
  RESUME_PAINT,
  FINISH_PAINT
};


/* brush application types  */
typedef enum _BrushHardness BrushHardness;
enum _BrushHardness
{
  HARD,    /* pencil */
  SOFT     /* paintbrush */
};


/* paint application modes  */
typedef enum _ApplyMode ApplyMode;
enum _ApplyMode
{
  CONSTANT,    /* pencil, paintbrush, airbrush, clone */
  INCREMENTAL  /* convolve, smudge */
};


/* structure definitions */
typedef struct _PaintCore16 PaintCore16;
typedef void * (* PaintFunc16)   (PaintCore16 *, struct _GimpDrawable *, int);

struct _PaintCore16
{
  /* core select object */
  struct _draw_core * core;

  /* various coords */
  double   startx, starty;
  double   curx, cury;
  double   lastx, lasty;

  /* state of buttons and keys */
  int      state;

  /* fade out and spacing info */
  double   distance;
  double   spacing;

  /* undo extents in image space coords */
  int      x1, y1;
  int      x2, y2;

  /* size and location of paint hit */
  int      x, y;
  int      w, h;

  /* buffers */
  struct _Canvas * canvas_tiles;
  struct _Canvas * undo_tiles;
  struct _Canvas * canvas_buf;
  struct _Canvas * orig_buf;  

  /* mask for current brush */
  struct _Canvas * brush_mask;

  /* tool-specific paint function */
  PaintFunc16     paint_func;
};




/* create and destroy a paint_core based tool */
struct _tool *     paint_core_16_new             (int type);

void               paint_core_16_free            (struct _tool *);


/* high level tool control functions */
int                paint_core_16_init            (PaintCore16 *,
                                                  struct _GimpDrawable *,
                                                  double x,
                                                  double y);

void               paint_core_16_interpolate     (PaintCore16 *,
                                                  struct _GimpDrawable *);

void               paint_core_16_finish          (PaintCore16 *,
                                                  struct _GimpDrawable *,
                                                  int toolid);

void               paint_core_16_cleanup         (void);



/* painthit buffer functions */
struct _Canvas *   paint_core_16_area            (PaintCore16 *,
                                                  struct _GimpDrawable *);

struct _Canvas *   paint_core_16_area_original   (PaintCore16 *,
                                                  struct _GimpDrawable *,
                                                  int x1,
                                                  int y1,
                                                  int x2,
                                                  int y2);

void               paint_core_16_area_paste      (PaintCore16 *,
                                                  struct _GimpDrawable *,
                                                  gfloat brush_opacity,
                                                  gfloat image_opacity,
                                                  BrushHardness brush_hardness,
                                                  ApplyMode apply_mode,
                                                  int paint_mode
                                                  );

void               paint_core_16_area_replace    (PaintCore16 *,
                                                  struct _GimpDrawable *,
                                                  gfloat brush_opacity,
                                                  gfloat image_opacity,
                                                  BrushHardness brush_hardness,
                                                  ApplyMode apply_mode);





/* paintcore for PDB functions to use */
extern PaintCore16  non_gui_paint_core_16;



/*=======================================

  this should be private with undo

  */

/*  Special undo type  */
typedef struct _PaintUndo16 PaintUndo16;

struct _PaintUndo16
{
  int             tool_ID;
  double          lastx;
  double          lasty;
};




#endif  /*  __PAINT_CORE_16_H__  */
