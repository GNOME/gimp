
#ifndef __GIMPRESSIONIST_H
#define __GIMPRESSIONIST_H

/* Includes necessary for the correct processing of this file. */
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "ppmtool.h"
/* Defines */

#define PLUG_IN_NAME    "plug_in_gimpressionist"
#define PLUG_IN_VERSION "v1.0, November 2003"
#define HELP_ID         "plug-in-gimppressionist"

#define PREVIEWSIZE     150
#define MAXORIENTVECT   50
#define MAXSIZEVECT     50

#define NUMORIENTRADIO 8
#define NUMSIZERADIO 8

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
  int        orientnum;
  double     orientfirst;
  double     orientlast;
  int        orienttype;
  double     brushrelief;
  double     brushscale;
  double     brushdensity;
  double     brushgamma;
  int        generalbgtype;
  double     generaldarkedge;
  double     paperrelief;
  double     paperscale;
  int        paper_invert;
  int        run;
  char       selectedbrush[200];
  char       selectedpaper[200];
  GimpRGB    color;
  int        general_paint_edges;
  int        placetype;
  vector_t   orientvector[MAXORIENTVECT];
  int        numorientvector;
  int        placement_center;
  double     brushaspect;
  double     orientangoff;
  double     orientstrexp;
  int        general_tileable;
  int        paper_overlay;
  int        orient_voronoi;
  int        colorbrushes;
  int        general_drop_shadow;
  double     generalshadowdarkness;
  int        sizenum;
  double     sizefirst;
  double     sizelast;
  int        sizetype;
  double     devthresh;

  smvector_t sizevector[MAXSIZEVECT];
  int        numsizevector;
  double     sizestrexp;
  int        sizevoronoi;

  int        general_shadow_depth;
  int        general_shadow_blur;

  int        colortype;
  double     colornoise;
} gimpressionist_vals_t;

/* Enumerations */

enum GENERAL_BG_TYPE_ENUM
{
    BG_TYPE_SOLID = 0,
    BG_TYPE_KEEP_ORIGINAL = 1,
    BG_TYPE_FROM_PAPER = 2,
    BG_TYPE_TRANSPARENT = 3,
};

enum ORIENTATION_ENUM
{
    ORIENTATION_VALUE = 0,
    ORIENTATION_RADIUS = 1,
    ORIENTATION_RANDOM = 2,
    ORIENTATION_RADIAL = 3,
    ORIENTATION_FLOWING = 4,
    ORIENTATION_HUE = 5,
    ORIENTATION_ADAPTIVE = 6,
    ORIENTATION_MANUAL = 7,
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

void create_orientationpage (GtkNotebook *);

void grabarea (void);
void store_values (void);
void restore_values (void);
gchar *findfile (const gchar *);

void unselectall (GtkWidget *list);
void reselect (GtkWidget *list, char *fname);
void readdirintolist (char *subdir, GtkWidget *view, char *selected);
void readdirintolist_extended (char *subdir, GtkWidget *view, char *selected,
                               gboolean with_filename_column,
                               gchar *(*get_object_name_cb)
                               (gchar *dir, gchar *filename, void *context),
                               void * context);
void orientation_restore (void);

GtkWidget *create_one_column_list (GtkWidget *parent,
			           void (*changed_cb)
			           (GtkTreeSelection *selection, 
                                    gpointer data));

void brush_reload (const gchar *fn, struct ppm *p);

void create_orientmap_dialog (void);
void update_orientmap_dialog (void);
double get_direction (double x, double y, int from);

void create_sizemap_dialog (void);
double getsiz_proto (double x, double y, int n, smvector_t *vec,
                     double smstrexp, int voronoi);


void set_colorbrushes (const gchar *fn);
int  create_gimpressionist (void);

double dist (double x, double y, double dx, double dy);

void restore_default_values (void);

GtkWidget *create_radio_button (GtkWidget *box, int orienttype,
                                void (*callback)(GtkWidget *wg, void *d),
                                gchar *label, gchar *help_string,
                                GSList **radio_group, 
                                GtkWidget **buttons_array
                               );

#endif /* #ifndef __GIMPRESSIONIST_H */

