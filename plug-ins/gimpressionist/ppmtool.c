#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ppmtool.h"
#include "gimpressionist.h"

int readline(FILE *f, char *buffer, int len)
{
again:
  if(!fgets(buffer, len, f))
    return -1;
  if(*buffer == '#') {
    goto again;
  }
  while(strlen(buffer) && buffer[strlen(buffer)-1] <= ' ')
    buffer[strlen(buffer)-1] = '\0';
  return 0;
}

void fatal(char *s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

void *safemalloc(int len)
{
  void *p = g_malloc(len);
  if(!p) {
    fprintf(stderr, "(When allocating %u bytes.)\n", len);
    fatal("Out of memory!\n");
  }
  return p;
}

void killppm(struct ppm *p)
{
  int y;
  for(y = 0; y < p->height; y++) {
    free(p->col[y]);
  }
  free(p->col);
  p->col = NULL;
  p->height = p->width = 0;
}


void newppm(struct ppm *p, int xs, int ys)
{
  int x,y;
  struct rgbcolor bgcol = {0,0,0};

#ifdef DEBUG
  if((xs < 1) || (ys < 1)) {
    fprintf(stderr, "Illegal size (%dx%d) specified!%c\n",xs, ys, 7);
  }
#endif

  if(xs < 1)
    xs = 1;
  if(ys < 1)
    ys = 1;

  p->width = xs;
  p->height = ys;
  p->col = (struct rgbcolor **)safemalloc(p->height * sizeof(struct rgbcolor *));
  for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y] = (struct rgbcolor *)safemalloc(p->width * sizeof(struct rgbcolor));
    for(x = 0; x < p->width; x++) {
      row[x].r = bgcol.r;
      row[x].g = bgcol.g;
      row[x].b = bgcol.b;
    }
  }
}

void getrgb(struct ppm *s, float xo, float yo, struct rgbcolor *d)
{
  float ix, iy;
  int x1, x2, y1, y2;
  float x1y1, x2y1, x1y2, x2y2;
  float r, g, b;
  int bail = 0;

  if(xo < 0.0) bail=1;
  else if(xo >= s->width-1) { xo = s->width-1; } /* bail=1; */
  if(yo < 0.0) bail=1;
  else if(yo >= s->height-1) { yo= s->height-1; } /* bail=1; */

  if(bail) {
    d->r = d->g = d->b = 0;
    return;
  }

  ix = (int)xo;
  iy = (int)yo;

  /*
  x1 = wrap(ix, s->width);
  x2 = wrap(ix+1, s->width);
  y1 = wrap(iy, s->height);
  y2 = wrap(iy+1, s->height);
  */
  x1 = ix; x2 = ix + 1;
  y1 = iy; y2 = iy + 1;

  /* printf("x1=%d y1=%d x2=%d y2=%d\n",x1,y1,x2,y2); */

  x1y1 = (1.0-xo+ix)*(1.0-yo+iy);
  x2y1 = (xo-ix)*(1.0-yo+iy);
  x1y2 = (1.0-xo+ix)*(yo-iy);
  x2y2 = (xo-ix)*(yo-iy);

  r = s->col[y1][x1].r * x1y1;
  if(x2y1 > 0.0)
    r += s->col[y1][x2].r * x2y1;
  if(x1y2 > 0.0)
    r += s->col[y2][x1].r * x1y2;
  if(x2y2 > 0.0)
    r += s->col[y2][x2].r * x2y2;

  g = s->col[y1][x1].g * x1y1;
  if(x2y1 > 0.0)
    g += s->col[y1][x2].g * x2y1;
  if(x1y2 > 0.0)
    g += s->col[y2][x1].g * x1y2;
  if(x2y2 > 0.0)
    g += s->col[y2][x2].g * x2y2;

  b = s->col[y1][x1].b * x1y1;
  if(x2y1 > 0.0)
    b += s->col[y1][x2].b * x2y1;
  if(x1y2 > 0.0)
    b += s->col[y2][x1].b * x1y2;
  if(x2y2 > 0.0)
    b += s->col[y2][x2].b * x2y2;

  d->r = r;
  d->g = g;
  d->b = b;

}


