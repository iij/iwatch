#ifndef INCLUDES_H
#define INCLUDES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef HAVE_STRLCAT
size_t	 strlcat(char *, const char *, size_t);
#endif /* !HAVE_STRLCAT */

#endif
