#include "config.h"

/* dunno where this comes from */
#undef VOIDUSED

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#define NEED_newCONSTSUB
#include "gppport.h"

#if !defined(PERLIO_IS_STDIO) && defined(HASATTRIBUTE)
# undef printf
#endif

#if 0 /* optimized away ;) */
#include <glib.h>
#endif

#if !defined(PERLIO_IS_STDIO) && defined(HASATTRIBUTE)
# define printf PerlIO_stdoutf
#endif

#include "../perl-intl.h"

#if HAVE_PDL

# include <pdlcore.h>
# undef croak
# define croak Perl_croak

#if 0
/* hack, undocumented, argh! */
static Core* PDL; /* Structure hold core C functions */

/* get pointer to PDL structure. */
static void need_pdl (void)
{
  SV *CoreSV;

  if (!PDL)
    {
      /* Get pointer to structure of core shared C routines */
      if (!(CoreSV = perl_get_sv("PDL::SHARE",FALSE)))
        Perl_croak(__("gimp-perl-pixel functions require the PDL::Core module"));

      PDL = (Core*) SvIV(CoreSV);
    }
}
#endif

#endif

#define is_dynamic(sv)				\
	(strEQ ((sv), "Gimp::Tile")		\
         || strEQ ((sv), "Gimp::PixelRgn")	\
         || strEQ ((sv), "Gimp::GDrawable"))

static HV *object_cache;
static int object_id = 100;

#define init_object_cache	if (!object_cache) object_cache = newHV()

static void destroy_object (SV *sv)
{
  if (object_cache && sv_isobject (sv))
    {
      if (is_dynamic (HvNAME(SvSTASH(SvRV(sv)))))
        {
          int id = SvIV(SvRV(sv));
          hv_delete (object_cache, (char *)&id, sizeof(id), G_DISCARD);
        }
      else
        croak ("Internal error: Gimp::Net #101, please report!");
    }
  else
    croak ("Internal error: Gimp::Net #100, please report!");
}

/* allocate this much as initial length */
#define INITIAL_PV	256
/* and increment in these steps */
#define PV_INC		512

/* types
 *
 * u			undef
 * a num sv*		array
 * p len cont		pv
 * i int		iv
 * b stash sv		blessed reference
 * r			simple reference
 * h len (key sv)*	hash (not yet supported!)
 * p			piddle (not yet supported!)
 *
 */

static void sv2net (int deobjectify, SV *s, SV *sv)
{
  if (SvLEN(s)-SvCUR(s) < 96)
    SvGROW (s, SvLEN(s) + PV_INC);

  if (SvROK(sv))
    {
      SV *rv = SvRV(sv);
      if (SvOBJECT (rv))
        {
          char *name = HvNAME (SvSTASH (rv));

          sv_catpvf (s, "b%x:%s", strlen (name), name);

          if (deobjectify && is_dynamic (name))
            {
              object_id++;

              SvREFCNT_inc(sv);
              hv_store (object_cache, (char *)&object_id, sizeof(object_id), sv, 0);
              
              sv_catpvf (s, "i%d:", object_id);
              return; /* well... */
            }
        } 
      else
        sv_catpvn (s, "r", 1);

      if (SvTYPE(rv) == SVt_PVAV)
        {
          AV *av = (AV*)rv;
          int i;

          sv_catpvf (s, "a%x:", (I32)av_len(av));
          for (i = 0; i <= av_len(av); i++)
            sv2net (deobjectify, s, *av_fetch(av,i,0));
        }
      else if (SvTYPE(rv) == SVt_PVMG)
        sv2net (deobjectify, s, rv);
      else
        croak ("Internal error: unable to convert reference in sv2net, please report!");
    }
  else if (SvOK(sv))
    {
      if (SvIOK(sv))
        sv_catpvf (s,"i%ld:", (long)SvIV(sv));
      else
        {
          char *str;
          STRLEN len;

          /* slower than necessary, just make it an pv */
          str = SvPV(sv,len);
          sv_catpvf (s, "p%x:", (int)len);
          sv_catpvn (s, str, len);
        }
    }
  else
    sv_catpvn (s, "u", 1);
}

static SV *net2sv (int objectify, char **_s)
{
  char *s = *_s;
  SV *sv;
  AV *av;
  unsigned int ui, n;
  int i, j;
  long l;
  char str[64];

  switch (*s++)
    {
      case 'u':
        sv = newSVsv (&PL_sv_undef);
        break;

      case 'i':
        sscanf (s, "%ld:%n", &l, &n); s += n;
        sv = newSViv ((IV)l);
        break;

      case 'p':
        sscanf (s, "%x:%n", &ui, &n); s += n;
        sv = newSVpvn (s, (STRLEN)ui);
        s += ui;
        break;

      case 'r':
        sv = newRV_noinc (net2sv (objectify, &s));
        break;
        
      case 'b':
        sscanf (s, "%x:%n", &ui, &n); s += n;
        if (ui >= sizeof str)
          croak ("Internal error: stashname too long, please report!");

        memcpy (str, s, ui); s += ui;
        str[ui] = 0;

        if (objectify && is_dynamic (str))
          {
            SV **cv;
            int id;

            sscanf (s, "i%ld:%n", &l, &n); s += n;

            cv = hv_fetch (object_cache, (char *)(id=l,&id), sizeof(id), 0);
            if (!cv)
              croak ("Internal error: asked to deobjectify an object not in the cache, please report!");

            sv = *cv;
            SvREFCNT_inc (sv);
          }
        else
          sv = sv_bless (newRV_noinc (net2sv (objectify, &s)), gv_stashpv (str, 1));

        break;

      case 'a':
        sscanf (s, "%x:%n", &i, &n); s += n;
        av = newAV ();
        av_extend (av, (I32)i);
        for (j = 0; j <= i; j++)
          av_store (av, (I32)j, net2sv (objectify, &s));

        sv = (SV*)av;
        break;

      default:
        croak ("Internal error: unable to handle argtype '%c' in net2sv, please report!", s[-1]);
    }

  *_s = s;
  return sv;
}

MODULE = Gimp::Net	PACKAGE = Gimp::Net

PROTOTYPES: ENABLE

SV *
args2net(deobjectify,...)
	int	deobjectify
	CODE:
        int index;

        if (deobjectify) init_object_cache;

        RETVAL = newSVpv ("", 0);
        (void) SvUPGRADE (RETVAL, SVt_PV);
        SvGROW (RETVAL, INITIAL_PV);

	for (index = 1; index < items; index++)
          sv2net (deobjectify, RETVAL, ST(index));

        OUTPUT:
        RETVAL

void
net2args(objectify,s)
  	int	objectify
	char *	s
        PPCODE:

        if (objectify) init_object_cache;

        /* this depends on a trailing zero! */
        while (*s)
	  XPUSHs (sv_2mortal (net2sv (objectify, &s)));

void
destroy_objects(...)
	CODE:
        int index;

        for (index = 0; index < items; index++)
          destroy_object (ST(index));

