/*
 * "$Id$"
 *
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   canon_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   canon_imageable_area() - Return the imageable area of the page.
 *   canon_print()          - Print an image to a CANON printer.
 *   canon_write()          - Send 6-color graphics using compression.
 *
 * Revision History:
 *
 *   See ChangeLog
 */


/* TODO-LIST
 *
 *   * implement the left border
 *
 */

#include <stdarg.h>
#include "print.h"

/*
 * Local functions...
 */

typedef struct {
  int model;
  int max_width;      /* maximum printable paper size */
  int max_height;
  int max_xdpi;
  int max_ydpi;
  int max_quality;
  int border_left;
  int border_right;
  int border_top;
  int border_bottom;
  int inks;           /* installable cartridges (CANON_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
} canon_cap_t;

static void canon_write_line(FILE *, canon_cap_t, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     int, int, int, int);

#define PHYSICAL_BPI 1440
#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)


/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 */
#define CANON_INK_K           1
#define CANON_INK_CMY         2
#define CANON_INK_CMYK        4
#define CANON_INK_CcMmYK      8
#define CANON_INK_CcMmYy     16
#define CANON_INK_CcMmYyK    32

#define CANON_INK_BLACK_MASK (CANON_INK_K|CANON_INK_CMYK|\
                              CANON_INK_CcMmYK|CANON_INK_CcMmYyK)

#define CANON_INK_PHOTO_MASK (CANON_INK_CcMmYy|CANON_INK_CcMmYK|\
                              CANON_INK_CcMmYyK)

/* document feeding */
#define CANON_SLOT_ASF1    1
#define CANON_SLOT_ASF2    2
#define CANON_SLOT_MAN1    4
#define CANON_SLOT_MAN2    8

/* model peculiarities */
#define CANON_CAP_DMT       1<<0    /* Drop Modulation Technology */
#define CANON_CAP_MSB_FIRST 1<<1    /* how to send data           */
#define CANON_CAP_CMD61     1<<2    /* uses command #0x61         */
#define CANON_CAP_CMD6d     1<<3    /* uses command #0x6d         */
#define CANON_CAP_CMD70     1<<4    /* uses command #0x70         */
#define CANON_CAP_CMD72     1<<5    /* uses command #0x72         */


static canon_cap_t canon_model_capabilities[] =
{
  /* default settings for unkown models */

  {   -1, 8*72,11*72,180,180,20,20,20,20, CANON_INK_K, CANON_SLOT_ASF1, 0 },

  /* tested models */

  { /* Canon BJC 4300 */
    4300,
    618, 936,      /* 8.58" x 13 " */
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_DMT
  },

  { /* Canon BJC 4400 */
    4400,
    9.5*72, 14*72,
    720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT
  },

  { /* Canon BJC 6000 */
    6000,
    618, 936,      /* 8.58" x 13 " */
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_DMT
  },

  { /* Canon BJC 8200 */
    8200,
    11*72, 17*72,
    1200,1200, 4,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_DMT | CANON_CAP_CMD72
  },

  /* untested models */

  { /* Canon BJC 1000 */
    1000,
    11*72, 17*72,
    360, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61
  },
  { /* Canon BJC 2000 */
    2000,
    11*72, 17*72,
    720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61
  },
  { /* Canon BJC 3000 */
    3000,
    11*72, 17*72,
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT
  },
  { /* Canon BJC 6100 */
    6100,
    11*72, 17*72,
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61
  },
  { /* Canon BJC 7000 */
    7000,
    11*72, 17*72,
    1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    0
  },
  { /* Canon BJC 7100 */
    7100,
    11*72, 17*72,
    1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYyK,
    CANON_SLOT_ASF1,
    0
  },

  /* extremely fuzzy models */

  { /* Canon BJC 5100 */
    5100,
    17*72, 22*72,
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_DMT
  },
  { /* Canon BJC 5500 */
    5500,
    22*72, 34*72,
    720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61
  },
  { /* Canon BJC 6500 */
    6500,
    17*72, 22*72,
    1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT
  },
  { /* Canon BJC 8500 */
    8500,
    17*72, 22*72,
    1200,1200, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    0
  },
};




static canon_cap_t canon_get_model_capabilities(int model)
{
  int i;
  int models= sizeof(canon_model_capabilities) / sizeof(canon_cap_t);
  for (i=0; i<models; i++) {
    if (canon_model_capabilities[i].model == model) {
      return canon_model_capabilities[i];
    }
  }
#ifdef DEBUG
  fprintf(stderr,"canon: model %d not found in capabilities list.\n",model);
#endif
  return canon_model_capabilities[0];
}

