#include "config.h"

#include <assert.h>
#include <stdio.h>

#include <libgimp/gimp.h>

#if GIMP_MAJOR_VERSION>1 || (GIMP_MAJOR_VERSION==1 && GIMP_MINOR_VERSION>=1)
# define GIMP11 1
# define GIMP_PARASITE 1
#endif

/* FIXME */
/* sys/param.h is redefining these! */
#undef MIN
#undef MAX

#if HAVE_PDL
#define PDL_clean_namespace
#include <pdlcore.h>
#undef croak
#define croak Perl_croak
#endif

/* various functions allocate static buffers, STILL.  */
#define MAX_STRING 4096

/* dunno where this comes from */
#undef VOIDUSED

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "gppport.h"

#ifndef pTHX_
#define pTHX_
#endif

#include "../perl-intl.h"

/* dirty is used in gimp.h AND in perl < 5.005 or with PERL_POLLUTE.  */
#ifdef dirty
# undef dirty
#endif

#include "extradefs.h"

#define GIMP_PKG	"Gimp::"	/* the package name */

#define PKG_COLOR	GIMP_PKG "Color"
#define PKG_REGION	GIMP_PKG "Region"
#define PKG_DISPLAY	GIMP_PKG "Display"
#define PKG_IMAGE	GIMP_PKG "Image"
#define PKG_LAYER	GIMP_PKG "Layer"
#define PKG_CHANNEL	GIMP_PKG "Channel"
#define PKG_DRAWABLE	GIMP_PKG "Drawable"
#define PKG_SELECTION	GIMP_PKG "Selection"
#define PKG_REGION	GIMP_PKG "Region"
#if GIMP_PARASITE
# define PKG_PARASITE	GIMP_PKG "Parasite"
#else
# define PKG_PARASITE	((char *)0)
#endif

#define PKG_GDRAWABLE	GIMP_PKG "GDrawable"
#define PKG_TILE	GIMP_PKG "Tile"
#define PKG_PIXELRGN	GIMP_PKG "PixelRgn"

#define PKG_ANY		((char *)0)

static char pkg_anyable[] = PKG_DRAWABLE ", " PKG_LAYER " or " PKG_CHANNEL;
#define PKG_ANYABLE	(pkg_anyable)

static int trace = TRACE_NONE;

#if HAVE_PDL

typedef GPixelRgn GPixelRgn_PDL;

/* hack, undocumented, argh! */
static Core* PDL; /* Structure hold core C functions */

/* get pointer to PDL structure. */
static void need_pdl (void)
{
  SV *CoreSV;

  if (!PDL)
    {
      /* the perl-server can't be bothered to do this itself! */
      perl_require_pv ("PDL::Core");

      /* Get pointer to structure of core shared C routines */
      CoreSV = perl_get_sv("PDL::SHARE", FALSE);
      if (!CoreSV)
        croak("gimp-perl-pixel functions require the PDL::Core module, which was not found");

      PDL = (Core*) SvIV(CoreSV);
    }
}

static pdl *new_pdl (int a, int b, int c)
{
  pdl *p = PDL->new();
  PDL_Long dims[3];
  int ndims = 0;

  if (c > 0) dims[ndims++] = c;
  if (b > 0) dims[ndims++] = b;
  if (a > 0) dims[ndims++] = a;

  PDL->setdims (p, dims, ndims);
  p->datatype = PDL_B;
  PDL->allocdata (p);

  return p;
}

static void old_pdl (pdl **p, short ndims, int dim0)
{
  PDL->make_physical (*p);
  PDL->converttype (p, PDL_B, PDL_PERM);

  if ((*p)->ndims < ndims + (dim0 > 1))
    croak (__("dimension mismatch, pdl has dimension %d but at least %d dimensions required"), (*p)->ndims, ndims + (dim0 > 1));

  if ((*p)->ndims > ndims + 1)
    croak (__("dimension mismatch, pdl has dimension %d but at most %d dimensions allowed"), (*p)->ndims, ndims + 1);

  if ((*p)->ndims > ndims && (*p)->dims[0] != dim0)
    croak (__("pixel size mismatch, pdl has %d channel pixels but %d channels are required"), (*p)->dims[0], dim0);
}

static void pixel_rgn_pdl_delete_data (pdl *p, int param)
{
  p->data = 0;
}

/* please optimize! */
static pdl *redim_pdl (pdl *p, int ndim, int newsize)
{
  pdl *r = PDL->null ();
  AV *dims, *dimincs;
  int i;

  dims    = newAV ();
  dimincs = newAV ();

  for (i = 0; i < p->ndims; i++)
    {
      av_push (dims   , newSViv (p->dims   [i]));
      av_push (dimincs, newSViv (p->dimincs[i]));
    }

  sv_setiv (*av_fetch (dims, ndim, 0), newsize);

  PDL->affine_new (p, r, 0, 
                   sv_2mortal (newRV_noinc ((SV*)dims)),
                   sv_2mortal (newRV_noinc ((SV*)dimincs)));

  return r;
}

#endif

/* set when its safe to call gimp functions.  */
static int gimp_is_initialized = 0;

typedef gint32 IMAGE;
typedef gint32 LAYER;
typedef gint32 CHANNEL;
typedef gint32 DRAWABLE;
typedef gint32 SELECTION;
typedef gint32 DISPLAY;
typedef gint32 REGION;
typedef gint32 COLOR;
typedef gpointer GPixelRgnIterator;

/* new SV with len len.  There _must_ be a better way, but newSV doesn't work.  */
static SV *newSVn (STRLEN len)
{
  SV *sv = newSVpv ("", 0);
  
  (void) SvUPGRADE (sv, SVt_PV);
  SvGROW (sv, len);
  SvCUR_set (sv, len);
  
  return sv;
}

static GHashTable *gdrawable_cache;

/* magic stuff.  literally.  */
static int gdrawable_free (pTHX_ SV *obj, MAGIC *mg)
{
  GDrawable *gdr = (GDrawable *)SvIV(obj);

  g_hash_table_remove (gdrawable_cache, (gpointer)gdr->id);
  gimp_drawable_detach (gdr);

  return 0;
}

static MGVTBL vtbl_gdrawable = {0, 0, 0, 0, gdrawable_free};

static SV *new_gdrawable (gint32 id)
{
   static HV *stash;
   SV *sv;
   
   if (!gdrawable_cache)
     gdrawable_cache = g_hash_table_new (g_direct_hash, g_direct_equal);

   assert (sizeof (gpointer) >= sizeof (id));

   if ((sv = (SV*)g_hash_table_lookup (gdrawable_cache, (gpointer)id)))
     SvREFCNT_inc (sv);
   else
     {
       GDrawable *gdr = gimp_drawable_get (id);

       if (!gdr)
         croak (__("unable to convert Gimp::Drawable into Gimp::GDrawable (id %d)"), id);

       if (!stash)
         stash = gv_stashpv (PKG_GDRAWABLE, 1);

       sv = newSViv ((IV) gdr);
       sv_magic (sv, 0, '~', 0, 0);
       mg_find (sv, '~')->mg_virtual = &vtbl_gdrawable;

       g_hash_table_insert (gdrawable_cache, (gpointer)id, (void *)sv);
     }
   
   return sv_bless (newRV_noinc (sv), stash);
}

static GDrawable *old_gdrawable (SV *sv)
{
  if (!(sv_derived_from (sv, PKG_GDRAWABLE)))
    croak (__("argument is not of type %s"), PKG_GDRAWABLE);

  /* the next line lacks any type of checking.  */
  return (GDrawable *)SvIV(SvRV(sv));
}

static /* drawable/tile/region stuff.  */
SV *new_tile (GTile *tile, SV *gdrawable)
{
  static HV *stash;
  HV *hv = newHV ();
  
  hv_store (hv, "_gdrawable"	,10, SvREFCNT_inc (gdrawable)	, 0);
  
  if (!stash)
    stash = gv_stashpv (PKG_TILE, 1);
  
  return sv_bless (newRV_noinc ((SV*)hv), stash);
}

static GTile *old_tile (SV *sv)
{
  if (!sv_derived_from (sv, PKG_TILE))
    croak (__("argument is not of type %s"), PKG_TILE);
  
  /* the next line lacks any type of checking.  */
  return (GTile *)SvIV(*(hv_fetch ((HV*)SvRV(sv), "_tile", 5, 0)));
}

/* magic stuff.  literally.  */
static int gpixelrgn_free (pTHX_ SV *obj, MAGIC *mg)
{
/*  GPixelRgn *pr = (GPixelRgn *)SvPV_nolen(obj); */
/* automatically done on detach */
/*  if (pr->dirty)
     gimp_drawable_flush (pr->drawable);*/

  return 0;
}

static MGVTBL vtbl_gpixelrgn = {0, 0, 0, 0, gpixelrgn_free};

