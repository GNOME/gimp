#include <gtk/gtk.h>

#include "libgimp/gimpmath.h"

#define PLUG_IN_NAME "plug_in_gimpressionist"
#define PLUG_IN_VERSION "v0.99.6, August 1999"

#ifndef DEFAULTPATH
#define DEFAULTPATH "~/.gimp/gimpressionist:/usr/local/share/gimp/gimpressionist"
#endif

#define PREVIEWSIZE 150
#define MAXORIENTVECT 50
#define MAXSIZEVECT 50

/* Type declaration and definitions */

struct vector_t {
  double x, y;
  double dir;
  double dx, dy;
  double str;
  int type;
};

struct smvector_t {
  double x, y;
  double siz;
  double str;
};

typedef struct {
  int orientnum;
  double orientfirst;
  double orientlast;
  int orienttype;
  double brushrelief;
  double brushscale;
  double brushdensity;
  double brushgamma;
  int generalbgtype;
  double generaldarkedge;
  double paperrelief;
  double paperscale;
  int paperinvert;
  int run;
  char selectedbrush[100];
  char selectedpaper[100];
  guchar color[3];
  int generalpaintedges;
  int placetype;
  struct vector_t orientvector[MAXORIENTVECT];
  int numorientvector;
  int placecenter;
  double brushaspect;
  double orientangoff;
  double orientstrexp;
  int generaltileable;
  int paperoverlay;
  int orientvoronoi;
  int colorbrushes;
  int generaldropshadow;
  double generalshadowdarkness;
  int sizenum;
  double sizefirst;
  double sizelast;
  int sizetype;
  double devthresh;

  struct smvector_t sizevector[MAXSIZEVECT];
  int numsizevector;
  double sizestrexp;
  int sizevoronoi;

  int generalshadowdepth;
  int generalshadowblur;

  int coloracc;
} gimpressionist_vals_t;

/* Globals */

extern GtkTooltips *tooltips;

extern char *standalone;

extern unsigned char logobuffer[];

extern gimpressionist_vals_t pcvals;
extern gimpressionist_vals_t defaultpcvals;
extern char *path;
extern struct ppm infile;
extern struct ppm inalpha;
extern GtkWidget *window;
extern GtkWidget *omwindow;

extern int brushfile;
extern struct ppm brushppm;

extern GtkWidget *brushlist;
extern GtkObject *brushscaleadjust;
extern GtkObject *brushaspectadjust;
extern GtkObject *brushreliefadjust;
extern GtkObject *brushdensityadjust;
extern GtkObject *brushgammaadjust;

extern GtkWidget *paperlist;
extern GtkObject *paperscaleadjust;
extern GtkObject *paperreliefadjust;
extern GtkWidget *paperinvert;
extern GtkWidget *paperoverlay;

extern GtkObject *orientnumadjust;
extern GtkObject *orientfirstadjust;
extern GtkObject *orientlastadjust;
extern int orientationtype;
extern GtkWidget *orientradio[];

extern GtkWidget *sizeradio[];

extern GtkObject *sizenumadjust;
extern GtkObject *sizefirstadjust;
extern GtkObject *sizelastadjust;

extern GtkObject *generaldarkedgeadjust;
extern int generalbgtype;
extern GtkWidget *generalpaintedges;
extern GtkWidget *generaltileable;
extern GtkWidget *generaldropshadow;
extern GtkObject *generalshadowadjust;
extern GtkObject *generalshadowdepth;
extern GtkObject *generalshadowblur;
extern GtkObject *devthreshadjust;
extern GtkObject *coloraccadjust;

extern GtkWidget *placecenter;

extern GtkWidget *previewbutton;

extern GtkWidget *presetsavebutton;

extern gint img_has_alpha;

/* Prototypes */

GList *parsepath(void);

int create_dialog(void);

void create_paperpage(GtkNotebook *);
void create_brushpage(GtkNotebook *);
void create_orientationpage(GtkNotebook *);
void create_sizepage(GtkNotebook *);
void create_generalpage(GtkNotebook *);
void create_presetpage(GtkNotebook *);
void create_placementpage(GtkNotebook *);

GtkWidget* create_preview();
void updatepreviewprev(GtkWidget *wg, void *d);

void grabarea(void);
void storevals(void);
void restorevals(void);
char *findfile(char *);

void unselectall(GtkWidget *list);
void reselect(GtkWidget *list, char *fname);
void readdirintolist(char *subdir, GtkWidget *list, char *selected);
void drawcolor(GtkWidget *w);
void orientchange(GtkWidget *wg, void *d, int num);
void sizechange(GtkWidget *wg, void *d, int num);
void placechange(GtkWidget *wg, void *d, int num);
void generalbgchange(GtkWidget *wg, void *d, int num);

void reloadbrush(char *fn, struct ppm *p);

void create_orientmap_dialog(void);
void update_orientmap_dialog(void);
int pixval(double dir);
double getdir(double x, double y, int from);

void create_sizemap_dialog(void);
double getsiz(double x, double y, int from);
