#ifndef __GIMPINTL_H__
#define __GIMPINTL_H__

#define LOCALEDIR LIBDIR "/locale"

/* Copied from gnome-i18n.h by Tom Tromey <tromey@creche.cygnus.com> */

#ifdef ENABLE_NLS
#include<libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#endif


#endif