/* coerce whatever was given into a gdrawable-sv */
static SV *force_gdrawable (SV *drawable)
{
  if (!(sv_derived_from (drawable, PKG_GDRAWABLE)))
    {
      if (sv_derived_from (drawable, PKG_DRAWABLE)
          || sv_derived_from (drawable, PKG_LAYER)
          || sv_derived_from (drawable, PKG_CHANNEL))
        drawable = sv_2mortal (new_gdrawable (SvIV (SvRV (drawable))));
      else
        croak (__("argument is not of type %s"), PKG_GDRAWABLE);
    }

  return drawable;
}

static SV *new_gpixelrgn (SV *gdrawable, int x, int y, int width, int height, int dirty, int shadow)
{
  static HV *stash;
  SV *sv = newSVn (sizeof (GPixelRgn));
  GPixelRgn *pr = (GPixelRgn *)SvPV_nolen(sv);

  if (!stash)
    stash = gv_stashpv (PKG_PIXELRGN, 1);
  
  gimp_pixel_rgn_init (pr, old_gdrawable (gdrawable),
                       x, y, width, height, dirty, shadow);

  sv_magic (sv, SvRV(gdrawable), '~', 0, 0);
  mg_find (sv, '~')->mg_virtual = &vtbl_gpixelrgn;

  return sv_bless (newRV_noinc (sv), stash);
}

static GPixelRgn *old_pixelrgn (SV *sv)
{
  if (!sv_derived_from (sv, PKG_PIXELRGN))
    croak (__("argument is not of type %s"), PKG_PIXELRGN);
  
  return (GPixelRgn *)SvPV_nolen(SvRV(sv));
}

static GPixelRgn *old_pixelrgn_pdl (SV *sv)
{
#if HAVE_PDL
  need_pdl ();
#endif
  return old_pixelrgn (sv);
}

/* tracing stuff.  */
static SV *trace_var = 0;
static PerlIO *trace_file = 0; /* FIXME: unportable.  */

static void
trace_init ()
{
  if (!trace_file)
    SvCUR_set (trace_var, 0);
}

#ifndef __STDC__
#error You need to compile with an ansi-c compiler!!!
#error Remove these lines to continue at your own risk!
#endif

#if __STDC_VERSION__ > 199900
#define trace_printf(...) \
	do { \
        	if (trace_file) PerlIO_printf (trace_file, __VA_ARGS__); \
		else		sv_catpvf (trace_var, __VA_ARGS__); \
        } while(0)
