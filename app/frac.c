/**
 * Fractal Compression routines for the XCF File Format
 * Yaroslav Faybishenko
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "tile.h"
#include "gimage.h"
#include "xcf.h"
#include "frac.h"

#define FRAC_DONT_WORK

#ifdef FRAC_DONT_WORK

void xcf_compress_frac_info (int _layer_type)
{
}

void xcf_save_compress_frac_init (int _dom_density, double quality)
{
}

void xcf_load_compress_frac_init (int _image_scale, int _iterations)
{
}

gint xcf_load_frac_compressed_tile (XcfInfo *info, Tile *tile)
{
  return 0;
}

gint xcf_save_frac_compressed_tile (XcfInfo *info, Tile *tile)
{
  return 0;
}

#else /* FRAC_DONT_WORK */

#define float double

typedef unsigned char image_data;
typedef unsigned long uns_long;

#define MIN_BITS 2
#define MAX_BITS 4
#define MAX_GREY 255
#define MAX_CONTRAST 1.0
#define CONTRAST_BITS    4
#define BRIGHTNESS_BITS  6
#define MAX_QCONTRAST   ((1<<CONTRAST_BITS)-1)   /* max quantized contrast */
#define MAX_QBRIGHTNESS ((1<<BRIGHTNESS_BITS)-1) /* max quantized brightness */

/* Number of classes. Each class corresponds to one specific ordering
 * of the image brightness in the four quadrants of a range or domain.
 * There are 4*3*2 = 24 classes. */
#define NCLASSES 24

#ifdef __STDC__
#  define OF(args)  args
#else
#  define OF(args)  ()
#endif


/* Compute the sum of pixel values or squared pixel values in a range
 * or domain from (x,y) to (x+size-1, y+size-1) included.
 * For a domain, the returned value is scaled by 4 or 16.0 respectively.
 * x, y and size must all be even. */
#define region_sum(cum,x,y,size) \
(cum[((y)+(size))>>1][((x)+(size))>>1] - cum[(y)>>1][((x)+(size))>>1] \
 - cum[((y)+(size))>>1][(x)>>1]          + cum[(y)>>1][(x)>>1])


#define square(pixel) (uns_long)(pixel)*(pixel)
#define dequantize(value, max, imax) ((double)(value)*(max)/(double)imax)


/* Information common to all domains of a certain size: info[s] describes
 * domains of size 1<<(s+1), corresponding to ranges of size 1<<s
 */
struct domain_info {
    gint pos_bits;   /* Number of bits required to encode a domain position */
    gint x_domains;  /* Number of domains in x (horizontal) dimension */
} dom_info[MAX_BITS+1];


/* Each domain is described by a 'domain_data' structure.
 * domain_head[c][s] is the head of the list of domains of class c
 * and size 1<<(s+1) (corresponding to ranges of size 1<<s).
 */
typedef struct domain_struct {
    gint x;                      /* horizontal position */
    gint y;                      /* vertical position */
    float d_sum;                /* sum of all values in the domain */
    float d_sum2;               /* sum of all squared values in the domain */
    struct domain_struct *next; /* next domain in same class */
} domain_data;


typedef struct map_struct {
    gint    contrast;   /* quantized best contrast between range and domain */
    gint    brightness; /* quantized best brightness offset */
    double error2;    /* sum of squared differences between range and domain */
} affine_map;


typedef struct range_struct {
    gint x;         /* horizontal position */
    gint y;         /* vertical position */
    gint s_log;     /* log base 2 of the range size */
    double r_sum;  /* sum of all values in the range */
    double r_sum2; /* sum of all squared values in the range */
} range_data;


typedef void (*process_func) OF((gint x, gint y, gint s_log));

typedef struct _BIT_FILE BIT_FILE;

struct _BIT_FILE
{
  FILE *file;
  unsigned char mask;
  int rack;
  int pacifier_counter;
  guint *cp;
};


BIT_FILE*     OpenInputBitFile (FILE *fp , guint *cp);
BIT_FILE*     OpenOutputBitFile (FILE *fp, guint *cp);
void          OutputBit (BIT_FILE *bit_file, int bit);
void          OutputBits (BIT_FILE *bit_file,
                          unsigned long code, int count);
int           InputBit (BIT_FILE *bit_file);
unsigned long InputBits (BIT_FILE *bit_file, int bit_count);
void          CloseInputBitFile (BIT_FILE *bit_file);
void          CloseOutputBitFile (BIT_FILE *bit_file);
void          FilePrintBinary (FILE *file, unsigned int code, int bits);

static void decompressTile (Tile *destTile, gint num_channels);
static void decompressChannelTile (guchar *destData, gint _x, gint _y);

static void decompose_into_channels (Tile *src,
			      guchar *channelTilesData[MAX_CHANNELS],
			      gint num_channels);

static void fractal_compress (guchar *srcData, gint x_size, gint y_size);
static gint size_sanity_check (Tile *tile);
static void compress_init (guchar *tile, gint x_size, gint y_size);
static void classify_domains (gint x_size, gint y_size, gint s);
static gint find_class(gint x, gint y, gint size);

static void pete_fatal  (char *shoutAtPete);
static void pete_warn   (char *tellPete);

static void **allocate     OF((gint rows, gint columns, gint elem_size));
static void *xalloc        OF((unsigned size));

static void dominfo_init   OF((gint x_size, gint y_size, gint density));
static gint  bitlength      OF((uns_long val));
static void compress_range   OF((gint x, gint y, gint s_log));
static void traverse_image OF((gint x, gint y, gint x_size, gint y_size, process_func process));
static gint  quantize       OF((double value, double max, gint imax));
static void compress_cleanup OF((gint y_size));
static void free_array     OF((void **array, gint rows));
static void decompress_range   OF((gint x, gint y, gint s_log));
static void refine_image       OF((void));
static void average_boundaries OF((void));

