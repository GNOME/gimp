#ifdef ENABLE_NLS
# include <libintl.h>
#else
# define gettext(s) (s)
# define textdomain(d) (d)
# define bindtextdomain(d,p) (p)
#endif

#define __(s) gettext (s)
#define N_(s) (s)