#elif __GNUC__
#define trace_printf(frmt,args...) \
        do { \
		if (trace_file) PerlIO_printf (trace_file, frmt, ## args); \
		else		sv_catpvf (trace_var, frmt, ## args); \
        } while(0)
#elif defined(__STDC__)

/* sigh */
#include <stdarg.h>
static void trace_printf (char *frmt, ...)
{
  va_list args;
  char buffer[MAX_STRING]; /* sorry... */
  
  va_start (args, frmt);
#ifdef HAVE_VSNPRINTF
  vsnprintf (buffer, sizeof buffer, frmt, args);
#else
  vsprintf (buffer, frmt, args);
#endif
  if (trace_file) PerlIO_printf (trace_file, "%s", buffer);
  else		  sv_catpv (trace_var, buffer);
}

#endif

static int
is_array (GParamType typ)
{
  return typ == PARAM_INT32ARRAY
      || typ == PARAM_INT16ARRAY
      || typ == PARAM_INT8ARRAY
      || typ == PARAM_FLOATARRAY
      || typ == PARAM_STRINGARRAY;
}

static int
perl_param_count (GParam *arg, int count)
{
  GParam *end = arg + count;
  
  while (arg < end)
    if (is_array (arg++->type))
      count--;
  
  return count;
}

/*
 * count actual parameter number
 */
static int
perl_paramdef_count (GParamDef *arg, int count)
{
  GParamDef *end = arg + count;
  
  while (arg < end)
    if (is_array (arg++->type))
      count--;
  
  return count;
}

/* horrors!  c wasn't designed for this!  */
#define dump_printarray(args,index,ctype,datatype,frmt) {\
  int j; \
  trace_printf ("["); \
  if (args[index].data.datatype || !args[index-1].data.d_int32) \
    { \
      for (j = 0; j < args[index-1].data.d_int32; j++) \
	trace_printf (frmt ## "%s", (ctype) args[index].data.datatype[j], \
	              j < args[index-1].data.d_int32 - 1 ? ", " : ""); \
    } \
  else \
    trace_printf (__("(UNINITIALIZED)")); \
  trace_printf ("]"); \
}

static void
dump_params (int nparams, GParam *args, GParamDef *params)
{
  static char *ptype[PARAM_END+1] = {
    "INT32"      , "INT16"      , "INT8"      , "FLOAT"      , "STRING"     ,
    "INT32ARRAY" , "INT16ARRAY" , "INT8ARRAY" , "FLOATARRAY" , "STRINGARRAY",
    "COLOR"      , "REGION"     , "DISPLAY"   , "IMAGE"      , "LAYER"      ,
    "CHANNEL"    , "DRAWABLE"   , "SELECTION" , "BOUNDARY"   , "PATH"       ,
#if GIMP_PARASITE
    "PARASITE"   ,
#endif
    "STATUS"     , "END"
  };
  int i;
  
  trace_printf ("(");
  
  if ((trace & TRACE_DESC) == TRACE_DESC)
    trace_printf ("\n\t");
  
  for (i = 0; i < nparams; i++)
    {
      if ((trace & TRACE_TYPE) == TRACE_TYPE)
        {
	  if ((unsigned int)params[i].type < PARAM_END+1)
	    trace_printf ("%s ", ptype[params[i].type]);
	  else
	    trace_printf ("T%d ", params[i].type);
        }
      
      if ((trace & TRACE_NAME) == TRACE_NAME)
	trace_printf ("%s=", params[i].name);
      
      switch (args[i].type)
	{
	  case PARAM_INT32:		trace_printf ("%d", args[i].data.d_int32); break;
	  case PARAM_INT16:		trace_printf ("%d", args[i].data.d_int16); break;
	  case PARAM_INT8:		trace_printf ("%d", (guint8) args[i].data.d_int8); break;
	  case PARAM_FLOAT:		trace_printf ("%f", args[i].data.d_float); break;
          case PARAM_STRING:		trace_printf ("\"%s\"", args[i].data.d_string ? args[i].data.d_string : "[null]"); break;
	  case PARAM_DISPLAY:		trace_printf ("%d", args[i].data.d_display); break;
	  case PARAM_IMAGE:		trace_printf ("%d", args[i].data.d_image); break;
	  case PARAM_LAYER:		trace_printf ("%d", args[i].data.d_layer); break;
	  case PARAM_CHANNEL:		trace_printf ("%d", args[i].data.d_channel); break;
	  case PARAM_DRAWABLE:		trace_printf ("%d", args[i].data.d_drawable); break;
	  case PARAM_SELECTION:		trace_printf ("%d", args[i].data.d_selection); break;
	  case PARAM_BOUNDARY:		trace_printf ("%d", args[i].data.d_boundary); break;
	  case PARAM_PATH:		trace_printf ("%d", args[i].data.d_path); break;
	  case PARAM_STATUS:		trace_printf ("%d", args[i].data.d_status); break;
	  case PARAM_INT32ARRAY:	dump_printarray (args, i, gint32, d_int32array, "%d"); break;
	  case PARAM_INT16ARRAY:	dump_printarray (args, i, gint16, d_int16array, "%d"); break;
	  case PARAM_INT8ARRAY:		dump_printarray (args, i, guint8, d_int8array , "%d"); break;
	  case PARAM_FLOATARRAY:	dump_printarray (args, i, gfloat, d_floatarray, "%f"); break;
	  case PARAM_STRINGARRAY:	dump_printarray (args, i, char* , d_stringarray, "'%s'"); break;
	  
	  case PARAM_COLOR:
	    trace_printf ("[%d,%d,%d]",
	                  args[i].data.d_color.red,
	                  args[i].data.d_color.green,
	                  args[i].data.d_color.blue);
	    break;

#if GIMP_PARASITE
	  case PARAM_PARASITE:
	    {
	      gint32 found = 0;

              if (args[i].data.d_parasite.name)
                {
                 trace_printf ("[%s, ", args[i].data.d_parasite.name);
                 if (args[i].data.d_parasite.flags & PARASITE_PERSISTENT)
                   {
                     trace_printf ("PARASITE_PERSISTENT");
                     found |= PARASITE_PERSISTENT;
                   }
                 
                 if (args[i].data.d_parasite.flags & ~found)
                   {
                     if (found)
                       trace_printf ("|");
                     trace_printf ("%d", args[i].data.d_parasite.flags & ~found);
                   }
                 
                 trace_printf (__(", %d bytes data]"), args[i].data.d_parasite.size);
               }
              else
                trace_printf (__("[undefined]"));
	    }
	    break;
#endif
	    
	  default:
	    trace_printf ("(?%d?)", args[i].type);
	}
      
      if ((trace & TRACE_DESC) == TRACE_DESC)
	trace_printf ("\t\"%s\"\n\t", params[i].description);
      else if (i < nparams - 1)
	trace_printf (", ");
      
    }
  
  trace_printf (")");
}

static int
convert_array2paramdef (AV *av, GParamDef **res)
{
  int count = 0;
  GParamDef *def = 0;
  
  if (av_len (av) >= 0)
    for(;;)
      {
	int idx;
	
	for (idx = 0; idx <= av_len (av); idx++)
	  {
	    SV *sv = *av_fetch (av, idx, 0);
	    SV *type = 0;
	    SV *name = 0;
	    SV *help = 0;

	    if (SvROK (sv) && SvTYPE (SvRV (sv)) == SVt_PVAV)
	      {
	        AV *av = (AV *)SvRV(sv);
	        SV **x;
	        
	        if ((x = av_fetch (av, 0, 0))) type = *x;
	        if ((x = av_fetch (av, 1, 0))) name = *x;
	        if ((x = av_fetch (av, 2, 0))) help = *x;
	      }
	    else if (SvIOK(sv))
	      type = sv;

	    if (type)
	      {
	        if (def)
	          {
	            if (is_array (SvIV (type)))
	              {
	                def->type = PARAM_INT32;
	                def->name = "array_size";
	                def->description = "the size of the following array";
	                def++;
	              }
	            
	            def->type = SvIV (type);
	            def->name = name ? SvPV_nolen (name) : 0;
	            def->description = help ? SvPV_nolen (help) : 0;
	            def++;
	          }
	        else
	          count += 1 + !!is_array (SvIV (type));
	      }
	    else
	      croak (__("malformed paramdef, expected [PARAM_TYPE,\"NAME\",\"DESCRIPTION\"] or PARAM_TYPE"));
	  }
	
	if (def)
	  break;
	
	*res = def = g_new (GParamDef, count);
      }
  else
    *res = 0;
  
  return count;
}

static SV *
newSV_paramdefs (GParamDef *p, int n)
{
   int i;
   AV *av = newAV ();

   av_extend (av, n-1);
   for (i=0; i<n; i++)
     {
       AV *a = newAV ();
       av_extend (a, 3-1);
       av_store (a, 0, newSViv (p->type));
       av_store (a, 1, newSVpv (p->name,0));
       av_store (a, 2, newSVpv (p->description,0));
       p++;

       av_store (av, i, newRV_noinc ((SV*)a));
     }

   return newRV_noinc ((SV*)av);
}

static HV *
param_stash (GParamType type)
{
  static HV *bless_hv[PARAM_END]; /* initialized to zero */
  static char *bless[PARAM_END] = {
	                    0		, 0		, 0		, 0		, 0		,
	                    0		, 0		, 0		, 0		, 0		,
	                    PKG_COLOR	, PKG_REGION	, PKG_DISPLAY	, PKG_IMAGE	, PKG_LAYER	,
	                    PKG_CHANNEL	, PKG_DRAWABLE	, PKG_SELECTION	, 0		, 0		,
#if GIMP_PARASITE
	                    PKG_PARASITE,
#endif
	                    0
	                   };
  
  if (bless [type] && !bless_hv [type])
    bless_hv [type] = gv_stashpv (bless [type], 1);
  
  return bless_hv [type];
}
  
/* automatically bless SV into PARAM_type.  */
/* for what it's worth, we cache the stashes.  */
static SV *
autobless (SV *sv, int type)
{
  HV *stash = param_stash (type);
  
  if (stash)
    sv = sv_bless (newRV_noinc (sv), stash);

  if (stash && !SvOBJECT(SvRV(sv)))
    croak ("jupp\n");
  
  return sv;
}

/* return gint32 from object, wether iv or rv.  */
static gint32
unbless (SV *sv, char *type, char *croak_str)
{
  if (sv_isobject (sv))
    if (type == PKG_ANY
	|| (type == PKG_ANYABLE && (sv_derived_from (sv, PKG_DRAWABLE)
	                            || sv_derived_from (sv, PKG_LAYER)
	                            || sv_derived_from (sv, PKG_CHANNEL)))
	|| sv_derived_from (sv, type))
      {
	if (SvTYPE (SvRV (sv)) == SVt_PVMG)
	  return SvIV (SvRV (sv));
	else
	  strcpy (croak_str, __("only blessed scalars accepted here"));
      }
    else
      sprintf (croak_str, __("argument type %s expected (not %s)"), type, HvNAME(SvSTASH(SvRV(sv))));
  else
    return SvIV (sv);
  
  return -1;
}

static gint32
unbless_croak (SV *sv, char *type)
{
   char croak_str[MAX_STRING];
   gint32 r;
   croak_str[0] = 0;

   r = unbless (sv, type, croak_str);

   if (croak_str [0])
      croak (croak_str);
   
   return r;
}

static void
canonicalize_colour (char *err, SV *sv, GParamColor *c)
{
  dSP;
  
  ENTER;
  SAVETMPS;
  
  PUSHMARK(SP);
  XPUSHs (sv);
  PUTBACK;
  
  if (perl_call_pv ("Gimp::canonicalize_colour", G_SCALAR) != 1)
    croak (__("FATAL: canonicalize_colour did not return a value!"));
  
  SPAGAIN;
  
  sv = POPs;
  if (SvROK(sv))
    {
      if (SvTYPE(SvRV(sv)) == SVt_PVAV)
	{
	  AV *av = (AV *)SvRV(sv);
	  if (av_len(av) == 2)
	    {
	      c->red   = SvIV(*av_fetch(av, 0, 0));
	      c->green = SvIV(*av_fetch(av, 1, 0));
	      c->blue  = SvIV(*av_fetch(av, 2, 0));
	    }
	  else
	    sprintf (err, __("a color must have three components (array elements)"));
	}
      else
	sprintf (err, __("illegal type for colour specification"));
    }
  else
    sprintf (err, __("unable to grok colour specification"));
  
  PUTBACK;
  FREETMPS;
  LEAVE;
}

/* check for common typoes.  */
static void check_for_typoe (char *croak_str, char *p)
{
  char b[80];

  g_snprintf (b, sizeof b, "%s_MODE", p);	if (perl_get_cv (b, 0)) goto gotit;
  g_snprintf (b, sizeof b, "%s_MASK", p);	if (perl_get_cv (b, 0)) goto gotit;
  g_snprintf (b, sizeof b, "SELECTION_%s", p);	if (perl_get_cv (b, 0)) goto gotit;
  g_snprintf (b, sizeof b, "%s_IMAGE", p);	if (perl_get_cv (b, 0)) goto gotit;

  strcpy (b, "1"); if (strEQ (p, "TRUE" )) goto gotit;
  strcpy (b, "0"); if (strEQ (p, "FALSE")) goto gotit;
  
  return;

gotit:
  sprintf (croak_str, __("Expected an INT32 but got '%s'. Maybe you meant '%s' instead and forgot to 'use strict'"), p, b);
}

/* check for 'enumeration types', i.e. integer constants. do not allow
   string constants here, and check for common typoes. */
static int check_int (char *croak_str, SV *sv)
{
  if (SvTYPE (sv) == SVt_PV && !SvIOKp(sv))
    {
      char *p = SvPV_nolen (sv);

      if (*p
          && *p != '0' && *p != '1' && *p != '2' && *p != '3' && *p != '4'
          && *p != '5' && *p != '6' && *p != '7' && *p != '8' && *p != '9'
          && *p != '-')
        {
          sprintf (croak_str, __("Expected an INT32 but got '%s'. Add '*1' if you really intend to pass in a string"), p);
          check_for_typoe (croak_str, p);
          return 0;
        }
    }
  return 1;
}

/* replacement newSVpv with only one argument.  */
#define neuSVpv(arg) ((arg) ? newSVpv((arg),0) : newSVsv (&PL_sv_undef))

/* replacement newSViv which casts to unsigned char.  */
#define newSVu8(arg) newSViv((unsigned char)(arg))

/* create sv's using newsv, from the array arg.  */
#define push_gimp_av(arg,datatype,newsv,as_ref) {		\
  int j;							\
  AV *av;							\
  if (as_ref)							\
    av = newAV ();						\
  else								\
    { av = 0; EXTEND (SP, arg[-1].data.d_int32); }		\
  for (j = 0; j < arg[-1].data.d_int32; j++)			\
    if (as_ref)							\
      av_push (av, newsv (arg->data.datatype[j]));		\
    else							\
      PUSHs (sv_2mortal (newsv (arg->data.datatype[j])));	\
  if (as_ref)							\
    PUSHs (sv_2mortal (newRV_noinc ((SV *)av)));		\
}

