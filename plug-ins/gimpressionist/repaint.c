#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <math.h>
#include <time.h>
#include "gimpressionist.h"
#include "ppmtool.h"
#include <gtk/gtk.h>
#include <libgimp/gimp.h>

gimpressionist_vals_t runningvals;

void prepbrush(struct ppm *p)
{
  int x, y;

  for(y = 0; y< p->height; y++) {
    for(x = 0; x < p->width; x++) {
      p->col[y][x].g = 0;
    }
  }

  for(y = 1; y< p->height; y++) {
    for(x = 1; x < p->width; x++) {
      int v = p->col[y][x].r - p->col[y-1][x-1].r;
      if(v < 0) v = 0;
      p->col[y][x].g = v;
    }
  }
}

double sumbrush(struct ppm *p)
{
  int x, y;
  double sum = 0;
  for(y = 0; y< p->height; y++) {
    for(x = 0; x < p->width; x++) {
      sum += p->col[y][x].r;
    }
  }
  return sum;
}

int gethue(struct rgbcolor *rgb)
{
  double h, v, temp, diff;
  if((rgb->r == rgb->g) && (rgb->r == rgb->b)) /* Gray */
    return 0;
  v = (rgb->r > rgb->g ? rgb->r : rgb->g);     /* v = st<F8>rste verdi */
  if(rgb->b > v) v = rgb->b;
  temp = (rgb->r > rgb->g ? rgb->g : rgb->r ); /* temp = minste */
  if(rgb->b < temp) temp = rgb->b;
  diff = v - temp;

  if(v == rgb->r)
    h = ((double)rgb->g - rgb->b) / diff;
  else if(v == rgb->g)
    h = ((double)rgb->b - rgb->r) / diff + 2;
  else /* v == rgb->b */
    h = ((double)rgb->r - rgb->g) / diff + 4;
  if(h < 0) h += 6;
  return h * 255.0 / 6.0;
}

int bestbrush(struct ppm *p, struct ppm *a, int tx, int ty,
	      struct ppm *brushes, int numbrush, double *brushsum,
	      int start, int step)
{
  double dev, thissum;
  double bestdev = 0.0;
  double r, g, b;
  int best = -1;
  int x, y, h;
  long i;
  GList *brlist = NULL;

  for(i = start; i < numbrush; i += step) {
    struct ppm *brush = &brushes[i];
    /* thissum = 0.0; */
    thissum = brushsum[i];

    r = g = b = 0.0;
    for(y = 0; y < brush->height; y++) {
      struct rgbcolor *row = p->col[ty+y];
      for(x = 0; x < brush->width; x++) {
	double v;
	if((h = brush->col[y][x].r)) {
	  /* thissum += h; */
	  v = h / 255.0;
	  r += row[tx+x].r * v;
	  g += row[tx+x].g * v;
	  b += row[tx+x].b * v;
	}
      }
    }
    r = r * 255.0 / thissum;
    g = g * 255.0 / thissum;
    b = b * 255.0 / thissum;

    dev = 0.0;
    for(y = 0; y < brush->height; y++) {
      struct rgbcolor *row = p->col[ty+y];
      for(x = 0; x < brush->width; x++) {
	double v;
	if((h = brush->col[y][x].r)) {
	  v = h / 255.0;
	  dev += abs(row[tx+x].r - r) * v;
	  dev += abs(row[tx+x].g - g) * v;
	  dev += abs(row[tx+x].b - b) * v;
	  if(img_has_alpha)
	    dev += a->col[ty+y][tx+x].r * v;
	}
      }
    }
    dev /= thissum;
    /* dev += rand() / (float)RAND_MAX * 0.05; */

    if((best == -1) || (dev < bestdev)) {
      g_list_free(brlist);
      brlist = NULL;
    }
    
    if(dev < bestdev || best < 0) {
      best = i;
      bestdev = dev;
      brlist = g_list_append(brlist, (void *)i);
    }
    if(dev < runningvals.devthresh) break;
  }

  if(!brlist) {
    fprintf(stderr, "What!? No brushes?!\n");
    return 0;
  }

  i = rand() % g_list_length(brlist);
  best = (long)((g_list_nth(brlist,i))->data);
  g_list_free(brlist);

  return best;
}

