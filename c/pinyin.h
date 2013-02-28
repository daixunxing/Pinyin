#define PY_UPPER_I 0x01
#define PY_UPPER_T 0x02
#define PY_INITIAL 0x04
#define PY_AUTOPAD 0x08
#define PY_WITHORI 0x10

enum {
	NON_GB = 0,
	GB2312_LEVEL_1, GB2312_LEVEL_2,
	GBK_LEVEL_3, GBK_LEVEL_4, GBK_LEVEL_5,
	GBK_USERDEF
};

typedef unsigned char GBCHAR;

typedef unsigned int GBCODE;

typedef struct {
	GBCODE code;
	int firstlen;
	char *pinyin;
} PYNODE;

typedef struct {
	PYNODE *c2p_head;
	int c2p_length;
} PYTABLE;

PYTABLE *
py_open(const char *tablefile);

void
py_close(PYTABLE *);

#define py_isgbk_high(high) ((GBCHAR) high >= 0x81)

int
py_isgbk(const char *str);

int
py_isgbk_func(GBCHAR high, GBCHAR low);

GBCODE
py_getcode(const char *str);

GBCODE
py_getcode_func(GBCHAR high, GBCHAR low);

PYNODE *
py_getpinyin(const char *str, const PYTABLE *);

PYNODE *
py_getpinyin_func(GBCHAR high, GBCHAR low, const PYTABLE *);

size_t
py_convstr(char **inbuf, size_t *inleft, char **outbuf, size_t *outleft,
	unsigned int flag, const PYTABLE *);