gint layer_type = 0;
double quality = 2.0;
gint dom_density = 2;
uns_long **cum_range;
domain_data *domain_head[NCLASSES][MAX_BITS+1];
float **cum_domain2;
image_data **range;
unsigned **domain;
float **cum_range2;
double max_error2;
char *progname;
BIT_FILE *frac_file;
gint image_scale = 1;
gint iterations = 8;
gint curProgress;
gint maxProgress;

XcfInfo *XCFFileInfo;

gint num_channels_arr [] = { 3, 4, 1, 2, 1, 2 };
/* RGB has 3 channels, RGBA - 4, GRAY - 1, GRAYA - 2, INDEXED - 1, INDEXEDA - 2, right? */


/* Domain density: domains of size s*s are located every (s>>dom_density)
 * pixels. The density factor can range from 0 to 2 (smallest domains
 * have a size of 8 and must start on even pixels boundaries). Density
 * factors 1 and 2 get better image quality but significantly slow
 * down compression. */

gint layer_type_init = 0;
gint load_initted = 0;
gint save_initted = 0;

void
xcf_compress_frac_info (gint _layer_type)
{
  layer_type = _layer_type;
  g_print ("layer_type = %i\n", layer_type);
  layer_type_init = 1;
}

void
xcf_load_compress_frac_init (gint _image_scale, gint _iterations)
{
  if (_image_scale != 1)
    pete_fatal ("You aren't supposed to allow for scale != 1 yet!");
  image_scale = _image_scale;
  iterations = _iterations;
  g_print ("image_scale = %i;  iterations = %i\n", image_scale, iterations);
  load_initted = 1;
}

void
xcf_save_compress_frac_init (gint _dom_density, double _quality)
{
  dom_density = _dom_density;
  quality = _quality;
  g_print ("dom_density = %i;  quality = %f\n", dom_density, quality);
  save_initted = 1;
}


gint
xcf_load_frac_compressed_tile (XcfInfo *info, Tile *tile)
{
  char type;

  gint x_size;         /* horizontal image size */
  gint y_size;         /* vertical image size */
  gint x_dsize;        /* horizontal size of decompressed image */
  gint y_dsize;        /* vertical size of decompressed image */

  if (!load_initted)
    pete_warn ("You forgot to make a call to load_init");
  else
    g_print ("Good job, pete\n");

  XCFFileInfo = info;

  tile_ref2 (tile, TRUE);

  frac_file = OpenInputBitFile (info->fp, &info->cp);
  if (frac_file == NULL )
    {
      g_error ("Error converting %s FILE * to BIT_FILE", info->filename);
      return 0;
    } else
      g_print (" OpenInputBitFile (info->fp, &info->cp) suceeded\n");

  /* Read the header of the fractal file: */
  type = InputBits (frac_file, 8);

  if (type != 'X') {
    g_error ("Sanity check failed - first byte not 'X'!");
    return 0;
  } else
    g_print ("Sanity check passed\n");

  x_size = (gint) InputBits (frac_file, 16);
  y_size = (gint) InputBits (frac_file, 16);
  dom_density = (gint) InputBits (frac_file, 2);

  g_print ("Image width = %i, height = %i\n", x_size, y_size);
  g_print ("Domain density = %i\n", dom_density);

  /* Allocate the scaled image: */
  x_dsize = x_size * image_scale;
  y_dsize = y_size * image_scale;
  g_print ("Scaled image width = %i, height = %i\n", x_size, y_size);

  range = (image_data**) allocate(y_dsize, x_dsize,
				 sizeof(image_data));

  g_print ("%i x %i range allocated\n", x_dsize, y_dsize);


  g_print ("Calling decompressTile (tile, num_channels=%i)\n",
	   num_channels_arr[layer_type]);
  decompressTile (tile, num_channels_arr[layer_type]);
  g_print ("Returned from decompressTile (tile, num_channels=%i)\n",
	   num_channels_arr[layer_type]);

  tile_unref (tile, TRUE);

  return 1;
}

static void
decompressTile (Tile *destTile, gint num_channels)
{
  gint i, j, k;
  guchar *channelData [6];
  guchar *cur_char;
  guchar type;

  for (i = 0; i < num_channels; i++) {

    type = InputBits (frac_file, 8);

    if (type != 'X')
      g_error ("Sanity check failed - first byte not 'X'!");

    channelData[i] = (guchar *) malloc (destTile->ewidth *
					destTile->eheight *
					sizeof (guchar) );
    decompressChannelTile ( channelData[i], destTile->ewidth, destTile->eheight );

  }

  g_print ("Squishing the channel tiles back\n");

  cur_char = destTile->data;

  for (i = 0; i < destTile->ewidth; i++)
    for (j = 0; j < destTile->eheight; j++)
      for (k = 0; k < num_channels; k++)   /* squish the channels back */
	*(cur_char)++ = *(channelData [k])++;


  g_print ("\tDone squishing\n");

  return;
}

