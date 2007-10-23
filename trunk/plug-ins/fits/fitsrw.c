/******************************************************************************/
/*                      Peter Kirchgessner                                    */
/*                      e-mail: peter@kirchgessner.net                        */
/*                      WWW   : http://www.kirchgessner.net                   */
/******************************************************************************/
/*  #BEG-HDR                                                                  */
/*                                                                            */
/*  Package       : FITS reading/writing library                              */
/*  Modul-Name    : fitsrw.c                                                  */
/*  Description   : Support of reading/writing FITS-files                     */
/*  Function(s)   : fits_new_filestruct    - (local) initialize file structure*/
/*                  fits_new_hdulist       - (local) initialize hdulist struct*/
/*                  fits_delete_filestruct - (local) delete file structure    */
/*                  fits_delete_recordlist - (local) delete record list       */
/*                  fits_delete_hdulist    - (local) delete hdu list          */
/*                  fits_nan_32            - (local) check IEEE NaN values    */
/*                  fits_nan_64            - (local) check IEEE NaN values    */
/*                  fits_get_error         - get error message                */
/*                  fits_set_error         - (local) set error message        */
/*                  fits_drop_error        - (local) remove an error message  */
/*                  fits_open              - open a FITS file                 */
/*                  fits_close             - close a FITS file                */
/*                  fits_add_hdu           - add a HDU to a FITS file         */
/*                  fits_add_card          - add a card to the HDU            */
/*                  fits_print_header      - print a single FITS header       */
/*                  fits_read_header       - (local) read in FITS header      */
/*                  fits_write_header      - write a FITS header              */
/*                  fits_decode_header     - (local) decode a header          */
/*                  fits_eval_pixrange     - (local) evaluate range of pixels */
/*                  fits_decode_card       - decode a card                    */
/*                  fits_search_card       - search a card in a record list   */
/*                  fits_image_info        - get information about image      */
/*                  fits_seek_image        - position to an image             */
/*                  fits_read_pixel        - read pixel values from file      */
/*                  fits_to_pgmraw         - convert FITS-file to PGM-file    */
/*                  pgmraw_to_fits         - convert PGM-file to FITS-file    */
/*                                                                            */
/*  Author        : P. Kirchgessner                                           */
/*  Date of Gen.  : 12-Apr-97                                                 */
/*  Last modified : 25-Aug-06                                                 */
/*  Version       : 0.12                                                      */
/*  Compiler Opt. :                                                           */
/*  Changes       :                                                           */
/*  #MOD-0001, nn, 20-Dec-97, Initialize some variables                       */
/*  #MOD-0002, pk, 16-Aug-06, Fix problems with internationalization          */
/*                                                                            */
/*  #END-HDR                                                                  */
/******************************************************************************/
/*  References:                                                               */
/*  - NOST, Definition of the Flexible Image Transport System (FITS),         */
/*    September 29, 1995, Standard, NOST 100-1.1                              */
/*    (ftp://nssdc.gsfc.nasa.gov/pub/fits/fits_standard_ps.Z)                 */
/*  - The FITS IMAGE Extension. A Proposal. J.D. Ponz, R.W. Thompson,         */
/*    J.R. Munoz, Feb. 7, 1992                                                */
/*    (ftp://www.cv.nrao.edu/fits/documents/standards/image.ps.gz)            */
/*                                                                            */
/******************************************************************************/

#define                                    VERSIO  0.12

/******************************************************************************/
/* FITS reading/writing library                                               */
/* Copyright (C) 1997 Peter Kirchgessner                                      */
/* (email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)          */
/* The library was developed for a FITS-plug-in to GIMP, the GNU Image        */
/* Manipulation Program. But it is completely independant to that (beside use */
/* of glib). If someone finds it useful for other purposes, try to keep it    */
/* independant from your application.                                         */
/******************************************************************************/
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/******************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "fitsrw.h"


/* Declaration of local funtions */

static FITS_FILE *fits_new_filestruct (void);
static FITS_HDU_LIST *fits_new_hdulist (void);
static void fits_delete_filestruct (FITS_FILE *ff);
static void fits_delete_recordlist (FITS_RECORD_LIST *rl);
static void fits_delete_hdulist (FITS_HDU_LIST *hl);
static int  fits_nan_32 (unsigned char *value);
static int  fits_nan_64 (unsigned char *value);
static void fits_set_error (char *errmsg);
static void fits_drop_error (void);
static FITS_RECORD_LIST *fits_read_header (FILE *fp, int *nrec);
static FITS_HDU_LIST *fits_decode_header (FITS_RECORD_LIST *hdr,
                        long hdr_offset, long dat_offset);
static int  fits_eval_pixrange (FILE *fp, FITS_HDU_LIST *hdu);


/* Error handling like a FIFO */
#define FITS_MAX_ERROR      16
#define FITS_ERROR_LENGTH  256
static int fits_n_error = 0;
static char fits_error[FITS_MAX_ERROR][FITS_ERROR_LENGTH];

/* What byte ordering for IEEE-format we are running on ? */
static int fits_ieee32_intel = 0;
static int fits_ieee32_motorola = 0;
static int fits_ieee64_intel = 0;
static int fits_ieee64_motorola = 0;

/* Macros */
#define FITS_RETURN(msg, val) { fits_set_error (msg); return (val); }
#define FITS_VRETURN(msg) { fits_set_error (msg); return; }

/* Get pixel values from memory. p must be an (unsigned char *) */
#define FITS_GETBITPIX16(p,val) val = ((p[0] << 8) | (p[1]))
#define FITS_GETBITPIX32(p,val) val = \
          ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3])

/* Get floating point values from memory. p must be an (unsigned char *). */
/* The floating point values must directly correspond */
/* to machine representation. Otherwise it does not work. */
#define FITS_GETBITPIXM32(p,val) \
 { if (fits_ieee32_intel) {unsigned char uc[4]; \
   uc[0] = p[3]; uc[1] = p[2]; uc[2] = p[1]; uc[3] = p[0]; \
   val = *(FITS_BITPIXM32 *)uc; } \
   else if (fits_ieee32_motorola) { val = *(FITS_BITPIXM32 *)p; } \
   else if (fits_ieee64_motorola) {FITS_BITPIXM64 m64; \
   unsigned char *uc= (unsigned char *)&m64; \
   uc[0]=p[0]; uc[1]=p[1]; uc[2]=p[2]; uc[3]=p[3]; uc[4]=uc[5]=uc[6]=uc[7]=0; \
   val = (FITS_BITPIXM32)m64; } \
   else if (fits_ieee64_intel) {FITS_BITPIXM64 i64; \
   unsigned char *uc= (unsigned char *)&i64; \
   uc[0]=uc[1]=uc[2]=uc[3]=0; uc[7]=p[0]; uc[6]=p[1]; uc[5]=p[2]; uc[4]=p[3]; \
   val = (FITS_BITPIXM32)i64;}\
}

#define FITS_GETBITPIXM64(p,val) \
 { if (fits_ieee64_intel) {unsigned char uc[8]; \
   uc[0] = p[7]; uc[1] = p[6]; uc[2] = p[5]; uc[3] = p[4]; \
   uc[4] = p[3]; uc[5] = p[2]; uc[6] = p[1]; uc[7] = p[0]; \
   val = *(FITS_BITPIXM64 *)uc; } else val = *(FITS_BITPIXM64 *)p; }

#define FITS_WRITE_BOOLCARD(fp,key,value) \
{char card[81]; \
 sprintf (card, "%-8.8s= %20s%50s", key, value ? "T" : "F", " "); \
 fwrite (card, 1, 80, fp); }

#define FITS_WRITE_LONGCARD(fp,key,value) \
{char card[81]; \
 sprintf (card, "%-8.8s= %20ld%50s", key, (long)value, " "); \
 fwrite (card, 1, 80, fp); }

#define FITS_WRITE_DOUBLECARD(fp,key,value) \
{char card[81], dbl[21], *istr; \
 g_ascii_formatd (dbl, sizeof(dbl), "%f", (gdouble)value); \
 istr = strstr (dbl, "e"); \
 if (istr) *istr = 'E'; \
 sprintf (card, "%-8.8s= %20.20s%50s", key, dbl, " "); \
 fwrite (card, 1, 80, fp); }

#define FITS_WRITE_STRINGCARD(fp,key,value) \
{char card[81]; int k;\
 sprintf (card, "%-8.8s= \'%s", key, value); \
 for (k = strlen (card); k < 81; k++) card[k] = ' '; \
 k = strlen (key); if (k < 8) card[19] = '\''; else card[11+k] = '\''; \
 fwrite (card, 1, 80, fp); }

