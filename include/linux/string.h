/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TOOLS_LINUX_STRING_H_
#define _TOOLS_LINUX_STRING_H_

#include <linux/types-user.h>	/* for size_t */
#include <stdbool.h>

void *memdup(const void *src, size_t len);

char **argv_split(const char *str, int *argcp);
void argv_free(char **argv);

int strtobool(const char *s, bool *res);

/*
 * glibc based builds needs the extern while uClibc doesn't.
 * However uClibc headers also define __GLIBC__ hence the hack below
 */
#if defined(__GLIBC__) && !defined(__UCLIBC__)
extern size_t strlcpy(char *dest, const char *src, size_t size);
#endif

char *str_error_r(int errnum, char *buf, size_t buflen);

char *strreplace(char *s, char old, char new);

/**
 * strstarts - does @str start with @prefix?
 * @str: string to examine
 * @prefix: prefix to look for.
 */
static inline bool strstarts(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

extern char * __must_check skip_spaces(const char *);

extern char *strim(char *);

static inline __must_check char *strstrip(char *str)
{
    return strim(str);
}

extern char *strchrnul(const char *s, int c);

/**
 * kbasename - return the last part of a pathname.
 *
 * @path: path to extract the filename from.
 */
static inline const char *kbasename(const char *path)
{
    const char *tail = strrchr(path, '/');
    return tail ? tail + 1 : path;
}

#ifndef __HAVE_ARCH_STRNCHR
extern char * strnchr(const char *, size_t, int);
#endif

//197 lines
int match_string(const char * const *array, size_t n, const char *string);

#endif /* _TOOLS_LINUX_STRING_H_ */