static void
decompressChannelTile (guchar *channelTileData, gint _x, gint _y)
{

  gint y, x;
  guchar *cur_char;

  /* Initialize the domain information as in the compressor: */

  g_print ("Calling dominfo_init (%i, %i, %i)\n",
	   _x, _y, dom_density);

  dominfo_init (_x, _y, dom_density);

  g_print ("\t- done\n");


  /* Read all the affine maps, by using the same recursive traversal
     of the image as the compressor:   */

  g_print ("Calling traverse_image (0, 0, %i, %i, decompress_range)\n",
	   _x, _y);
  traverse_image(0, 0, _x, _y, decompress_range);
  g_print ("\t- done\n");


  /* Iterate all affine maps over an initially random image. Since the
     affine maps are contractive, this process converges.   */

  iterations = 8;
  while (iterations-- > 0) {
    g_print ("Iteration #%i\n", iterations);
    refine_image();
  }

  /* Smooth the transition between adjacent ranges: */
  g_print ("Calling average_boundaries()");
  average_boundaries();
  g_print ("\t- done\n");

  cur_char = channelTileData;

  for (y = 0; y < _y; y++)
    for (x = 0; x < _x; x++)
      *(cur_char)++ = range[y][x];


  g_print ("Done copying data into destChannelTile\n");
  return;
}


gint
xcf_save_frac_compressed_tile (XcfInfo *info, Tile *tile)
{
  guchar *channelTilesData[MAX_CHANNELS];
  gint i, num_channels;

  tile_ref2 (tile, FALSE);

  if (!save_initted)
    pete_warn ("Using default values for save variables");

  if (! size_sanity_check (tile) )
    return 0;

  num_channels = num_channels_arr[layer_type];

  for (i = 0; i < num_channels; i++)
    channelTilesData[i] = (guchar *) malloc (tile->eheight * tile->ewidth * sizeof (guchar));

    g_print ("channelTilesData address = %p\n", channelTilesData);
    decompose_into_channels (tile, channelTilesData, num_channels);
    g_print ("\tchannelTilesData address = %p\n", channelTilesData);

    frac_file = OpenOutputBitFile (info->fp, &info->cp);

  OutputBits(frac_file, (uns_long)'X', 8);

  for (i = 0; i < num_channels; i++)
    {
      fractal_compress (channelTilesData[i], tile->ewidth, tile->eheight);
      compress_cleanup (tile->eheight);
    }

  CloseOutputBitFile (frac_file);

  tile_unref (tile, FALSE);

  return 1;

}


static gint
size_sanity_check(Tile *tile)
{
  char message[200];

  if (tile->ewidth % 4 != 0 || tile->eheight % 4 != 0) {

    g_warning (message, "Width = %i, Height = %i\nTile sizes must be multiple of 4",
	       tile->ewidth, tile->eheight);

    return 0;
  }

  return 1;

}

static void
fractal_compress (guchar *srcTileData, gint x_size, gint y_size)
{
  gint s;                /* size index for domains; their size is 1<<(s+1) */

  /* Allocate and initialize the image data and cumulative image data: */
  compress_init (srcTileData, x_size, y_size);

  /* Initialize the domain size information as in the decompressor: */
  dominfo_init(x_size, y_size, dom_density);

  /* Classify all domains: */
  for (s = MIN_BITS; s <= MAX_BITS; s++)
    classify_domains (x_size, y_size, s);

  OutputBits(frac_file, (uns_long)x_size, 16);
  OutputBits(frac_file, (uns_long)y_size, 16);
  OutputBits(frac_file, (uns_long)dom_density, 2);

  OutputBits(frac_file, (uns_long)'X', 8);



  max_error2 = quality*quality;
  traverse_image(0, 0, x_size, y_size, compress_range);
}


/* Allocate and initialize the image data and cumulative image data.
 * must be a one-channel tile */
static void
compress_init (guchar *srcData, gint x_size, gint y_size) {

  gint x, y;

  uns_long r_sum;       /* cumulative range and domain data */
  double r_sum2;        /* cumulative squared range data */
  double d_sum2;        /* cumulative squared domain data */

  guchar *cur;

  range =   (image_data**)allocate(y_size,     x_size,   sizeof(image_data));
  domain    = (unsigned**)allocate(y_size/2,   x_size/2,   sizeof(unsigned));
  cum_range = (uns_long**)allocate(y_size/2+1, x_size/2+1, sizeof(uns_long));
  cum_range2  =  (float**)allocate(y_size/2+1, x_size/2+1, sizeof(float));
  cum_domain2 =  (float**)allocate(y_size/2+1, x_size/2+1, sizeof(float));


  /* transfer srcData (from the tile) ginto our own array */

  cur = srcData;

  for (y = 0; y < y_size; y++)
    for (x = 0; x < x_size; x++)
      range[y][x] = *cur++;

  /* Compute the 'domain' image from the 'range' image. Each pixel in
   * the domain image is the sum of 4 pixels in the range image.  We
   * don't average (divide by 4) to avoid losing precision.
   */
  for (y=0; y < y_size/2; y++)
    for (x=0; x < x_size/2; x++)
      domain[y][x] = (unsigned)range[y<<1][x<<1] + range[y<<1][(x<<1)+1]
	+ range[(y<<1)+1][x<<1] + range[(y<<1)+1][(x<<1)+1];

  /* Compute the cumulative data, which will avoid repeated computations
   * later (see the region_sum() macro below).
   */
  for (x=0; x <= x_size/2; x++)
    {
      cum_range[0][x] = 0;
      cum_range2[0][x] = cum_domain2[0][x] = 0.0;
    }

  for (y=0; y < y_size/2; y++)
    {

      d_sum2 = r_sum2 = 0.0;
      r_sum = cum_range[y+1][0] = 0;
      cum_range2[y+1][0] = cum_domain2[y+1][0] = 0.0;

      for (x=0; x < x_size/2; x++)
	{
	  r_sum += domain[y][x];
	  cum_range[y+1][x+1] = cum_range[y][x+1] + r_sum;

	  d_sum2 += (double)square(domain[y][x]);
	  cum_domain2[y+1][x+1] = cum_domain2[y][x+1] + d_sum2;

	  r_sum2 += (double) (square(range[y<<1][x<<1])
			      + square(range[y<<1][(x<<1)+1])
			      + square(range[(y<<1)+1][x<<1])
			      + square(range[(y<<1)+1][(x<<1)+1]));
	  cum_range2[y+1][x+1] = cum_range2[y][x+1] + r_sum2;
	}

    }
}

