#ifndef __GIMPINTL_H__
#define __GIMPINTL_H__

/* Copied from gnome-i18n.h by Tom Tromey <tromey@creche.cygnus.com> *
 * Heavily modified by Daniel Egger <Daniel.Egger@t-online.de>       *
 * So be sure to hit me instead of him if something is wrong here    */ 

#ifndef LOCALEDIR
#define LOCALEDIR g_strconcat (gimp_data_directory (), \
			       G_DIR_SEPARATOR_S, \
			       "locale", \
			       NULL)
#endif

#ifdef ENABLE_NLS
#    include <libintl.h>
#    define _(String) gettext (String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif

#define INIT_LOCALE( domain )			\
	gtk_set_locale ();			\
	setlocale (LC_NUMERIC, "C");		\
	bindtextdomain (domain, LOCALEDIR);	\
	textdomain (domain);								
#endif