static int
canon_media_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Plain Paper"))           return  1;
  if (!strcmp(name,"Transparencies"))        return  2;
  if (!strcmp(name,"Back Print Film"))       return  3;
  if (!strcmp(name,"Fabric Sheets"))         return  4;
  if (!strcmp(name,"Envelope"))              return  5;
  if (!strcmp(name,"High Resolution Paper")) return  6;
  if (!strcmp(name,"T-Shirt Transfers"))     return  7;
  if (!strcmp(name,"High Gloss Film"))       return  8;
  if (!strcmp(name,"Glossy Photo Paper"))    return  9;
  if (!strcmp(name,"Glossy Photo Cards"))    return 10;
  if (!strcmp(name,"Photo Paper Pro"))       return 11;

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown media type '%s' - reverting to plain\n",name);
#endif
  return 1;
}

static int
canon_source_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Auto Sheet Feeder"))    return 4;
  if (!strcmp(name,"Manual with Pause"))    return 0;
  if (!strcmp(name,"Manual without Pause")) return 1;

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown source type '%s' - reverting to auto\n",name);
#endif
  return 4;
}

static int
canon_printhead_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Black"))       return 0;
  if (!strcmp(name,"Color"))       return 1;
  if (!strcmp(name,"Black/Color")) return 2;
  if (!strcmp(name,"Photo/Color")) return 3;
  if (!strcmp(name,"Photo"))       return 4;

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown head combo '%s' - reverting to black\n",name);
#endif
  return 0;
}

static unsigned char
canon_size_type(const vars_t *v, canon_cap_t caps)
{
  const papersize_t *pp = get_papersize_by_size(v->page_height, v->page_width);
  if (pp)
    {
      const char *name = pp->name;
      /* built ins: */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"B5"))          return 0x08;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"Envelope 10")) return 0x16;
      if (!strcmp(name,"Envelope DL")) return 0x17;
      if (!strcmp(name,"Letter+"))     return 0x2a;
      if (!strcmp(name,"A4+"))         return 0x2b;
      if (!strcmp(name,"Canon 4x2"))   return 0x2d;
    }
  /* custom */

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown paper size '%s' - using custom\n",name);
#endif
  return 0;
}

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

const char *
canon_default_resolution(const printer_t *printer)
{
  canon_cap_t caps= canon_get_model_capabilities(printer->model);
  if (!(caps.max_xdpi%300))
    return "300x300 DPI";
  else
    return "180x180 DPI";
}

