#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pinyin.h"

#define TABLE_LINESIZE 128

static int
py_getpinyin_bsearch(const void *keyval, const void *datam)
{
	return (int) *(GBCODE *) keyval - ((PYNODE *) datam)->code;
}

static size_t
py_getpinyin_firstlen(const char *pinyin)
{
	const char *p;
	for (p = pinyin; *p != '\0' && *p != ' '; p++)
		;
	return (size_t) (p - pinyin);
}

PYTABLE *
py_open(const char *tablefile)
{
	PYTABLE *ptable;
	PYNODE *pnode;
	GBCODE last;
	FILE *fp;
	int len;
	char line[TABLE_LINESIZE];

	// Open table file
	if ((fp = fopen(tablefile, "r")) == NULL)
		return NULL;

	// Alloc
	if ((ptable = malloc(sizeof (PYTABLE))) == NULL)
		return NULL;

	// Count
	for (ptable->c2p_length = 0; fgets(line, TABLE_LINESIZE, fp); ptable->c2p_length++)
		;
	if ((ptable->c2p_head = malloc(ptable->c2p_length * sizeof (PYNODE))) == NULL)
		return NULL;

	// Store
	fseek(fp, 0, SEEK_SET);
	for (last = 0, pnode = ptable->c2p_head; fgets(line, TABLE_LINESIZE, fp); pnode++) {
		pnode->code = py_getcode(line);
		if (pnode->code <= last) // we have to use a sorted table because bsearch
			return NULL;		 // so you can use `export LC_ALL=C && sort table` to sort

		// Stripe new line
		len = strlen(line);
		if (*(line + len - 1) == '\n')
			*(line + len - 1) = '\0';
		pnode->pinyin = strdup(strchr(line, ' ') + 1);
		pnode->firstlen = py_getpinyin_firstlen(pnode->pinyin);


		last = pnode->code;
	}

	return ptable;
}

void
py_close(PYTABLE *ptable)
{
	PYNODE *pnode;

	for (pnode = ptable->c2p_head; pnode < ptable->c2p_head + ptable->c2p_length; pnode++)
		free(pnode->pinyin);
	free(ptable->c2p_head);
	free(ptable);
}

int
py_isgbk(const char *str)
{
	return py_isgbk_func((GBCHAR) *str, (GBCHAR) *(str + 1));
}

int
py_isgbk_func(GBCHAR high, GBCHAR low)
{
	if (!py_isgbk_high(high))
		return NON_GB; // fast return

	/**/ if (high >= 0xA1 && high <= 0xA9 && low >= 0xA1 && low <= 0xFE)
		return GB2312_LEVEL_1;
	else if (high >= 0xB0 && high <= 0xF7 && low >= 0xA1 && low <= 0xFE)
		return GB2312_LEVEL_2;
	else if (high >= 0x81 && high <= 0xA0 && low >= 0x40 && low <= 0xFE && low != 0x7F)
		return GBK_LEVEL_3;
	else if (high >= 0xAA && high <= 0xFE && low >= 0x40 && low <= 0xA0 && low != 0x7F)
		return GBK_LEVEL_4;
	else if (high >= 0xA8 && high <= 0xA9 && low >= 0x40 && low <= 0xA0 && low != 0x7F)
		return GBK_LEVEL_5;
	else if (high >= 0xAA && high <= 0xAF && low >= 0xA1 && low <= 0xFE)
		return GBK_USERDEF;
	else if (high >= 0xF8 && high <= 0xFE && low >= 0xA1 && low <= 0xFE)
		return GBK_USERDEF;
	else if (high >= 0xA1 && high <= 0xA7 && low >= 0x40 && low <= 0xA0 && low != 0x7F)
		return GBK_USERDEF;
	else
		return NON_GB;
}

GBCODE
py_getcode(const char *str)
{
	return py_getcode_func((GBCHAR) *str, (GBCHAR) *(str + 1));
}

GBCODE
py_getcode_func(GBCHAR high, GBCHAR low)
{
	return (GBCODE) (high << 8) + low;
}

PYNODE *
py_getpinyin(const char *str, const PYTABLE *ptable)
{
	return py_getpinyin_func((GBCHAR) *str, (GBCHAR) *(str + 1), ptable);
}

PYNODE *
py_getpinyin_func(GBCHAR high, GBCHAR low, const PYTABLE *ptable)
{
	GBCODE code;

	code = py_getcode_func(high, low);
	return bsearch(
		&code,
		ptable->c2p_head,
		ptable->c2p_length,
		sizeof (PYNODE),
		py_getpinyin_bsearch
	);
}

size_t
py_convstr(char **inbuf, size_t *inleft, char **outbuf, size_t *outleft,
	unsigned int flag, const PYTABLE *ptable)
{
	PYNODE *pnode, *lastpy;
	char *p;
	char *outbuf_st = *outbuf; // for Auto padding
	size_t inlen, outlen, addpad, addori;

	while (*inleft > 0) {

		// Get current char pinyin
		if (*inleft > 1 && py_isgbk(*inbuf)) {
			inlen = 2;
			if ((pnode = py_getpinyin(*inbuf, ptable)) != NULL)
				outlen = flag & PY_INITIAL ? 1 : pnode->firstlen;
			else
				outlen = 2;
		} else if (*inleft == 1 && py_isgbk_high(**inbuf)) {
			break;
		} else {
			inlen = outlen = 1;
			pnode = NULL;
		}

		// Auto padding
		if (flag & PY_AUTOPAD && (
			(pnode != NULL && *outbuf > outbuf_st && !isspace(*(*outbuf - 1)))
			|| (pnode == NULL && lastpy != NULL && !isspace(**inbuf))
		))
			addpad = 1;
		else
			addpad = 0;

		// With Original
		if (pnode != NULL && flag & PY_WITHORI)
			addori = 4;
		else
			addori = 0;

		// Check out left
		if (outlen + addpad + addori > *outleft)
			return (size_t) -1L;

		// Concate
		if (addpad)
			*((*outbuf)++) = ' ';
		if (pnode != NULL) {
			if (addori) {
				strncat(*outbuf, *inbuf, 2);
				*outbuf += 2;
				*((*outbuf)++) = '(';
			}
			*((*outbuf)++) = flag & PY_UPPER_I ? toupper(*pnode->pinyin) : *pnode->pinyin;
			for (p = pnode->pinyin + 1; p < pnode->pinyin + outlen; p++) {
				*((*outbuf)++) = flag & PY_UPPER_T ? toupper(*p) : *p;
			}
			if (addori)
				*((*outbuf)++) = ')';
		} else {
			strncat(*outbuf, *inbuf, outlen);
			*outbuf += outlen;
		}

		// Set pointer
		lastpy = pnode;
		*inleft -= inlen;
		*inbuf += inlen;
		*outleft -= outlen + addpad + addori;
	}

	return (size_t) 0L;
}