static void
push_gimp_sv (GParam *arg, int array_as_ref)
{
  dSP;
  SV *sv = 0;
  
  switch (arg->type)
    {
      case PARAM_INT32:		sv = newSViv(arg->data.d_int32	); break;
      case PARAM_INT16:		sv = newSViv(arg->data.d_int16	); break;
      case PARAM_INT8:		sv = newSVu8(arg->data.d_int8	); break;
      case PARAM_FLOAT:		sv = newSVnv(arg->data.d_float	); break;
      case PARAM_STRING:	sv = neuSVpv(arg->data.d_string ); break;
	
      case PARAM_DISPLAY:
      case PARAM_IMAGE:	
      case PARAM_LAYER:	
      case PARAM_CHANNEL:
      case PARAM_DRAWABLE:
      case PARAM_SELECTION:
      case PARAM_BOUNDARY:
      case PARAM_PATH:	
      case PARAM_STATUS:
         
        {
          int id;
 
          switch (arg->type) {
            case PARAM_DISPLAY:		id = arg->data.d_display; break;
            case PARAM_IMAGE:		id = arg->data.d_image; break;
            case PARAM_LAYER:		id = arg->data.d_layer; break;
            case PARAM_CHANNEL:		id = arg->data.d_channel; break;
            case PARAM_DRAWABLE:	id = arg->data.d_drawable; break;
            case PARAM_SELECTION:	id = arg->data.d_selection; break;
            case PARAM_BOUNDARY:	id = arg->data.d_boundary; break;
            case PARAM_PATH:		id = arg->data.d_path; break;
            case PARAM_STATUS:		id = arg->data.d_status; break;
            default:			abort ();
          }

          if (id == -1)
            PUSHs (newSVsv (&PL_sv_undef));
          else
            sv = newSViv (id);
        }
        break;

      case PARAM_COLOR:
	{
	  /* difficult */
	  AV *av = newAV ();
	  av_push (av, newSViv (arg->data.d_color.red));
	  av_push (av, newSViv (arg->data.d_color.green));
	  av_push (av, newSViv (arg->data.d_color.blue));
	  sv = (SV *)av; /* no newRV_inc, since we're getting autoblessed! */
	}
	break;

#if GIMP_PARASITE
      case PARAM_PARASITE:
        if (arg->data.d_parasite.name)
          {
            AV *av = newAV ();
            av_push (av, neuSVpv (arg->data.d_parasite.name));
            av_push (av, newSViv (arg->data.d_parasite.flags));
            av_push (av, newSVpv (arg->data.d_parasite.data, arg->data.d_parasite.size));
            sv = (SV *)av;
          }

	break;
#endif
      
      /* did I say difficult before????  */
      case PARAM_INT32ARRAY:	push_gimp_av (arg, d_int32array , newSViv, array_as_ref); break;
      case PARAM_INT16ARRAY:	push_gimp_av (arg, d_int16array , newSViv, array_as_ref); break;
      case PARAM_INT8ARRAY:	push_gimp_av (arg, d_int8array  , newSVu8, array_as_ref); break;
      case PARAM_FLOATARRAY:	push_gimp_av (arg, d_floatarray , newSVnv, array_as_ref); break;
      case PARAM_STRINGARRAY:	push_gimp_av (arg, d_stringarray, neuSVpv, array_as_ref); break;
	
      default:
	croak (__("dunno how to return param type %d"), arg->type);
    }
  
  if (sv)
    PUSHs (sv_2mortal (autobless (sv, arg->type)));
  
  PUTBACK;
}

#define SvPv(sv) (SvOK(sv) ? SvPV_nolen(sv) : 0)
#define Sv32(sv) unbless ((sv), PKG_ANY, croak_str)

#define av2gimp(arg,sv,datatype,type,svxv) { \
  if (SvROK (sv) && SvTYPE(SvRV(sv)) == SVt_PVAV) \
    { \
      int i; \
      AV *av = (AV *)SvRV(sv); \
      arg[-1].data.d_int32 = av_len (av) + 1; \
      arg->data.datatype = g_new (type, av_len (av) + 1); \
      for (i = 0; i <= av_len (av); i++) \
	arg->data.datatype[i] = svxv (*av_fetch (av, i, 0)); \
    } \
  else \
    { \
      sprintf (croak_str, __("perl-arrayref required as datatype for a gimp-array")); \
      arg->data.datatype = 0; \
    } \
}

#define sv2gimp_extract_noref(fun,str) \
	fun(sv); \
	if (SvROK(sv)) \
	  sprintf (croak_str, __("Unable to convert a reference to type '%s'"), str); \
      	break;
/*
 * convert a perl scalar into a GParam, return true if
 * the argument has been consumed.
 */
static int
convert_sv2gimp (char *croak_str, GParam *arg, SV *sv)
{
  switch (arg->type)
    {
      case PARAM_INT32:		check_int (croak_str, sv);
         			arg->data.d_int32	= sv2gimp_extract_noref (SvIV, "INT32");
      case PARAM_INT16:		arg->data.d_int16	= sv2gimp_extract_noref (SvIV, "INT16");
      case PARAM_INT8:		arg->data.d_int8	= sv2gimp_extract_noref (SvIV, "INT8");
      case PARAM_FLOAT:		arg->data.d_float	= sv2gimp_extract_noref (SvNV, "FLOAT");
      case PARAM_STRING:	arg->data.d_string	= sv2gimp_extract_noref (SvPv, "STRING");

      case PARAM_DISPLAY:
      case PARAM_IMAGE:	
      case PARAM_LAYER:	
      case PARAM_CHANNEL:
      case PARAM_DRAWABLE:
      case PARAM_SELECTION:
      case PARAM_BOUNDARY:
      case PARAM_PATH:	
      case PARAM_STATUS:

        if (SvOK(sv))
          switch (arg->type) {
            case PARAM_DISPLAY:		arg->data.d_display	= unbless(sv, PKG_DISPLAY  , croak_str); break;
            case PARAM_LAYER:		arg->data.d_layer	= unbless(sv, PKG_ANYABLE  , croak_str); break;
            case PARAM_CHANNEL:		arg->data.d_channel	= unbless(sv, PKG_ANYABLE  , croak_str); break;
            case PARAM_DRAWABLE:	arg->data.d_drawable	= unbless(sv, PKG_ANYABLE  , croak_str); break;
            case PARAM_SELECTION:	arg->data.d_selection	= unbless(sv, PKG_SELECTION, croak_str); break;
            case PARAM_BOUNDARY:	arg->data.d_boundary	= sv2gimp_extract_noref (SvIV, "BOUNDARY"); break;
            case PARAM_PATH:		arg->data.d_path	= sv2gimp_extract_noref (SvIV, "PATH"); break;
            case PARAM_STATUS:		arg->data.d_status	= sv2gimp_extract_noref (SvIV, "STATUS"); break;
            case PARAM_IMAGE:
              {
                if (sv_derived_from (sv, PKG_DRAWABLE))
                  arg->data.d_image = gimp_drawable_image_id    (unbless(sv, PKG_DRAWABLE, croak_str));
                else if (sv_derived_from (sv, PKG_LAYER   ))
                  arg->data.d_image = gimp_layer_get_image_id   (unbless(sv, PKG_LAYER   , croak_str));
                else if (sv_derived_from (sv, PKG_CHANNEL ))
                  arg->data.d_image = gimp_channel_get_image_id (unbless(sv, PKG_CHANNEL , croak_str));
                else if (sv_derived_from (sv, PKG_IMAGE) || !SvROK (sv))
                  {
                                        arg->data.d_image      = unbless(sv, PKG_IMAGE   , croak_str); break;
                  }
                else
                  strcpy (croak_str, __("argument incompatible with type IMAGE"));

                return 0;
              }

            default:
              abort ();
          }
        else
          switch (arg->type) {
            case PARAM_DISPLAY:		arg->data.d_display	= -1; break;
            case PARAM_LAYER:		arg->data.d_layer	= -1; break;
            case PARAM_CHANNEL:		arg->data.d_channel	= -1; break;
            case PARAM_DRAWABLE:	arg->data.d_drawable	= -1; break;
            case PARAM_SELECTION:	arg->data.d_selection	= -1; break;
            case PARAM_BOUNDARY:	arg->data.d_boundary	= -1; break;
            case PARAM_PATH:		arg->data.d_path	= -1; break;
            case PARAM_STATUS:		arg->data.d_status	= -1; break;
            case PARAM_IMAGE:		arg->data.d_image	= -1; return 0; break;
            default:			abort ();
          }
      	
      	break;
      	
      case PARAM_COLOR:
	canonicalize_colour (croak_str, sv, &arg->data.d_color);
	break;

#if GIMP_PARASITE
      case PARAM_PARASITE:
	if (SvROK(sv))
	  {
	    if (SvTYPE(SvRV(sv)) == SVt_PVAV)
	      {
	        AV *av = (AV *)SvRV(sv);
	        if (av_len(av) == 2)
	          {
                    STRLEN size;

	            arg->data.d_parasite.name  = SvPv(*av_fetch(av, 0, 0));
	            arg->data.d_parasite.flags = SvIV(*av_fetch(av, 1, 0));
	            arg->data.d_parasite.data  = SvPV(*av_fetch(av, 2, 0), size);

                    arg->data.d_parasite.size = size;
	          }
	        else
	          sprintf (croak_str, __("illegal parasite specification, expected three array members"));
	      }
	    else
	      sprintf (croak_str, __("illegal parasite specification, arrayref expected"));
	  }
	else
	  sprintf (croak_str, __("illegal parasite specification, reference expected"));
	
	break;
#endif
      
      case PARAM_INT32ARRAY:	av2gimp (arg, sv, d_int32array , gint32 , Sv32); break;
      case PARAM_INT16ARRAY:	av2gimp (arg, sv, d_int16array , gint16 , SvIV); break;
      case PARAM_INT8ARRAY:	av2gimp (arg, sv, d_int8array  , gint8  , SvIV); break;
      case PARAM_FLOATARRAY:	av2gimp (arg, sv, d_floatarray , gdouble, SvNV); break;
      case PARAM_STRINGARRAY:	av2gimp (arg, sv, d_stringarray, gchar *, SvPv); break;
	
      default:
	sprintf (croak_str, __("dunno how to pass arg type %d"), arg->type);
    }
  
  return 1;
}