#define FITS_WRITE_CARD(fp,value) \
{char card[81]; \
 sprintf (card, "%-80.80s", value); \
 fwrite (card, 1, 80, fp); }


/* Macro to convert a double value to a string using '.' as decimal point */
#define FDTOSTR(buf,val) g_ascii_dtostr (buf, sizeof(buf), (gdouble)(val))


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_scanfdouble - (local) scan a string for a double value   */
/*                                                                           */
/* Parameters:                                                               */
/* const char *buf  [I] : the string to scan                                 */
/* double *value    [O] : where to write the double value to                 */
/*                                                                           */
/* Scan a string for a double value represented in "C" locale.               */
/* On success 1 is returned. On failure 0 is returned.                       */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static int fits_scanfdouble (const char *buf, double *value)
{
 int retval = 0;
 gchar *bufcopy = g_strdup (buf);

 /* We should use g_ascii_strtod. This also allows scanning of hexadecimal */
 /* values like 0x02. But we want the behaviour of sscanf ("0x02","%lf",...*/
 /* that gives 0.0 in this case. So check the string if we have a hex-value*/

 if ( bufcopy )
 {
   gchar *bufptr = bufcopy;

   /* Remove leading white space */
   g_strchug (bufcopy);

   /* Skip leading sign character */
   if ( (*bufptr == '-') || (*bufptr == '+') )
     bufptr++;

   /* Start of hex value ? Take this as 0.0 */
   if ( (bufptr[0] == '0') && (g_ascii_toupper (bufptr[1]) == 'X') )
   {
     *value = 0.0;
     retval = 1;
   }
   else
   {
     if ( *bufptr == '.' ) /* leading decimal point ? Skip it */
       bufptr++;

     if (g_ascii_isdigit (*bufptr)) /* Expect the complete string is decimal */
     {
       gchar *endptr;
       gdouble gvalue = g_ascii_strtod (bufcopy, &endptr);
       if ( errno == 0 )
       {
         *value = gvalue;
         retval = 1;
       }
     }
   }
   g_free (bufcopy);
 }
 return retval;
}

/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_new_filestruct - (local) initialize file structure       */
/*                                                                           */
/* Parameters:                                                               */
/* -none-                                                                    */
/*                                                                           */
/* Returns a pointer to an initialized fits file structure.                  */
/* On failure, a NULL-pointer is returned.                                   */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static FITS_FILE *fits_new_filestruct (void)