/* Classify all domains of a certain size. This is done only once to save
 * computations later. Each domain is inserted in a linked list according
 * to its class and size.
 */
static void
classify_domains(gint x_size, gint y_size, gint s) {
    domain_data *dom = NULL; /* pointer to new domain */
    gint x, y;                /* horizontal and vertical domain position */
    gint class;               /* domain class */
    gint dom_size = 1<<(s+1); /* domain size */
    gint dom_dist = dom_size >> dom_density; /* distance between domains */

    /* Initialize all domain lists to be empty: */
    for (class = 0; class < NCLASSES; class++)
        domain_head[class][s] = NULL;

    /* Classify all domains of this size: */

    for (y = 0; y <= y_size - dom_size; y += dom_dist)

      for (x = 0; x <= x_size - dom_size; x += dom_dist)
	{
	  dom = (domain_data *)xalloc(sizeof(domain_data));
	  dom->x = x;
	  dom->y = y;
	  dom->d_sum  = 0.25  *(double)region_sum(cum_range, x, y, dom_size);
	  dom->d_sum2 = 0.0625*(double)region_sum(cum_domain2, x, y, dom_size);
	  class = find_class(x, y, dom_size);
	  dom->next = domain_head[class][s];
	  domain_head[class][s] = dom;
	}

    /* Check that each domain class contains at least one domain.
     * If a class is empty, we do as if it contains the last created
     * domain (which is actually of a different class).
     */

    for (class = 0; class < NCLASSES; class++)

      if (domain_head[class][s] == NULL)
	  {
	    domain_data *dom2 = (domain_data *) xalloc(sizeof(domain_data));
	    *dom2 = *dom;
	    dom2->next = NULL;
	    domain_head[class][s] = dom2;
	  }

}

/* Classify a range or domain.  The class is determined by the
 * ordering of the image brightness in the four quadrants of the range
 * or domain. For each quadrant we compute the number of brighter
 * quadrants; this is sufficient to uniquely determine the
 * class. class 0 has quadrants in order of decreasing brightness;
 * class 23 has quadrants in order of increasing brightness.
 *
 * IN assertion: x, y and size are all multiple of 4.
 */
static gint
find_class(gint x, gint y, gint size) {
    gint class = 0;               /* the result class */
    gint i,j;                     /* quadrant indices */
    uns_long sum[4];             /* sums for each quadrant */
    static gint delta[3] = {6, 2, 1}; /* table used to compute the class number */
    gint size1 = size >> 1;

    /* Get the cumulative values of each quadrant. By the IN assertion,
     * size1, x+size1 and y+size1 are all even. */

    sum[0] = region_sum(cum_range, x,       y,       size1);
    sum[1] = region_sum(cum_range, x,       y+size1, size1);
    sum[2] = region_sum(cum_range, x+size1, y+size1, size1);
    sum[3] = region_sum(cum_range, x+size1, y,       size1);

    /* Compute the class from the ordering of these values */
    for (i = 0;   i <= 2; i++)
    for (j = i+1; j <= 3; j++)
      if (sum[i] < sum[j]) class += delta[i];

    return class;
}

static void
decompose_into_channels (Tile *tile,
			 guchar *channelTilesData[MAX_CHANNELS],
			 gint num_channels) {

  gint i, j, k;
  guchar **begin, *cur_char_tile;

  g_print ("Decomposing into %i num_channels\n", num_channels);

  cur_char_tile = tile->data;
  begin = &channelTilesData[0];

  for (i = 0; i < tile->eheight; i++)
    for (j = 0; j < tile->ewidth; j++)
      for (k = 0; k < num_channels; k++)
	*(channelTilesData[k])++ = *(cur_char_tile)++;

  channelTilesData = begin;
  return;
}


/* Allocate a two dimensional array. For portability to 16-bit
  architectures with segments limited to 64K, we allocate one
  array per row, so the two dimensional array is allocated
  as an array of arrays.
 */
static void
**allocate(gint rows, gint columns, gint elem_size) {
  gint row;
  void **array = (void**)xalloc(rows * sizeof(void *));
  for (row = 0; row < rows; row++)
    array[row] = (void*)xalloc(columns * elem_size);
  return array;
}

/* Initialize the domain information dom_info. This must be done in the
 * same manner in the compressor and the decompressor.
 */
static void dominfo_init(gint x_size, gint y_size, gint density) {
  gint s;            /* size index for domains; their size is 1<<(s+1) */

  for (s = MIN_BITS; s <= MAX_BITS; s++)
    {
      gint y_domains;            /* number of domains vertically */
      gint dom_size = 1<<(s+1);  /* domain size */

      /* The distance between two domains is the domain size 1<<(s+1)
       * shifted right by the domain density, so it is a power of two.
       */
      dom_info[s].x_domains = ((x_size - dom_size)>>(s + 1 - density)) + 1;
      y_domains             = ((y_size - dom_size)>>(s + 1 - density)) + 1;

      /* Number of bits required to encode a domain position: */
      dom_info[s].pos_bits =  bitlength
	((uns_long)dom_info[s].x_domains * y_domains - 1);
    }
}

