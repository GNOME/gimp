struct rgbcolor {
  unsigned char r, g, b;
};

struct ppm {
  int width;
  int height;
  struct rgbcolor **col;
};

void fatal(char *s);
void *safemalloc(int len);
void killppm(struct ppm *p);
void newppm(struct ppm *p, int xs, int ys);
void getrgb(struct ppm *s, float xo, float yo, struct rgbcolor *d);
void resize(struct ppm *p, int nx, int ny);
void rescale(struct ppm *p, double scale);
void resize_fast(struct ppm *p, int nx, int ny);
void loadppm(char *fn, struct ppm *p);
void saveppm(struct ppm *p, char *fn);
void copyppm(struct ppm *s, struct ppm *p);
void fill(struct ppm *p, struct rgbcolor *c);
void freerotate(struct ppm *p, double amount);
void pad(struct ppm *p, int left,int right, int top, int bottom, struct rgbcolor *bg);
void edgepad(struct ppm *p, int left,int right, int top, int bottom);
void autocrop(struct ppm *p, int room);
void crop(struct ppm *p, int lx, int ly, int hx, int hy);
void ppmgamma(struct ppm *p, float e, int r, int g, int b);
void ppmbrightness(struct ppm *p, float e, int r, int g, int b);
void putrgb_fast(struct ppm *s, float xo, float yo, struct rgbcolor *d);
void putrgb(struct ppm *s, float xo, float yo, struct rgbcolor *d);
void drawline(struct ppm *p, float fx, float fy, float tx, float ty, struct rgbcolor *col);

void repaint(struct ppm *p, struct ppm *a);

void blur(struct ppm *p, int xrad, int yrad);

void mkgrayplasma(struct ppm *p, float turb);