/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
canon_parameters(const printer_t *printer,	/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**p= 0,
                **valptrs= 0;

  static char   *media_types[] =
                {
                  ("Plain Paper"),
                  ("Transparencies"),
                  ("Back Print Film"),
                  ("Fabric Sheets"),
                  ("Envelope"),
                  ("High Resolution Paper"),
                  ("T-Shirt Transfers"),
                  ("High Gloss Film"),
                  ("Glossy Photo Paper"),
                  ("Glossy Photo Cards"),
                  ("Photo Paper Pro")
                };
  static char   *media_sources[] =
                {
                  ("Auto Sheet Feeder"),
                  ("Manual with Pause"),
                  ("Manual without Pause"),
                };

  canon_cap_t caps= canon_get_model_capabilities(printer->model);

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0) {
    int length_limit, width_limit;
    const papersize_t *papersizes = get_papersizes();
    valptrs = malloc(sizeof(char *) * known_papersizes());
    *count = 0;

    width_limit = caps.max_width;
    length_limit = caps.max_height;

    for (i = 0; i < known_papersizes(); i++) {
      if (strlen(papersizes[i].name) > 0 &&
	  papersizes[i].width <= width_limit &&
	  papersizes[i].length <= length_limit) {
	valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	strcpy(valptrs[*count], papersizes[i].name);
	(*count)++;
      }
    }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    int x= caps.max_xdpi, y= caps.max_ydpi;
    int c= 0;
    valptrs = malloc(sizeof(char *) * 10);
    if (!(caps.max_xdpi%300)) {

      if ( 300<=x && 300<=y)
	valptrs[c++]= c_strdup("300x300 DPI");
      if ( 300<=x && 300<=y && (caps.features&CANON_CAP_DMT))
	valptrs[c++]= c_strdup("300x300 DPI DMT");
      if ( 600<=x && 600<=y)
	valptrs[c++]= c_strdup("600x600 DPI");
      if ( 600<=x && 600<=y && (caps.features&CANON_CAP_DMT))
	valptrs[c++]= c_strdup("600x600 DPI DMT");
      if (1200==x && 600==y)
	valptrs[c++]= c_strdup("1200x600 DPI");
      if (1200<=x && 1200<=y)
	valptrs[c++]= c_strdup("1200x1200 DPI");

    } else if (!(caps.max_xdpi%180)) {

      if ( 180<=x && 180<=y)
	valptrs[c++]= c_strdup("180x180 DPI");
      if ( 360<=x && 360<=y)
	valptrs[c++]= c_strdup("360x360 DPI");
      if ( 360<=x && 360<=y && (caps.features&CANON_CAP_DMT))
	valptrs[c++]= c_strdup("360x360 DPI DMT");
      if ( 720==x && 360==y)
	valptrs[c++]= c_strdup("720x360 DPI");
      if ( 720<=x && 720<=y)
	valptrs[c++]= c_strdup("720x720 DPI");
      if (1440==x && 720==y)
	valptrs[c++]= c_strdup("1440x720 DPI");
      if (1440<=x &&1440<=y)
	valptrs[c++]= c_strdup("1440x1440 DPI");

    } else {
#ifdef DEBUG
      fprintf(stderr,"canon: unknown resolution multiplier for model %d\n",
	      caps.model);
#endif
      return 0;
    }
    *count= c;
    p= valptrs;
  }
  else if (strcmp(name, "InkType") == 0)
  {
    int c= 0;
    valptrs = malloc(sizeof(char *) * 5);
    if ((caps.inks & CANON_INK_K))
      valptrs[c++]= c_strdup("Black");
    if ((caps.inks & CANON_INK_CMY))
      valptrs[c++]= c_strdup("Color");
    if ((caps.inks & CANON_INK_CMYK))
      valptrs[c++]= c_strdup("Black/Color");
    if ((caps.inks & CANON_INK_CcMmYK))
      valptrs[c++]= c_strdup("Photo/Color");
    if ((caps.inks & CANON_INK_CcMmYy))
      valptrs[c++]= c_strdup("Photo/Color");
    *count = c;
    p = valptrs;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    *count = 11;
    p = media_types;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    *count = 3;
    p = media_sources;
  }
  else
    return (NULL);

  valptrs = malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    valptrs[i] = c_strdup(p[i]);

  return (valptrs);
}


/*
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

void
canon_imageable_area(const printer_t *printer,	/* I - Printer model */
		     const vars_t *v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  canon_cap_t caps= canon_get_model_capabilities(printer->model);

  default_media_size(printer, v, &width, &length);

  *left   = caps.border_left;
  *right  = width - caps.border_right;
  *top    = length - caps.border_top;
  *bottom = caps.border_bottom;
}

void
canon_limit(const printer_t *printer,	/* I - Printer model */
	    const vars_t *v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  canon_cap_t caps= canon_get_model_capabilities(printer->model);
  *width =	caps.max_width;
  *length =	caps.max_height;
}

/*
 * 'canon_cmd()' - Sends a command with variable args
 */
static void
canon_cmd(FILE *prn, /* I - the printer         */
	  char *ini, /* I - 2 bytes start code  */
	  char cmd,  /* I - command code        */
	  int  num,  /* I - number of arguments */
	  ...        /* I - the args themselves */
	  )
{
  static int bufsize= 0;
  static unsigned char *buffer;
  int i;
  va_list ap;

  if (!buffer || (num > bufsize)) {
    if (buffer)
      free(buffer);
    buffer = malloc(num);
    bufsize= num;
    if (!buffer) {
#ifdef DEBUG
      fprintf(stderr,"\ncanon: *** buffer allocation failed...\n");
      fprintf(stderr,"canon: *** command 0x%02x with %d args dropped\n\n",
	      cmd,num);
#endif
      return;
    }
  }
  if (num) {
    va_start(ap, num);
    for (i=0; i<num; i++)
      buffer[i]= (unsigned char) va_arg(ap, int);
    va_end(ap);
  }

  fwrite(ini,2,1,prn);
  if (cmd) {
    fputc(cmd,prn);
    fputc((num & 255),prn);
    fputc((num >> 8 ),prn);
    fwrite(buffer,num,1,prn);
  }
}

#ifdef DEBUG
#define PUT(WHAT,VAL,RES) fprintf(stderr,"canon: "WHAT" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))
#else
#define PUT(WHAT,VAL,RES) do {} while (0)
#endif

