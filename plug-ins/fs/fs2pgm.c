/*
 * fs2pgm.c -- converts a face-saver image to a portable graymap image.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int errors;
static char *progname;
static void error(char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "%s: ", progname);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  errors++;
}

int main(int argc, char **argv)
{
  int arg;

  progname = strrchr(argv[0], '/') ? strrchr(argv[0], '/')+1 : argv[0];
  for (arg=1; arg<argc; arg++)
    {
      FILE *ifd = fopen(argv[arg], "r");
      if (ifd == NULL)
	error("can't open file \"%s\"\n", argv[arg]);
      else
	{
	  char buf[1000];
	  int body=0, ix=0, iy=0, iz=0, px=0, py=0, pz=0;

	  while (!body && fgets(buf, sizeof(buf), ifd))
	    if (strncasecmp(buf, "picdata:", 8) == 0)
	      sscanf(buf+8, "%d %d %d", &px, &py, &pz);
	    else if (strncasecmp(buf, "image:", 6) == 0)
	      sscanf(buf+6, "%d %d %d", &ix, &iy, &iz);
	    else if (buf[strspn(buf, " \t\n")] == '\0')
	      body = 1;

	  if (!body) error("%s: missing body\n", argv[arg]);
	  else if (!pz) error("%s: missing or short pixdata\n", argv[arg]);
	  else if (!iz) error("%s: missing or short image\n", argv[arg]);
	  else
	    {
	      int pixval, pixno=0;
	      char *pixmap = malloc(px*py+1);
	      while (fscanf(ifd, "%2x", &pixval) == 1)
		{
		  pixmap[pixno] = pixval & 0xff;
		  pixno++;
		}
	      if (0)
		fprintf(stderr, "%3d %3d %3d %3d %3d %3d %5d\n", // argv[arg], 
			ix, iy, iz, px, py, pz, pixno);
	      if (pixno != px*py)
		error("%s: expected %d (%dx%d) pixels, found %d%s\n",
		      argv[arg], px*py, px, py, pixno,
		      pixno<px*py ? "." : " or more." );
	      else
		{
		  FILE *ofd;
		  char *ofn = malloc(strlen(argv[arg])+10);
		  strcpy(ofn, argv[arg]);
		  if (strrchr(ofn, '.') > strrchr(ofn, '/'))
		    *strrchr(ofn, '.') = '\0';
		  strcat(ofn, ".pgm");

		  if ((ofd=fopen(ofn, "w")) == NULL)
		    error("%s: can't open output file \"%s\"\n",
			  argv[arg], ofn);
		  else
		    {
		      int x, y;
		      fprintf(ofd, "P5\n%d %d\n%d\n", px, py, 255);
		      for (y=0; y<py; y++)
			for (x=0; x<px; x++)
			  fputc(pixmap[(py-y)*px + x], ofd);
		    }
		  free(ofn);
		}
	      free(pixmap);
	    }
	}
    }
  return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