{FITS_FILE *ff;

 ff = (FITS_FILE *)malloc (sizeof (FITS_FILE));
 if (ff == NULL) return (NULL);

 memset ((char *)ff, 0, sizeof (*ff));
 return (ff);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_new_hdulist    - (local) initialize hdulist structure    */
/*                                                                           */
/* Parameters:                                                               */
/* -none-                                                                    */
/*                                                                           */
/* Returns a pointer to an initialized hdulist structure.                    */
/* On failure, a NULL-pointer is returned.                                   */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static FITS_HDU_LIST *fits_new_hdulist (void)

{FITS_HDU_LIST *hdl;

 hdl = (FITS_HDU_LIST *)malloc (sizeof (FITS_HDU_LIST));
 if (hdl == NULL) return (NULL);

 memset ((char *)hdl, 0, sizeof (*hdl));
 hdl->pixmin = hdl->pixmax = hdl->datamin = hdl->datamax = 0.0;
 hdl->bzero = hdl->bscale = 0.0;

 return (hdl);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_delete_filestruct - (local) delete file structure        */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff   [I] : pointer to fits file structure                      */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Frees all memory allocated by the file structure.                         */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static void fits_delete_filestruct (FITS_FILE *ff)

{
 if (ff == NULL) return;

 fits_delete_hdulist (ff->hdu_list);
 ff->hdu_list = NULL;

 ff->fp = NULL;
 free ((char *)ff);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_delete_recordlist - (local) delete record list           */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_RECORD_LIST *rl  [I] : record list to delete                         */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static void fits_delete_recordlist (FITS_RECORD_LIST *rl)

{FITS_RECORD_LIST *next;

 while (rl != NULL)
 {
   next = rl->next_record;
   rl->next_record = NULL;
   free ((char *)rl);
   rl = next;
 }
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_delete_hdulist - (local) delete hdu list                 */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_HDU_LIST *hl  [I] : hdu list to delete                               */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static void fits_delete_hdulist (FITS_HDU_LIST *hl)

{FITS_HDU_LIST *next;

 while (hl != NULL)
 {
   fits_delete_recordlist (hl->header_record_list);
   next = hl->next_hdu;
   hl->next_hdu = NULL;
   free ((char *)hl);
   hl = next;
 }
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_nan_32 - (local) check for IEEE NaN values (32 bit)      */
/*                                                                           */
/* Parameters:                                                               */
/* unsigned char *v     [I] : value to check                                 */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function returns 1 if the value is a NaN. Otherwise 0 is returned.    */
/* The byte sequence at v must start with the sign/eponent byte.             */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static int fits_nan_32 (unsigned char *v)

{register unsigned long k;

 k = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
 k &= 0x7fffffff;  /* Dont care about the sign bit */

 /* See NOST Definition of the Flexible Image Transport System (FITS), */
 /* Appendix F, IEEE special formats. */
 return (   ((k >= 0x7f7fffff) && (k <= 0x7fffffff))
         || ((k >= 0x00000001) && (k <= 0x00800000)));
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_nan_64 - (local) check for IEEE NaN values (64 bit)      */
/*                                                                           */
/* Parameters:                                                               */
/* unsigned char *v     [I] : value to check                                 */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function returns 1 if the value is a NaN. Otherwise 0 is returned.    */
/* The byte sequence at v must start with the sign/eponent byte.             */
/* (currently we ignore the low order 4 bytes of the mantissa. Therefore     */
/* this function is the same as for 32 bits).                                */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static int fits_nan_64 (unsigned char *v)

{register unsigned long k;

 k = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
 k &= 0x7fffffff;  /* Dont care about the sign bit */

 /* See NOST Definition of the Flexible Image Transport System (FITS), */
 /* Appendix F, IEEE special formats. */
 return (   ((k >= 0x7f7fffff) && (k <= 0x7fffffff))
         || ((k >= 0x00000001) && (k <= 0x00800000)));
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_get_error - get an error message                         */
/*                                                                           */
/* Parameters:                                                               */
/* -none-                                                                    */
/*                                                                           */
/* If an error message has been set, a pointer to the message is returned.   */
/* Otherwise a NULL pointer is returned.                                     */
/* An inquired error message is removed from the error FIFO.                 */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

char *fits_get_error (void)

{static char errmsg[FITS_ERROR_LENGTH];
 int k;

 if (fits_n_error <= 0) return (NULL);
 strcpy (errmsg, fits_error[0]);

 for (k = 1; k < fits_n_error; k++)
   strcpy (fits_error[k-1], fits_error[k]);

 fits_n_error--;

 return (errmsg);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_set_error - (local) set an error message                 */
/*                                                                           */
/* Parameters:                                                               */
/* char *errmsg   [I] : Error message to set                                 */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Places the error message in the FIFO. If the FIFO is full,                */
/* the message is discarded.                                                 */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static void fits_set_error (char *errmsg)

{
  if (fits_n_error < FITS_MAX_ERROR)
  {
    strncpy (fits_error[fits_n_error], errmsg, FITS_ERROR_LENGTH);
    fits_error[fits_n_error++][FITS_ERROR_LENGTH-1] = '\0';
  }
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_drop_error - (local) remove an error message             */
/*                                                                           */
/* Parameters:                                                               */
/* -none-                                                                    */
/*                                                                           */
/* Removes the last error message from the error message FIFO                */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static void fits_drop_error (void)

{
 if (fits_n_error > 0) fits_n_error--;
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_open - open a FITS file                                  */
/*                                                                           */
/* Parameters:                                                               */
/* char *filename   [I] : name of file to open                               */
/* char *openmode   [I] : mode to open the file ("r", "w")                   */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* On success, a FITS_FILE-pointer is returned. On failure, a NULL-          */
/* pointer is returned.                                                      */
/* The functions scans through the file loading each header and analyzing    */
/* them.                                                                     */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

FITS_FILE *fits_open (const char *filename, const char *openmode)

{int reading, writing, n_rec, n_hdr;
 long fpos_header, fpos_data;
 FILE *fp;
 FITS_FILE *ff;
 FITS_RECORD_LIST *hdrlist;
 FITS_HDU_LIST *hdulist, *last_hdulist;

 /* initialize */

 hdulist = NULL;
 last_hdulist = NULL;

 /* Check the IEEE-format we are running on */
 {float one32 = 1.0;
  double one64 = 1.0;
  unsigned char *op32 = (unsigned char *)&one32;
  unsigned char *op64 = (unsigned char *)&one64;

  if (sizeof (float) == 4)
  {
    fits_ieee32_intel = (op32[3] == 0x3f);
    fits_ieee32_motorola = (op32[0] == 0x3f);
  }
  if (sizeof (double) == 8)
  {
    fits_ieee64_intel = (op64[7] == 0x3f);
    fits_ieee64_motorola = (op64[0] == 0x3f);
  }
 }

 if ((filename == NULL) || (*filename == '\0') || (openmode == NULL))
   FITS_RETURN ("fits_open: Invalid parameters", NULL);

 reading = (strcmp (openmode, "r") == 0);
 writing = (strcmp (openmode, "w") == 0);
 if ((!reading) && (!writing))
   FITS_RETURN ("fits_open: Invalid openmode", NULL);

 fp = g_fopen (filename, reading ? "rb" : "wb");
 if (fp == NULL) FITS_RETURN ("fits_open: fopen() failed", NULL);

 ff = fits_new_filestruct ();
 if (ff == NULL)
 {
   fclose (fp);
   FITS_RETURN ("fits_open: No more memory", NULL);
 }

 ff->fp = fp;
 ff->openmode = *openmode;

 if (writing) return (ff);

 for (n_hdr = 0; ; n_hdr++)   /* Read through all HDUs */
 {
   fpos_header = ftell (fp);    /* Save file position of header */
   hdrlist = fits_read_header (fp, &n_rec);

   if (hdrlist == NULL)
   {
     if (n_hdr > 0)        /* At least one header must be present. */
       fits_drop_error (); /* If we got a header already, drop the error */
     break;
   }
   fpos_data = ftell (fp);      /* Save file position of data */

                           /* Decode the header */
   hdulist = fits_decode_header (hdrlist, fpos_header, fpos_data);
   if (hdulist == NULL)
   {
     fits_delete_recordlist (hdrlist);
     break;
   }
   ff->n_hdu++;
   ff->n_pic += hdulist->numpic;

   if (hdulist->used.blank_value) ff->blank_used = 1;
   if (hdulist->used.nan_value) ff->nan_used = 1;

   if (n_hdr == 0)
     ff->hdu_list = hdulist;
   else
     last_hdulist->next_hdu = hdulist;
   last_hdulist = hdulist;
                           /* Evaluate the range of pixel data */
   fits_eval_pixrange (fp, hdulist);

   /* Reposition to start of next header */
   if (fseek (fp, hdulist->data_offset+hdulist->data_size, SEEK_SET) < 0)
     break;
 }

 return (ff);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_close - close a FITS file                                */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff  [I] : FITS file pointer                                    */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

void fits_close (FITS_FILE *ff)

{
 if (ff == NULL) FITS_VRETURN ("fits_close: Invalid parameter");

 fclose (ff->fp);

 fits_delete_filestruct (ff);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_add_hdu - add a HDU to the file                          */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff  [I] : FITS file pointer                                    */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Adds a new HDU to the list kept in ff. A pointer to the new HDU is        */
/* returned. On failure, a NULL-pointer is returned.                         */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

FITS_HDU_LIST *fits_add_hdu (FITS_FILE *ff)

{FITS_HDU_LIST *newhdu, *hdu;

 if (ff->openmode != 'w')
   FITS_RETURN ("fits_add_hdu: file not open for writing", NULL);

 newhdu = fits_new_hdulist ();
 if (newhdu == NULL) return (NULL);

 if (ff->hdu_list == NULL)
 {
   ff->hdu_list = newhdu;
 }
 else
 {
   hdu = ff->hdu_list;
   while (hdu->next_hdu != NULL)
     hdu = hdu->next_hdu;
   hdu->next_hdu = newhdu;
 }

 return (newhdu);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_add_card - add a card to the HDU                         */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_HDU_LIST *hdulist [I] : HDU listr                                    */
/* char *card             [I] : card to add                                  */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The card must follow the standards of FITS. The card must not use a       */
/* keyword that is written using *hdulist itself. On success 0 is returned.  */
/* On failure -1 is returned.                                                */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

int fits_add_card (FITS_HDU_LIST *hdulist, char *card)

{int k;

 if (hdulist->naddcards >= FITS_NADD_CARDS) return (-1);

 k = strlen (card);
 if (k < FITS_CARD_SIZE)
 {
   memset (&(hdulist->addcards[hdulist->naddcards][k]), ' ', FITS_CARD_SIZE-k);
   memcpy (hdulist->addcards[(hdulist->naddcards)++], card, k);
 }
 else
 {
   memcpy (hdulist->addcards[(hdulist->naddcards)++], card, FITS_CARD_SIZE);
 }
 return (0);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_print_header - print the internal representation         */
/*                                 of a single header                        */
/* Parameters:                                                               */
/* FITS_HDU_LIST *hdr  [I] : pointer to the header                           */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

void fits_print_header (FITS_HDU_LIST *hdr)

{int k;
 char buf[G_ASCII_DTOSTR_BUF_SIZE];

 if (hdr->used.simple)
   printf ("Content of SIMPLE-header:\n");
 else
   printf ("Content of XTENSION-header %s:\n", hdr->xtension);
 printf ("header_offset : %ld\n", hdr->header_offset);
 printf ("data_offset   : %ld\n", hdr->data_offset);
 printf ("data_size     : %ld\n", hdr->data_size);
 printf ("used data_size: %ld\n", hdr->udata_size);
 printf ("bytes p.pixel : %d\n", hdr->bpp);
 printf ("pixmin        : %s\n", FDTOSTR (buf, hdr->pixmin));
 printf ("pixmax        : %s\n", FDTOSTR (buf, hdr->pixmax));

 printf ("naxis         : %d\n", hdr->naxis);
 for (k = 1; k <= hdr->naxis; k++)
   printf ("naxis%-3d      : %d\n", k, hdr->naxisn[k-1]);

 printf ("bitpix        : %d\n", hdr->bitpix);

 if (hdr->used.blank)
   printf ("blank         : %ld\n", hdr->blank);
 else
   printf ("blank         : not used\n");

 if (hdr->used.datamin)
   printf ("datamin       : %s\n", FDTOSTR (buf, hdr->datamin));
 else
   printf ("datamin       : not used\n");
 if (hdr->used.datamax)
   printf ("datamax       : %s\n", FDTOSTR (buf, hdr->datamax));
 else
   printf ("datamax       : not used\n");

 if (hdr->used.gcount)
   printf ("gcount        : %ld\n", hdr->gcount);
 else
   printf ("gcount        : not used\n");
 if (hdr->used.pcount)
   printf ("pcount        : %ld\n", hdr->pcount);
 else
   printf ("pcount        : not used\n");

 if (hdr->used.bscale)
   printf ("bscale        : %s\n", FDTOSTR (buf, hdr->bscale));
 else
   printf ("bscale        : not used\n");
 if (hdr->used.bzero)
   printf ("bzero         : %s\n", FDTOSTR (buf, hdr->bzero));
 else
   printf ("bzero         : not used\n");
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_read_header - (local) read FITS header                   */
/*                                                                           */
/* Parameters:                                                               */
/* FILE *fp   [I] : file pointer                                             */
/* int  *nrec [O] : number of records read                                   */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Reads in all header records up to the record that keeps the END-card.     */
/* A pointer to the record list is returned on success.                      */
/* On failure, a NULL-pointer is returned.                                   */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static FITS_RECORD_LIST *fits_read_header (FILE *fp, int *nrec)

{char record[FITS_RECORD_SIZE];
 FITS_RECORD_LIST *start_list = NULL, *cu_record = NULL, *new_record;
 FITS_DATA *fdat;
 int k, simple, xtension;

 *nrec = 0;

 k = fread (record, 1, FITS_RECORD_SIZE, fp);
 if (k != FITS_RECORD_SIZE)
   FITS_RETURN ("fits_read_header: Error in read of first record", NULL);

 simple = (strncmp (record, "SIMPLE  ", 8) == 0);
 xtension = (strncmp (record, "XTENSION", 8) == 0);
 if ((!simple) && (!xtension))
   FITS_RETURN ("fits_read_header: Missing keyword SIMPLE or XTENSION", NULL);

 if (simple)
 {
   fdat = fits_decode_card (record, typ_fbool);
   if (fdat && !fdat->fbool)
     fits_set_error ("fits_read_header (warning): keyword SIMPLE does not have\
 value T");
 }

 for (;;)   /* Process all header records */
 {
   new_record = (FITS_RECORD_LIST *)malloc (sizeof (FITS_RECORD_LIST));
   if (new_record == NULL)
   {
     fits_delete_recordlist (start_list);
     FITS_RETURN ("fits_read_header: Not enough memory", NULL);
   }
   memcpy (new_record->data, record, FITS_RECORD_SIZE);
   new_record->next_record = NULL;
   (*nrec)++;

   if (start_list == NULL)      /* Add new record to the list */
     start_list = new_record;
   else
     cu_record->next_record = new_record;

   cu_record = new_record;
                                /* Was this the last record ? */
   if (fits_search_card (cu_record, "END") != NULL) break;

    k = fread (record, 1, FITS_RECORD_SIZE, fp);
    if (k != FITS_RECORD_SIZE)
      FITS_RETURN ("fits_read_header: Error in read of record", NULL);
 }
 return (start_list);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_write_header - write a FITS header                       */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff [I] : FITS-file pointer                                     */
/* FITS_HDU_LIST [I] : pointer to header                                     */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Writes a header to the file. On success, 0 is returned. On failure,       */
/* -1 is returned.                                                           */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

int fits_write_header (FITS_FILE *ff, FITS_HDU_LIST *hdulist)

{int numcards;
 int k;

 if (ff->openmode != 'w')
   FITS_RETURN ("fits_write_header: file not open for writing", -1);

 numcards = 0;

 if (hdulist->used.simple)
 {
   FITS_WRITE_BOOLCARD (ff->fp, "SIMPLE", 1);
   numcards++;
 }
 else if (hdulist->used.xtension)
 {
   FITS_WRITE_STRINGCARD (ff->fp, "XTENSION", hdulist->xtension);
   numcards++;
 }

 FITS_WRITE_LONGCARD (ff->fp, "BITPIX", hdulist->bitpix);
 numcards++;

 FITS_WRITE_LONGCARD (ff->fp, "NAXIS", hdulist->naxis);
 numcards++;

 for (k = 0; k < hdulist->naxis; k++)
 {char naxisn[10];
   sprintf (naxisn, "NAXIS%d", k+1);
   FITS_WRITE_LONGCARD (ff->fp, naxisn, hdulist->naxisn[k]);
   numcards++;
 }

 if (hdulist->used.extend)
 {
   FITS_WRITE_BOOLCARD (ff->fp, "EXTEND", hdulist->extend);
   numcards++;
 }

 if (hdulist->used.groups)
 {
   FITS_WRITE_BOOLCARD (ff->fp, "GROUPS", hdulist->groups);
   numcards++;
 }

 if (hdulist->used.pcount)
 {
   FITS_WRITE_LONGCARD (ff->fp, "PCOUNT", hdulist->pcount);
   numcards++;
 }
 if (hdulist->used.gcount)
 {
   FITS_WRITE_LONGCARD (ff->fp, "GCOUNT", hdulist->gcount);
   numcards++;
 }

 if (hdulist->used.bzero)
 {
   FITS_WRITE_DOUBLECARD (ff->fp, "BZERO", hdulist->bzero);
   numcards++;
 }
 if (hdulist->used.bscale)
 {
   FITS_WRITE_DOUBLECARD (ff->fp, "BSCALE", hdulist->bscale);
   numcards++;
 }

 if (hdulist->used.datamin)
 {
   FITS_WRITE_DOUBLECARD (ff->fp, "DATAMIN", hdulist->datamin);
   numcards++;
 }
 if (hdulist->used.datamax)
 {
   FITS_WRITE_DOUBLECARD (ff->fp, "DATAMAX", hdulist->datamax);
   numcards++;
 }

 if (hdulist->used.blank)
 {
   FITS_WRITE_LONGCARD (ff->fp, "BLANK", hdulist->blank);
   numcards++;
 }

 /* Write additional cards */
 if (hdulist->naddcards > 0)
 {
   fwrite (hdulist->addcards, FITS_CARD_SIZE, hdulist->naddcards, ff->fp);
   numcards += hdulist->naddcards;
 }

 FITS_WRITE_CARD (ff->fp, "END");
 numcards++;

 k = (numcards*FITS_CARD_SIZE) % FITS_RECORD_SIZE;
 if (k)  /* Must the record be filled up ? */
 {
   while (k++ < FITS_RECORD_SIZE)
     putc (' ', ff->fp);
 }


 return (ferror (ff->fp) ? -1 : 0);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_decode_header - (local) decode a header                  */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_RECORD_LIST *hdr  [I] : the header record list                       */
/* long hdr_offset        [I] : fileposition of header                       */
/* long dat_offset        [I] : fileposition of data (end of header)         */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function decodes the mostly used data within the header and generates */
/* a FITS_HDU_LIST-entry. On failure, a NULL-pointer is returned.            */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static FITS_HDU_LIST *fits_decode_header (FITS_RECORD_LIST *hdr,
                        long hdr_offset, long dat_offset)

{FITS_HDU_LIST *hdulist;
 FITS_DATA *fdat;
 char errmsg[80], key[9];
 int k, bpp, random_groups;
 long mul_axis, data_size, bitpix_supported;

#define FITS_DECODE_CARD(mhdr,mkey,mfdat,mtyp) \
 {strcpy (key, mkey); \
  mfdat = fits_decode_card (fits_search_card (mhdr, mkey), mtyp); \
  if (mfdat == NULL) goto err_missing; }

#define FITS_TRY_CARD(mhdr,mhdu,mkey,mvar,mtyp,unionvar) \
 {FITS_DATA *mfdat = fits_decode_card (fits_search_card (mhdr,mkey), mtyp); \
  mhdu->used.mvar = (mfdat != NULL); \
  if (mhdu->used.mvar) mhdu->mvar = mfdat->unionvar; }

 hdulist = fits_new_hdulist ();
 if (hdulist == NULL)
   FITS_RETURN ("fits_decode_header: Not enough memory", NULL);

 /* Initialize the header data */
 hdulist->header_offset = hdr_offset;
 hdulist->data_offset = dat_offset;

 hdulist->used.simple = (strncmp (hdr->data, "SIMPLE  ", 8) == 0);
 hdulist->used.xtension = (strncmp (hdr->data, "XTENSION", 8) == 0);
 if (hdulist->used.xtension)
 {
   fdat = fits_decode_card (fits_search_card (hdr, "XTENSION"), typ_fstring);
   strcpy (hdulist->xtension, fdat->fstring);
 }

 FITS_DECODE_CARD (hdr, "NAXIS", fdat, typ_flong);
 hdulist->naxis = fdat->flong;

 FITS_DECODE_CARD (hdr, "BITPIX", fdat, typ_flong);
 bpp = hdulist->bitpix = (int)fdat->flong;
 if (   (bpp != 8) && (bpp != 16) && (bpp != 32)
     && (bpp != -32) && (bpp != -64))
 {
   strcpy (errmsg, "fits_decode_header: Invalid BITPIX-value");
   goto err_return;
 }
 if (bpp < 0) bpp = -bpp;
 bpp /= 8;
 hdulist->bpp = bpp;

 FITS_TRY_CARD (hdr, hdulist, "GCOUNT", gcount, typ_flong, flong);
 FITS_TRY_CARD (hdr, hdulist, "PCOUNT", pcount, typ_flong, flong);

 FITS_TRY_CARD (hdr, hdulist, "GROUPS", groups, typ_fbool, fbool);
 random_groups = hdulist->used.groups && hdulist->groups;

 FITS_TRY_CARD (hdr, hdulist, "EXTEND", extend, typ_fbool, fbool);

 if (hdulist->used.xtension)  /* Extension requires GCOUNT and PCOUNT */
 {
   if ((!hdulist->used.gcount) || (!hdulist->used.pcount))
   {
     strcpy (errmsg, "fits_decode_header: Missing GCOUNT/PCOUNT for XTENSION");
     goto err_return;
   }
 }

 mul_axis = 1;

 /* Find all NAXISx-cards */
 for (k = 1; k <= FITS_MAX_AXIS; k++)
 {char naxisn[9];

   sprintf (naxisn, "NAXIS%-3d", k);
   fdat = fits_decode_card (fits_search_card (hdr, naxisn), typ_flong);
   if (fdat == NULL)
   {
     k--;   /* Save the last NAXISk read */
     break;
   }
   hdulist->naxisn[k-1] = (int)fdat->flong;
   if (hdulist->naxisn[k-1] < 0)
   {
     strcpy (errmsg, "fits_decode_header: Negative value in NAXISn");
     goto err_return;
   }
   if ((k == 1) && (random_groups))
   {
     if (hdulist->naxisn[0] != 0)
     {
       strcpy (errmsg, "fits_decode_header: Random groups with NAXIS1 != 0");
       goto err_return;
     }
   }
   else
     mul_axis *= hdulist->naxisn[k-1];
 }

 if ((hdulist->naxis > 0) && (k < hdulist->naxis))
 {
   strcpy (errmsg, "fits_decode_card: Not enough NAXISn-cards");
   goto err_return;
 }

 /* If we have only one dimension, just set the second to size one. */
 /* So we dont have to check for naxis < 2 in some places. */
 if (hdulist->naxis < 2)
   hdulist->naxisn[1] = 1;
 if (hdulist->naxis < 1)
 {
   mul_axis = 0;
   hdulist->naxisn[0] = 1;
 }

 if (hdulist->used.xtension)
   data_size = bpp*hdulist->gcount*(hdulist->pcount + mul_axis);
 else
   data_size = bpp*mul_axis;
 hdulist->udata_size = data_size;  /* Used data size without padding */

 /* Datasize must be a multiple of the FITS logical record size */
 data_size = (data_size + FITS_RECORD_SIZE - 1) / FITS_RECORD_SIZE;
 data_size *= FITS_RECORD_SIZE;
 hdulist->data_size = data_size;


 FITS_TRY_CARD (hdr, hdulist, "BLANK", blank, typ_flong, flong);

 FITS_TRY_CARD (hdr, hdulist, "DATAMIN", datamin, typ_fdouble, fdouble);
 FITS_TRY_CARD (hdr, hdulist, "DATAMAX", datamax, typ_fdouble, fdouble);

 FITS_TRY_CARD (hdr, hdulist, "BZERO", bzero, typ_fdouble, fdouble);
 FITS_TRY_CARD (hdr, hdulist, "BSCALE", bscale, typ_fdouble, fdouble);

 /* Evaluate number of interpretable images for this HDU */
 hdulist->numpic = 0;

 /* We must support this format */
 bitpix_supported =    (hdulist->bitpix > 0)
                    || (   (hdulist->bitpix == -64)
                        && (fits_ieee64_intel || fits_ieee64_motorola))
                    || (   (hdulist->bitpix == -32)
                        && (   fits_ieee32_intel || fits_ieee32_motorola
                            || fits_ieee64_intel || fits_ieee64_motorola));

 if (bitpix_supported)
 {
   if (hdulist->used.simple)
   {
     if (hdulist->naxis > 0)
     {
       hdulist->numpic = 1;
       for (k = 3; k <= hdulist->naxis; k++)
         hdulist->numpic *= hdulist->naxisn[k-1];
     }
   }
   else if (   hdulist->used.xtension
            && (strncmp (hdulist->xtension, "IMAGE", 5) == 0))
   {
     if (hdulist->naxis > 0)
     {
       hdulist->numpic = 1;
       for (k = 3; k <= hdulist->naxis; k++)
         hdulist->numpic *= hdulist->naxisn[k-1];
     }
   }
 }
 else
 {char msg[160];
   sprintf (msg, "fits_decode_header: IEEE floating point format required for\
 BITPIX=%d\nis not supported on this machine", hdulist->bitpix);
   fits_set_error (msg);
 }

 hdulist->header_record_list = hdr;  /* Add header records to the list */
 return (hdulist);

err_missing:
 sprintf (errmsg, "fits_decode_header: missing/invalid %s card", key);

err_return:
 fits_delete_hdulist (hdulist);
 fits_set_error (errmsg);
 return (NULL);

#undef FITS_DECODE_CARD
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_eval_pixrange - (local) evaluate range of pixel data     */
/*                                                                           */
/* Parameters:                                                               */
/* FILE *fp               [I] : file pointer                                 */
/* FITS_HDU_LIST *hdu     [I] : pointer to header                            */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The Function sets the values hdu->pixmin and hdu->pixmax. On success 0    */
/* is returned. On failure, -1 is returned.                                  */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

static int fits_eval_pixrange (FILE *fp, FITS_HDU_LIST *hdu)

{register int maxelem;
#define FITSNPIX 4096
 unsigned char pixdat[FITSNPIX];
 int nelem, bpp;
 int blank_found = 0, nan_found = 0;

 if (fseek (fp, hdu->data_offset, SEEK_SET) < 0)
   FITS_RETURN ("fits_eval_pixrange: cant position file", -1);

 bpp = hdu->bpp;                  /* Number of bytes per pixel */
 nelem = hdu->udata_size / bpp;   /* Number of data elements */

 switch (hdu->bitpix)
 {
   case 8: {
     register FITS_BITPIX8 pixval;
     register unsigned char *ptr;
     FITS_BITPIX8 minval = 255, maxval = 0;
     FITS_BITPIX8 blankval;

     while (nelem > 0)
     {
       maxelem = sizeof (pixdat)/bpp;
       if (nelem < maxelem) maxelem = nelem;
       nelem -= maxelem;
       if (fread ((char *)pixdat, bpp, maxelem, fp) != maxelem)
         FITS_RETURN ("fits_eval_pixrange: error on read bitpix 8 data", -1);

       ptr = pixdat;
       if (hdu->used.blank)
       {
         blankval = (FITS_BITPIX8)hdu->blank;
         while (maxelem-- > 0)
         {
           pixval = (FITS_BITPIX8)*(ptr++);
           if (pixval != blankval)
           {
             if (pixval < minval) minval = pixval;
             else if (pixval > maxval) maxval = pixval;
           }
           else blank_found = 1;
         }
       }
       else
       {
         while (maxelem-- > 0)
         {
           pixval = (FITS_BITPIX8)*(ptr++);
           if (pixval < minval) minval = pixval;
           else if (pixval > maxval) maxval = pixval;
         }
       }
     }
     hdu->pixmin = minval;
     hdu->pixmax = maxval;
     break; }

   case 16: {
     register FITS_BITPIX16 pixval;
     register unsigned char *ptr;
     FITS_BITPIX16 minval = 0x7fff, maxval = ~0x7fff;

     while (nelem > 0)
     {
       maxelem = sizeof (pixdat)/bpp;
       if (nelem < maxelem) maxelem = nelem;
       nelem -= maxelem;
       if (fread ((char *)pixdat, bpp, maxelem, fp) != maxelem)
         FITS_RETURN ("fits_eval_pixrange: error on read bitpix 16 data", -1);

       ptr = pixdat;
       if (hdu->used.blank)
       {FITS_BITPIX16 blankval = (FITS_BITPIX16)hdu->blank;

         while (maxelem-- > 0)
         {
           FITS_GETBITPIX16 (ptr, pixval);
           ptr += 2;
           if (pixval != blankval)
           {
             if (pixval < minval) minval = pixval;
             else if (pixval > maxval) maxval = pixval;
           }
           else blank_found = 1;
         }
       }
       else
       {
         while (maxelem-- > 0)
         {
           FITS_GETBITPIX16 (ptr, pixval);
           ptr += 2;
           if (pixval < minval) minval = pixval;
           else if (pixval > maxval) maxval = pixval;
         }
       }
     }
     hdu->pixmin = minval;
     hdu->pixmax = maxval;
     break; }


   case 32: {
     register FITS_BITPIX32 pixval;
     register unsigned char *ptr;
     FITS_BITPIX32 minval = 0x7fffffff, maxval = ~0x7fffffff;

     while (nelem > 0)
     {
       maxelem = sizeof (pixdat)/bpp;
       if (nelem < maxelem) maxelem = nelem;
       nelem -= maxelem;
       if (fread ((char *)pixdat, bpp, maxelem, fp) != maxelem)
         FITS_RETURN ("fits_eval_pixrange: error on read bitpix 32 data", -1);

       ptr = pixdat;
       if (hdu->used.blank)
       {FITS_BITPIX32 blankval = (FITS_BITPIX32)hdu->blank;

         while (maxelem-- > 0)
         {
           FITS_GETBITPIX32 (ptr, pixval);
           ptr += 4;
           if (pixval != blankval)
           {
             if (pixval < minval) minval = pixval;
             else if (pixval > maxval) maxval = pixval;
           }
           else blank_found = 1;
         }
       }
       else
       {
         while (maxelem-- > 0)
         {
           FITS_GETBITPIX32 (ptr, pixval);
           ptr += 4;
           if (pixval < minval) minval = pixval;
           else if (pixval > maxval) maxval = pixval;
         }
       }
     }
     hdu->pixmin = minval;
     hdu->pixmax = maxval;
     break; }

   case -32: {
     register FITS_BITPIXM32 pixval;
     register unsigned char *ptr;
     FITS_BITPIXM32 minval, maxval;
     int first = 1;

     /* initialize */

     pixval = 0;
     minval = 0;
     maxval = 0;

     while (nelem > 0)
     {
       maxelem = sizeof (pixdat)/bpp;
       if (nelem < maxelem) maxelem = nelem;
       nelem -= maxelem;
       if (fread ((char *)pixdat, bpp, maxelem, fp) != maxelem)
         FITS_RETURN ("fits_eval_pixrange: error on read bitpix -32 data", -1);

       ptr = pixdat;
       while (maxelem-- > 0)
       {
         if (!fits_nan_32 (ptr))
         {
           FITS_GETBITPIXM32 (ptr, pixval);
           ptr += 4;
           if (first)
           {
             first = 0;
             minval = maxval = pixval;
           }
           else if (pixval < minval) { minval = pixval; }
           else if (pixval > maxval) { maxval = pixval; }
         }
         else nan_found = 1;
       }
     }
     hdu->pixmin = minval;
     hdu->pixmax = maxval;
     break; }

   case -64: {
     register FITS_BITPIXM64 pixval;
     register unsigned char *ptr;
     FITS_BITPIXM64 minval, maxval;
     int first = 1;

     /* initialize */

     minval = 0;
     maxval = 0;

     while (nelem > 0)
     {
       maxelem = sizeof (pixdat)/bpp;
       if (nelem < maxelem) maxelem = nelem;
       nelem -= maxelem;
       if (fread ((char *)pixdat, bpp, maxelem, fp) != maxelem)
         FITS_RETURN ("fits_eval_pixrange: error on read bitpix -64 data", -1);

       ptr = pixdat;
       while (maxelem-- > 0)
       {
         if (!fits_nan_64 (ptr))
         {
           FITS_GETBITPIXM64 (ptr, pixval);
           ptr += 8;
           if (first)
           {
             first = 0;
             minval = maxval = pixval;
           }
           else if (pixval < minval) { minval = pixval; }
           else if (pixval > maxval) { maxval = pixval; }
         }
         else nan_found = 1;
       }
     }
     hdu->pixmin = minval;
     hdu->pixmax = maxval;
     break; }
 }
 if (nan_found) hdu->used.nan_value = 1;
 if (blank_found) hdu->used.blank_value = 1;

 return (0);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_decode_card - decode a card                              */
/*                                                                           */
/* Parameters:                                                               */
/* const char *card   [I] : pointer to card image                            */
/* FITS_DATA_TYPES data_type [I] : datatype to decode                        */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* Decodes a card and returns a pointer to the union, keeping the data.      */
/* If card is NULL or on failure, a NULL-pointer is returned.                */
/* If the card does not have the value indicator, an error is generated,     */
/* but its tried to decode the card. The data is only valid up to the next   */
/* call of the function.                                                     */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

FITS_DATA *fits_decode_card (const char *card, FITS_DATA_TYPES data_type)

{static FITS_DATA data;
 long l_long;
 double l_double;
 char l_card[FITS_CARD_SIZE+1], msg[256];
 char *cp, *dst, *end;
 int ErrCount = 0;

 if (card == NULL) return (NULL);

 memcpy (l_card, card, FITS_CARD_SIZE);
 l_card[FITS_CARD_SIZE] = '\0';

 if (strncmp (card+8, "= ", 2) != 0)
 {
   sprintf (msg, "fits_decode_card (warning): Missing value indicator\
 '= ' for %8.8s", l_card);
   fits_set_error (msg);
   ErrCount++;
 }

 switch (data_type)
 {
   case typ_bitpix8:
     data.bitpix8 = (FITS_BITPIX8)(l_card[10]);
     break;

   case typ_bitpix16:
     if (sscanf (l_card+10, "%ld", &l_long) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_bitpix16");
       ErrCount++;
       break;
     }
     data.bitpix16 = (FITS_BITPIX16)l_long;
     break;

   case typ_bitpix32:
     if (sscanf (l_card+10, "%ld", &l_long) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_bitpix32");
       ErrCount++;
       break;
     }
     data.bitpix32 = (FITS_BITPIX32)l_long;
     break;

   case typ_bitpixm32:
     if (fits_scanfdouble (l_card+10, &l_double) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_bitpixm32");
       ErrCount++;
       break;
     }
     data.bitpixm32 = (FITS_BITPIXM32)l_double;
     break;

   case typ_bitpixm64:
     if (fits_scanfdouble (l_card+10, &l_double) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_bitpixm64");
       ErrCount++;
       break;
     }
     data.bitpixm64 = (FITS_BITPIXM64)l_double;
     break;

   case typ_fbool:
     cp = l_card+10;
     while (*cp == ' ') cp++;
     if (*cp == 'T')
     {
       data.fbool = 1;
     }
     else if (*cp == 'F')
     {
       data.fbool = 0;
     }
     else
     {
       fits_set_error ("fits_decode_card: error decoding typ_fbool");
       ErrCount++;
       break;
     }
     break;

   case typ_flong:
     if (sscanf (l_card+10, "%ld", &l_long) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_flong");
       ErrCount++;
       break;
     }
     data.flong = (FITS_BITPIX32)l_long;
     break;

   case typ_fdouble:
     if (fits_scanfdouble (l_card+10, &l_double) != 1)
     {
       fits_set_error ("fits_decode_card: error decoding typ_fdouble");
       ErrCount++;
       break;
     }
     data.fdouble = (FITS_BITPIXM32)l_double;
     break;

   case typ_fstring:
     cp = l_card+10;
     if (*cp != '\'')
     {
       fits_set_error ("fits_decode_card: missing \' decoding typ_fstring");
       ErrCount++;
       break;
     }

     dst = data.fstring;
     cp++;
     end = l_card+FITS_CARD_SIZE-1;
     for (;;)   /* Search for trailing quote */
     {
       if (*cp != '\'')    /* All characters but quote are used. */
       {
         *(dst++) = *cp;
       }
       else                /* Maybe there is a quote in the string */
       {
         if (cp >= end) break;  /* End of card ? finished */
         if (*(cp+1) != '\'') break;
         *(dst++) = *(cp++);
       }
       if (cp >= end) break;
       cp++;
     }
     *dst = '\0';
     break;
 }

 return ((ErrCount == 0) ? &data : NULL);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_search_card - search a card in the record list           */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_RECORD_LIST *rl  [I] : record list to search                         */
/* char *keyword         [I] : keyword identifying the card                  */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* A card is searched in the reord list. Only the first eight characters of  */
/* keyword are significant. If keyword is less than 8 characters, its filled */
/* with blanks.                                                              */
/* If the card is found, a pointer to the card is returned.                  */
/* The pointer does not point to a null-terminated string. Only the next     */
/* 80 bytes are allowed to be read.                                          */
/* On failure a NULL-pointer is returned.                                    */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

char *fits_search_card (FITS_RECORD_LIST *rl, char *keyword)

{int key_len, k;
 char *card;
 char key[9];

 key_len = strlen (keyword);
 if (key_len > 8) key_len = 8;
 if (key_len == 0)
   FITS_RETURN ("fits_search_card: Invalid parameter", NULL);

 strcpy (key, "        ");
 memcpy (key, keyword, key_len);

 while (rl != NULL)
 {
   card = (char *)rl->data;
   for (k = 0; k < FITS_RECORD_SIZE / FITS_CARD_SIZE; k++)
   {
     if (strncmp (card, key, 8) == 0) return (card);
     card += FITS_CARD_SIZE;
   }
   rl = rl->next_record;
 }
 return (NULL);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_image_info - get information about an image              */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff         [I] : FITS file structure                           */
/* int picind            [I] : Index of picture in file (1,2,...)            */
/* int *hdupicind        [O] : Index of picture in HDU (1,2,...)             */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function returns on success a pointer to a FITS_HDU_LIST. hdupicind   */
/* then gives the index of the image within the HDU.                         */
/* On failure, NULL is returned.                                             */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

FITS_HDU_LIST *fits_image_info (FITS_FILE *ff, int picind, int *hdupicind)

{FITS_HDU_LIST *hdulist;
 int firstpic, lastpic;

 if (ff == NULL)
   FITS_RETURN ("fits_image_info: ff is NULL", NULL);

 if (ff->openmode != 'r')
   FITS_RETURN ("fits_image_info: file not open for reading", NULL);

 if ((picind < 1) || (picind > ff->n_pic))
   FITS_RETURN ("fits_image_info: picind out of range", NULL);

 firstpic = 1;
 for (hdulist = ff->hdu_list; hdulist != NULL; hdulist = hdulist->next_hdu)
 {
   if (hdulist->numpic <= 0) continue;
   lastpic = firstpic+hdulist->numpic-1;
   if (picind <= lastpic)  /* Found image in current HDU ? */
     break;

   firstpic = lastpic+1;
 }
 *hdupicind = picind - firstpic + 1;
 return (hdulist);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_seek_image - position to a specific image                */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff         [I] : FITS file structure                           */
/* int picind            [I] : Index of picture to seek (1,2,...)            */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function positions the file pointer to a specified image.             */
/* The function returns on success a pointer to a FITS_HDU_LIST. This pointer*/
/* must also be used when reading data from the image.                       */
/* On failure, NULL is returned.                                             */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

FITS_HDU_LIST *fits_seek_image (FITS_FILE *ff, int picind)

{FITS_HDU_LIST *hdulist;
 int hdupicind;
 long offset, pic_size;

 hdulist = fits_image_info (ff, picind, &hdupicind);
 if (hdulist == NULL) return (NULL);

 pic_size = hdulist->bpp * hdulist->naxisn[0] * hdulist->naxisn[1];
 offset = hdulist->data_offset + (hdupicind-1)*pic_size;
 if (fseek (ff->fp, offset, SEEK_SET) < 0)
   FITS_RETURN ("fits_seek_image: Unable to position to image", NULL);

 return (hdulist);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_read_pixel - read pixel values from a file               */
/*                                                                           */
/* Parameters:                                                               */
/* FITS_FILE *ff           [I] : FITS file structure                         */
/* FITS_HDU_LIST *hdulist  [I] : pointer to hdulist that describes image     */
/* int npix                [I] : number of pixel values to read              */
/* FITS_PIX_TRANSFORM *trans [I]: pixel transformation                       */
/* void *buf               [O] : buffer where to place transformed pixels    */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function reads npix pixel values from the file, transforms them       */
/* checking for blank/NaN pixels and stores the transformed values in buf.   */
/* The number of transformed pixels is returned. If the returned value is    */
/* less than npix (or even -1), an error has occurred.                       */
/* hdulist must be a pointer returned by fits_seek_image(). Before starting  */
/* to read an image, fits_seek_image() must be called. Even for successive   */
/* images.                                                                   */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

int fits_read_pixel (FITS_FILE *ff, FITS_HDU_LIST *hdulist, int npix,
                     FITS_PIX_TRANSFORM *trans, void *buf)

{double offs, scale;
 double datadiff, pixdiff;
 unsigned char pixbuffer[4096], *pix, *cdata;
 unsigned char creplace;
 int transcount = 0;
 long tdata, tmin, tmax;
 int maxelem;
 FITS_BITPIX8 bp8, bp8blank;
 FITS_BITPIX16 bp16, bp16blank;
 FITS_BITPIX32 bp32, bp32blank;
 FITS_BITPIXM32 bpm32;
 FITS_BITPIXM64 bpm64;

 /* initialize */

 bpm32 = 0;

 if (ff->openmode != 'r') return (-1);   /* Not open for reading */
 if (trans->dsttyp != 'c') return (-1);  /* Currently we only return chars */
 if (npix <= 0) return (npix);

 datadiff = trans->datamax - trans->datamin;
 pixdiff = trans->pixmax - trans->pixmin;

 offs = trans->datamin - trans->pixmin*datadiff/pixdiff;
 scale = datadiff / pixdiff;

 tmin = (long)trans->datamin;
 tmax = (long)trans->datamax;
 if (tmin < 0) tmin = 0; else if (tmin > 255) tmin = 255;
 if (tmax < 0) tmax = 0; else if (tmax > 255) tmax = 255;

 cdata = (unsigned char *)buf;
 creplace = (unsigned char)trans->replacement;

 switch (hdulist->bitpix)
 {
   case 8:
     while (npix > 0)  /* For all pixels to read */
     {
       maxelem = sizeof (pixbuffer) / hdulist->bpp;
       if (maxelem > npix) maxelem = npix;
       if (fread ((char *)pixbuffer, hdulist->bpp, maxelem, ff->fp) != maxelem)
         return (-1);
       npix -= maxelem;

       pix = pixbuffer;
       if (hdulist->used.blank)
       {
         bp8blank = (FITS_BITPIX8)hdulist->blank;
         while (maxelem--)
         {
           bp8 = (FITS_BITPIX8)*(pix++);
           if (bp8 == bp8blank)      /* Is it a blank pixel ? */
             *(cdata++) = creplace;
           else                      /* Do transform */
           {
             tdata = (long)(bp8 * scale + offs);
             if (tdata < tmin) tdata = tmin;
             else if (tdata > tmax) tdata = tmax;
             *(cdata++) = (unsigned char)tdata;
           }
           transcount++;
         }
       }
       else
       {
         while (maxelem--)
         {
           bp8 = (FITS_BITPIX8)*(pix++);
           tdata = (long)(bp8 * scale + offs);
           if (tdata < tmin) tdata = tmin;
           else if (tdata > tmax) tdata = tmax;
           *(cdata++) = (unsigned char)tdata;
           transcount++;
         }
       }
     }
     break;

   case 16:
     while (npix > 0)  /* For all pixels to read */
     {
       maxelem = sizeof (pixbuffer) / hdulist->bpp;
       if (maxelem > npix) maxelem = npix;
       if (fread ((char *)pixbuffer, hdulist->bpp, maxelem, ff->fp) != maxelem)
         return (-1);
       npix -= maxelem;

       pix = pixbuffer;
       if (hdulist->used.blank)
       {
         bp16blank = (FITS_BITPIX16)hdulist->blank;
         while (maxelem--)
         {
           FITS_GETBITPIX16 (pix, bp16);
           if (bp16 == bp16blank)
             *(cdata++) = creplace;
           else
           {
             tdata = (long)(bp16 * scale + offs);
             if (tdata < tmin) tdata = tmin;
             else if (tdata > tmax) tdata = tmax;
             *(cdata++) = (unsigned char)tdata;
           }
           transcount++;
           pix += 2;
         }
       }
       else
       {
         while (maxelem--)
         {
           FITS_GETBITPIX16 (pix, bp16);
           tdata = (long)(bp16 * scale + offs);
           if (tdata < tmin) tdata = tmin;
           else if (tdata > tmax) tdata = tmax;
           *(cdata++) = (unsigned char)tdata;
           transcount++;
           pix += 2;
         }
       }
     }
     break;

   case 32:
     while (npix > 0)  /* For all pixels to read */
     {
       maxelem = sizeof (pixbuffer) / hdulist->bpp;
       if (maxelem > npix) maxelem = npix;
       if (fread ((char *)pixbuffer, hdulist->bpp, maxelem, ff->fp) != maxelem)
         return (-1);
       npix -= maxelem;

       pix = pixbuffer;
       if (hdulist->used.blank)
       {
         bp32blank = (FITS_BITPIX32)hdulist->blank;
         while (maxelem--)
         {
           FITS_GETBITPIX32 (pix, bp32);
           if (bp32 == bp32blank)
             *(cdata++) = creplace;
           else
           {
             tdata = (long)(bp32 * scale + offs);
             if (tdata < tmin) tdata = tmin;
             else if (tdata > tmax) tdata = tmax;
             *(cdata++) = (unsigned char)tdata;
           }
           transcount++;
           pix += 4;
         }
       }
       else
       {
         while (maxelem--)
         {
           FITS_GETBITPIX32 (pix, bp32);
           tdata = (long)(bp32 * scale + offs);
           if (tdata < tmin) tdata = tmin;
           else if (tdata > tmax) tdata = tmax;
           *(cdata++) = (unsigned char)tdata;
           transcount++;
           pix += 4;
         }
       }
     }
     break;

   case -32:
     while (npix > 0)  /* For all pixels to read */
     {
       maxelem = sizeof (pixbuffer) / hdulist->bpp;
       if (maxelem > npix) maxelem = npix;
       if (fread ((char *)pixbuffer, hdulist->bpp, maxelem, ff->fp) != maxelem)
         return (-1);
       npix -= maxelem;

       pix = pixbuffer;
       while (maxelem--)
       {
         if (fits_nan_32 (pix))    /* An IEEE special value ? */
           *(cdata++) = creplace;
         else                      /* Do transform */
         {
           FITS_GETBITPIXM32 (pix, bpm32);
           tdata = (long)(bpm32 * scale + offs);
           if (tdata < tmin) tdata = tmin;
           else if (tdata > tmax) tdata = tmax;
           *(cdata++) = (unsigned char)tdata;
         }
         transcount++;
         pix += 4;
       }
     }
     break;

   case -64:
     while (npix > 0)  /* For all pixels to read */
     {
       maxelem = sizeof (pixbuffer) / hdulist->bpp;
       if (maxelem > npix) maxelem = npix;
       if (fread ((char *)pixbuffer, hdulist->bpp, maxelem, ff->fp) != maxelem)
         return (-1);
       npix -= maxelem;

       pix = pixbuffer;
       while (maxelem--)
       {
         if (fits_nan_64 (pix))
           *(cdata++) = creplace;
         else
         {
           FITS_GETBITPIXM64 (pix, bpm64);
           tdata = (long)(bpm64 * scale + offs);
           if (tdata < tmin) tdata = tmin;
           else if (tdata > tmax) tdata = tmax;
           *(cdata++) = (unsigned char)tdata;
         }
         transcount++;
         pix += 8;
       }
     }
     break;
 }
 return (transcount);
}


#ifndef FITS_NO_DEMO
/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : fits_to_pgmraw - convert FITS-file to raw PGM-file            */
/*                                                                           */
/* Parameters:                                                               */
/* char *fitsfile          [I] : name of fitsfile                            */
/* char *pgmfile           [I] : name of pgmfile                             */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function converts a FITS-file to a raw PGM-file. The PGM-file will    */
/* be upside down, because the orientation for storing the image is          */
/* different. On success, 0 is returned. On failure, -1 is returned.         */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

int fits_to_pgmraw (char *fitsfile, char *pgmfile)

{FITS_FILE *fitsin = NULL;
 FILE *pgmout = NULL;
 FITS_HDU_LIST *hdu;
 FITS_PIX_TRANSFORM trans;
 int retval = -1, nbytes, maxbytes;
 char buffer[1024];

 fitsin = fits_open (fitsfile, "r");  /* Open FITS-file for reading */
 if (fitsin == NULL) goto err_return;

 if (fitsin->n_pic < 1) goto err_return;  /* Any picture in it ? */

 hdu = fits_seek_image (fitsin, 1);       /* Position to the first image */
 if (hdu == NULL) goto err_return;
 if (hdu->naxis < 2) goto err_return;     /* Enough dimensions ? */

 pgmout = g_fopen (pgmfile, "wb");
 if (pgmout == NULL) goto err_return;

                                   /* Write PGM header with width/height */
 fprintf (pgmout, "P5\n%d %d\n255\n", hdu->naxisn[0], hdu->naxisn[1]);

 /* Set up transformation for FITS pixel values to 0...255 */
 /* It maps trans.pixmin to trans.datamin and trans.pixmax to trans.datamax. */
 /* Values out of range [datamin, datamax] are clamped */
 trans.pixmin = hdu->pixmin;
 trans.pixmax = hdu->pixmax;
 trans.datamin = 0.0;
 trans.datamax = 255.0;
 trans.replacement = 0.0;  /* Blank/NaN replacement value */
 trans.dsttyp = 'c';       /* Output type is character */

 nbytes = hdu->naxisn[0]*hdu->naxisn[1];
 while (nbytes > 0)
 {
   maxbytes = sizeof (buffer);
   if (maxbytes > nbytes) maxbytes = nbytes;

   /* Read pixels and transform them */
   if (fits_read_pixel (fitsin, hdu, maxbytes, &trans, buffer) != maxbytes)
     goto err_return;

   if (fwrite (buffer, 1, maxbytes, pgmout) != maxbytes)
     goto err_return;

   nbytes -= maxbytes;
 }
 retval = 0;

err_return:

 if (fitsin) fits_close (fitsin);
 if (pgmout) fclose (pgmout);

 return (retval);
}


/*****************************************************************************/
/* #BEG-PAR                                                                  */
/*                                                                           */
/* Function  : pgmraw to fits - convert raw PGM-file to FITS-file            */
/*                                                                           */
/* Parameters:                                                               */
/* char *pgmfile           [I] : name of pgmfile                             */
/* char *fitsfile          [I] : name of fitsfile                            */
/*                  ( mode : I=input, O=output, I/O=input/output )           */
/*                                                                           */
/* The function converts a raw PGM-file to a FITS-file. The FITS-file will   */
/* be upside down, because the orientation for storing the image is          */
/* different. On success, 0 is returned. On failure, -1 is returned.         */
/*                                                                           */
/* #END-PAR                                                                  */
/*****************************************************************************/

int pgmraw_to_fits (char *pgmfile, char *fitsfile)

{FITS_FILE *fitsout = NULL;
 FILE *pgmin = NULL;
 FITS_HDU_LIST *hdu;
 char buffer[1024];
 int width, height, numbytes, maxbytes;
 int retval = -1;

 fitsout = fits_open (fitsfile, "w");
 if (fitsout == NULL) goto err_return;

 pgmin = g_fopen (pgmfile, "r");
 if (pgmin == NULL) goto err_return;

 /* Read signature of PGM file */
 if (fgets (buffer, sizeof (buffer), pgmin) == NULL) goto err_return;
 if ((buffer[0] != 'P') || (buffer[1] != '5')) goto err_return;

 /* Skip comments upto width/height */
 do
 {
   if (fgets (buffer, sizeof (buffer), pgmin) == NULL) goto err_return;
 } while (buffer[0] == '#');

 if (sscanf (buffer, "%d%d", &width, &height) != 2) goto err_return;
 if ((width < 1) || (height < 1)) goto err_return;

 /* Skip comments upto maxval */
 do
 {
   if (fgets (buffer, sizeof (buffer), pgmin) == NULL) goto err_return;
 } while (buffer[0] == '#');
 /* Ignore maxval */

 hdu = fits_add_hdu (fitsout);     /* Create a HDU for the FITS file */
 if (hdu == NULL) goto err_return;

 hdu->used.simple = 1;         /* Set proper values */
 hdu->bitpix = 8;
 hdu->naxis = 2;
 hdu->naxisn[0] = width;
 hdu->naxisn[1] = height;
 hdu->used.datamin = 1;
 hdu->datamin = 0.0;
 hdu->used.datamax = 1;
 hdu->datamax = 255.0;
 hdu->used.bzero = 1;
 hdu->bzero = 0.0;
 hdu->used.bscale = 1;
 hdu->bscale = 1.0;

 fits_add_card (hdu, "");
 fits_add_card (hdu, "HISTORY THIS FITS FILE WAS GENERATED BY FITSRW\
 USING PGMRAW_TO_FITS");

 /* Write the header. Blocking is done automatically */
 if (fits_write_header (fitsout, hdu) < 0) goto err_return;

 /* The primary array plus blocking must be written manually */
 numbytes = width * height;

 while (numbytes > 0)
 {
   maxbytes = sizeof (buffer);
   if (maxbytes > numbytes) maxbytes = numbytes;

   if (fread (buffer, 1, maxbytes, pgmin) != maxbytes) goto err_return;
   if (fwrite (buffer, 1, maxbytes, fitsout->fp) != maxbytes) goto err_return;

   numbytes -= maxbytes;
 }

 /* Do blocking */
 numbytes = (width * height) % FITS_RECORD_SIZE;
 if (numbytes)
 {
   while (numbytes++ < FITS_RECORD_SIZE)
     if (putc (0, fitsout->fp) == EOF) goto err_return;
 }
 retval = 0;

err_return:

 if (fitsout) fits_close (fitsout);
 if (pgmin) fclose (pgmin);

 return (retval);
}

#endif