static void
canon_init_printer(FILE *prn, canon_cap_t caps,
		   int output_type, char *media_str,
		   const vars_t *v, int print_head,
		   char *source_str,
		   int xdpi, int ydpi,
		   int page_width, int page_height,
		   int top, int left,
		   int use_dmt)
{
#define MEDIACODES 11
  static unsigned char mediacode_63[] = {
    0x00,0x00,0x02,0x03,0x04,0x08,0x07,0x03,0x06,0x05,0x05,0x09
  };
  static unsigned char mediacode_6c[] = {
    0x00,0x00,0x02,0x03,0x04,0x08,0x07,0x03,0x06,0x05,0x0a,0x09
  };

  #define ESC28 "\x1b\x28"
  #define ESC5b "\x1b\x5b"
  #define ESC40 "\x1b\x40"

  unsigned char
    arg_63_1 = 0x00,
    arg_63_2 = 0x00, /* plain paper */
    arg_63_3 = caps.max_quality, /* output quality  */
    arg_6c_1 = 0x00,
    arg_6c_2 = 0x01, /* plain paper */
    arg_6d_1 = 0x03, /* color printhead? */
    arg_6d_2 = 0x00, /* 00=color  02=b/w */
    arg_6d_3 = 0x00, /* only 01 for bjc8200 */
    arg_6d_a = 0x03, /* A4 paper */
    arg_6d_b = 0x00,
    arg_70_1 = 0x02, /* A4 printable area */
    arg_70_2 = 0xa6,
    arg_70_3 = 0x01,
    arg_70_4 = 0xe0,
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  int media= canon_media_type(media_str,caps);
  int source= canon_source_type(source_str,caps);

  int printable_width=  page_width*10/12;
  int printable_height= page_height*10/12;

  arg_6d_a= canon_size_type(v,caps);
  if (!arg_6d_a) arg_6d_b= 1;

  if (caps.model<3000)
    arg_63_1= arg_6c_1= 0x10;
  else
    arg_63_1= arg_6c_1= 0x30;

  if (output_type==OUTPUT_GRAY) arg_63_1|= 0x01;
  arg_6c_1|= (source & 0x0f);

  if (print_head==0) arg_6d_1= 0x03;
  else if (print_head<=2) arg_6d_1= 0x02;
  else if (print_head<=4) arg_6d_1= 0x04;
  if (output_type==OUTPUT_GRAY) arg_6d_2= 0x02;

  if (caps.model==8200) arg_6d_3= 0x01;

  arg_70_1= (printable_height >> 8) & 0xff;
  arg_70_2= (printable_height) & 0xff;
  arg_70_3= (printable_width >> 8) & 0xff;
  arg_70_4= (printable_width) & 0xff;

  if (xdpi==1440) arg_74_2= 0x04;
  if (ydpi>=720)  arg_74_3= 0x09;

  if (media<MEDIACODES) {
    arg_63_2= mediacode_63[media];
    arg_6c_2= mediacode_6c[media];
  }

  if (use_dmt) {
    arg_74_1= 0x02;
    arg_74_2= 0x80;
    arg_74_3= 0x09;
  }

  /* workaround for the bjc8200 - not really understood */
  if (caps.model==8200 && use_dmt) {
    arg_74_1= 0xff;
    arg_74_2= 0x90;
    arg_74_3= 0x04;
  }

  /*
#ifdef DEBUG
  fprintf(stderr,"canon: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_height,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
#endif
  */

  /* init printer */

  canon_cmd(prn,ESC5b,0x4b, 2, 0x00,0x0f);
  if (caps.features & CANON_CAP_CMD61)
    canon_cmd(prn,ESC5b,0x61, 1, 0x00,0x01);
  canon_cmd(prn,ESC28,0x62, 1, 0x01);
  canon_cmd(prn,ESC28,0x71, 1, 0x01);

  if (caps.features & CANON_CAP_CMD6d)
    canon_cmd(prn,ESC28,0x6d,12, arg_6d_1,
	      0xff,0xff,0x00,0x00,0x07,0x00,
	      arg_6d_a,arg_6d_b,arg_6d_2,0x00,arg_6d_3);

  /* set resolution */

  canon_cmd(prn,ESC28,0x64, 4, (ydpi >> 8 ), (ydpi & 255),
	                        (xdpi >> 8 ), (xdpi & 255));

  canon_cmd(prn,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);

  canon_cmd(prn,ESC28,0x63, 3, arg_63_1, arg_63_2, arg_63_3);

  if (caps.features & CANON_CAP_CMD70)
    canon_cmd(prn,ESC28,0x70, 8, arg_70_1, arg_70_2, 0x00, 0x00,
	                         arg_70_3, arg_70_4, 0x00, 0x00);

  canon_cmd(prn,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);

  if (caps.features & CANON_CAP_CMD72)
    canon_cmd(prn,ESC28,0x72, 1, 0x61); /* whatever for - 8200 might need it */

  /* some linefeeds */

  top= (top*ydpi)/72;
  PUT("topskip ",top,ydpi);
  canon_cmd(prn,ESC28,0x65, 2, (top >> 8 ),(top & 255));
}