/* Allocate memory and check that the allocation was successful. */
static void
*xalloc(unsigned size) {
  void *p = malloc(size);
  if (p == NULL) {
    g_error ("insufficient memory\n");
    exit (1); /* shouldn't we really be doing something else? */
  }
  return p;
}

/* Return the number of bits needed to represent an integer:
 * 0 to 1 -> 1,
 * 2 to 3 -> 2,
 * 3 to 7 -> 3, etc...
 * This function could be made faster with a lookup table.
 */
static gint
bitlength(uns_long val) {
    gint bits = 1;

    if (val > 0xffff) bits += 16, val >>= 16;
    if (val > 0xff)   bits += 8,  val >>= 8;
    if (val > 0xf)    bits += 4,  val >>= 4;
    if (val > 0x3)    bits += 2,  val >>= 2;
    if (val > 0x1)    bits += 1;
    return bits;
}


/* Find the best affine mapping from a range to a domain. This is done
 * by minimizing the sum of squared errors as a function of the contrast
 * and brightness:  sum on all range pixels ri and domain pixels di of
 *      square(contrast*domain[di] + brightness - range[ri])
 * and solving the resulting equations to get contrast and brightness.
 */
static void
find_map(range_data *rangep, domain_data *dom, affine_map *map)
{
  /*    range_data  *rangep; range information (input parameter)
	domain_data *dom;    domain information (input parameter)
	affine_map  *map;    resulting map (output parameter) */

  gint ry;            /* vertical position inside the range */
  gint dy = dom->y >> 1; /* vertical position inside the domain */
  uns_long rd = 0;   /* sum of range*domain values (scaled by 4) */
  double rd_sum;     /* sum of range*domain values (normalized) */
  double contrast;   /* optimal contrast between range and domain */
  double brightness; /* optimal brightness offset between range and domain */
  double qbrightness;/* brightness after quantization */
  double max_scaled; /* maximum scaled value = contrast*MAX_GREY */
  gint r_size = 1 << rangep->s_log;                 /* the range size */
  double pixels = (double)((long)r_size*r_size); /* total number of pixels */

  for (ry = rangep->y; ry < rangep->y + r_size; ry++, dy++)
    {

      register image_data *r = &range[ry][rangep->x];
      register unsigned   *d = &domain[dy][dom->x >> 1];
      gint i = r_size >> 2;

      /* The following loop is the most time consuming part of the whole
       * program, so it is unrolled a little. I rely on r_size being a
       * multiple of 4 (ranges smaller than 4 don't make sense because
       * of the very bad compression). rd cannot overflow with unsigned
       * 32-bit arithmetic since MAX_BITS <= 7 implies r_size <= 128.
       */
      do {
	rd += (uns_long)(*r++)*(*d++);
	rd += (uns_long)(*r++)*(*d++);
	rd += (uns_long)(*r++)*(*d++);
	rd += (uns_long)(*r++)*(*d++);
      } while (--i != 0);
    }
  rd_sum = 0.25*rd;

  /* Compute and quantize the contrast: */
  contrast = pixels * dom->d_sum2 - dom->d_sum * dom->d_sum;
  if (contrast != 0.0) {
    contrast = (pixels*rd_sum - rangep->r_sum*dom->d_sum)/contrast;
  }
  map->contrast = quantize(contrast, MAX_CONTRAST, MAX_QCONTRAST);

  /* Recompute the contrast as in the decompressor: */
  contrast = dequantize(map->contrast, MAX_CONTRAST, MAX_QCONTRAST);

  /* Compute and quantize the brightness. We actually quantize the value
   * (brightness + 255*contrast) to get a positive value:
   *    -contrast*255 <= brightness <= 255
   * so 0 <= brightness + 255*contrast <= 255 + contrast*255
   */
  brightness = (rangep->r_sum - contrast*dom->d_sum)/pixels;
  max_scaled = contrast*MAX_GREY;
  map->brightness = quantize(brightness + max_scaled,
			     max_scaled + MAX_GREY, MAX_QBRIGHTNESS);

  /* Recompute the quantized brightness as in the decompressor: */
  qbrightness = dequantize(map->brightness, max_scaled + MAX_GREY,
			   MAX_QBRIGHTNESS) - max_scaled;

  /* Compute the sum of squared errors, which is the quantity we are
   * trying to minimize: */
  map->error2 = contrast*(contrast*dom->d_sum2 - 2.0*rd_sum) + rangep->r_sum2
    + qbrightness*pixels*(qbrightness - 2.0*brightness);
}


/* Split a rectangle sub-image into a square and potentially two rectangles,
 * then split the square and rectangles recursively if necessary.  To simplify
 * the algorithm, the size of the square is chosen as a power of two.
 * If the square if small enough as a range, call the appropriate compression
 * or decompression function for this range.
 * IN assertions: x, y, x_size and y_size are multiple of 4. */
static void
traverse_image (gint x, gint y, gint x_size, gint y_size, process_func process)
{
  /* x, y;        sub-image horizontal and vertical position
     x_size, y_size;   sub-image horizontal and vertical sizes
     process_func process; the compression or decompression function */

  gint s_size;  /* size of the square; s_size = 1<<s_log */
  gint s_log;   /* log base 2 of this size */

  s_log = bitlength(x_size < y_size ? (uns_long)x_size : (uns_long)y_size)-1;
  s_size = 1 << s_log;

  /* Since x_size and y_size are >= 4, s_log >= MIN_BITS */


  /* Split the square recursively if it is too large for a range: */

  if (s_log > MAX_BITS)
    {
      traverse_image(x,          y,          s_size/2, s_size/2, process);
      traverse_image(x+s_size/2, y,          s_size/2, s_size/2, process);
      traverse_image(x,          y+s_size/2, s_size/2, s_size/2, process);
        traverse_image(x+s_size/2, y+s_size/2, s_size/2, s_size/2, process);
    }
  else
    {
      /* Compress or decompress the square as a range: */
      (*process)(x, y, s_log);
      }

  /* Traverse the rectangle on the right of the square: */
  if (x_size > s_size) {
    traverse_image(x + s_size, y, x_size - s_size, y_size, process);

    /* Since x_size and s_size are multiple of 4, x + s_size and
     * x_size - s_size are also multiple of 4.
     */
  }

  /* Traverse the rectangle below the square: */
  if (y_size > s_size)
    traverse_image(x, y + s_size, s_size, y_size - s_size, process);

}


