/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * 
 */

#ifndef __GFIG_H__
#define __GFIG_H__

#include "gfig-dobject.h"

#define MAX_LOAD_LINE    256
#define SQ_SIZE 8

extern gint line_no;
extern gint preview_width, preview_height;
extern gboolean drawing_pic;
extern gint need_to_scale;
extern gint32        gfig_image;
extern gint32        gfig_drawable;
extern GdkGC  *gfig_gc;
extern gdouble scale_x_factor, scale_y_factor;

extern GtkWidget    *gfig_preview;
extern GtkWidget    *pic_preview;
extern Dobject *tmp_line;
extern gint obj_show_single;

typedef enum
{
  RECT_GRID = 0,
  POLAR_GRID,
  ISO_GRID
} GridType;

typedef struct
{
  gint     gridspacing;
  GridType gridtype;
  gboolean drawgrid;
  gboolean snap2grid;
  gboolean lockongrid;
  gboolean showcontrol;
} GfigOpts;

typedef enum
{
  ADD = 0,
  SUBTRACT,
  REPLACE,
  INTERSECT
} SelectionType;
    

typedef enum
{
  ARC_SEGMENT = 0,
  ARC_SECTOR
} ArcType;

typedef enum
{
  FILL_FOREGROUND = 0,
  FILL_BACKGROUND,
  FILL_PATTERN
} FillType;

typedef enum
{
  FILL_EACH = 0,
  FILL_AFTER
} FillWhen;

typedef struct
{
  SelectionType type;           /* ADD etc .. */
  gint          antia;          /* Boolean for Antia */
  gint          feather;        /* Feather it ? */
  gdouble       feather_radius; /* Radius to feather */
  ArcType       as_pie;         /* Arc type selection segment/sector */
  FillType      fill_type;      /* Fill type for selection */
  FillWhen      fill_when;      /* Fill on each selection or after all? */
  gdouble       fill_opacity;   /* You can guess this one */
} selection_option;

typedef enum
{
  ORIGINAL_LAYER = 0,
  SINGLE_LAYER,
  MULTI_LAYER
} DrawonLayers;

typedef enum
{
  LAYER_TRANS_BG = 0,
  LAYER_BG_BG,
  LAYER_FG_BG,
  LAYER_WHITE_BG,
  LAYER_COPY_BG
} LayersBGType;

typedef enum
{
  PAINT_BRUSH_TYPE = 0,
  PAINT_SELECTION_TYPE,
  PAINT_SELECTION_FILL_TYPE
} PaintType;

typedef enum
{
  BRUSH_BRUSH_TYPE = 0,
  BRUSH_PENCIL_TYPE,
  BRUSH_AIRBRUSH_TYPE,
  BRUSH_PATTERN_TYPE
} BrushType;


typedef struct
{
  GfigOpts      opts;
  gboolean      showimage;
  gint          maxundo;
  gboolean      showpos;
  gdouble       brushfade;
  gdouble       brushgradient;
  gdouble       airbrushpressure;
  DrawonLayers  onlayers;
  LayersBGType  onlayerbg;
  PaintType     painttype;
  gboolean      reverselines;
  gboolean      scaletoimage;
  gdouble       scaletoimagefp;
  gboolean      approxcircles;
  BrushType     brshtype;
  DobjType      otype;
} SelectItVals;

typedef struct DFigObj
{
  gchar     *name;        /* Trailing name of file  */
  gchar     *filename;    /* Filename itself */
  gchar     *draw_name;   /* Name of the drawing */
  gfloat     version;     /* Version number of data file */
  GfigOpts   opts;        /* Options enforced when fig saved */
  DAllObjs  *obj_list;    /* Objects that make up this list */
  gint       obj_status;  /* See above for possible values */
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *pixmap_widget;
} GFigObj;  

extern GFigObj  *current_obj;
extern GFigObj  *pic_obj;

extern selection_option selopt;
extern SelectItVals selvals;

void       add_to_all_obj          (GFigObj * fobj, Dobject *obj);

gchar *get_line (gchar *buf,
		 gint   s,
		 FILE  *from,
		 gint   init);

void		scale_to_xy	(gdouble *list,
				 gint     size);
void		scale_to_original_xy (gdouble *list,
				      gint     size);

void reverse_pairs_list (gdouble *list,
			 gint     size);

void gfig_paint (BrushType brush_type,
		 gint32    drawable_ID,
		 gint      seg_count,
		 gdouble   line_pnts[]);

void draw_circle (GdkPoint *p);
void draw_sqr (GdkPoint *p);

void    list_button_update      (GFigObj *obj);
void	num_sides_dialog 	(gchar *d_title,
				 gint  *num_sides,
				 gint  *which_way,
				 gint   adj_min,
				 gint   adj_max);
void	toggle_obj_type		(GtkWidget *widget,
				 gpointer   data);

void	setup_undo              (void);
void	draw_grid_clear		(void);
void	prepend_to_all_obj      (GFigObj *fobj, 
				 DAllObjs *nobj);

void	gfig_draw_arc		(gint x, 
				 gint y, 
				 gint width, 
				 gint height, 
				 gint angle1, 
				 gint angle2);

void	gfig_draw_line		(gint x0, 
				 gint y0, 
				 gint x1, 
				 gint y1);

#endif /* __GFIG_H__ */
