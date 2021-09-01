#ifndef _ORANGES_STRING_H_
#define _ORANGES_STRING_H_
PUBLIC void *memcpy(void *pDest, void *pSrc, int iSize);
PUBLIC void memset(void *p_dest, char ch, int size);
PUBLIC int memcmp(const void * s1, const void *s2, int n);
PUBLIC void *strcpy(void *p_dst, void *p_src);
PUBLIC int strlen(char *p_str);
PUBLIC int strcmp(const char * s1, const char *s2);
#define	phys_copy	memcpy
#define	phys_set	memset
#endif