void resize(struct ppm *p, int nx, int ny)
{
  int x, y;
  float xs = p->width/(float)nx;
  float ys = p->height/(float)ny;
  struct ppm tmp = {0,0,NULL};

  newppm(&tmp, nx, ny);
  for(y = 0; y < ny; y++) {
    for(x = 0; x < nx; x++) {
      getrgb(p, x*xs, y*ys, &tmp.col[y][x]);
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void rescale(struct ppm *p, double sc)
{
  resize(p, p->width * sc, p->height * sc);
}

void resize_fast(struct ppm *p, int nx, int ny)
{
  int x, y;
  float xs = p->width/(float)nx;
  float ys = p->height/(float)ny;
  struct ppm tmp = {0,0,NULL};

  newppm(&tmp, nx, ny);
  for(y = 0; y < ny; y++) {
    for(x = 0; x < nx; x++) {
      int rx = x*xs, ry = y*ys;
      memcpy(&tmp.col[y][x], &p->col[ry][rx], 3);
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}


struct _BrushHeader
{
  unsigned int   header_size; /*  header_size = sz_BrushHeader + brush name  */
  unsigned int   version;     /*  brush file version #  */
  unsigned int   width;       /*  width of brush  */
  unsigned int   height;      /*  height of brush  */
  unsigned int   bytes;       /*  depth of brush in bytes--always 1 */
  unsigned int   magic_number;/*  GIMP brush magic number  */
  unsigned int   spacing;     /*  brush spacing  */
};

void msb2lsb(unsigned int *i)
{
  unsigned char *p = (unsigned char *)i, c;
  c = p[1]; p[1] = p[2]; p[2] = c;
  c = p[0]; p[0] = p[3]; p[3] = c;
}

void loadgbr(char *fn, struct ppm *p)
{
  FILE *f;
  struct _BrushHeader hdr;
  unsigned char *ptr;
  int x, y;

  f = fopen(fn, "rb");
  if(!f) {
    ptr = findfile(fn);
    f = fopen(ptr, "rb");
  }

  if(p->col) killppm(p);

  if(!f) {
    fprintf(stderr, "loadgbr: Unable to open file \"%s\"!\n", fn);
    newppm(p, 10,10);
    return;
  }

  fread(&hdr, 1, sizeof(struct _BrushHeader), f);

  for(x = 0; x < 7; x++)
    msb2lsb(&((unsigned int *)&hdr)[x]);

  newppm(p, hdr.width, hdr.height);

  ptr = safemalloc(hdr.width);
  fseek(f, hdr.header_size, SEEK_SET);
  for(y = 0; y < p->height; y++) {
    fread(ptr, p->width, 1, f);
    for(x = 0; x < p->width; x++) {
      p->col[y][x].r = p->col[y][x].g = p->col[y][x].b = ptr[x];
    }
  }
  fclose(f);
  free(ptr);
}

void loadppm(char *fn, struct ppm *p)
{
  char line[200];
  int x,y, pgm = 0, c;
  FILE *f;

  if(!strcmp(&fn[strlen(fn)-4], ".gbr")) {
    loadgbr(fn, p);
    return;
  }

  f = fopen(fn, "rb");
  if(!f) f = fopen(findfile(fn), "rb");

  if(p->col) killppm(p);

  if(!f) {
    fprintf(stderr, "loadppm: Unable to open file \"%s\"!\n", fn);
    newppm(p, 10,10);
    return;
    /* fatal("Aborting!"); */
  }

  readline(f, line, 200);
  if(strcmp(line, "P6")) {
    if(strcmp(line, "P5")) {
      fclose(f);
      printf("loadppm: File \"%s\" not PPM/PGM? (line=\"%s\")%c\n", fn, line, 7);
      newppm(p, 10,10);
      return;
      /* fatal("Aborting!"); */
    }
    pgm = 1;
  }
  readline(f, line, 200);
  p->width = atoi(line);
  p->height = atoi(strchr(line, ' ')+1);
  readline(f, line, 200);
  if(strcmp(line, "255")) {
    printf("loadppm: File \"%s\" not valid PPM/PGM? (line=\"%s\")%c\n", fn, line, 7);
    newppm(p, 10,10);
    return;
    /* fatal("Aborting!"); */
  }
  p->col = (struct rgbcolor **)safemalloc(p->height * sizeof(struct rgbcolor *));

  if(!pgm)
    for(y = 0; y < p->height; y++) {
      p->col[y] = (struct rgbcolor *)safemalloc(p->width * sizeof(struct rgbcolor));
      fread(p->col[y], p->width * sizeof(struct rgbcolor), 1, f);
  } else /* if pgm */ {
    for(y = 0; y < p->height; y++) {
      p->col[y] = (struct rgbcolor *)safemalloc(p->width * sizeof(struct rgbcolor));
      fread(p->col[y], p->width, 1, f);
      for(x = p->width-1; x>=0; x--) {
	c = *((unsigned char *)(p->col[y])+x);
	p->col[y][x].r = p->col[y][x].g = p->col[y][x].b = c;
      }
    }
  }
  fclose(f);
}

void fill(struct ppm *p, struct rgbcolor *c)
{
  int x, y;
 
  if((c->r == c->g) && (c->r == c->b)) {
    unsigned char col = c->r;
    for(y = 0; y < p->height; y++) {
      memset(p->col[y], col, p->width*3);
    }
  } else {
    for(y = 0; y < p->height; y++) {
      struct rgbcolor *row = p->col[y];
      for(x = 0; x < p->width; x++) {
	row[x].r = c->r;
	row[x].g = c->g;
	row[x].b = c->b;
      }
    }
  }
}

void copyppm(struct ppm *s, struct ppm *p)
{
  int y;
  if(p->col)
    killppm(p);
  p->width = s->width;
  p->height = s->height;
  p->col = (struct rgbcolor **)safemalloc(p->height * sizeof(struct rgbcolor *));
  for(y = 0; y < p->height; y++) {
    p->col[y] = (struct rgbcolor *)safemalloc(p->width * sizeof(struct rgbcolor));
    memcpy(p->col[y], s->col[y], p->width * sizeof(struct rgbcolor));
  }
}

void freerotate(struct ppm *p, double amount)
{
  int x, y;
  double nx, ny;
  double R, a;
  struct ppm tmp = {0,0,NULL};
  double f = amount*M_PI*2/360.0;

  a = p->width/(float)p->height;
  R = p->width<p->height?p->width/2:p->height/2;

  newppm(&tmp, p->width, p->height);
  for(y = 0; y < p->height; y++) {
    for(x = 0; x < p->width; x++) {
      double r, d;
      nx = fabs(x-p->width/2.0);
      ny = fabs(y-p->height/2.0);
      r = sqrt(nx*nx + ny*ny);

      d = atan2((y-p->height/2.0),(x-p->width/2.0));

      nx = (p->width/2.0 + cos(d-f) * r);
      ny = (p->height/2.0 + sin(d-f) * r);
      getrgb(p, nx, ny, &tmp.col[y][x]);
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void crop(struct ppm *p, int lx, int ly, int hx, int hy)
{
  struct ppm tmp = {0,0,NULL};
  int x, y;

  newppm(&tmp, hx-lx, hy-ly);
  for(y = ly; y < hy; y++)
    for(x = lx; x < hx; x++)
      memcpy(&tmp.col[y-ly][x-lx], &p->col[y][x], sizeof(struct rgbcolor));
  killppm(p);
  p->col = tmp.col;
  p->width = tmp.width;
  p->height = tmp.height;
}

void autocrop(struct ppm *p, int room)
{
  int lx = 0, hx = p->width, ly = 0, hy = p->height;
  int x, y, n = 0;
  struct rgbcolor tc;
  struct ppm tmp = {0,0,NULL};

  /* upper */
  memcpy(&tc, &p->col[0][0], sizeof(struct rgbcolor));
  for(y = 0; y < p->height; y++) {
    n = 0;
    for(x = 0; x < p->width; x++) {
      if(memcmp(&tc, &p->col[y][x], sizeof(struct rgbcolor))) { n++; break; }
    }
    if(n) break;
  }
  if(n) ly = y;
  /* printf("ly = %d\n", ly); */

  /* lower */
  memcpy(&tc, &p->col[p->height-1][0], sizeof(struct rgbcolor));
  for(y = p->height-1; y >= 0; y--) {
    n = 0;
    for(x = 0; x < p->width; x++) {
      if(memcmp(&tc, &p->col[y][x], sizeof(struct rgbcolor))) { n++; break; }
    }
    if(n) break;
  }
  if(n) hy = y+1;
  if(hy >= p->height) hy = p->height - 1;
  /* printf("hy = %d\n", hy); */

  /* left */
  memcpy(&tc, &p->col[ly][0], sizeof(struct rgbcolor));
  for(x = 0; x < p->width; x++) {
    n = 0;
    for(y = ly; y <= hy && y < p->height; y++) {
      if(memcmp(&tc, &p->col[y][x], sizeof(struct rgbcolor))) { n++; break; }
    }
    if(n) break;
  }
  if(n) lx = x;
  /* printf("lx = %d\n", lx); */

  /* right */
  memcpy(&tc, &p->col[ly][p->width-1], sizeof(struct rgbcolor));
  for(x = p->width-1; x >= 0; x--) {
    n = 0;
    for(y = ly; y <= hy; y++) {
      if(memcmp(&tc, &p->col[y][x], sizeof(struct rgbcolor))) { n++; break; }
    }
    if(n) break;
  }
  if(n) hx = x+1;
  /* printf("hx = %d\n", hx); */

  lx -= room; if(lx<0) lx = 0;
  ly -= room; if(ly<0) ly = 0;
  hx += room; if(hx>=p->width) hx = p->width-1;
  hy += room; if(hy>=p->height) hy = p->height-1;

  newppm(&tmp, hx-lx, hy-ly);
  for(y = ly; y < hy; y++)
    for(x = lx; x < hx; x++)
      memcpy(&tmp.col[y-ly][x-lx], &p->col[y][x], sizeof(struct rgbcolor));
  killppm(p);
  p->col = tmp.col;
  p->width = tmp.width;
  p->height = tmp.height;
}

void pad(struct ppm *p, int left,int right, int top, int bottom, struct rgbcolor *bg)
{
  int x, y;
  struct ppm tmp = {0,0,NULL};

  newppm(&tmp, p->width+left+right, p->height+top+bottom);
  for(y = 0; y < tmp.height; y++) {
    struct rgbcolor *row, *srcrow;
    row = tmp.col[y];
    if((y < top) || (y >= tmp.height-bottom)) {
      for(x = 0; x < tmp.width; x++) {
        row[x].r = bg->r;
        row[x].g = bg->g;
        row[x].b = bg->b;
      }
      continue;
    }
    srcrow = p->col[y-top];
    for(x = 0; x < left; x++) {
      row[x].r = bg->r;
      row[x].g = bg->g;
      row[x].b = bg->b;
    }
    for(; x < tmp.width-right; x++) {
      tmp.col[y][x].r = srcrow[x-left].r;
      tmp.col[y][x].g = srcrow[x-left].g;
      tmp.col[y][x].b = srcrow[x-left].b;
    }
    for(; x < tmp.width; x++) {
      row[x].r = bg->r;
      row[x].g = bg->g;
      row[x].b = bg->b;
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void saveppm(struct ppm *p, char *fn)
{
  int y;
  FILE *f = fopen(fn, "wb");
  fprintf(f, "P6\n%d %d\n255\n", p->width, p->height);
  for(y = 0; y < p->height; y++)
    fwrite(p->col[y], p->width, 3, f);
  fclose(f);
}

void edgepad(struct ppm *p, int left,int right, int top, int bottom)
{
  int x,y;
  struct ppm tmp = {0,0,NULL};
  struct rgbcolor testcol = {0,255,0};

  newppm(&tmp, p->width+left+right, p->height+top+bottom);
  fill(&tmp, &testcol);
  for(y = 0; y < top; y++) {
    memcpy(&tmp.col[y][left], &p->col[0][0], p->width * sizeof(struct rgbcolor));
  }
  for(; y-top < p->height; y++) {
    memcpy(&tmp.col[y][left], &p->col[y-top][0], p->width * sizeof(struct rgbcolor));
  }
  for(; y < tmp.height; y++) {
    memcpy(&tmp.col[y][left], &p->col[p->height-1][0], p->width * sizeof(struct rgbcolor));
  }
  for(y = 0; y < tmp.height; y++) {
    struct rgbcolor *col;
    struct rgbcolor *tmprow;

    tmprow = tmp.col[y];

    col = &tmp.col[y][left];
    for(x = 0; x < left; x++) {
      memcpy(&tmprow[x], col, sizeof(struct rgbcolor));
    }
      
    col = &tmp.col[y][tmp.width-right-1];
    for(x = 0; x < right; x++) {
      memcpy(&tmprow[x+tmp.width-right-1], col, sizeof(struct rgbcolor));
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void ppmgamma(struct ppm *p, float e, int r, int g, int b)
{
  int x, y;
  unsigned char xlat[256];
  if(e > 0.0) for(x = 0; x < 256; x++) {
    xlat[x] = pow((x/255.0),(1.0/e))*255.0;
  } else if(e < 0.0) for(x = 0; x < 256; x++) {
    xlat[255-x] = pow((x/255.0),(-1.0/e))*255.0;
  } else for(x = 0; x < 256; x++) { xlat[x] = 0; }

  if(r) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].r = xlat[row[x].r];
    }
  }
  if(g) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].g = xlat[row[x].g];
    }
  }
  if(b) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].b = xlat[row[x].b];
    }
  }
}

void ppmbrightness(struct ppm *p, float e, int r, int g, int b)
{
  int x, y;
  unsigned char xlat[256];
  for(x = 0; x < 256; x++) {
    xlat[x] = x*e;
  } 

  if(r) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].r = xlat[row[x].r];
    }
  }
  if(g) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].g = xlat[row[x].g];
    }
  }
  if(b) for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++) {
      row[x].b = xlat[row[x].b];
    }
  }
}