/* Compress a range by searching a match with all domains of the same class.
 * Split the range if the mean square error with the best domain is larger
 * than max_error2.
 * IN assertion: MIN_BITS <= s_log <= MAX_BITS */

static void
compress_range(gint x, gint y, gint s_log) {
  /* s_log is  log base 2 of the range size */

  gint r_size = 1<<s_log; /* size of the range */
  gint class;             /* range class */
  domain_data *dom;      /* used to iterate over all domains of this class */
  domain_data *best_dom = NULL; /* pointer to the best domain */
  range_data range;      /* range information for this range */
  affine_map map;        /* affine map for current domain  */
  affine_map best_map;   /* best map for this range */
  uns_long dom_number;   /* domain number */


  /* Compute the range class and cumulative sums: */
  class = find_class(x, y, r_size);
  range.r_sum =  (double)region_sum(cum_range,  x, y, r_size);
  range.r_sum2 = (double)region_sum(cum_range2, x, y, r_size);
  range.x = x;
  range.y = y;
  range.s_log = s_log;

  /* Searching all classes can improve image quality but significantly slows
   * down compression. Compile with -DCOMPLETE_SEARCH if you can wait, pete */
#ifdef COMPLETE_SEARCH
  for (class = 0; class < NCLASSES; class++)
#endif
    for (dom = domain_head[class][s_log];  dom != NULL; dom = dom->next) {
      /* Find the optimal map from the range to the domain: */
      find_map(&range, dom, &map);
      if (best_dom == NULL || map.error2 < best_map.error2) {
	best_map = map;
	best_dom = dom;
      }
    }

  /* Output the best affine map if the mean square error with the
   * best domain is smaller than max_error2, or if it not possible
   * to split the range because it is too small: */
  if (s_log == MIN_BITS ||best_map.error2 <= max_error2*((long)r_size*r_size))
    {
      /* If the range is too small to be split, the decompressor knows
       * this, otherwise we must indicate that the range has not been split: */

      if (s_log != MIN_BITS)
	OutputBit(frac_file, 1);  /* affine map follows */

      OutputBits(frac_file, (uns_long)best_map.contrast, CONTRAST_BITS);

      OutputBits(frac_file, (uns_long)best_map.brightness, BRIGHTNESS_BITS);


      /* When the contrast is null, the decompressor does not need to know
       * which domain was selected: */

      if (best_map.contrast == 0) return;

      dom_number = (uns_long)best_dom->y * dom_info[s_log].x_domains
	+ (uns_long)best_dom->x;

      /* The distance between two domains is the domain size 1<<(s_log+1)
       * shifted right by the domain_density, so it is a power of two.
       * The domain x and y positions have (s_log + 1 - dom_density) zero
       * bits each, which we don't have to transmit.
       */

      OutputBits(frac_file, dom_number >> (s_log + 1 - dom_density),
		 dom_info[s_log].pos_bits);
    }
  else
    {
      /* Tell the decompressor that no affine map follows because
       * this range has been split:
       */
      OutputBit(frac_file, 0);

      /* Split the range into 4 squares and process them recursively: */
      compress_range(x,          y,          s_log-1);
      compress_range(x+r_size/2, y,          s_log-1);
      compress_range(x,          y+r_size/2, s_log-1);
      compress_range(x+r_size/2, y+r_size/2, s_log-1);

    }

}

/* Quantize a value in the range 0.0 .. max to the range 0..imax
 * ensuring that 0.0 is encoded as 0 and max as imax.
 */
static gint
quantize(double value, double max, gint imax) {
    gint ival = (gint) floor((value/max)*(double)(imax+1));

    if (ival < 0)
      return 0;

    if (ival > imax)
      return imax;

    return ival;
}


/* Free all dynamically allocated data structures for compression. */
static void
compress_cleanup (gint y_size)
{
  gint s;                   /* size index for domains */
  gint class;               /* class number */
  domain_data *dom, *next;  /* domain pointers */

  free_array((void**)range,       y_size);
  free_array((void**)domain,      y_size/2);
  free_array((void**)cum_range,   y_size/2 + 1);
  free_array((void**)cum_range2,  y_size/2 + 1);
  free_array((void**)cum_domain2, y_size/2 + 1);

  for (s = MIN_BITS; s <= MAX_BITS; s++)
    for (class = 0; class < NCLASSES; class++)
      for (dom = domain_head[class][s]; dom != NULL; dom = next) {
        next = dom->next;
        free(dom);
      }

}

/* Free a two dimensional array allocated as a set of rows. */
static void free_array (void **array, gint rows)
{
  gint row;

  for (row = 0; row < rows; row++)
    free(array[row]);

}

/* ------------------------------IO Routines ------------------------ */

#define PACIFIER_COUNT 2047

BIT_FILE
*OpenOutputBitFile (FILE *fp, guint *cp )
{
    BIT_FILE *bit_file;

    bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
    if ( bit_file == NULL )
      return( bit_file );
    bit_file->file = fp;
    bit_file->cp = cp;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
    return( bit_file );
}

