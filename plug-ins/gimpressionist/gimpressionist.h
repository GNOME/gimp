
#define PLUG_IN_NAME    "plug_in_gimpressionist"
#define PLUG_IN_VERSION "v1.0, November 2003"
#define HELP_ID         "plug-in-gimppressionist"

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
  int        paperinvert;
  int        run;
  char       selectedbrush[200];
  char       selectedpaper[200];
  GimpRGB    color;
  int        generalpaintedges;
  int        placetype;
  vector_t   orientvector[MAXORIENTVECT];
  int        numorientvector;
  int        placecenter;
  double     brushaspect;
  double     orientangoff;
  double     orientstrexp;
  int        generaltileable;
  int        paperoverlay;
  int        orientvoronoi;
  int        colorbrushes;
  int        generaldropshadow;
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

  int        generalshadowdepth;
  int        generalshadowblur;

  int        colortype;
  double     colornoise;
} gimpressionist_vals_t;

/* Globals */

extern char *standalone;

extern gimpressionist_vals_t pcvals;
extern gimpressionist_vals_t defaultpcvals;
extern char *path;
extern struct ppm infile;
extern struct ppm inalpha;
extern GtkWidget *window;

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
extern GtkWidget *generalcolbutton;
extern GtkObject *generalshadowadjust;
extern GtkObject *generalshadowdepth;
extern GtkObject *generalshadowblur;
extern GtkObject *devthreshadjust;

extern GtkWidget *colortype;
extern GtkObject *colornoiseadjust;

extern GtkWidget *placecenter;

extern GtkWidget *previewbutton;

extern GtkWidget *presetsavebutton;

extern gboolean img_has_alpha;

extern GRand *gr;

/* Prototypes */

GList *parsepath(void);

void create_paperpage(GtkNotebook *);
void create_brushpage(GtkNotebook *);
void create_orientationpage(GtkNotebook *);
void create_sizepage(GtkNotebook *);
void create_generalpage(GtkNotebook *);
void create_presetpage(GtkNotebook *);
void create_placementpage(GtkNotebook *);
void create_colorpage(GtkNotebook *);

GtkWidget* create_preview (void);
void       updatepreview  (GtkWidget *wg, gpointer d);

void grabarea(void);
void storevals(void);
void restorevals(void);
gchar *findfile(const gchar *);

void unselectall(GtkWidget *list);
void reselect(GtkWidget *list, char *fname);
void readdirintolist(char *subdir, GtkWidget *view, char *selected);
void orientchange(GtkWidget *wg, void *d, int num);
void sizechange(GtkWidget *wg, void *d, int num);
void placechange(int num);
void colorchange(int num);
void generalbgchange(GtkWidget *wg, void *d, int num);

GtkWidget *createonecolumnlist(GtkWidget *parent,
			       void (*changed_cb)
			       (GtkTreeSelection *selection, gpointer data));

void reloadbrush(const gchar *fn, struct ppm *p);

void create_orientmap_dialog(void);
void update_orientmap_dialog(void);
int pixval(double dir);
double getdir(double x, double y, int from);

void create_sizemap_dialog(void);
double getsiz(double x, double y, int from);
