#include "config.h"

/* dunno where this comes from */
#undef VOIDUSED

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#define NEED_newCONSTSUB
#include "gppport.h"

/* dirty is used in gimp.h AND in perl < 5.005 or with PERL_POLLUTE.  */
#ifdef dirty
# undef dirty
#endif
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#ifdef GIMP_HAVE_EXPORT
#include <libgimp/gimpexport.h>
#endif

/* libgimo requires a rather broken interface. this must be here because..
 * well, nobody knows why... ARGH! */
GimpPlugInInfo PLUG_IN_INFO = { 0, 0, 0, 0 };

#if 0
static void gimp_pattern_select_widget_callback (gchar *name, gint width,
	gint height, gint bpp, gchar *mask, gint closing, gpointer nameref)
{
  SV *sv = (SV *)nameref;
  sv_setpv (sv, name);
}
#endif

static void need_gtk (void)
{
  gint argc = 0;
  
  gtk_init_check (&argc, 0); /* aaaaargh */
}

MODULE = Gimp::UI	PACKAGE = Gimp::UI

PROTOTYPES: ENABLE

#if defined(GIMP_HAVE_EXPORT)

gint32
export_image(image_ID,drawable_ID,format_name,capabilities)
	SV *	image_ID
        SV *	drawable_ID
        gchar *	format_name
        gint	capabilities
        CODE:
        gint32 image = SvIV(SvRV(image_ID));
        gint32 drawable = SvIV(SvRV(drawable_ID));
        need_gtk ();
        RETVAL = gimp_export_image (&image, &drawable, format_name, capabilities);
        sv_setiv (SvRV(image_ID), image);
        sv_setiv (SvRV(drawable_ID), drawable);
	OUTPUT:
        image_ID
        drawable_ID
        RETVAL

#endif

#if 0
#if UI
#if GIMP11

GtkWidget *
_new_pattern_select(dname, ipattern, nameref)
	gchar *	dname
	gchar *	ipattern
	SV *	nameref
	CODE:
	{
		if (!SvROK (nameref))
		  croak (__("last argument to gimp_pattern_select_widget must be scalar ref"));
		
		nameref = SvRV (nameref);
		SvUPGRADE (nameref, SVt_PV);
		
		RETVAL = gimp_pattern_select_widget (dname, ipattern,
				gimp_pattern_select_widget_callback, (gpointer) nameref);
	}
	OUTPUT:
	RETVAL

#endif
#endif
#endif