BIT_FILE
*OpenInputBitFile (FILE *fp, guint *cp)
{
    BIT_FILE *bit_file;
    bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
    if ( bit_file == NULL )
      return( bit_file );
    bit_file->file = fp;
    bit_file->cp = cp;
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
    return( bit_file );
}

void
CloseOutputBitFile (BIT_FILE *bit_file )
{
  if ( bit_file->mask != 0x80 )
    {
      if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
	g_error( "Fatal error in CloseOutputBitFile!\n" );
      else
	(*(bit_file->cp)) += 1;
    }
  /*  fclose (bit_file->file );
      free ((char *) bit_file);  */
}

void
CloseInputBitFile (BIT_FILE *bit_file)
{
  /* fclose( bit_file->file );
  free( (char *) bit_file ); */
}

void
OutputBit (BIT_FILE *bit_file, gint bit )
{
  if ( bit )
    bit_file->rack |= bit_file->mask;

  bit_file->mask >>= 1;

  if ( bit_file->mask == 0 ) {

    if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
      g_error( "Fatal error in OutputBit!\n" );

    else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
      putc( '.', stdout );

    else /* putc succeeded */
      (*(bit_file->cp)) += 1;

    bit_file->rack = 0;
    bit_file->mask = 0x80;
  }
}

void
OutputBits (BIT_FILE *bit_file, unsigned long code, gint count)
{
  unsigned long mask;

  mask = 1L << ( count - 1 );

  while ( mask != 0) {

    if ( mask & code )
      bit_file->rack |= bit_file->mask;

    bit_file->mask >>= 1;

    if ( bit_file->mask == 0 ) {
      if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
	g_error( "Fatal error in OutputBit!\n" );

      else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
	putc( '.', stdout );

      else /* putc suceeded */
	(*(bit_file->cp)) += 1;

      bit_file->rack = 0;
      bit_file->mask = 0x80;

    }

    mask >>= 1;

  }

}

gint
InputBit (BIT_FILE *bit_file)
{
    gint value;

    if ( bit_file->mask == 0x80 )
      {
        bit_file->rack = getc( bit_file->file );

	(*(bit_file->cp)) += 1;

        if ( bit_file->rack == EOF )
	  g_error( "Fatal error in InputBit!\n" );

	if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
	  putc( '.', stdout );
      }

    value = bit_file->rack & bit_file->mask;

    bit_file->mask >>= 1;

    if ( bit_file->mask == 0 )
	bit_file->mask = 0x80;

    return( value ? 1 : 0 );
}

unsigned long
InputBits (BIT_FILE *bit_file, gint bit_count)
{
    unsigned long mask;
    unsigned long return_value;

    mask = 1L << ( bit_count - 1 );

    return_value = 0;

    while ( mask != 0)
      {
	if ( bit_file->mask == 0x80 )
	  {
	    bit_file->rack = getc( bit_file->file );
	    (*(bit_file->cp)) += 1;
	    if ( bit_file->rack == EOF )
	      g_error( "Fatal error in InputBit!\n" );
	    if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
	      putc( '.', stdout );
	  }

	if ( bit_file->rack & bit_file->mask )
	  return_value |= mask;

	mask >>= 1;
        bit_file->mask >>= 1;

	if ( bit_file->mask == 0 )
            bit_file->mask = 0x80;
    }

    return (return_value);
}

void
FilePrintBinary (FILE *file, guint code, gint bits )
{
    guint mask;

    mask = 1 << ( bits - 1 );
    while ( mask != 0 ) {
        if ( code & mask )
            fputc( '1', file );
        else
            fputc( '0', file );
        mask >>= 1;
    }
}


                /* Functions used for decompression */


/* An affine map is described by a contrast, a brightness offset, a range
   and a domain. The contrast and brightness are kept as integer values
   to speed up the decompression on machines with slow floating point. */

typedef struct map_info_struct
{
    gint contrast;   /* contrast scaled by 16384 (to maintain precision) */
    gint brightness; /* brightness offset scaled by 128 */
    gint x;          /* horizontal position of the range */
    gint y;          /* vertical position of the range */
    gint size;       /* range size */
    gint dom_x;      /* horizontal position of the domain */
    gint dom_y;      /* vertical position of the domain */
    struct map_info_struct *next; /* next map */
} map_info;

map_info *map_head = NULL; /* head of the linked list of all affine maps */

/* Read the affine map for a range, or split the range if the compressor
   did so in the function compress_range().
 */
