#include "config.h"
#include "perl-intl.h"

#include <libgimp/gimp.h>

#include <locale.h>

/* FIXME */
/* sys/param.h is redefining these! */
#undef MIN
#undef MAX

/* dunno where this comes from */
#undef VOIDUSED

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#define NEED_newCONSTSUB
#include "gppport.h"

/* FIXME */
/* dirty is used in gimp.h.  */
#ifdef dirty
# undef dirty
#endif
#include "extradefs.h"

#if GIMP_MAJOR_VERSION>1 || (GIMP_MAJOR_VERSION==1 && GIMP_MINOR_VERSION>=1)
# define GIMP11 1
# define GIMP_PARASITE 1
#endif

#ifndef HAVE_EXIT
/* expect iso-c here.  */
# include <signal.h>
#endif

MODULE = Gimp	PACKAGE = Gimp

PROTOTYPES: ENABLE

BOOT:
#ifdef ENABLE_NLS
	setlocale (LC_MESSAGES, ""); /* calling twice doesn't hurt, no? */
        bindtextdomain ("gimp-perl", datadir "/locale");
        textdomain ("gimp-perl");
#endif

void
_exit()
	CODE:
#ifdef HAVE__EXIT
	_exit(0);
#elif defined(SIGKILL)
	raise(SIGKILL);
#else
	raise(9);
#endif
	abort();

char *
bindtextdomain(d,dir)
	char *	d
	char *	dir

char *
textdomain(d)
	char *	d

char *
gettext(s)
	char *	s

char *
dgettext(d,s)
	char *	d
	char *	s

char *
__(s)
	char *	s
        PROTOTYPE: $

void
xs_exit(status)
	int	status
	CODE:
	exit (status);

