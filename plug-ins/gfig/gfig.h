/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_H__
#define __GFIG_H__

#include "gfig-types.h"
#include "gfig-style.h"

#define MAX_UNDO         10
#define MIN_UNDO         1

struct _GimpGfig
{
  GimpPlugIn      parent_instance;
  GtkApplication *app;

  GtkWidget      *top_level_dlg;
  gboolean        success;

  GtkBuilder     *builder;
};

#define PLUG_IN_PROC   "plug-in-gfig"
#define PLUG_IN_BINARY "gfig"
#define PLUG_IN_ROLE   "gimp-gfig"

#define GIMP_TYPE_GFIG  (gimp_gfig_get_type ())
G_DECLARE_FINAL_TYPE (GimpGfig, gimp_gfig, GIMP, GFIG, GimpPlugIn)

typedef struct
{
  gint     gridspacing;
  GridType gridtype;
  gboolean drawgrid;
  gboolean snap2grid;
  gboolean lockongrid;
  gboolean showcontrol;
  gdouble grid_radius_min;
  gdouble grid_radius_interval;
  gdouble grid_rotation;
  gdouble grid_granularity;
  gint grid_sectors_desired;
} GfigOpts;

typedef struct
{
  GimpChannelOps type;           /* ADD etc .. */
  gint           antia;          /* Boolean for Antia */
  gint           feather;        /* Feather it ? */
  gdouble        feather_radius; /* Radius to feather */
  ArcType        as_pie;         /* Arc type selection segment/sector */
  FillType       fill_type;      /* Fill type for selection */
  gdouble        fill_opacity;   /* You can guess this one */
} selection_option;

void       object_start            (GdkPoint *pnt, gint);
void       object_operation        (GdkPoint *pnt, gint);
void       object_operation_start  (GimpGfig *gfig,
                                    GdkPoint *pnt,
                                    gint shift_down);
void       object_operation_end    (GdkPoint *pnt, gint);
void       object_end              (GimpGfig *gfig,
                                    GdkPoint *pnt,
                                    gint shift_down);

#define MAX_LOAD_LINE    256
#define SQ_SIZE 8

extern gint       line_no;
extern gint       preview_width, preview_height;
extern gint       need_to_scale;
extern gdouble    scale_x_factor, scale_y_factor;
extern GdkPixbuf *back_pixbuf;

extern GtkWidget    *pic_preview;
extern gint          obj_show_single;

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
  BrushType     brshtype;
  DobjType      otype;
} SelectItVals;

struct _GFigObj
{
  gchar     *name;        /* Trailing name of file  */
  gchar     *filename;    /* Filename itself */
  gchar     *draw_name;   /* Name of the drawing */
  gfloat     version;     /* Version number of data file */
  GfigOpts   opts;        /* Options enforced when fig saved */
  GList     *obj_list;    /* Objects that make up this list */
  gint       obj_status;  /* See above for possible values */
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *pixmap_widget;
};

/* this is temp, should be able to get rid of */
typedef struct BrushDesc
{
  GimpBrush     *brush;
  gdouble        opacity;
  gint           spacing;
  GimpLayerMode  paint_mode;
  gint           width;
  gint           height;
  guchar        *pv_buf;  /* Buffer where brush placed */
  gint16         x_off;
  gint16         y_off;
  const gchar   *popup;
} BrushDesc;

typedef struct
{
  gboolean     debug_styles;
  gboolean     show_background;  /* show thumbnail of image behind figure */
  GimpImage    *image;           /* Gimp image */
  GimpDrawable *drawable;        /* Gimp drawable to paint on */
  GFigObj     *current_obj;
  GfigObject  *selected_obj;
  GtkWidget   *preview;
  Style        default_style;
  BrushDesc    bdesc;
  GtkWidget   *fg_color_button;
  GtkWidget   *bg_color_button;
  GtkWidget   *brush_select;
  GtkWidget   *pattern_select;
  GtkWidget   *gradient_select;
  GtkWidget   *fillstyle_combo;
  GtkWidget   *paint_type_toggle;
  GeglColor   *fg_color;
  GeglColor   *bg_color;
  gboolean     enable_repaint;
  gboolean     using_new_layer;
} GFigContext;

extern GFigContext *gfig_context;

extern selection_option selopt;
extern SelectItVals selvals;

void       add_to_all_obj          (GimpGfig   *gfig,
                                    GFigObj    *fobj,
                                    GfigObject *obj);

gchar *get_line (gchar *buf,
                 gint   s,
                 FILE  *from,
                 gint   init);

gint            gfig_scale_x    (gint      x);
gint            gfig_scale_y    (gint      y);
void            scale_to_xy     (gdouble *list,
                                 gint     size);
void            scale_to_original_xy (gdouble *list,
                                      gint     size);

void gfig_paint (BrushType     brush_type,
                 GimpDrawable *drawable,
                 gint          seg_count,
                 gdouble       line_pnts[]);

void draw_item   (cairo_t *cr,
                  gboolean fill);
void draw_circle (GdkPoint *p,
                  gboolean  selected,
                  cairo_t  *cr);
void draw_sqr    (GdkPoint *p,
                  gboolean  selected,
                  cairo_t  *cr);

void       list_button_update   (GFigObj *obj);

GtkWidget *num_sides_widget     (const gchar *d_title,
                                 gint        *num_sides,
                                 gint        *which_way,
                                 gint         adj_min,
                                 gint         adj_max);

void    setup_undo              (GimpGfig    *gfig);
void    draw_grid_clear         (void);
void    prepend_to_all_obj      (GimpGfig    *gfig,
                                 GFigObj     *fobj,
                                 GList       *nobj);

void    gfig_draw_arc           (gint x,
                                 gint y,
                                 gint width,
                                 gint height,
                                 gint angle1,
                                 gint angle2,
                                 cairo_t *cr);

void    gfig_draw_line          (gint x0,
                                 gint y0,
                                 gint x1,
                                 gint y1,
                                 cairo_t *cr);

void      gfig_paint_callback   (void);
GFigObj  *gfig_load             (GimpGfig    *gfig,
                                 const gchar *filename,
                                 const gchar *name);
void   gfig_name_encode         (gchar *dest,
                                 gchar *src);
void   gfig_name_decode         (gchar       *dest,
                                 const gchar *src);

gint   gfig_list_pos            (GFigObj *gfig);
gint   gfig_list_insert         (GFigObj *gfig);
void   gfig_free                (GFigObj *gfig);

void   save_options             (GString *string);

GString   *gfig_save_as_string     (void);
gboolean   gfig_save_as_parasite   (void);
GFigObj   *gfig_load_from_parasite (GimpGfig *gfig);
GFigObj   *gfig_new                (void);
void       gfig_save_callbk        (void);
void       paint_layer_fill        (gdouble x1,
                                    gdouble y1,
                                    gdouble x2,
                                    gdouble y2);



extern GtkWidget    *top_level_dlg;
extern GList        *gfig_list;
extern gdouble       org_scale_x_factor, org_scale_y_factor;

#endif /* __GFIG_H__ */
