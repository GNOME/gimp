#include <libgimp/gimp.h>

#define TRUE             1
#define FALSE            0
#define MAXCOLORS       256
#define Image		gint32

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)
#define WriteOK(file,buffer,len) (fwrite(buffer, len, 1, file) != 0)

extern gint32 ToL(guchar *);
extern void FromL(gint32, guchar *);
extern gint16 ToS(guchar *);
extern void FromS(gint16, guchar *);
extern gint32 ReadBMP (char *);
extern gint WriteBMP (char *,gint32,gint32);
extern gint ReadColorMap(FILE *, unsigned char[256][3], int, int, int *);
extern Image ReadImage(FILE *, int, int, unsigned char[256][3], int, int, int, int, int);
extern void WriteColorMap(FILE *, int *, int *, int *, int);
extern void WriteImage(FILE *,char *,int,int,int,int,int,int);

extern int interactive_bmp;
extern char *prog_name;
extern char *filename;
extern FILE *errorfile;

static struct 
  {
    unsigned long bfSize;		/* 02 */
    unsigned long reserverd;		/* 06 */
    unsigned long bfOffs;		/* 0A */
    unsigned long biSize;		/* 0E */
  }Bitmap_File_Head;

static struct
  {   
    unsigned long biWidth;		/* 12 */
    unsigned long biHeight;		/* 16 */
    unsigned short biPlanes;            /* 1A */
    unsigned short biBitCnt;	        /* 1C */
    unsigned long biCompr;		/* 1E */
    unsigned long biSizeIm;		/* 22 */
    unsigned long biXPels;		/* 26 */
    unsigned long biYPels;		/* 2A */
    unsigned long biClrUsed;		/* 2E */
    unsigned long biClrImp;		/* 32 */
    					/* 36 */
  }Bitmap_Head;
  
static struct
  {   
    unsigned short bcWidth;             /* 12 */
    unsigned short bcHeight;	        /* 14 */
    unsigned short bcPlanes;            /* 16 */
    unsigned short bcBitCnt;	        /* 18 */
  }Bitmap_OS2_Head;                