/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */
static unsigned char *canon_alloc_buffer(int size)
{
  unsigned char *buf= malloc(size);
  if (buf) memset(buf,0,size);
  return buf;
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      and sets first line to 0s
 *                      accepts NULL pointers as buf
 *                  !!! buf must contain more than (num) lines !!!
 *                      also sets first line to 0s if num<1
 */
static void
canon_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}

/*
 * 'canon_print()' - Print an image to a CANON printer.
 */
void
canon_print(const printer_t *printer,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
	    const vars_t    *v)
{
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  char 		*resolution = v->resolution;
  char          *media_type = v->media_type;
  char          *media_source = v->media_source;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  char          *ink_type = v->ink_type;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*lyellow;	/* Light yellow bitmap data */
  int           delay_k,
                delay_c,
                delay_m,
                delay_y,
                delay_lc,
                delay_lm,
                delay_ly,
                delay_max;
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_length,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		length,		/* Length of raster data */
                buf_length,     /* Length of raster data buffer (dmt) */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
                image_width,
                image_bpp;
  int           use_dmt = 0;
  void *	dither;
  double        the_levels[] = { 0.5, 0.75, 1.0 };
  vars_t	nv;

  canon_cap_t caps= canon_get_model_capabilities(model);
  int printhead= canon_printhead_type(ink_type,caps);

  memcpy(&nv, v, sizeof(vars_t));
  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  */

  /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if (nv.image_type == IMAGE_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }

  if (printhead == 0 || caps.inks == CANON_INK_K)
    output_type = OUTPUT_GRAY;
  else if (image_bpp < 3 && cmap == NULL && output_type == OUTPUT_COLOR)
    output_type = OUTPUT_GRAY_COLOR;

  /*
   * Choose the correct color conversion function...
   */

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);

 /*
  * Figure out the output resolution...
  */

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);
#ifdef DEBUG
  fprintf(stderr,"canon: resolution=%dx%d\n",xdpi,ydpi);
