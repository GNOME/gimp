#include <libgimp/gimp.h>

#define MAXCOLORS       256
#define Image		gint32

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define Write(file,buffer,len)   fwrite(buffer, len, 1, file)
#define WriteOK(file,buffer,len) (Write(buffer, len, file) != 0)

extern gint32       ToL           (guchar *);
extern void         FromL         (gint32,
				   guchar *);
extern gint16       ToS           (guchar *);
extern void         FromS         (gint16,
				   guchar *);
extern gint32       ReadBMP       (gchar *);
extern GStatusType  WriteBMP      (gchar *,
				   gint32,
				   gint32);
extern gint         ReadColorMap  (FILE *,
				   guchar[256][3],
				   gint,
				   gint,
				   gint *);
extern Image        ReadImage     (FILE *,
				   gint,
				   gint,
				   guchar[256][3],
				   gint,
				   gint,
				   gint,
				   gint,
				   gint);
extern void         WriteColorMap (FILE *,
				   gint *,
				   gint *,
				   gint *,
				   gint);
extern void         WriteImage    (FILE *,
				   guchar *,
				   gint,
				   gint,
				   gint,
				   gint,
				   gint,
				   gint,
				   gint);

extern gint   interactive_bmp;
extern gchar *prog_name;
extern gchar *filename;
extern FILE  *errorfile;

extern struct Bitmap_File_Head_Struct
{
  gulong   bfSize;      /* 02 */
  gulong   reserverd;   /* 06 */
  gulong   bfOffs;      /* 0A */
  gulong   biSize;      /* 0E */
} Bitmap_File_Head;

extern struct Bitmap_Head_Struct
{
  gulong   biWidth;     /* 12 */
  gulong   biHeight;    /* 16 */
  gushort  biPlanes;    /* 1A */
  gushort  biBitCnt;    /* 1C */
  gulong   biCompr;     /* 1E */
  gulong   biSizeIm;    /* 22 */
  gulong   biXPels;     /* 26 */
  gulong   biYPels;     /* 2A */
  gulong   biClrUsed;   /* 2E */
  gulong   biClrImp;    /* 32 */
                        /* 36 */
} Bitmap_Head;
  
extern struct Bitmap_OS2_Head_Struct
{   
  gushort  bcWidth;     /* 12 */
  gushort  bcHeight;    /* 14 */
  gushort  bcPlanes;    /* 16 */
  gushort  bcBitCnt;    /* 18 */
} Bitmap_OS2_Head;                

