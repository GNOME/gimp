#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "ppmtool.h"

int pfix(int n)
{
  if(n<1) return 1;
  if(n>255) return 255;
  return n;
}

void mkplasma_sub(struct ppm *p, int x1, int x2, int y1, int y2, float turb)
{
  int xr, yr, nx, ny;
  xr = abs(x1-x2);
  yr = abs(y1-y2);

  if((xr==0) && (yr==0)) return;

  nx = (x1+x2)/2;
  ny = (y1+y2)/2;
  if(!p->col[y1][nx].r)
    p->col[y1][nx].r = pfix((p->col[y1][x1].r+p->col[y1][x2].r)/2.0+
                           turb*(rand()%xr-xr/2.0));
  if(!p->col[y2][nx].r)
    p->col[y2][nx].r = pfix((p->col[y2][x1].r+p->col[y2][x2].r)/2.0+
                           turb*(rand()%xr-xr/2.0));
  if(!p->col[ny][x1].r)
    p->col[ny][x1].r = pfix((p->col[y1][x1].r+p->col[y2][x1].r)/2.0+
                         turb*(rand()%yr-yr/2.0));
  if(!p->col[ny][x2].r)
    p->col[ny][x2].r = pfix((p->col[y1][x2].r+p->col[y2][x2].r)/2.0+
                         turb*(rand()%yr-yr/2.0));
  if(!p->col[ny][nx].r)
    p->col[ny][nx].r = 
      pfix((p->col[y1][x1].r+p->col[y1][x2].r+p->col[y2][x1].r+
            p->col[y2][x2].r)/4.0+turb*(rand()%(xr+yr)/2.0-(xr+yr)/4.0));

  if(xr>1) {
    mkplasma_sub(p,x1,nx,y1,ny, turb);
    mkplasma_sub(p,nx,x2,y1,ny, turb);
  }
  if(yr>1) {
    mkplasma_sub(p,x1,nx,ny,y2, turb);
    mkplasma_sub(p,nx,x2,ny,y2, turb);
  }
}

void mkplasma_red(struct ppm *p, float turb)
{
  int x=0, y=0;

  for(x = 0; x < p->width; x++)
    for(y = 0; y < p->height; y++)
      p->col[y][x].r = 0;
  x--; y--;
  p->col[0][0].r = 1+rand()%255;
  p->col[y][0].r = 1+rand()%255;
  p->col[0][x].r = 1+rand()%255;
  p->col[y][x].r = 1+rand()%255;
  mkplasma_sub(p, 0, x, 0, y, turb);
}

void mkgrayplasma(struct ppm *p, float turb)
{
  int x,y;

  mkplasma_red(p, turb);
  for(y = 0; y < p->height; y++) {
    struct rgbcolor *row = p->col[y];
    for(x = 0; x < p->width; x++)
      row[x].g = row[x].b = row[x].r;
  }
}
