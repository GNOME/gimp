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

#ifndef __GIMPRESSIONIST_H
#define __GIMPRESSIONIST_H

/* Includes necessary for the correct processing of this file. */
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "ppmtool.h"
/* Defines */

#define PLUG_IN_PROC    "plug-in-gimpressionist"
#define PLUG_IN_VERSION "v1.0, November 2003"
#define PLUG_IN_BINARY  "gimpressionist"
#define PLUG_IN_ROLE    "gimp-gimpressionist"

#define PREVIEWSIZE     150
#define MAXORIENTVECT   50
#define MAXSIZEVECT     50

/* Type declaration and definitions */

typedef struct vector
{
  double x, y;
  double dir;
  double dx, dy;
  double str;
  int    type;
} vector_t;

typedef struct smvector
{
  double x, y;
  double siz;
  double str;
} smvector_t;

typedef struct
{
  int        orient_num;
  double     orient_first;
  double     orient_last;
  int        orient_type;
  double     brush_relief;
  double     brush_scale;
  double     brush_density;
  double     brushgamma;
  int        general_background_type;
  double     general_dark_edge;
  double     paper_relief;
  double     paper_scale;
  int        paper_invert;
  int        run;
  char       selected_brush[200];
  char       selected_paper[200];
  GimpRGB    color;
  int        general_paint_edges;
  int        place_type;
  vector_t   orient_vectors[MAXORIENTVECT];
  int        num_orient_vectors;
  int        placement_center;
  double     brush_aspect;
  double     orient_angle_offset;
  double     orient_strength_exponent;
  int        general_tileable;
  int        paper_overlay;
  int        orient_voronoi;
  int        color_brushes;
  int        general_drop_shadow;
  double     general_shadow_darkness;
  int        size_num;
  double     size_first;
  double     size_last;
  int        size_type;
  double     devthresh;

  smvector_t size_vectors[MAXSIZEVECT];
  int        num_size_vectors;
  double     size_strength_exponent;
  int        size_voronoi;

  int        general_shadow_depth;
  int        general_shadow_blur;

  int        color_type;
  double     color_noise;
} gimpressionist_vals_t;

/* Enumerations */

enum GENERAL_BG_TYPE_ENUM
{
    BG_TYPE_SOLID = 0,
    BG_TYPE_KEEP_ORIGINAL = 1,
    BG_TYPE_FROM_PAPER = 2,
    BG_TYPE_TRANSPARENT = 3,
};

enum PRESETS_LIST_COLUMN_ENUM
{
  PRESETS_LIST_COLUMN_FILENAME = 0,
  PRESETS_LIST_COLUMN_OBJECT_NAME = 1,
};

/* Globals */

extern gimpressionist_vals_t pcvals;


/* Prototypes */

GList *parsepath (void);
void free_parsepath_cache (void);

void grabarea (void);
void store_values (void);
void restore_values (void);
gchar *findfile (const gchar *);

void unselectall (GtkWidget *list);
void reselect (GtkWidget *list, char *fname);
void readdirintolist (const char *subdir, GtkWidget *view, char *selected);
void readdirintolist_extended (const char *subdir,
                               GtkWidget *view, char *selected,
                               gboolean with_filename_column,
                               gchar *(*get_object_name_cb) (const gchar *dir,
                                                             gchar *filename,
                                                             void *context),
                               void * context);

GtkWidget *create_one_column_list (GtkWidget *parent,
			           void (*changed_cb)
			           (GtkTreeSelection *selection,
                                    gpointer data));

void brush_reload (const gchar *fn, struct ppm *p);

double get_direction (double x, double y, int from);

void create_sizemap_dialog (GtkWidget *parent);
double getsiz_proto (double x, double y, int n, smvector_t *vec,
                     double smstrexp, int voronoi);


void set_colorbrushes (const gchar *fn);
int  create_gimpressionist (void);

double dist (double x, double y, double dx, double dy);

void restore_default_values (void);

GtkWidget *create_radio_button (GtkWidget *box, int orient_type,
                                void (*callback)(GtkWidget *wg, void *d),
                                const gchar *label,
                                const gchar *help_string,
                                GSList **radio_group,
                                GtkWidget **buttons_array
                               );

#define CLAMP_UP_TO(x, max) (CLAMP((x),(0),(max-1)))

#endif /* #ifndef __GIMPRESSIONIST_H */