/* do not free actual string or parasite data */
static void
destroy_params (GParam *arg, int count)
{
  int i;
  
  for (i = 0; i < count; i++)
    switch (arg[i].type)
      {
	case PARAM_INT32ARRAY:	g_free (arg[i].data.d_int32array); break;
	case PARAM_INT16ARRAY:	g_free (arg[i].data.d_int16array); break;
	case PARAM_INT8ARRAY:	g_free (arg[i].data.d_int8array); break;
	case PARAM_FLOATARRAY:	g_free (arg[i].data.d_floatarray); break;
	case PARAM_STRINGARRAY:	g_free (arg[i].data.d_stringarray); break;
	  
	default: ;
      }
  
  g_free (arg);
}

#ifdef GIMP_HAVE_DESTROY_PARAMDEFS
#define destroy_paramdefs gimp_destroy_paramdefs
#else
static void
destroy_paramdefs (GParamDef *arg, int count)
{
  int i;
  
  for (i = 0; i < count; i++)
    {
      g_free (arg[i].name);
      g_free (arg[i].description);
    }
  
  g_free (arg);
}
#endif

#ifdef GIMP_HAVE_PROCEDURAL_DB_GET_DATA_SIZE
#define get_data_size gimp_get_data_size
#else
static guint32
get_data_size (gchar *id)
{
  GParam *return_vals;
  int nreturn_vals;
  int length;

  return_vals = gimp_run_procedure ("gimp_procedural_db_get_data",
                                    &nreturn_vals,
                                    PARAM_STRING, id,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    length = return_vals[1].data.d_int32;
  else
    length = 0;

  gimp_destroy_params (return_vals, nreturn_vals);

  return length;
}
#endif

static void simple_perl_call (char *function, char *arg1)
{
   dSP;

   ENTER;
   SAVETMPS;

   PUSHMARK (SP);
   XPUSHs (sv_2mortal (newSVpv (arg1, 0)));

   PUTBACK;
   perl_call_pv (function, G_VOID);
   SPAGAIN;

   FREETMPS;
   LEAVE;
}

#define gimp_die_msg(msg) simple_perl_call ("Gimp::die_msg" , (msg))
#define try_call(cb)      simple_perl_call ("Gimp::callback", (cb) )

static void pii_init (void) { try_call ("-init" ); }
static void pii_query(void) { try_call ("-query"); }
static void pii_quit (void) { try_call ("-quit" ); }

static void pii_run(char *name, int nparams, GParam *param, int *xnreturn_vals, GParam **xreturn_vals)
{
  static GParam *return_vals;
  static int nreturn_vals;
  
  dSP;

  int i, count;
  char *err_msg = 0;
  
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int _nparams;
  GParamDef *params;
  GParamDef *return_defs;

   if (return_vals) /* the libgimp is soooooooo braindamaged. */
     {
       destroy_params (return_vals, nreturn_vals);
       return_vals = 0;
     }

  if (gimp_query_procedure (name, &proc_blurb, &proc_help, &proc_author,
	                   &proc_copyright, &proc_date, &proc_type, &_nparams, &nreturn_vals,
	                   &params, &return_defs) == TRUE)
    {
      g_free (proc_blurb);
      g_free (proc_help);
      g_free (proc_author);
      g_free (proc_copyright);
      g_free (proc_date);
      destroy_paramdefs (params, _nparams);
      
      PUSHMARK(SP);

      EXTEND (SP, 3);
      PUSHs (sv_2mortal (newSVpv ("-run", 4)));
      PUSHs (sv_2mortal (newSVpv (name, 0)));
      
      if (nparams)
	{
	  EXTEND (SP, perl_param_count (param, nparams));
	  PUTBACK;
	  for (i = 0; i < nparams; i++)
	    {
	      if (i < nparams-1 && is_array (param[i+1].type))
	        i++;
	      
	      push_gimp_sv (param+i, nparams > 2);
	    }
	  
	  SPAGAIN;
	}
      else
	PUTBACK;
      
      count = perl_call_pv ("Gimp::callback", G_EVAL
	                    | (nreturn_vals == 0 ? G_VOID : nreturn_vals == 1 ? G_SCALAR : G_ARRAY));
      SPAGAIN;
      
      if (SvTRUE (ERRSV))
	{
          if (strEQ ("IGNORE THIS MESSAGE\n", SvPV_nolen (ERRSV)))
            {
              nreturn_vals = 0;
              return_vals = g_new (GParam, 1);
              return_vals->type = PARAM_STATUS;
              return_vals->data.d_status = STATUS_SUCCESS;
              *xnreturn_vals = nreturn_vals+1;
              *xreturn_vals = return_vals;
            }
          else
            err_msg = g_strdup (SvPV_nolen (ERRSV));
	}
      else
	{
	  int i;
	  char errmsg [MAX_STRING];
	  errmsg [0] = 0;
	  
	  return_vals = (GParam *) g_new0 (GParam, nreturn_vals+1);
	  return_vals->type = PARAM_STATUS;
	  return_vals->data.d_status = STATUS_SUCCESS;
	  *xnreturn_vals = nreturn_vals+1;
	  *xreturn_vals = return_vals++;

	  for (i = nreturn_vals; i-- && count; )
	     {
	       return_vals[i].type = return_defs[i].type;
	       if ((i >= nreturn_vals-1 || !is_array (return_defs[i+1].type))
	           && convert_sv2gimp (errmsg, &return_vals[i], TOPs))
	         {
	           --count;
	           (void) POPs;
	         }
	       
	       if (errmsg [0])
	         {
	           err_msg = g_strdup (errmsg);
	           break;
	         }
	     }
	  
	  if (count && !err_msg)
	    err_msg = g_strdup_printf (__("plug-in returned %d more values than expected"), count);
	}
      
      destroy_paramdefs (return_defs, nreturn_vals);
      
      PUTBACK;
    }
  else
    err_msg = g_strdup_printf (__("being called as '%s', but '%s' not registered in the pdb"), name, name);
  
  if (err_msg)
    {
      gimp_die_msg (err_msg);
      g_free (err_msg);
      
      if (return_vals)
	destroy_params (*xreturn_vals, nreturn_vals+1);
      
      nreturn_vals = 0;
      return_vals = g_new (GParam, 1);
      return_vals->type = PARAM_STATUS;
      return_vals->data.d_status = STATUS_EXECUTION_ERROR;
      *xnreturn_vals = nreturn_vals+1;
      *xreturn_vals = return_vals;
    }
}

GPlugInInfo PLUG_IN_INFO = { pii_init, pii_quit, pii_query, pii_run };

MODULE = Gimp::Lib	PACKAGE = Gimp::Lib

PROTOTYPES: ENABLE

#
# usage:
# set_trace (int new_trace_mask);
# set_trace (\$variable_to_trace_into);
# set_trace (*STDOUT);
#
I32
set_trace (var)
	CODE:
	{
		SV *sv = ST (0);
		
		RETVAL = trace;
		
		if (SvROK (sv) || SvTYPE (sv) == SVt_PVGV)
		  {
	            if (trace_var)
		      SvREFCNT_dec (trace_var), trace_var = 0;
		    
		    if (SvTYPE (sv) == SVt_PVGV) /* pray it's a filehandle!  */
		      trace_file = IoOFP (GvIO (sv));
		    else
		      {
		        trace_file = 0;
		        sv = SvRV (sv);
		        SvREFCNT_inc (sv);
		        (void) SvUPGRADE (sv, SVt_PV);
		        trace_var = sv;
		      }
		  }
		else
		  trace = SvIV (ST (0));
	}
	OUTPUT:
	RETVAL

SV *
_autobless (sv,type)
	SV *	sv
	gint32	type
	CODE:
	RETVAL = autobless (newSVsv (sv), type);
	OUTPUT:
	RETVAL

PROTOTYPES: DISABLE

int
gimp_main(...)
	PREINIT:
	CODE:
		SV *sv;
		
		if ((sv = perl_get_sv ("Gimp::help", FALSE)) && SvTRUE (sv))
		  RETVAL = 0;
		else
		  {
		    char *argv [10];
		    int argc = 0;
		    
		    if (items == 0)
		      {
		        AV *av = perl_get_av ("ARGV", FALSE);
		        
		        argv [argc++] = SvPV_nolen (perl_get_sv ("0", FALSE));
		        if (av && av_len (av) < 10-1)
		          {
		            while (argc-1 <= av_len (av))
		              argv [argc] = SvPV_nolen (*av_fetch (av, argc-1, 0)),
		              argc++;
		          }
		        else
		          croak ("internal error (please report): too many arguments to main");
		      }
		    else
		      croak (__("arguments to main not yet supported!"));
		    
	            gimp_is_initialized = 1;
		    RETVAL = gimp_main (argc, argv);
	            gimp_is_initialized = 0;
                    /*exit (0);*/ /*D*//* shit, some memory problem here, so just exit */
		  }
	OUTPUT:
	RETVAL

PROTOTYPES: ENABLE

int
initialized()
	CODE:
	RETVAL = gimp_is_initialized;
	OUTPUT:
	RETVAL

int
gimp_major_version()
   	CODE:
	RETVAL = gimp_major_version;
	OUTPUT:
	RETVAL

int
gimp_minor_version()
   	CODE:
	RETVAL = gimp_minor_version;
	OUTPUT:
	RETVAL

int
gimp_micro_version()
   	CODE:
	RETVAL = gimp_micro_version;
	OUTPUT:
	RETVAL

# checks wether a gimp procedure exists
int
_gimp_procedure_available(proc_name)
	char * proc_name
	CODE:
	{
		char *proc_blurb;	
		char *proc_help;
		char *proc_author;
		char *proc_copyright;
		char *proc_date;
		int proc_type;
		int nparams;
		int nreturn_vals;
		GParamDef *params;
		GParamDef *return_vals;

                if (!gimp_is_initialized)
                  croak ("_gimp_procedure_available called without an active connection");
		
		if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
		    &proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
		    &params, &return_vals) == TRUE)
		  {
		    g_free (proc_blurb);
		    g_free (proc_help);
		    g_free (proc_author);
		    g_free (proc_copyright);
		    g_free (proc_date);
		    destroy_paramdefs (params, nparams);
		    destroy_paramdefs (return_vals, nreturn_vals);
		    RETVAL = TRUE;
		  }
		else
		  RETVAL = FALSE;
		    
	}
	OUTPUT:
	RETVAL