#endif

  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") &&
      (caps.features & CANON_CAP_DMT) &&
      nv.image_type != IMAGE_MONOCHROME) {
    use_dmt= 1;
#ifdef DEBUG
    fprintf(stderr,"canon: using drop modulation technology\n");
#endif
  }

 /*
  * Compute the output size...
  */

  canon_imageable_area(printer, &nv, &page_left, &page_right,
                       &page_bottom, &page_top);

  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

  default_media_size(printer, &nv, &n, &page_length);

  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_top   ",page_top,72);
  PUT("page_bottom",page_bottom,72);
  PUT("page_left  ",page_left,72);
  PUT("page_right ",page_right,72);
  PUT("page_width ",page_width,72);
  PUT("page_height",page_height,72);
  PUT("page_length",page_length,72);
  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);
  */

  Image_progress_init(image);

  PUT("top     ",top,72);
  PUT("left    ",left,72);

  canon_init_printer(prn, caps, output_type, media_type,
		     &nv, printhead, media_source,
		     xdpi, ydpi, page_width, page_height,
		     top,left,use_dmt);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  /*
  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);
  */

  left = ydpi * left / 72;

  PUT("leftskip",left,xdpi);

  if(xdpi==1440){
    delay_k= 0;
    delay_c= 112;
    delay_m= 224;
    delay_y= 336;
    delay_lc= 112;
    delay_lm= 224;
    delay_ly= 0;
    delay_max= 336;
#ifdef DEBUG
    fprintf(stderr,"canon: delay on!\n");
#endif
  } else {
    delay_k= delay_c= delay_m= delay_y= delay_lc= delay_lm= delay_ly=0;
    delay_max=0;
#ifdef DEBUG
    fprintf(stderr,"canon: delay off!\n");
#endif
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (use_dmt) {
    buf_length= length*2;
  } else {
    buf_length= length;
  }

#ifdef DEBUG
  fprintf(stderr,"canon: buflength is %d!\n",buf_length);
#endif

  if (output_type == OUTPUT_GRAY) {
    black   = canon_alloc_buffer(buf_length*(delay_k+1));
    cyan    = NULL;
    magenta = NULL;
    lcyan   = NULL;
    lmagenta= NULL;
    yellow  = NULL;
    lyellow = NULL;
  } else {
    cyan    = canon_alloc_buffer(buf_length*(delay_c+1));
    magenta = canon_alloc_buffer(buf_length*(delay_m+1));
    yellow  = canon_alloc_buffer(buf_length*(delay_y+1));

    if ((caps.inks & CANON_INK_BLACK_MASK))
      black = canon_alloc_buffer(buf_length*(delay_k+1));
    else
      black = NULL;

    if (printhead==3 && (caps.inks & (CANON_INK_PHOTO_MASK))) {
      lcyan = canon_alloc_buffer(buf_length*(delay_lc+1));
      lmagenta = canon_alloc_buffer(buf_length*(delay_lm+1));
      if ((caps.inks & CANON_INK_CcMmYy))
	lyellow = canon_alloc_buffer(buf_length*(delay_lc+1));
      else
	lyellow = NULL;
    } else {
      lcyan = NULL;
      lmagenta = NULL;
      lyellow = NULL;
    }
  }
#ifdef DEBUG
  fprintf(stderr,"canon: driver will use colors ");
  if (cyan)     fputc('C',stderr);
  if (lcyan)    fputc('c',stderr);
  if (magenta)  fputc('M',stderr);
  if (lmagenta) fputc('m',stderr);
  if (yellow)   fputc('Y',stderr);
  if (lyellow)  fputc('y',stderr);
  if (black)    fputc('K',stderr);
  fprintf(stderr,"\n");
#endif

  nv.density *= ydpi / xdpi;
  if (nv.density > 1.0)
    nv.density = 1.0;
  compute_lut(256, &nv);

  if (xdpi > ydpi)
    dither = init_dither(image_width, out_width, 1, xdpi / ydpi, &nv);
  else
    dither = init_dither(image_width, out_width, ydpi / xdpi, 1, &nv);

  dither_set_black_levels(dither, 1.0, 1.0, 1.0);
  dither_set_black_lower(dither, .8 / ((1 << (use_dmt+1)) - 1));
  /*
  if (use_glossy_film)
  */
  dither_set_black_upper(dither, .999);
  /*
  else
    dither_set_black_upper(dither, .999);
  */

  if (!use_dmt) {
    dither_set_light_inks(dither,
			  (lcyan)   ? (0.3333) : (0.0),
			  (lmagenta)? (0.3333) : (0.0),
			  (lyellow) ? (0.3333) : (0.0), nv.density);
  }

  switch (nv.image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      dither_set_ink_spread(dither, 14);
      break;
    }
  dither_set_density(dither, nv.density);

  if (use_dmt)
    {
      dither_set_c_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_m_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_y_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_k_ranges_simple(dither, 3, the_levels, nv.density);
    }
 /*
  * Output the page...
  */

  in  = malloc(image_width * image_bpp);
  out = malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  for (y = 0; y < out_height; y ++)
  {
    if ((y & 255) == 0)
      Image_note_progress(image, y, out_height);

    if (errline != errlast)
    {
      errlast = errline;
      Image_get_row(image, in, errline);
    }

    (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv);

    if (output_type == OUTPUT_GRAY)
      {
	if (nv.image_type == IMAGE_MONOCHROME)
	  dither_fastblack(out, y, dither, black);
	else
	  dither_black(out, y, dither, black);
      } else {
	dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, lyellow, black);
      }

#ifdef DEBUG
    /* fprintf(stderr,","); */
#endif

    canon_write_line(prn, caps, ydpi,
		     black,    delay_k,
		     cyan,     delay_c,
		     magenta,  delay_m,
		     yellow,   delay_y,
		     lcyan,    delay_lc,
		     lmagenta, delay_lm,
		     lyellow,  delay_ly,
		     length, out_width, left, use_dmt);

#ifdef DEBUG
    /* fprintf(stderr,"!"); */
#endif

    canon_advance_buffer(black,   buf_length,delay_k);
    canon_advance_buffer(cyan,    buf_length,delay_c);
    canon_advance_buffer(magenta, buf_length,delay_m);
    canon_advance_buffer(yellow,  buf_length,delay_y);
    canon_advance_buffer(lcyan,   buf_length,delay_lc);
    canon_advance_buffer(lmagenta,buf_length,delay_lm);
    canon_advance_buffer(lyellow, buf_length,delay_ly);

    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }
  Image_progress_conclude(image);

  free_dither(dither);

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
#ifdef DEBUG
    fprintf(stderr,"\ncanon: flushing %d possibly delayed buffers\n",
	    delay_max);
