#ifndef ENABLE_NLS
# ifdef HAVE_LIBINTL_H
#  define ENABLE_NLS
# endif
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
#else
# define gettext(s) (s)
# define dgettext(d,s) (s)
# define textdomain(d) (d)
# define bindtextdomain(d,p) (p)
#endif

#define __(s) gettext (s)
#define N_(s) (s)