# checks wether a gimp procedure exists
void
gimp_query_procedure(proc_name)
	char * proc_name
	PPCODE:
	{
		char *proc_blurb;	
		char *proc_help;
		char *proc_author;
		char *proc_copyright;
		char *proc_date;
		int proc_type;
		int nparams;
		int nreturn_vals;
		GParamDef *params;
		GParamDef *return_vals;
		
                if (!gimp_is_initialized)
                  croak ("gimp_query_procedure called without an active connection");

		if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
		    &proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
		    &params, &return_vals) == TRUE)
		  {
                    EXTEND (SP,8);
                    PUSHs (newSVpv (proc_blurb,0));	g_free (proc_blurb);
		    PUSHs (newSVpv (proc_help,0));	g_free (proc_help);
		    PUSHs (newSVpv (proc_author,0));	g_free (proc_author);
		    PUSHs (newSVpv (proc_copyright,0));	g_free (proc_copyright);
		    PUSHs (newSVpv (proc_date,0));	g_free (proc_date);
		    PUSHs (newSViv (proc_type));
                    PUSHs (newSV_paramdefs (params, nparams));		destroy_paramdefs (params, nparams);
		    PUSHs (newSV_paramdefs (return_vals, nreturn_vals));destroy_paramdefs (return_vals, nreturn_vals);
		  }
	}

void
gimp_call_procedure (proc_name, ...)
	char *	proc_name
	PPCODE:
	{
		char croak_str[MAX_STRING] = "";
		char *proc_blurb;	
		char *proc_help;
		char *proc_author;
		char *proc_copyright;
		char *proc_date;
		int proc_type;
		int nparams;
		int nreturn_vals;
		GParam *args = 0;
		GParam *values = 0;
		int nvalues;
		GParamDef *params;
		GParamDef *return_vals;
                int i=0, j=0; /* work around bogus warning.  */

                if (!gimp_is_initialized)
                  croak ("gimp_call_procedure(%s,...) called without an active connection", proc_name);

		if (trace)
		  trace_init ();
		
		if (trace & TRACE_CALL)
		  trace_printf ("%s", proc_name);
		
		if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
		    &proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
		    &params, &return_vals) == TRUE)
		  {
		    int runmode = nparams
		                  && params[0].type == PARAM_INT32
		                  && !strcmp (params[0].name, "run_mode");
		    
		    g_free (proc_blurb);
		    g_free (proc_help);
		    g_free (proc_author);
		    g_free (proc_copyright);
		    g_free (proc_date);
		    
                    if (nparams)
                      args = (GParam *) g_new0 (GParam, nparams);

                    for (i = 0, j = 1; i < nparams && j < items; i++)
                      {
                        args[i].type = params[i].type;
                        if (i == 0 && runmode)
                           {
                             if (sv_isa (ST(j), "Gimp::run_mode"))
                               {
                                 args->data.d_int32 = SvIV(SvRV(ST(j)));
                                 j++;
                               }
                             else
                               args->data.d_int32 = RUN_NONINTERACTIVE;
                           }
                        else if ((!SvROK(ST(j)) || i >= nparams-1 || !is_array (params[i+1].type))
                                 && convert_sv2gimp (croak_str, &args[i], ST(j)))
                          j++;
                    
                        if (croak_str [0])
                          {
                            if (trace & TRACE_CALL)
                              {
                                dump_params (i, args, params);
                                trace_printf (__(" = [argument error]\n"));
                              }
                        
                            goto error;
                          }
                      }
                  
                    if (trace & TRACE_CALL)
                      {
                        dump_params (i, args, params);
                        trace_printf (" = ");
                      }
                    
                    if (i < nparams || j < items)
                      {
                        if (trace & TRACE_CALL)
                          trace_printf (__("[unfinished]\n"));
                        
                        sprintf (croak_str, __("%s arguments for function '%s'"),
                                 i < nparams ? __("not enough") : __("too many"), proc_name);
                        
                        if (nparams)
                          destroy_params (args, nparams);
                      }
                    else
                      {
                        values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
                        
                        if (nparams)
                          destroy_params (args, nparams);
                    
                        if (trace & TRACE_CALL)
                          {
                            dump_params (nvalues-1, values+1, return_vals);
                            trace_printf ("\n");
                          }
                        
                        if (values && values[0].type == PARAM_STATUS)
                          {
                            if (values[0].data.d_status == STATUS_EXECUTION_ERROR)
                              sprintf (croak_str, __("%s: procedural database execution failed"), proc_name);
                            else if (values[0].data.d_status == STATUS_CALLING_ERROR)
                              sprintf (croak_str, __("%s: procedural database execution failed on invalid input arguments"), proc_name);
                            else if (values[0].data.d_status == STATUS_SUCCESS)
                              {
                                EXTEND(SP, perl_paramdef_count (return_vals, nvalues-1));
                                PUTBACK;
                                for (i = 0; i < nvalues-1; i++)
                                  {
                                    if (i < nvalues-2 && is_array (values[i+2].type))
                                      i++;
                                    
                                    push_gimp_sv (values+i+1, nvalues > 2+1);
                                  }
                                
                                SPAGAIN;
                              }
                            else
                              sprintf (croak_str, "unsupported status code: %d, fatal error\n", values[0].data.d_status);
                          }
                        else
                          sprintf (croak_str, "gimp didn't return an execution status, fatal error");
                        
                      }
                    
                    error:

                    if (values)
                      gimp_destroy_params (values, nreturn_vals);
                    
                    destroy_paramdefs (params, nparams);
                    destroy_paramdefs (return_vals, nreturn_vals);
                    
		
                    if (croak_str[0])
                      croak (croak_str);
                  }
                else
                  croak (__("gimp procedure '%s' not found"), proc_name);
	}