#endif
    for (y= 0; y<delay_max; y++) {

      canon_write_line(prn, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c,
		       magenta,  delay_m,
		       yellow,   delay_y,
		       lcyan,    delay_lc,
		       lmagenta, delay_lm,
		       lyellow,  delay_ly,
		       length, out_width, left, use_dmt);

#ifdef DEBUG
      /* fprintf(stderr,"-"); */
#endif

      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);
    }
  }

 /*
  * Cleanup...
  */

  free_lut(&nv);
  free(in);
  free(out);

  if (black != NULL)    free(black);
  if (cyan != NULL)     free(cyan);
  if (magenta != NULL)  free(magenta);
  if (yellow != NULL)   free(yellow);
  if (lcyan != NULL)    free(lcyan);
  if (lmagenta != NULL) free(lmagenta);
  if (lyellow != NULL)  free(lyellow);

  /* eject page */
  fputc(0x0c,prn);

  /* say goodbye */
  canon_cmd(prn,ESC28,0x62,1,0);
  if (caps.features & CANON_CAP_CMD61)
    canon_cmd(prn,ESC5b,0x61, 1, 0x00,0x00);
  canon_cmd(prn,ESC40,0,0);
  fflush(prn);
}

/*
 * 'canon_fold_lsb_msb()' fold 2 lines in order lsb/msb
 */

static void
canon_fold_lsb_msb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 1) |
      ((line[0] & (1 << 6)) >> 2) |
      ((line[0] & (1 << 5)) >> 3) |
      ((line[0] & (1 << 4)) >> 4) |
      ((line[single_length] & (1 << 7)) >> 0) |
      ((line[single_length] & (1 << 6)) >> 1) |
      ((line[single_length] & (1 << 5)) >> 2) |
      ((line[single_length] & (1 << 4)) >> 3);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 3) |
      ((line[0] & (1 << 2)) << 2) |
      ((line[0] & (1 << 1)) << 1) |
      ((line[0] & (1 << 0)) << 0) |
      ((line[single_length] & (1 << 3)) << 4) |
      ((line[single_length] & (1 << 2)) << 3) |
      ((line[single_length] & (1 << 1)) << 2) |
      ((line[single_length] & (1 << 0)) << 1);
    line++;
    outbuf += 2;
  }
}

/*
 * 'canon_fold_msb_lsb()' fold 2 lines in order msb/lsb
 */

static void
canon_fold_msb_lsb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 0) |
      ((line[0] & (1 << 6)) >> 1) |
      ((line[0] & (1 << 5)) >> 2) |
      ((line[0] & (1 << 4)) >> 3) |
      ((line[single_length] & (1 << 7)) >> 1) |
      ((line[single_length] & (1 << 6)) >> 2) |
      ((line[single_length] & (1 << 5)) >> 3) |
      ((line[single_length] & (1 << 4)) >> 4);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 4) |
      ((line[0] & (1 << 2)) << 3) |
      ((line[0] & (1 << 1)) << 2) |
      ((line[0] & (1 << 0)) << 1) |
      ((line[single_length] & (1 << 3)) << 3) |
      ((line[single_length] & (1 << 2)) << 2) |
      ((line[single_length] & (1 << 1)) << 1) |
      ((line[single_length] & (1 << 0)) << 0);
    line++;
    outbuf += 2;
  }
}