BOOT:
{
   HV *stash = gv_stashpvn("Gimp", 4, TRUE);
   
   newCONSTSUB(stash,"PARAM_BOUNDARY",newSViv(PARAM_BOUNDARY));
   newCONSTSUB(stash,"PARAM_CHANNEL",newSViv(PARAM_CHANNEL));
   newCONSTSUB(stash,"PARAM_COLOR",newSViv(PARAM_COLOR));
   newCONSTSUB(stash,"PARAM_DISPLAY",newSViv(PARAM_DISPLAY));
   newCONSTSUB(stash,"PARAM_DRAWABLE",newSViv(PARAM_DRAWABLE));
   newCONSTSUB(stash,"PARAM_END",newSViv(PARAM_END));
   newCONSTSUB(stash,"PARAM_FLOAT",newSViv(PARAM_FLOAT));
   newCONSTSUB(stash,"PARAM_FLOATARRAY",newSViv(PARAM_FLOATARRAY));
   newCONSTSUB(stash,"PARAM_IMAGE",newSViv(PARAM_IMAGE));
   newCONSTSUB(stash,"PARAM_INT16",newSViv(PARAM_INT16));
   newCONSTSUB(stash,"PARAM_INT16ARRAY",newSViv(PARAM_INT16ARRAY));
   newCONSTSUB(stash,"PARAM_INT32",newSViv(PARAM_INT32));
   newCONSTSUB(stash,"PARAM_INT32ARRAY",newSViv(PARAM_INT32ARRAY));
   newCONSTSUB(stash,"PARAM_INT8",newSViv(PARAM_INT8));
   newCONSTSUB(stash,"PARAM_INT8ARRAY",newSViv(PARAM_INT8ARRAY));
   newCONSTSUB(stash,"PARAM_LAYER",newSViv(PARAM_LAYER));
   newCONSTSUB(stash,"PARAM_PATH",newSViv(PARAM_PATH));
   newCONSTSUB(stash,"PARAM_REGION",newSViv(PARAM_REGION));
   newCONSTSUB(stash,"PARAM_SELECTION",newSViv(PARAM_SELECTION));
   newCONSTSUB(stash,"PARAM_STATUS",newSViv(PARAM_STATUS));
   newCONSTSUB(stash,"PARAM_STRING",newSViv(PARAM_STRING));
   newCONSTSUB(stash,"PARAM_STRINGARRAY",newSViv(PARAM_STRINGARRAY));
#if GIMP_PARASITE
   newCONSTSUB(stash,"PARAM_PARASITE",newSViv(PARAM_PARASITE));

   newCONSTSUB(stash,"PARASITE_PERSISTENT",newSViv(PARASITE_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_UNDOABLE",newSViv(PARASITE_UNDOABLE));

   newCONSTSUB(stash,"PARASITE_ATTACH_PARENT",newSViv(PARASITE_ATTACH_PARENT));
   newCONSTSUB(stash,"PARASITE_PARENT_PERSISTENT",newSViv(PARASITE_PARENT_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_PARENT_UNDOABLE",newSViv(PARASITE_PARENT_UNDOABLE));
   
   newCONSTSUB(stash,"PARASITE_ATTACH_GRANDPARENT",newSViv(PARASITE_ATTACH_GRANDPARENT));
   newCONSTSUB(stash,"PARASITE_GRANDPARENT_PERSISTENT",newSViv(PARASITE_GRANDPARENT_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_GRANDPARENT_UNDOABLE",newSViv(PARASITE_GRANDPARENT_UNDOABLE));
#endif
   
   newCONSTSUB(stash,"PROC_EXTENSION",newSViv(PROC_EXTENSION));
   newCONSTSUB(stash,"PROC_PLUG_IN",newSViv(PROC_PLUG_IN));
   newCONSTSUB(stash,"PROC_TEMPORARY",newSViv(PROC_TEMPORARY));
   
   newCONSTSUB(stash,"STATUS_CALLING_ERROR",newSViv(STATUS_CALLING_ERROR));
   newCONSTSUB(stash,"STATUS_EXECUTION_ERROR",newSViv(STATUS_EXECUTION_ERROR));
   newCONSTSUB(stash,"STATUS_PASS_THROUGH",newSViv(STATUS_PASS_THROUGH));
   newCONSTSUB(stash,"STATUS_SUCCESS",newSViv(STATUS_SUCCESS));
   
   newCONSTSUB(stash,"TRACE_NONE",newSViv(TRACE_NONE));
   newCONSTSUB(stash,"TRACE_CALL",newSViv(TRACE_CALL));
   newCONSTSUB(stash,"TRACE_TYPE",newSViv(TRACE_TYPE));
   newCONSTSUB(stash,"TRACE_NAME",newSViv(TRACE_NAME));
   newCONSTSUB(stash,"TRACE_DESC",newSViv(TRACE_DESC));
   newCONSTSUB(stash,"TRACE_ALL" ,newSViv(TRACE_ALL ));
   /**/
#if HAVE_DIVIDE_MODE || IN_GIMP
   /*newCONSTSUB(stash,"DIVIDE_MODE",newSViv(DIVIDE_MODE));*/
#endif
}

MODULE = Gimp	PACKAGE = Gimp::RAW

# some raw byte/bit-manipulation (e.g. for avi and miff), use PDL instead
# mostly undocumented as well...

void
reverse_v_inplace (datasv, bpl)
	SV *	datasv
        IV	bpl
        CODE:
        char *line, *data, *end;
        STRLEN h;

        data = SvPV (datasv, h); h /= bpl;
        end = data + (h-1) * bpl;

        New (0, line, bpl, char);

        while (data < end)
          {
            Move (data, line, bpl, char);
            Move (end, data, bpl, char);
            Move (line, end, bpl, char);

            data += bpl;
            end -= bpl;
          }

        Safefree (line);

	OUTPUT:
        datasv

void
convert_32_24_inplace (datasv)
	SV *	datasv
        CODE:
        STRLEN dc;
        char *data, *src, *dst, *end;

        data = SvPV (datasv, dc); end = data + dc;

        for (src = dst = data; src < end; )
          {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
                     *src++;
          }

        SvCUR_set (datasv, dst - data);
	OUTPUT:
        datasv

void
convert_24_15_inplace (datasv)
	SV *	datasv
        CODE:
        STRLEN dc;
        char *data, *src, *dst, *end;

        U16 m31d255[256];

        for (dc = 256; dc--; )
          m31d255[dc] = (dc*31+127)/255;

        data = SvPV (datasv, dc); end = data + dc;

        for (src = dst = data; src < end; )
          {
            unsigned int r = *(U8 *)src++;
            unsigned int g = *(U8 *)src++;
            unsigned int b = *(U8 *)src++;

            U16 rgb = m31d255[r]<<10 | m31d255[g]<<5 | m31d255[b];
            *dst++ = rgb & 0xff;
            *dst++ = rgb >> 8;
          }

        SvCUR_set (datasv, dst - data);
	OUTPUT:
        datasv

void
convert_15_24_inplace (datasv)
	SV *	datasv
        CODE:
        STRLEN dc, de;
        char *data, *src, *dst;

        U8 m255d31[32];

        for (dc = 32; dc--; )
          m255d31[dc] = (dc*255+15)/31;

        data = SvPV (datasv, dc); dc &= ~1;
        de = dc + (dc >> 1);
        SvGROW (datasv, de);
        SvCUR_set (datasv, de);
        data = SvPV (datasv, de); src = data + dc;

        dst = data + de;

        while (src != dst)
          {
            U16 rgb = *(U8 *)--src << 8 | *(U8 *)--src;

            *(U8 *)--dst = m255d31[ rgb & 0x001f       ];
            *(U8 *)--dst = m255d31[(rgb & 0x03e0) >>  5];
            *(U8 *)--dst = m255d31[(rgb & 0x7c00) >> 10];
          }

	OUTPUT:
        datasv

