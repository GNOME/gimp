/******************************************************************************/
/*                      Peter Kirchgessner                                    */
/*                      e-mail: pkirchg@aol.com                               */
/*                      WWW   : http://members.aol.com/pkirchg                */
/******************************************************************************/
/*  #BEG-HDR                                                                  */
/*                                                                            */
/*  Package       : FITS reading/writing library                              */
/*  Modul-Name    : fitsrw.h                                                  */
/*  Description   : Include file for FITS-r/w-library                         */
/*  Function(s)   :                                                           */
/*  Author        : P. Kirchgessner                                           */
/*  Date of Gen.  : 12-Apr-97                                                 */
/*  Last modified : 17-May-97                                                 */
/*  Version       : 0.10                                                      */
/*  Compiler Opt. :                                                           */
/*  Changes       :                                                           */
/*                                                                            */
/*  #END-HDR                                                                  */
/******************************************************************************/

#ifndef __FITS_IO_H__
#define __FITS_IO_H__

#define FITS_CARD_SIZE      80
#define FITS_RECORD_SIZE  2880
#define FITS_MAX_AXIS      999

#define FITS_NADD_CARDS    128

/* Data representations */
typedef unsigned char FITS_BITPIX8;
typedef short         FITS_BITPIX16;
typedef long          FITS_BITPIX32;
typedef float         FITS_BITPIXM32;
typedef double        FITS_BITPIXM64;

typedef int           FITS_BOOL;
typedef long          FITS_LONG;
typedef double        FITS_DOUBLE;
typedef char          FITS_STRING[FITS_CARD_SIZE];

typedef enum
{
  typ_bitpix8,
  typ_bitpix16,
  typ_bitpix32,
  typ_bitpixm32,
  typ_bitpixm64,
  typ_fbool,
  typ_flong,
  typ_fdouble,
  typ_fstring
} FITS_DATA_TYPES;

/* How to transform FITS pixel values */
typedef struct
{
  gdouble pixmin, pixmax;    /* Pixel values [pixmin,pixmax] should be mapped */
  gdouble datamin, datamax;  /* to [datamin,datamax] */
  gdouble replacement;       /* datavalue to use for blank or NaN pixels */
  gchar   dsttyp;            /* Destination typ ('c' = char) */
} FITS_PIX_TRANSFORM;

typedef union
{
  FITS_BITPIX8   bitpix8;
  FITS_BITPIX16  bitpix16;
  FITS_BITPIX32  bitpix32;
  FITS_BITPIXM32 bitpixm32;
  FITS_BITPIXM64 bitpixm64;

  FITS_BOOL   fbool;
  FITS_LONG   flong;
  FITS_DOUBLE fdouble;
  FITS_STRING fstring;
} FITS_DATA;

typedef struct fits_record_list        /* Record list */
{
  gchar                    data[FITS_RECORD_SIZE];
  struct fits_record_list *next_record;
} FITS_RECORD_LIST;


typedef struct fits_hdu_list    /* Header and Data Unit List */
{
  glong header_offset;             /* Offset of header in the file */
  glong data_offset;               /* Offset of data in the file */
  glong data_size;                 /* Size of data in the HDU (including pad)*/
  glong udata_size;                /* Size of used data in the HDU (excl. pad) */
  gint  bpp;                       /* Bytes per pixel */
  gint  numpic;                    /* Number of interpretable images in HDU */
  gint  naddcards;                 /* Number of additional cards */
  gchar addcards[FITS_NADD_CARDS][FITS_CARD_SIZE];
  struct
  {
    gboolean nan_value;            /* NaN's found in data ? */
    gboolean blank_value;          /* Blanks found in data ? */
                                   /* Flags specifying if some cards are used */
    gchar blank;                   /* The corresponding data below is only */
    gchar datamin;                 /* valid, if the flag is set. */
    gchar datamax;
    gchar simple;                  /* This indicates a simple HDU */
    gchar xtension;                /* This indicates an extension */
    gchar gcount;
    gchar pcount;
    gchar bzero;
    gchar bscale;
    gchar groups;
    gchar extend;
  } used;
  gdouble pixmin, pixmax;          /* Minimum/Maximum pixel values */
                                   /* Some decoded data of the HDU: */
  gint naxis;                      /* Number of axes */
  gint naxisn[FITS_MAX_AXIS];      /* Sizes of axes (NAXIS1 --> naxisn[0]) */
  gint bitpix;                     /* Data representation (8,16,32,-16,-32) */
                                   /* When using the following data, */
                                   /* the used-flags must be checked before. */
  glong   blank;                   /* Blank value */
  gdouble datamin, datamax;        /* Minimum/Maximum physical data values */
  gchar   xtension[FITS_CARD_SIZE];/* Type of extension */
  glong   gcount, pcount;          /* Used by XTENSION */
  gdouble bzero, bscale;           /* Transformation values */
  gint    groups;                  /* Random groups indicator */
  gint    extend;                  /* Extend flag */

  FITS_RECORD_LIST *header_record_list; /* Header records read in */

  struct fits_hdu_list *next_hdu;
} FITS_HDU_LIST;


typedef struct
{
  FILE     *fp;               /* File pointer to fits file */
  gchar     openmode;         /* Mode the file was opened (0, 'r', 'w') */

  gint      n_hdu;            /* Number of HDUs in file */
  gint      n_pic;            /* Total number of interpretable pictures */
  gboolean  nan_used;         /* NaN's used in the file ? */
  gboolean  blank_used;       /* Blank's used in the file ? */

  FITS_HDU_LIST *hdu_list;    /* Header and Data Unit List */
} FITS_FILE;


/* User callable functions of the FITS-library */

FITS_FILE     * fits_open         (const gchar        *filename,
                                   const gchar        *openmode);
void            fits_close        (FITS_FILE          *ff);
FITS_HDU_LIST * fits_add_hdu      (FITS_FILE          *ff);
gint            fits_add_card     (FITS_HDU_LIST      *hdulist,
                                   const gchar        *card);
gint            fits_write_header (FITS_FILE          *ff,
                                   FITS_HDU_LIST      *hdulist);
FITS_HDU_LIST * fits_image_info   (FITS_FILE          *ff,
                                   gint                picind,
                                   gint               *hdupicind);
FITS_HDU_LIST * fits_seek_image   (FITS_FILE          *ff,
                                   gint                picind);
void            fits_print_header (FITS_HDU_LIST      *hdr);
FITS_DATA     * fits_decode_card  (const gchar        *card,
                                   FITS_DATA_TYPES     data_type);
gchar         * fits_search_card  (FITS_RECORD_LIST   *rl,
                                   gchar              *keyword);
gint            fits_read_pixel   (FITS_FILE          *ff,
                                   FITS_HDU_LIST      *hdulist,
                                   gint                npix,
                                   FITS_PIX_TRANSFORM *trans,
                                   void               *buf);

gchar         * fits_get_error    (void);

/* Demo functions */
#define FITS_NO_DEMO
gint            fits_to_pgmraw    (gchar              *fitsfile,
                                   gchar              *pgmfile);
gint            pgmraw_to_fits    (gchar              *pgmfile,
                                   gchar              *fitsfile);

#endif /* __FITS_IO_H__ */
