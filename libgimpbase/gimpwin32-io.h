/*
 * Compatibilty defines, you mostly need this as unistd.h replacement
 */
#ifndef __GIMP_WIN32_IO_H__
#define __GIMP_WIN32_IO_H__

#include <io.h>
#include <direct.h>

#define mkdir(n,a) _mkdir(n)

#define chmod(n,f) _chmod(n,f)

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif
#ifndef _O_TEMPORARY
#define _O_TEMPORARY 0
#endif

#ifndef W_OK
#define W_OK 2
#endif

#endif