void
gimp_install_procedure(name, blurb, help, author, copyright, date, menu_path, image_types, type, params, return_vals)
	char *	name
	char *	blurb
	char *	help
	char *	author
	char *	copyright
	char *	date
	SV *	menu_path
	SV *	image_types
	int	type
	SV *	params
	SV *	return_vals
	ALIAS:
		gimp_install_temp_proc = 1
	CODE:
        if (SvROK(params) && SvTYPE(SvRV(params)) == SVt_PVAV
            && SvROK(return_vals) && SvTYPE(SvRV(return_vals)) == SVt_PVAV)
          {
            GParamDef *apd; int nparams;
            GParamDef *rpd; int nreturn_vals;
            
            nparams      = convert_array2paramdef ((AV *)SvRV(params)     , &apd);
            nreturn_vals = convert_array2paramdef ((AV *)SvRV(return_vals), &rpd);
            
            if (ix)
              gimp_install_temp_proc(name,blurb,help,author,copyright,date,SvPv(menu_path),SvPv(image_types),
                                     type,nparams,nreturn_vals,apd,rpd,pii_run);
            else
              {
	        gimp_plugin_domain_register ("gimp-perl", datadir "/locale");

                gimp_install_procedure(name,blurb,help,author,copyright,date,SvPv(menu_path),SvPv(image_types),
                                       type,nparams,nreturn_vals,apd,rpd);
              }
            
            g_free (rpd);
            g_free (apd);
          }
        else
          croak (__("params and return_vals must be array refs (even if empty)!"));

void
gimp_uninstall_temp_proc(name)
	char *	name

void
gimp_lib_quit()
	CODE:
	gimp_quit ();

void
gimp_set_data(id, data)
	SV *	id
	SV *	data;
	CODE:
	{
		STRLEN dlen;
		void *dta;
		
		dta = SvPV (data, dlen);

		gimp_set_data (SvPV_nolen (id), dta, dlen);
	}

void
gimp_get_data(id)
	SV *	id;
	PPCODE:
	{
		SV *data;
		STRLEN dlen;
		
		dlen = get_data_size (SvPV_nolen (id));
		/* I count on dlen being zero if "id" doesn't exist.  */
		data = newSVpv ("", 0);
		gimp_get_data (SvPV_nolen (id), SvGROW (data, dlen+1));
		SvCUR_set (data, dlen);
		*((char *)SvPV_nolen (data) + dlen) = 0;
	        XPUSHs (sv_2mortal (data));
	}

gdouble
gimp_gamma()

gint
gimp_install_cmap()

gint
gimp_use_xshm()

void
gimp_color_cube()
	PPCODE:
	{
	 	guchar *cc = gimp_color_cube ();

		EXTEND (SP, 4);

		PUSHs (sv_2mortal (newSViv (cc [0])));
		PUSHs (sv_2mortal (newSViv (cc [1])));
		PUSHs (sv_2mortal (newSViv (cc [2])));
		PUSHs (sv_2mortal (newSViv (cc [3])));
	}
	

char *
gimp_gtkrc()

#ifdef GIMP11
char *
gimp_directory()

char *
gimp_data_directory()

SV *
gimp_personal_rc_file(basename)
	char *	basename
	CODE:
        basename = gimp_personal_rc_file (basename);
        RETVAL = sv_2mortal (newSVpv (basename, 0));
        g_free (basename);
        OUTPUT:
        RETVAL

#endif

guint
gimp_tile_width()

guint
gimp_tile_height()

void
gimp_tile_cache_size(kilobytes)
	gulong	kilobytes

void
gimp_tile_cache_ntiles(ntiles)
	gulong	ntiles

SV *
gimp_drawable_get(drawable_ID)
	DRAWABLE	drawable_ID
	CODE:
        RETVAL = new_gdrawable (drawable_ID);
	OUTPUT:
	RETVAL

void
gimp_drawable_flush(drawable)
	GDrawable *	drawable

SV *
gimp_pixel_rgn_init(gdrawable, x, y, width, height, dirty, shadow)
	SV *	gdrawable
	int	x
	int	y
	int	width
	int	height
	int	dirty
	int	shadow
	CODE:
        RETVAL = new_gpixelrgn (force_gdrawable (gdrawable),x,y,width,height,dirty,shadow);
	OUTPUT:
	RETVAL

void
gimp_pixel_rgn_resize(pr, x, y, width, height)
	GPixelRgn *	pr
	int	x
	int	y
	int	width
	int	height
	CODE:
	gimp_pixel_rgn_resize (pr, x, y, width, height);

GPixelRgnIterator
gimp_pixel_rgns_register(...)
	CODE:
        if (items == 1)
	  RETVAL = gimp_pixel_rgns_register (1, old_pixelrgn (ST (0)));
        else if (items == 2)
	  RETVAL = gimp_pixel_rgns_register (2, old_pixelrgn (ST (0)), old_pixelrgn (ST (1)));
        else if (items == 3)
	  RETVAL = gimp_pixel_rgns_register (3, old_pixelrgn (ST (0)), old_pixelrgn (ST (1)), old_pixelrgn (ST (2)));
        else
          croak (__("gimp_pixel_rgns_register supports only 1, 2 or 3 arguments, upgrade to gimp-1.1 and report this error"));
        OUTPUT:
        RETVAL

SV *
gimp_pixel_rgns_process(pri_ptr)
	GPixelRgnIterator	pri_ptr
        CODE:
        RETVAL = boolSV (gimp_pixel_rgns_process (pri_ptr));
        OUTPUT:
        RETVAL

# struct accessor functions

