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

typedef guchar FitsBitpix8;
typedef gint16 FitsBitpix16;
typedef gint32 FitsBitpix32;
typedef float  FitsBitpixM32;
typedef double FitsBitpixM64;
typedef gint32 FitsBool;
typedef gint32 FitsLong;
typedef double FitsDouble;
typedef char   FitsString[FITS_CARD_SIZE];

typedef enum
{
  FITS_DATA_TYPE_BITPIX_8,
  FITS_DATA_TYPE_BITPIX_16,
  FITS_DATA_TYPE_BITPIX_32,
  FITS_DATA_TYPE_BITPIX_M32,
  FITS_DATA_TYPE_BITPIX_M64,
  FITS_DATA_TYPE_FBOOL,
  FITS_DATA_TYPE_FLONG,
  FITS_DATA_TYPE_FDOUBLE,
  FITS_DATA_TYPE_FSTRING
} FitsDataType;

typedef union
{
  FitsBitpix8   bitpix8;
  FitsBitpix16  bitpix16;
  FitsBitpix32  bitpix32;
  FitsBitpixM32 bitpixm32;
  FitsBitpixM64 bitpixm64;
  FitsBool      fbool;
  FitsLong      flong;
  FitsDouble    fdouble;
  FitsString    fstring;
} FitsData;


/* How to transform FITS pixel values */

typedef struct _FitsPixTransform FitsPixTransform;

struct _FitsPixTransform
{
  gdouble pixmin, pixmax;    /* Pixel values [pixmin,pixmax] should be mapped */
  gdouble datamin, datamax;  /* to [datamin,datamax] */
  gdouble replacement;       /* datavalue to use for blank or NaN pixels */
  gchar   dsttyp;            /* Destination typ ('c' = char) */
};


/* Record list */

typedef struct _FitsRecordList FitsRecordList;

struct _FitsRecordList
{
  gchar           data[FITS_RECORD_SIZE];
  FitsRecordList *next_record;
};


/* Header and Data Unit List */

typedef struct _FitsHduList FitsHduList;

struct _FitsHduList
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

  FitsRecordList *header_record_list; /* Header records read in */

  FitsHduList    *next_hdu;
};


typedef struct _FitsFile FitsFile;

struct _FitsFile
{
  FILE     *fp;               /* File pointer to fits file */
  gchar     openmode;         /* Mode the file was opened (0, 'r', 'w') */

  gint      n_hdu;            /* Number of HDUs in file */
  gint      n_pic;            /* Total number of interpretable pictures */
  gboolean  nan_used;         /* NaN's used in the file ? */
  gboolean  blank_used;       /* Blank's used in the file ? */

  FitsHduList *hdu_list;    /* Header and Data Unit List */
};


/* User callable functions of the FITS-library */

FitsFile    * fits_open         (const gchar      *filename,
                                 const gchar      *openmode);
void          fits_close        (FitsFile         *ff);
FitsHduList * fits_add_hdu      (FitsFile         *ff);
gint          fits_add_card     (FitsHduList      *hdulist,
                                 const gchar      *card);
gint          fits_write_header (FitsFile         *ff,
                                 FitsHduList      *hdulist);
FitsHduList * fits_image_info   (FitsFile         *ff,
                                 gint              picind,
                                 gint             *hdupicind);
FitsHduList * fits_seek_image   (FitsFile         *ff,
                                 gint              picind);
void          fits_print_header (FitsHduList      *hdr);
FitsData    * fits_decode_card  (const gchar      *card,
                                 FitsDataType      data_type);
gchar       * fits_search_card  (FitsRecordList   *rl,
                                 gchar            *keyword);
gint          fits_read_pixel   (FitsFile         *ff,
                                 FitsHduList      *hdulist,
                                 gint              npix,
                                 FitsPixTransform *trans,
                                 void             *buf);

gchar       * fits_get_error    (void);


/* Demo functions */

#define FITS_NO_DEMO
gint          fits_to_pgmraw    (gchar            *fitsfile,
                                 gchar            *pgmfile);
gint          pgmraw_to_fits    (gchar            *pgmfile,
                                 gchar            *fitsfile);

#endif /* __FITS_IO_H__ */