static void
canon_pack(unsigned char *line,
	   int length,
	   unsigned char *comp_buf,
	   unsigned char **comp_ptr)
{
  unsigned char *start;			/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */

  /*
   * Compress using TIFF "packbits" run-length encoding...
   */

  (*comp_ptr) = comp_buf;

  while (length > 0)
    {
      /*
       * Get a run of non-repeated chars...
       */

      start  = line;
      line   += 2;
      length -= 2;

      while (length > 0 && (line[-2] != line[-1] || line[-1] != line[0]))
	{
	  line ++;
	  length --;
	}

      line   -= 2;
      length += 2;

      /*
       * Output the non-repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = tcount - 1;
	  memcpy((*comp_ptr) + 1, start, tcount);

	  (*comp_ptr) += tcount + 1;
	  start    += tcount;
	  count    -= tcount;
	}

      if (length <= 0)
	break;

      /*
       * Find the repeated sequences...
       */

      start  = line;
      repeat = line[0];

      line ++;
      length --;

      while (length > 0 && *line == repeat)
	{
	  line ++;
	  length --;
	}

      /*
       * Output the repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = 1 - tcount;
	  (*comp_ptr)[1] = repeat;

	  (*comp_ptr) += 2;
	  count    -= tcount;
	}
    }
}


/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(FILE          *prn,		/* I - Print file or command */
	    canon_cap_t   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           dmt)
{
  unsigned char
    comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    in_fold[COMPBUFWIDTH],
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int newlength;
  unsigned char color;

 /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  /* fold lsb/msb pairs if drop modulation is active */

  if (dmt) {
    if (1) {
      if (caps.features & CANON_CAP_MSB_FIRST)
	canon_fold_msb_lsb(line,length,in_fold);
      else
	canon_fold_lsb_msb(line,length,in_fold);
      in_ptr= in_fold;
    }
    length*= 2;
    offset*= 2;
  }

  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;
  if (offset) {
    int offset2= (offset+4)/8;
    while (offset2>0) {
      unsigned char toffset = offset2 > 128 ? 128 : offset2;
      comp_data[0] = 1 - toffset;
      comp_data[1] = 0;
      comp_data += 2;
      offset2-= toffset;
    }
  }

  canon_pack(in_ptr, length, comp_data, &comp_ptr);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */

  if (*empty) {
#ifdef DEBUG
    /* fprintf(stderr,"<%d%c>",*empty,("CMYKcmy"[coloridx])); */
#endif
    fwrite("\x1b\x28\x65\x02\x00", 5, 1, prn);
    fputc((*empty) >> 8 , prn);
    fputc((*empty) & 255, prn);
    *empty= 0;
  }

 /* Send a line of raster graphics... */

  fwrite("\x1b\x28\x41", 3, 1, prn);
  putc((newlength+1) & 255, prn);
  putc((newlength+1) >> 8, prn);
  color= "CMYKcmy"[coloridx];
  if (!color) color= 'K';
  putc(color,prn);
  fwrite(comp_buf, newlength, 1, prn);
  putc('\x0d', prn);
  return 1;
}


static void
canon_write_line(FILE          *prn,	/* I - Print file or command */
		 canon_cap_t   caps,	/* I - Printer model */
		 int           ydpi,	/* I - Vertical resolution */
		 unsigned char *k,	/* I - Output bitmap data */
		 int           dk,	/* I -  */
		 unsigned char *c,	/* I - Output bitmap data */
		 int           dc,	/* I -  */
		 unsigned char *m,	/* I - Output bitmap data */
		 int           dm,	/* I -  */
		 unsigned char *y,	/* I - Output bitmap data */
		 int           dy,	/* I -  */
		 unsigned char *lc,	/* I - Output bitmap data */
		 int           dlc,	/* I -  */
		 unsigned char *lm,	/* I - Output bitmap data */
		 int           dlm,	/* I -  */
		 unsigned char *ly,	/* I - Output bitmap data */
		 int           dly,	/* I -  */
		 int           l,	/* I - Length of bitmap data */
		 int           width,	/* I - Printed width */
		 int           offset,  /* I - horizontal offset */
		 int           dmt)
{
  static int empty= 0;
  int written= 0;

  if (k) written+=
    canon_write(prn, caps, k+ dk*l,  l, 3, ydpi, &empty, width, offset, dmt);
  if (y) written+=
    canon_write(prn, caps, y+ dy*l,  l, 2, ydpi, &empty, width, offset, dmt);
  if (m) written+=
    canon_write(prn, caps, m+ dm*l,  l, 1, ydpi, &empty, width, offset, dmt);
  if (c) written+=
    canon_write(prn, caps, c+ dc*l,  l, 0, ydpi, &empty, width, offset, dmt);
  if (ly) written+=
    canon_write(prn, caps, ly+dly*l, l, 6, ydpi, &empty, width, offset, dmt);
  if (lm) written+=
    canon_write(prn, caps, lm+dlm*l, l, 5, ydpi, &empty, width, offset, dmt);
  if (lc) written+=
    canon_write(prn, caps, lc+dlc*l, l, 4, ydpi, &empty, width, offset, dmt);

  if (written)
    fwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, prn);
  else
    empty++;
}
