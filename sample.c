#include <stdio.h>
#include <string.h>
#include "c/pinyin.h"

int main(int argc, char *argv[])
{
	PYTABLE *ptable;
	char str_gbk[BUFSIZ], str_py[BUFSIZ], *inbuf, *outbuf;
	size_t insize, outsize, nconv;


	// Read input
	int c;
	char *p;
	for (p = str_gbk; (c = getchar()) != EOF && p < str_gbk + BUFSIZ; p++)
		*p = c;
	*p = '\0';


	// py_open
	if ((ptable = py_open("table/gbk.gb")) == NULL) {
		fprintf(stderr, "error: can not open pinyin table or table isn't sorted\n");
		return 1;
	}

	// convert String to Pinyin
	inbuf = str_gbk, insize = strlen(str_gbk);
	outbuf = str_py, outsize = sizeof str_py;

	nconv = py_convstr(&inbuf, &insize, &outbuf, &outsize, (PY_AUTOPAD | PY_WITHORI | PY_UPPER_I), ptable);
	if (nconv == (size_t) -1) {
		fprintf(stderr, "error: out buffer is too small\n");
		return -1;
	}

	// Display
	*outbuf = '\0';
	printf("%s\n", str_py);


	// py_close
	py_close(ptable);

	return 0;
}