static void
decompress_range(gint x, gint y, gint s_log)
{
  /* x, y;  horizontal and vertical position of the range /
     s_log;    log base 2 of the range size */

    gint r_size = 1<<s_log; /* range size */
    map_info *map;         /* pointer to affine map information */
    double contrast;       /* contrast between range and domain */
    double brightness;     /* brightness offset between range and domain */
    double max_scaled;     /* maximum scaled value = contrast*MAX_GREY */
    uns_long dom_number;   /* domain number */

    /* Read an affine map if the compressor has written one at this point: */
    if (s_log == MIN_BITS || InputBit(frac_file))
      {
        map = (map_info *)xalloc(sizeof(map_info));
        map->next = map_head;
        map_head = map;

	map->x = x;
        map->y = y;
        map->size = r_size;
        map->contrast   = (gint)InputBits(frac_file, CONTRAST_BITS);
        map->brightness = (gint)InputBits(frac_file, BRIGHTNESS_BITS);

        contrast = dequantize(map->contrast, MAX_CONTRAST, MAX_QCONTRAST);
        max_scaled = contrast*MAX_GREY;
        brightness = dequantize(map->brightness, max_scaled + MAX_GREY,
                                MAX_QBRIGHTNESS) - max_scaled;

        /* Scale the brightness by 128 to maintain precision later, while
         * avoiding overflow with 16-bit arithmetic:
         *     -255 <= -contrast*255 <= brightness <= 255
         * so -32767 < brightness*128 < 32767
         */
        map->brightness = (gint)(brightness*128.0);

        /* When the contrast is null, the compressor did not encode the
         * domain number:
         */
        if (map->contrast != 0) {

            /* Scale the contrast by 16384 to maintain precision later.
             *   0.0 <= contrast <= 1.0 so 0 <= contrast*16384 <= 16384
             */
            map->contrast = (gint)(contrast*16384.0);

            /* Read the domain number, and add the zero bits that the
             * compressor did not transmit:
             */
            dom_number = InputBits(frac_file, dom_info[s_log].pos_bits);

            map->dom_x = (gint)(dom_number % dom_info[s_log].x_domains)
                          << (s_log + 1 - dom_density);
            map->dom_y = (gint)(dom_number / dom_info[s_log].x_domains)
                          << (s_log + 1 - dom_density);
        } else {
            /* For a null contrast, use an arbitrary domain: */
            map->dom_x = map->dom_y = 0;
        }

        /* Scale the range and domain if necessary. This implementation
         * uses only an integer scale to make sure that the union of all
         * ranges is exactly the scaled image, that ranges never overlap,
         * and that all range sizes are even.
         */
        if (image_scale != 1) {
            map->x *= image_scale;
            map->y *= image_scale;
            map->size *= image_scale;
            map->dom_x *= image_scale;
            map->dom_y *= image_scale;
        }
    } else {
        /* Split the range into 4 squares and process them recursively
         * as in the compressor:
         */
        decompress_range(x,          y,          s_log-1);
        decompress_range(x+r_size/2, y,          s_log-1);
        decompress_range(x,          y+r_size/2, s_log-1);
        decompress_range(x+r_size/2, y+r_size/2, s_log-1);
    }
}

/* Refine the image by applying one round of all affine maps on the
  image. The "pure" method would compute a separate new image and then
  copy it to the original image. However the convergence towards the
  final image happens to be quicker if we overwrite the same image
  while applying the affine maps; for the same quality of reconstructed
  image we need fewer iterations. Overwriting the same image also
  reduces the memory requirements.
 */
static void
refine_image()
{
    map_info *map;   /* pointer to current affine map */
    long brightness; /* brightness offset of the map, scaled by 65536 */
    long val;        /* new pixel value */
    gint y;           /* vertical position in range */
    gint dom_y;       /* vertical position in domain */
    gint j;

    for (map = map_head; map != NULL; map = map->next) {

        /* map->brightness is scaled by 128, so scale it again by 512 to
         * get a total scale factor of 65536:
         */
        brightness = (long)map->brightness << 9;

        dom_y = map->dom_y;
        for (y = map->y; y < map->y + map->size; y++) {

            /* The following loop is the most time consuming, so we move
             * some address calculations outside the loop:
             */
	  /*	  if (!range)
	    g_error ("range is null!\n");
	  else
	    g_print ("range is alright\n");
	    */
	  image_data *r  = &range[y][map->x];
	  image_data *d  = &range[dom_y++][map->dom_x];
	  image_data *d1 = &range[dom_y++][map->dom_x];
	  j = map->size;
	  do {
	    val  = *d++ + *d1++;
	    val += *d++ + *d1++;
                /* val is now scaled by 4 and map->contrast is scaled by 16384,
		   so val * map->contrast will be scaled by 65536. */
	    val = val * map->contrast + brightness;
	    if (val < 0) val = 0;
	    val >>= 16; /* get rid of the 65536 scaling */
	    if (val >= MAX_GREY) val = MAX_GREY;

	    *r++ = (image_data)val;
	  } while (--j != 0);
        }
    }
}

/* Go through all ranges to smooth the transition between adjacent
   ranges, except those of minimal size.
   */
static void
average_boundaries()
{
    map_info *map;   /* pointer to current affine map */
    unsigned val;    /* sum of pixel value for current and adjacent ranges */
    gint x;           /* horizontal position in current range */
    gint y;           /* vertical position in current range */

    for (map = map_head; map != NULL; map = map->next) {

        if (map->size == (1<<MIN_BITS)) continue; /* range too small */

        if (map->x > 1) {
	  /* Smooth the left boundary of the range and the right boundary
	     of the adjacent range(s) to the left:
	     */
	  for (y = map->y; y < map->y + map->size; y++) {
                 val  = range[y][map->x - 1] + range[y][map->x];
                 range[y][map->x - 1] =
                     (image_data)((range[y][map->x - 2] + val)/3);
                 range[y][map->x] =
                     (image_data)((val + range[y][map->x + 1])/3);
            }
        }
        if (map->y > 1)  {
            /* Smooth the top boundary of the range and the bottom boundary
             * of the range(s) above:
             */
            for (x = map->x; x < map->x + map->size; x++) {
                 val  = range[map->y - 1][x] + range[map->y][x];
                 range[map->y - 1][x] =
                     (image_data)((range[map->y - 2][x] + val)/3);
                 range[map->y][x] =
                     (image_data)((val + range[map->y + 1][x])/3);
            }
        }
    }
}


static void
pete_warn (char *tellPete)
{
  g_warning ("Pete, %s\n", tellPete);
  return;
}

static void
pete_fatal (char *shoutAtPete)
{
  g_error ("Pete, you are a dumbass because %s\n", shoutAtPete);
}

#endif /* FRAC_DONT_WORK */