void applybrush(struct ppm *brush,
		struct ppm *shadow,
		struct ppm *p, struct ppm *a,
		int tx, int ty, int r, int g, int b)
{
  struct ppm tmp;
  struct ppm atmp;
  double v, h;
  int x, y;
  double edgedarken = 1.0 - runningvals.generaldarkedge;
  double relief = runningvals.brushrelief / 100.0;
  int shadowdepth = pcvals.generalshadowdepth;
  int shadowblur = pcvals.generalshadowblur;

  /*
  if((tx < 0) || (ty < 0)) {
    fprintf(stderr, "applybrush: Huh!? tx=%d ty=%d\n",tx,ty);
    return;
  }
  */

  memcpy(&tmp, p, sizeof(struct ppm));
  if(img_has_alpha) memcpy(&atmp, a, sizeof(struct ppm));

  if(shadow) {
    int sx = tx + shadowdepth - shadowblur*2;
    int sy = ty + shadowdepth - shadowblur*2;
    for(y = 0; y < shadow->height; y++) {
      struct rgbcolor *row, *arow = NULL;
      if((sy + y) < 0) continue;
      if((sy + y) >= tmp.height) break;
      row = tmp.col[sy+y];
      if(img_has_alpha) arow = atmp.col[sy+y];
      for(x = 0; x < shadow->width; x++) {
	if((sx + x) < 0) continue;
	if((sx + x) >= tmp.width) break;
	h = shadow->col[y][x].b;
	if(!h) continue;
	v = 1.0 - (h / 255.0 * runningvals.generalshadowdarkness / 100.0);
	row[sx+x].r *= v;
	row[sx+x].g *= v;
	row[sx+x].b *= v;
	if(img_has_alpha) arow[sx+x].r *= v;
      }
    }
  }

  for(y = 0; y < brush->height; y++) {
    struct rgbcolor *row = tmp.col[ty+y];
    struct rgbcolor *arow = NULL;
    if(img_has_alpha) arow = atmp.col[ty+y];
    for(x = 0; x < brush->width; x++) {
      h = brush->col[y][x].r;
      if(!h) continue;

      if(runningvals.colorbrushes) {
	v = 1.0 - brush->col[y][x].b / 255.0;
	row[tx+x].r *= v;
	row[tx+x].g *= v;
	row[tx+x].b *= v;
	if(img_has_alpha) arow[tx+x].r *= v;
      }
      v = (1.0 - h / 255.0) * edgedarken;
      row[tx+x].r *= v;
      row[tx+x].g *= v;
      row[tx+x].b *= v;
      if(img_has_alpha) arow[tx+x].r *= v;

      v = h / 255.0;
      row[tx+x].r += r * v;
      row[tx+x].g += g * v;
      row[tx+x].b += b * v;
    }
  }

  if(relief > 0.001) {
    for(y = 1; y < brush->height; y++) {
      struct rgbcolor *row = tmp.col[ty+y];
      for(x = 1; x < brush->width; x++) {
	h = brush->col[y][x].g * relief;
	if(h < 0.001) continue;
	if(h > 255) h = 255;
	row[tx+x].r = (row[tx+x].r * (255-h) + 255 * h) / 255;
	row[tx+x].g = (row[tx+x].g * (255-h) + 255 * h) / 255;
	row[tx+x].b = (row[tx+x].b * (255-h) + 255 * h) / 255;
      }
    }
  }
}

