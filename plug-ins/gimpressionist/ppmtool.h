#include <glib.h>

typedef struct ppm {
  int width;
  int height;
  unsigned char *col;
} ppm_t;

void fatal(char *s);
void killppm(ppm_t *p);
void newppm(ppm_t *p, int xs, int ys);
void getrgb(ppm_t *s, float xo, float yo, unsigned char *d);
void resize(ppm_t *p, int nx, int ny);
void rescale(ppm_t *p, double scale);
void resize_fast(ppm_t *p, int nx, int ny);
void loadppm(const char *fn, ppm_t *p);
void saveppm(ppm_t *p, const char *fn);
void copyppm(ppm_t *s, ppm_t *p);
void fill(ppm_t *p, guchar *c);
void freerotate(ppm_t *p, double amount);
void pad(ppm_t *p, int left,int right, int top, int bottom, guchar *);
void edgepad(ppm_t *p, int left,int right, int top, int bottom);
void autocrop(ppm_t *p, int room);
void crop(ppm_t *p, int lx, int ly, int hx, int hy);
void ppmgamma(ppm_t *p, float e, int r, int g, int b);
void ppmbrightness(ppm_t *p, float e, int r, int g, int b);
void putrgb_fast(ppm_t *s, float xo, float yo, guchar *d);
void putrgb(ppm_t *s, float xo, float yo, guchar *d);
void drawline(ppm_t *p, float fx, float fy, float tx, float ty, guchar *col);

void repaint(ppm_t *p, ppm_t *a);

void blur(ppm_t *p, int xrad, int yrad);

void mkgrayplasma(ppm_t *p, float turb);