void blur(struct ppm *p, int xrad, int yrad)
{
  int x, y;
  int tx, ty;
  struct ppm tmp = {0,0,NULL};
  int r, g, b, n;

  newppm(&tmp, p->width, p->height);
  for(y = 0; y < p->height; y++) {
    for(x = 0; x < p->width; x++) {
      r = g = b = n = 0;
      for(ty = y-yrad; ty <= y+yrad; ty++) {
        for(tx = x-xrad; tx <= x+xrad; tx++) {
          if(ty<0) continue;
          if(ty>=p->height) continue;
          if(tx<0) continue;
          if(tx>=p->width) continue;
          r += p->col[ty][tx].r;
          g += p->col[ty][tx].g;
          b += p->col[ty][tx].b;
          n++;
        }
      }
      tmp.col[y][x].r = r / n;
      tmp.col[y][x].g = g / n;
      tmp.col[y][x].b = b / n;
    }
  }
  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void putrgb_fast(struct ppm *s, float xo, float yo, struct rgbcolor *d)
{
  struct rgbcolor *tp = &s->col[(int)(yo+0.5)][(int)(xo+0.5)];
  tp->r = d->r;
  tp->g = d->g;
  tp->b = d->b;
}

void putrgb(struct ppm *s, float xo, float yo, struct rgbcolor *d)
{
  int x, y;
  float aa, ab, ba, bb;

  x = xo;
  y = yo;

  if((x < 0) || (y < 0) || (x >= s->width-1) || (y >= s->height-1))
    return;

  xo -= x;
  yo -= y;

  aa = (1.0-xo)*(1.0-yo);
  ab = xo*(1.0-yo);
  ba = (1.0-xo)*yo;
  bb = xo*yo;

  s->col[y][x].r *= (1.0-aa);
  s->col[y][x].g *= (1.0-aa);
  s->col[y][x].b *= (1.0-aa);
  s->col[y][x+1].r *= (1.0-ab);
  s->col[y][x+1].g *= (1.0-ab);
  s->col[y][x+1].b *= (1.0-ab);
  s->col[y+1][x].r *= (1.0-ba);
  s->col[y+1][x].g *= (1.0-ba);
  s->col[y+1][x].b *= (1.0-ba);
  s->col[y+1][x+1].r *= (1.0-bb);
  s->col[y+1][x+1].g *= (1.0-bb);
  s->col[y+1][x+1].b *= (1.0-bb);

  s->col[y][x].r += aa * d->r;
  s->col[y][x].g += aa * d->g;
  s->col[y][x].b += aa * d->b;
  s->col[y][x+1].r += ab * d->r;
  s->col[y][x+1].g += ab * d->g;
  s->col[y][x+1].b += ab * d->b;
  s->col[y+1][x].r += ba * d->r;
  s->col[y+1][x].g += ba * d->g;
  s->col[y+1][x].b += ba * d->b;
  s->col[y+1][x+1].r += bb * d->r;
  s->col[y+1][x+1].g += bb * d->g;
  s->col[y+1][x+1].b += bb * d->b;
}

void drawline(struct ppm *p, float fx, float fy, float tx, float ty, struct rgbcolor *col)
{
  float i;
  float d, x, y;
  if(fabs(fx-tx) > fabs(fy-ty)) {
    if(fx > tx) { i=tx; tx=fx; fx=i; i=ty; ty=fy; fy=i; }
    d = (ty-fy)/(tx-fx);
    y = fy;
    for(x = fx; x <= tx; x+=1.0) {
      putrgb(p, x, y, col);
      y += d;
    }
  } else {
    if(fy > ty) { i=tx; tx=fx; fx=i; i=ty; ty=fy; fy=i; }
    d = (tx-fx)/(ty-fy);
    x = fx;
    for(y = fy; y <= ty; y+=1.0) {
      putrgb(p, x, y, col);
      x += d;
    }
  }
}