void repaint(struct ppm *p, struct ppm *a)
{
  int x, y;
  int tx = 0, ty = 0;
  struct ppm tmp = {0,0,NULL};
  struct ppm atmp = {0,0,NULL};
  int r, g, b, n, h, i, j, on, sn;
  int numbrush, maxbrushwidth, maxbrushheight;
  struct rgbcolor back = {0,0,0};
  struct ppm *brushes, *shadows;
  struct ppm *brush, *shadow = NULL;
  double *brushsum;
  int cx, cy, maxdist;
  double scale, relief, startangle, anglespan, density, bgamma;
  double thissum;
  int max_progress;
  struct ppm paperppm = {0,0,NULL};
  struct ppm dirmap = {0,0,NULL};
  struct ppm sizmap = {0,0,NULL};
  int *xpos = NULL, *ypos = NULL;
  int step = 1;
  int progstep;
  static int running = 0;

  int dropshadow = pcvals.generaldropshadow;
  int shadowblur = pcvals.generalshadowblur;

  if(running)
    return;
  running++;

  memcpy(&runningvals, &pcvals, sizeof(pcvals));

  /* Shouldn't be necessary, but... */
  if(img_has_alpha)
    if((p->width != a->width) || (p->height != a->height)) {
      fprintf(stderr, "Huh? Image size != alpha size?\n");
      return;
    }

  srand(time(NULL) + getpid());

  numbrush = runningvals.orientnum * runningvals.sizenum;
  startangle = runningvals.orientfirst;
  anglespan = runningvals.orientlast;

  density = runningvals.brushdensity;

  if(runningvals.placetype == 1) density /= 3.0;

  bgamma = runningvals.brushgamma;

  brushes = safemalloc(numbrush * sizeof(struct ppm));
  brushsum = safemalloc(numbrush * sizeof(double));

  if(dropshadow)
    shadows = safemalloc(numbrush * sizeof(struct ppm));
  else
    shadows = NULL;

  brushes[0].col = NULL;
  if(brushfile)
    reloadbrush(runningvals.selectedbrush, &brushes[0]);
  else
    copyppm(&brushppm, &brushes[0]);

  resize(&brushes[0], brushes[0].width, brushes[0].height * pow(10,runningvals.brushaspect));
  scale = runningvals.sizelast / MAX(brushes[0].width, brushes[0].height);

  if(bgamma != 1.0)
    ppmgamma(&brushes[0], 1.0/bgamma, 1,1,1);
  
  resize(&brushes[0], brushes[0].width * scale, brushes[0].height * scale);
  i = 1 + sqrt(brushes[0].width * brushes[0].width +
	       brushes[0].height * brushes[0].height);
  pad(&brushes[0], i-brushes[0].width, i-brushes[0].width,
      i-brushes[0].height, i-brushes[0].height, &back);
  
  for(i = 1; i < numbrush; i++) {
    brushes[i].col = NULL;
    copyppm(&brushes[0], &brushes[i]);
  }

  for(i = 0; i < runningvals.sizenum; i++) {
    double sv;
    if(runningvals.sizenum > 1)
      sv = i / (runningvals.sizenum - 1.0);
    else sv = 1.0;
    for(j = 0; j < runningvals.orientnum; j++) {
      h = j + i * runningvals.orientnum;
      freerotate(&brushes[h],
		 startangle + j * anglespan / runningvals.orientnum);
      rescale(&brushes[h], (sv * runningvals.sizefirst + (1.0-sv) * runningvals.sizelast) / runningvals.sizelast);
      autocrop(&brushes[h],1);
    } 
  }

  /* Brush-debugging */
  /*
  for(i = 0; i < numbrush; i++) {
    char tmp[1000];
    sprintf(tmp, "/tmp/_brush%03d.ppm", i);
    saveppm(&brushes[i], tmp);
  }
  */

  for(i = 0; i < numbrush; i++) {
    if(!runningvals.colorbrushes)
      prepbrush(&brushes[i]);
    brushsum[i] = sumbrush(&brushes[i]);
  }

  brush = &brushes[0];
  thissum = brushsum[0];

  maxbrushwidth = maxbrushheight = 0;
  for(i = 0; i < numbrush; i++) {
    if(brushes[i].width > maxbrushwidth) maxbrushwidth = brushes[i].width;
    if(brushes[i].height > maxbrushheight) maxbrushheight = brushes[i].height;
  }
  
  for(i = 0; i < numbrush; i++) {
    int xp, yp;
    struct rgbcolor blk = {0,0,0};

    xp = maxbrushwidth - brushes[i].width;
    yp = maxbrushheight - brushes[i].height;
    if(xp || yp)
      pad(&brushes[i], xp/2, xp-xp/2, yp/2, yp-yp/2, &blk);
  }

  if(dropshadow) {
    for(i = 0; i < numbrush; i++) {
      shadows[i].col = NULL;
      copyppm(&brushes[i], &shadows[i]);
      ppmgamma(&shadows[i], 0, 1,1,0);
      pad(&shadows[i], shadowblur*2, shadowblur*2, 
	  shadowblur*2, shadowblur*2, &back);
      for(j = 0; j < shadowblur; j++)
	blur(&shadows[i], 2, 2);
      /* autocrop(&shadows[i],1); */
    }
    /*
    maxbrushwidth += shadowdepth*3;
    maxbrushheight += shadowdepth*3;
    */
  }
  
  /* For extra annoying debugging :-) */
  /*
  saveppm(brushes, "/tmp/__brush.ppm");
  if(shadows) saveppm(shadows, "/tmp/__shadow.ppm");
  system("xv /tmp/__brush.ppm & xv /tmp/__shadow.ppm & ");
  */

  if(runningvals.generalpaintedges) {
    edgepad(p, maxbrushwidth, maxbrushwidth, maxbrushheight, maxbrushheight);
    if(img_has_alpha)
      edgepad(a, maxbrushwidth, maxbrushwidth, maxbrushheight, maxbrushheight);
  }

  if(img_has_alpha) {
    /* Initially fully transparent */
    if(runningvals.generalbgtype == 3) {
      struct rgbcolor tmpcol = {255,255,255};
      newppm(&atmp, a->width, a->height);
      fill(&atmp, &tmpcol);
    } else {
      copyppm(a, &atmp);
    }
  }

  if(runningvals.generalbgtype == 0) {
    struct rgbcolor tmpcol;
    newppm(&tmp, p->width, p->height);
    memcpy(&tmpcol, runningvals.color, 3);
    fill(&tmp, &tmpcol);
  } else if(runningvals.generalbgtype == 1) {
    copyppm(p, &tmp);
  } else {
    scale = runningvals.paperscale / 100.0;
    newppm(&tmp, p->width, p->height);
    loadppm(runningvals.selectedpaper, &paperppm);
    resize(&paperppm, paperppm.width * scale, paperppm.height * scale);
    if(runningvals.paperinvert)
      ppmgamma(&paperppm, -1.0, 1, 1, 1);
    for(x = 0; x < tmp.width; x++) {
      int rx = x % paperppm.width;
      for(y = 0; y < tmp.height; y++) {
	int ry = y % paperppm.height;
	memcpy(&tmp.col[y][x], &paperppm.col[ry][rx], 3);
      }
    }
  }

  cx = p->width / 2;
  cy = p->height / 2;
  maxdist = sqrt(cx*cx+cy*cy);

  if(runningvals.orienttype == 0) { /* Value */
    newppm(&dirmap, p->width, p->height);
    for(y = 0; y < dirmap.height; y++) {
      struct rgbcolor *dstrow = dirmap.col[y];
      struct rgbcolor *srcrow = p->col[y];
      for(x = 0; x < dirmap.width; x++) {
	dstrow[x].r = (srcrow[x].r + srcrow[x].g + srcrow[x].b) / 3;
      }
    }
  } else if(runningvals.orienttype == 1) { /* Radius */
    newppm(&dirmap, p->width, p->height);
    for(y = 0; y < dirmap.height; y++) {
      struct rgbcolor *dstrow = dirmap.col[y];
      double ysqr = (cy-y)*(cy-y);
      for(x = 0; x < dirmap.width; x++) {
	dstrow[x].r = sqrt((cx-x)*(cx-x)+ysqr) * 255 / maxdist;
      }
    }
  } else if(runningvals.orienttype == 3) { /* Radial */
    newppm(&dirmap, p->width, p->height);
    for(y = 0; y < dirmap.height; y++) {
      struct rgbcolor *dstrow = dirmap.col[y];
      for(x = 0; x < dirmap.width; x++) {
	dstrow[x].r = (M_PI + atan2(cy-y, cx-x)) * 255.0 / (M_PI*2);
      }
    }
  } else if(runningvals.orienttype == 4) { /* Flowing */
    newppm(&dirmap, p->width / 6 + 5, p->height / 6 + 5);
    mkgrayplasma(&dirmap, 15);
    blur(&dirmap, 2, 2);
    blur(&dirmap, 2, 2);
    resize(&dirmap, p->width, p->height);
    blur(&dirmap, 2, 2);
    if(runningvals.generalpaintedges)
      edgepad(&dirmap, maxbrushwidth, maxbrushheight,maxbrushwidth, maxbrushheight);
  } else if(runningvals.orienttype == 5) { /* Hue */
    newppm(&dirmap, p->width, p->height);
    for(y = 0; y < dirmap.height; y++) {
      struct rgbcolor *dstrow = dirmap.col[y];
      struct rgbcolor *srcrow = p->col[y];
      for(x = 0; x < dirmap.width; x++) {
	dstrow[x].r = gethue(&srcrow[x]);
      }
    }
  } else if(runningvals.orienttype == 6) {
    struct rgbcolor tmpcol = {0,0,0};
    newppm(&dirmap, p->width, p->height);
    fill(&dirmap, &tmpcol);

  } else if(runningvals.orienttype == 7) { /* Manual */
    newppm(&dirmap, p->width-maxbrushwidth*2, p->height-maxbrushheight*2);
    for(y = 0; y < dirmap.height; y++) {
      struct rgbcolor *dstrow = dirmap.col[y];
      double tmpy = y / (double)dirmap.height;
      for(x = 0; x < dirmap.width; x++) {
	dstrow[x].r = pixval(90-getdir(x / (double)dirmap.width, tmpy, 1));
      }
    }
    edgepad(&dirmap, maxbrushwidth, maxbrushwidth, maxbrushheight, maxbrushheight);
  }

  if(runningvals.sizetype == 0) { /* Value */
    newppm(&sizmap, p->width, p->height);
    for(y = 0; y < sizmap.height; y++) {
      struct rgbcolor *dstrow = sizmap.col[y];
      struct rgbcolor *srcrow = p->col[y];
      for(x = 0; x < sizmap.width; x++) {
	dstrow[x].r = (srcrow[x].r + srcrow[x].g + srcrow[x].b) / 3;
      }
    }
  } else if(runningvals.sizetype == 1) { /* Radius */
    newppm(&sizmap, p->width, p->height);
    for(y = 0; y < sizmap.height; y++) {
      struct rgbcolor *dstrow = sizmap.col[y];
      double ysqr = (cy-y)*(cy-y);
      for(x = 0; x < sizmap.width; x++) {
	dstrow[x].r = sqrt((cx-x)*(cx-x)+ysqr) * 255 / maxdist;
      }
    }
  } else if(runningvals.sizetype == 3) { /* Radial */
    newppm(&sizmap, p->width, p->height);
    for(y = 0; y < sizmap.height; y++) {
      struct rgbcolor *dstrow = sizmap.col[y];
      for(x = 0; x < sizmap.width; x++) {
	dstrow[x].r = (M_PI + atan2(cy-y, cx-x)) * 255.0 / (M_PI*2);
      }
    }
  } else if(runningvals.sizetype == 4) { /* Flowing */
    newppm(&sizmap, p->width / 6 + 5, p->height / 6 + 5);
    mkgrayplasma(&sizmap, 15);
    blur(&sizmap, 2, 2);
    blur(&sizmap, 2, 2);
    resize(&sizmap, p->width, p->height);
    blur(&sizmap, 2, 2);
    if(runningvals.generalpaintedges)
      edgepad(&sizmap, maxbrushwidth, maxbrushheight,maxbrushwidth, maxbrushheight);
  } else if(runningvals.sizetype == 5) { /* Hue */
    newppm(&sizmap, p->width, p->height);
    for(y = 0; y < sizmap.height; y++) {
      struct rgbcolor *dstrow = sizmap.col[y];
      struct rgbcolor *srcrow = p->col[y];
      for(x = 0; x < sizmap.width; x++) {
	dstrow[x].r = gethue(&srcrow[x]);
      }
    }
  } else if(runningvals.sizetype == 6) {
    struct rgbcolor tmpcol = {0,0,0};
    newppm(&sizmap, p->width, p->height);
    fill(&sizmap, &tmpcol);

  } else if(runningvals.sizetype == 7) { /* Manual */
    newppm(&sizmap, p->width-maxbrushwidth*2, p->height-maxbrushheight*2);
    for(y = 0; y < sizmap.height; y++) {
      struct rgbcolor *dstrow = sizmap.col[y];
      double tmpy = y / (double)sizmap.height;
      for(x = 0; x < sizmap.width; x++) {
	dstrow[x].r = 255 * (1.0 - getsiz(x / (double)sizmap.width, tmpy, 1));
      }
    }
    edgepad(&sizmap, maxbrushwidth, maxbrushwidth, maxbrushheight, maxbrushheight);
  }
  /*
  saveppm(&sizmap, "/tmp/_sizmap.ppm");
  */
  if(runningvals.placetype == 0) {
    i = tmp.width * tmp.height / (maxbrushwidth * maxbrushheight);
    i *= density;
  } else if(runningvals.placetype == 1) {
    i = (int)(tmp.width * density / maxbrushwidth) *
      (int)(tmp.height * density / maxbrushheight);
    step = i;
    /* fprintf(stderr, "step=%d i=%d\n", step, i); */
  }
  if(i < 1) i = 1;

  max_progress = i;
  progstep = max_progress / 30;
  if(progstep < 10) progstep = 10;

  if(runningvals.placetype == 1) {
    int j;
    xpos = safemalloc(i * sizeof(int));
    ypos = safemalloc(i * sizeof(int));
    for(j = 0; j < i; j++) {
      int factor = (int)(tmp.width * density / maxbrushwidth + 0.5);
      if(factor < 1) factor = 1;
      xpos[j] = maxbrushwidth/2 + (j % factor) * maxbrushwidth / density;
      ypos[j] = maxbrushheight/2 + (j / factor) * maxbrushheight / density;
    }
    for(j = 0; j < i; j++) {
      int a, b;
      a = rand()%i;
      b = xpos[j]; xpos[j] = xpos[a]; xpos[a] = b;
      b = ypos[j]; ypos[j] = ypos[a]; ypos[a] = b;
    }
  }

  for(; i; i--) {
    if(i % progstep == 0) {
      if(runningvals.run) {
	if(!standalone)
	  gimp_progress_update(0.8 - 0.8*((double)i / max_progress));
	else {
	  fprintf(stderr, "."); fflush(stderr);
	}
      } else {
	char tmps[40];
	sprintf(tmps, "%.1f %%", 100 * (1.0 - ((double)i / max_progress)));
#if GTK_MINOR_VERSION == 0
	gtk_label_set(GTK_LABEL(GTK_BUTTON(previewbutton)->child), tmps);
#else
	gtk_label_set_text(GTK_LABEL(GTK_BUTTON(previewbutton)->child), tmps);
#endif
	while(gtk_events_pending())
	  gtk_main_iteration();
      }
    }

    if(runningvals.placetype == 0) {
      tx = rand() % (tmp.width - maxbrushwidth) + maxbrushwidth/2;
      ty = rand() % (tmp.height - maxbrushheight) + maxbrushheight/2;
    } else if(runningvals.placetype == 1) {
      tx = xpos[i-1];
      ty = ypos[i-1];
    }
    if(runningvals.placecenter) {
      double z = rand()*0.75 / RAND_MAX;
      tx = tx * (1.0-z) + tmp.width/2 * z;
      ty = ty * (1.0-z) + tmp.height/2 * z;
    }

    if((tx < maxbrushwidth/2) || (ty < maxbrushwidth/2) ||
       (tx + maxbrushwidth/2 >= p->width) ||
       (ty + maxbrushheight/2 >= p->height)) {
      /*
      fprintf(stderr, "Internal Error; invalid coords: (%d,%d) i=%d\n", tx, ty, i);
      */
      continue;
    }

    if(img_has_alpha) {
      if(a->col[ty][tx].r > 128)
	continue;
    }

    n = sn = on = 0;

    switch(runningvals.orienttype) {
    case 2: /* Random */
      on = rand() % runningvals.orientnum;
      break;
    case 0: /* Value */
    case 1: /* Radius */
    case 3: /* Radial */
    case 4: /* Flowing */
    case 5: /* Hue */
    case 7: /* Manual */
      on = runningvals.orientnum * dirmap.col[ty][tx].r / 255;
      break;
    case 6: /* Adaptive */
      break; /* Handled below */
    default:
      fprintf(stderr, "Internal error; Unknown orientationtype\n");
      on = 0;
      break;
    }

    switch(runningvals.sizetype) {
    case 2: /* Random */
      sn = rand() % runningvals.sizenum;
      break;
    case 0: /* Value */
    case 1: /* Radius */
    case 3: /* Radial */
    case 4: /* Flowing */
    case 5: /* Hue */
    case 7: /* Manual */
      sn = runningvals.sizenum * sizmap.col[ty][tx].r / 255;
      break;
    case 6: /* Adaptive */
      break; /* Handled below */
    default:
      fprintf(stderr, "Internal error; Unknown sizetype\n");
      sn = 0;
      break;
    }

    /* Handle Adaptive selections */
    if((runningvals.orienttype == 6) && (runningvals.sizetype == 6)) {
      n = bestbrush(p, a, tx-maxbrushwidth/2, ty-maxbrushheight/2,
		    brushes, numbrush, brushsum, 0, 1);
    } else if(runningvals.orienttype == 6) {
      int st = sn * runningvals.orientnum;
      n = bestbrush(p, a, tx-maxbrushwidth/2, ty-maxbrushheight/2,
		    brushes, st+runningvals.orientnum, brushsum, st, 1);
    } else if(runningvals.sizetype == 6) {
      n = bestbrush(p, a, tx-maxbrushwidth/2, ty-maxbrushheight/2,
		    brushes, numbrush, brushsum, on, runningvals.orientnum);
    } else {
      n = sn * runningvals.orientnum + on;
    }

    /* Should never happen, but hey... */
    if(n < 0) n = 0;
    else if(n >= numbrush) n = numbrush - 1;

    tx -= maxbrushwidth/2;
    ty -= maxbrushheight/2;

    brush = &brushes[n];
    if(dropshadow)
      shadow = &shadows[n];
    thissum = brushsum[n];

    /* Calculate color - avg. of in-brush pixels */
    r = g = b = 0;
    for(y = 0; y < brush->height; y++) {
      struct rgbcolor *row = p->col[ty+y];
      for(x = 0; x < brush->width; x++) {
	double v;
	if((h = brush->col[y][x].r)) {
	  v = h / 255.0;
	  r += row[tx+x].r * v;
	  g += row[tx+x].g * v;
	  b += row[tx+x].b * v;
	}
      }
    }
    r = r * 255.0 / thissum;
    g = g * 255.0 / thissum;
    b = b * 255.0 / thissum;

    /* Color = center pixel - Looks bad... */
    /* 
    r = p->col[ty+brush->height/2][tx+brush->width/2].r;
    g = p->col[ty+brush->height/2][tx+brush->width/2].g;
    b = p->col[ty+brush->height/2][tx+brush->width/2].b;
    */

    applybrush(brush, shadow, &tmp, &atmp, tx,ty, r,g,b);
    if(runningvals.generaltileable && runningvals.generalpaintedges) {
      int origwidth = tmp.width - 2 * maxbrushwidth;
      int origheight = tmp.height - 2 * maxbrushheight;
      int dox = 0, doy = 0;
      if(tx < maxbrushwidth) {
	applybrush(brush, shadow, &tmp, &atmp, tx+origwidth,ty, r,g,b);
	dox = -1;
      } else if(tx > origwidth) {
	applybrush(brush, shadow, &tmp, &atmp, tx-origwidth,ty, r,g,b);
	dox = 1;
      }
      if(ty < maxbrushheight) {
	applybrush(brush, shadow, &tmp, &atmp, tx,ty+origheight, r,g,b);
	doy = 1;
      } else if(ty > origheight) {
	applybrush(brush, shadow, &tmp, &atmp, tx,ty-origheight, r,g,b);
	doy = -1;
      }
      if(doy) {
	if(dox < 0)
	  applybrush(brush, shadow, &tmp, &atmp, tx+origwidth,ty+doy*origheight,r,g,b);
	if(dox > 0)
	  applybrush(brush, shadow, &tmp, &atmp, tx-origwidth,ty+doy*origheight,r,g,b);
      }
    }
  }
  for(i = 0; i < numbrush; i++) {
    killppm(&brushes[i]);
  }
  free(brushes);
  if(shadows)
    free(shadows);
  free(brushsum);

  if(runningvals.generalpaintedges) {
    crop(&tmp, maxbrushwidth, maxbrushheight, tmp.width - maxbrushwidth, tmp.height - maxbrushheight);
    if(img_has_alpha)
      crop(&atmp, maxbrushwidth, maxbrushheight, atmp.width - maxbrushwidth, atmp.height - maxbrushheight);
  }

  killppm(p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;

  if(img_has_alpha) {
    killppm(a);
    a->width = atmp.width;
    a->height = atmp.height;
    a->col = atmp.col;
  }

  relief = runningvals.paperrelief / 100.0;
  if(relief > 0.001) {
    scale = runningvals.paperscale / 100.0;

    if(paperppm.col) {
      memcpy(&tmp, &paperppm, sizeof(struct ppm));
      paperppm.col = NULL;
    } else {
      tmp.col = NULL;
      loadppm(runningvals.selectedpaper, &tmp);
      resize(&tmp, tmp.width * scale, tmp.height * scale);
      if(runningvals.paperinvert)
	ppmgamma(&tmp, -1.0, 1,1,1);
    }
    for(x = 0; x < p->width; x++) {
      double h, v;
      int px = x % tmp.width, py;
      for(y = 0; y < p->height; y++) {
	py = y % tmp.height;
	if(runningvals.paperoverlay)
	  h = (tmp.col[py][px].r-128) * relief;
	else
	  h = (tmp.col[py][px].r - (int)tmp.col[(py+1)%tmp.height][(px+1)%tmp.width].r) / -2.0 * relief;
	if(h <= 0.0) {
	  v = 1.0 + h/128.0;
	  if(v < 0.0) v = 0.0; else if(v > 1.0) v = 1.0;
	  p->col[y][x].r *= v;
	  p->col[y][x].g *= v;
	  p->col[y][x].b *= v;
	} else {
	  v = h/128.0;
	  if(v < 0.0) v = 0.0; else if(v > 1.0) v = 1.0;
	  p->col[y][x].r = p->col[y][x].r * (1.0-v) + 255 * v;
	  p->col[y][x].g = p->col[y][x].g * (1.0-v) + 255 * v;
	  p->col[y][x].b = p->col[y][x].b * (1.0-v) + 255 * v;
	}
      }
    }
    killppm(&tmp);
  }

  if(paperppm.col) killppm(&paperppm);
  if(dirmap.col) killppm(&dirmap);
  if(sizmap.col) killppm(&sizmap);
  if(runningvals.run) {
    if(!standalone)
      gimp_progress_update(0.8);
    else {
      fprintf(stderr, ".\n"); fflush(stderr);
    }
  } else {
#if GTK_MINOR_VERSION == 0
    gtk_label_set(GTK_LABEL(GTK_BUTTON(previewbutton)->child), " Update ");
#else
    gtk_label_set_text(GTK_LABEL(GTK_BUTTON(previewbutton)->child), " Update ");
#endif
  }
  running = 0;
}