guint
gimp_gdrawable_width(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->width;
	OUTPUT:
	RETVAL

guint
gimp_gdrawable_height(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->height;
	OUTPUT:
	RETVAL

guint
gimp_gdrawable_ntile_rows(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->ntile_rows;
	OUTPUT:
	RETVAL

guint
gimp_gdrawable_ntile_cols(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->ntile_cols;
	OUTPUT:
	RETVAL

guint
gimp_gdrawable_bpp(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->bpp;
	OUTPUT:
	RETVAL

gint32
gimp_gdrawable_id(gdrawable)
	GDrawable *gdrawable
	CODE:
        RETVAL = gdrawable->id;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_x(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->x;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_y(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->y;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_w(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->w;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_h(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->h;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_rowstride(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->rowstride;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_bpp(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->bpp;
	OUTPUT:
	RETVAL

guint
gimp_pixel_rgn_shadow(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->shadow;
	OUTPUT:
	RETVAL

gint32
gimp_pixel_rgn_drawable(pr)
	GPixelRgn *pr
	CODE:
        RETVAL = pr->drawable->id;
	OUTPUT:
	RETVAL

guint
gimp_tile_ewidth(tile)
	GTile *tile
	CODE:
        RETVAL = tile->ewidth;
	OUTPUT:
	RETVAL

guint
gimp_tile_eheight(tile)
	GTile *tile
	CODE:
        RETVAL = tile->eheight;
	OUTPUT:
	RETVAL

guint
gimp_tile_bpp(tile)
	GTile *tile
	CODE:
        RETVAL = tile->bpp;
	OUTPUT:
	RETVAL

guint
gimp_tile_shadow(tile)
	GTile *tile
	CODE:
        RETVAL = tile->shadow;
	OUTPUT:
	RETVAL

guint
gimp_tile_dirty(tile)
	GTile *tile
	CODE:
        RETVAL = tile->dirty;
	OUTPUT:
	RETVAL

DRAWABLE
gimp_tile_drawable(tile)
	GTile *tile
	CODE:
        RETVAL = tile->drawable->id;
	OUTPUT:
	RETVAL

SV *
gimp_pixel_rgn_get_row2(pr, x, y, width)
	GPixelRgn *	pr
	int	x
	int	y
	int	width
	CODE:
        RETVAL = newSVn (width * pr->bpp);
	gimp_pixel_rgn_get_row (pr, (guchar *)SvPV_nolen(RETVAL), x, y, width);
	OUTPUT:
	RETVAL

SV *
gimp_pixel_rgn_get_rect2(pr, x, y, width, height)
	GPixelRgn *	pr
	int	x
	int	y
	int	width
	int	height
	CODE:
        RETVAL = newSVn (width * height * pr->bpp);
	gimp_pixel_rgn_get_rect (pr, (guchar *)SvPV_nolen(RETVAL), x, y, width, height);
	OUTPUT:
	RETVAL

void
gimp_pixel_rgn_set_rect2(pr, data, x, y, w=pr->w)
	GPixelRgn *	pr
	SV *	data
	int	x
	int	y
        int	w
	CODE:
{
        STRLEN dlen; guchar *dta = (guchar *)SvPV (data, dlen);
	gimp_pixel_rgn_set_rect (pr, dta, x, y, w, dlen / (w*pr->bpp));
}

#if HAVE_PDL

SV *
gimp_drawable_get_tile(gdrawable, shadow, row, col)
	SV *	gdrawable
	gint	shadow
	gint	row
	gint	col
	CODE:
        need_pdl ();
	RETVAL = new_tile (gimp_drawable_get_tile (old_gdrawable (gdrawable), shadow, row, col), gdrawable);
	OUTPUT:
	RETVAL

SV *
gimp_drawable_get_tile2(gdrawable, shadow, x, y)
	SV *	gdrawable
	gint	shadow
	gint	x
	gint	y
	CODE:
        need_pdl ();
	RETVAL = new_tile (gimp_drawable_get_tile2 (old_gdrawable (gdrawable), shadow, x, y), gdrawable);
	OUTPUT:
	RETVAL

pdl *
gimp_pixel_rgn_get_pixel(pr, x, y)
	GPixelRgn_PDL *	pr
	int	x
	int	y
	CODE:
        RETVAL = new_pdl (0, 0, pr->bpp);
	gimp_pixel_rgn_get_pixel (pr, RETVAL->data, x, y);
	OUTPUT:
	RETVAL

pdl *
gimp_pixel_rgn_get_row(pr, x, y, width)
	GPixelRgn_PDL *	pr
	int	x
	int	y
	int	width
	CODE:
        RETVAL = new_pdl (0, width, pr->bpp);
	gimp_pixel_rgn_get_row (pr, RETVAL->data, x, y, width);
	OUTPUT:
	RETVAL

pdl *
gimp_pixel_rgn_get_col(pr, x, y, height)
	GPixelRgn_PDL *	pr
	int	x
	int	y
	int	height
	CODE:
        RETVAL = new_pdl (height, 0, pr->bpp);
	gimp_pixel_rgn_get_col (pr, RETVAL->data, x, y, height);
	OUTPUT:
	RETVAL

pdl *
gimp_pixel_rgn_get_rect(pr, x, y, width, height)
	GPixelRgn_PDL *	pr
	int	x
	int	y
	int	width
	int	height
	CODE:
        RETVAL = new_pdl (height, width, pr->bpp);
	gimp_pixel_rgn_get_rect (pr, RETVAL->data, x, y, width, height);
	OUTPUT:
	RETVAL

void
gimp_pixel_rgn_set_pixel(pr, pdl, x, y)
	GPixelRgn_PDL *	pr
	pdl *	pdl
	int	x
	int	y
	CODE:
        old_pdl (&pdl, 0, pr->bpp);
	gimp_pixel_rgn_set_pixel (pr, pdl->data, x, y);

void
gimp_pixel_rgn_set_row(pr, pdl, x, y)
	GPixelRgn_PDL *	pr
	pdl *	pdl
	int	x
	int	y
	CODE:
        old_pdl (&pdl, 1, pr->bpp);
	gimp_pixel_rgn_set_row (pr, pdl->data, x, y, pdl->dims[pdl->ndims-1]);

void
gimp_pixel_rgn_set_col(pr, pdl, x, y)
	GPixelRgn_PDL *	pr
	pdl *	pdl
	int	x
	int	y
	CODE:
        old_pdl (&pdl, 1, pr->bpp);
	gimp_pixel_rgn_set_col (pr, pdl->data, x, y, pdl->dims[pdl->ndims-1]);

void
gimp_pixel_rgn_set_rect(pr, pdl, x, y)
	GPixelRgn_PDL *	pr
	pdl *	pdl
	int	x
	int	y
	CODE:
        old_pdl (&pdl, 2, pr->bpp);
	gimp_pixel_rgn_set_rect (pr, pdl->data, x, y, pdl->dims[pdl->ndims-2], pdl->dims[pdl->ndims-1]);

pdl *
gimp_pixel_rgn_data(pr,newdata=0)
	GPixelRgn_PDL *	pr
        pdl * newdata
	CODE:
        if (newdata)
	  {
            guchar *src;
            guchar *dst;
            int y, stride;

            old_pdl (&newdata, 2, pr->bpp);
            stride = pr->bpp * newdata->dims[newdata->ndims-2];

            if ((int)pr->h != newdata->dims[newdata->ndims-1])
              croak (__("pdl height != region height"));

            for (y   = 0, src = newdata->data, dst = pr->data;
                 y < (int)pr->h;
                 y++    , src += stride      , dst += pr->rowstride)
              Copy (src, dst, stride, char);

            RETVAL = newdata;
          }
        else
          {
            pdl *p = PDL->new();
            PDL_Long dims[3];

            dims[0] = pr->bpp;
            dims[1] = pr->rowstride / pr->bpp;
            dims[2] = pr->h;

            PDL->setdims (p, dims, 3);
            p->datatype = PDL_B;
            p->data = pr->data;
            p->state |= PDL_DONTTOUCHDATA | PDL_ALLOCATED;
            PDL->add_deletedata_magic(p, pixel_rgn_pdl_delete_data, 0);

            if ((int)pr->w != dims[1])
              p = redim_pdl (p, 1, pr->w);

            RETVAL = p;
          }
	OUTPUT:
	RETVAL

# ??? optimize these two functions so tile_*ref will only be called once on
# construction/destruction.

SV *
gimp_tile_get_data(tile)
	GTile *	tile
	CODE:
        need_pdl ();
        croak (__("gimp_tile_get_data is not yet implemented\n"));
	gimp_tile_ref (tile);
	gimp_tile_unref (tile, 0);
	OUTPUT:
	RETVAL

void
gimp_tile_set_data(tile,data)
	GTile *	tile
	SV *	data
	CODE:
        croak (__("gimp_tile_set_data is not yet implemented\n")); /*(void *)data;*/
	gimp_tile_ref_zero (tile);
	gimp_tile_unref (tile, 1);

#else

void
gimp_pixel_rgn_data(...)
	ALIAS:
          gimp_drawable_get_tile	= 1
          gimp_drawable_get_tile2	= 2
          gimp_pixel_rgn_get_pixel	= 3
          gimp_pixel_rgn_get_row	= 4
          gimp_pixel_rgn_get_col	= 5
          gimp_pixel_rgn_get_rect	= 6
          gimp_pixel_rgn_set_pixel	= 7
          gimp_pixel_rgn_set_row	= 8
          gimp_pixel_rgn_set_col	= 9
          gimp_pixel_rgn_set_rect	= 10
          gimp_tile_get_data		= 11
          gimp_tile_set_data		= 12
	CODE:
        croak (__("This module was built without support for PDL."));
 
#endif

BOOT:
	trace_file = PerlIO_stderr ();

#
# this function overrides a pdb function for speed
#

void
gimp_patterns_get_pattern_data(name)
	SV *	name
	PPCODE:
	{
		GParam *return_vals;
		int nreturn_vals;
		
		return_vals = gimp_run_procedure ("gimp_patterns_get_pattern_data",
		                                  &nreturn_vals,
		                                  PARAM_STRING, SvPV_nolen (name),
		                                  PARAM_END);
		
		if (nreturn_vals == 7
		    && return_vals[0].data.d_status == STATUS_SUCCESS)
		  {
		    EXTEND (SP, 5);
		    
		    PUSHs (sv_2mortal (newSVpv (        return_vals[1].data.d_string, 0)));
		    PUSHs (sv_2mortal (newSViv (        return_vals[2].data.d_int32)));
		    PUSHs (sv_2mortal (newSViv (        return_vals[3].data.d_int32)));
		    PUSHs (sv_2mortal (newSViv (        return_vals[4].data.d_int32)));
		    PUSHs (sv_2mortal (newSVpvn((char *)return_vals[6].data.d_int8array, return_vals[5].data.d_int32)));
		  }
		
		gimp_destroy_params (return_vals, nreturn_vals);
	}

void
_gimp_progress_init (message)
	gchar *	message
        CODE:
        gimp_progress_init (message);

#ifdef GIMP_HAVE_DEFAULT_DISPLAY

DISPLAY
gimp_default_display()

#endif

# functions using different calling conventions:
#void
#gimp_channel_get_color(channel_ID, red, green, blue)
#	CHANNEL	channel_ID
#	guchar *	red
#	guchar *	green
#	guchar *	blue
#gint32 *
#gimp_image_get_channels(image_ID, nchannels)
#	IMAGE	image_ID
#	gint *	nchannels
#guchar *
#gimp_image_get_cmap(image_ID, ncolors)
#	IMAGE	image_ID
#	gint *	ncolors
#gint32 *
#gimp_image_get_layers(image_ID, nlayers)
#	IMAGE	image_ID
#	gint *	nlayers
#gint32
#gimp_layer_new(image_ID, name, width, height, type, opacity, mode)
#	gint32	image_ID
#	char *	name
#	guint	width
#	guint	height
#	GDrawableType	type
#	gdouble	opacity
#	GLayerMode	mode
#gint32
#gimp_layer_copy(layer_ID)
#	gint32	layer_ID
#void
#gimp_channel_set_color(channel_ID, red, green, blue)
#	gint32	channel_ID
#	guchar	red
#	guchar	green
#	guchar	blue
#gint
#gimp_drawable_mask_bounds(drawable_ID, x1, y1, x2, y2)
#	DRAWABLE	drawable_ID
#	gint *	x1
#	gint *	y1
#	gint *	x2
#	gint *	y2
#void
#gimp_drawable_offsets(drawable_ID, offset_x, offset_y)
#	DRAWABLE	drawable_ID
#	gint *	offset_x
#	gint *	offset_y

# ??? almost synonymous to gimp_list_images

#gint32 *
#gimp_query_images(nimages)
#	int *	nimages

