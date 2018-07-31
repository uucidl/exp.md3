#define unused(a) ((void)a)
#define cast(t,p) ((t)(p))
#define szof(a) ((int)sizeof(a))
#define cntof(a) ((int)(sizeof(a)/sizeof((a)[0])))

#define flag(n) ((1u)<<(n))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define clamp(a,v,b) (max(min(b,v),a))
#define zero(d,sz) memset(d,0,(size_t)(sz))
#define copy(d,s,sz) memcpy(d,s,(size_t)(sz))
#define offsetof(st,m) ((int)((uintptr_t)&(((st*)0)->m)))
#define containerof(ptr,type,member) (type*)((void*)((char*)(1?(ptr):&((type*)0)->member)-offsof(type, member)))
#define between(x,a,b) ((a)<=(x) && (x)<=(b))
#define inbox(px,py,x,y,w,h) (between(px,x,x+w) && between(py,y,y+h))
#define intersect(x,y,w,h,X,Y,W,H) ((x)<(X)+(W) && (x)+(w)>(X) && (y)<(Y)+(H) && (y)+(h)>(Y))

#define alignof(t) ((int)((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0))
#define isaligned(x,mask) (!((uintptr_t)(x) & (mask-1)))
#define type_aligned(x,t) isaligned(x, alignof(t))
#define align_mask(a) ((a)-1)
#define align_down_masked(n,m) ((n) & ~(m))
#define align_down(n,a) align_down_masked(n, align_mask(a))
#define align_up(n,a) align_down((n) + align_mask(a), (a))
#define align_down_ptr(p,a) ((void*)align_down((uintptr_t)(p),(uintptr_t)(a)))
#define align_up_ptr(p,a) ((void*)align_up((uintptr_t)(p),(uintptr_t)(a)))

#define stringify(x) #x
#define stringifyi(x) stringify(x)
#define strjoini(a, b) a ## b
#define strjoind(a, b) strjoini(a,b)
#define strjoin(a, b) strjoind(a,b)
#define uniqid(name) strjoin(name, __FILE__ ## __LINE__)
#define compiler_assert(exp) typedef char uniqid(_compile_assert_array)[(exp)?1:-1]

/* ---------------------------------------------------------------------------
 *                                  Misc
 * --------------------------------------------------------------------------- */
static void
panic(const char *fmt, ...)
{
    va_list args;
    fflush(stdout);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}
static void*
xrealloc(void *p, int size)
{
    p = realloc(p, (size_t)size);
    if (!p) {
        perror("xrealloc failed!\n");
        exit(1);
    } return p;
}
static void*
xmalloc(int size)
{
    void *p = calloc(1,(size_t)size);
    if (!p) {
        perror("xmalloc failed!\n");
        exit(1);
    } return p;
}
static void*
xcalloc(int n, int size)
{
    void *p = calloc((size_t)n,(size_t)size);
    if (!p) {
        perror("xmalloc failed!\n");
        exit(1);
    } return p;
}

/* ---------------------------------------------------------------------------
 *                                  Math
 * --------------------------------------------------------------------------- */
static int
floori(float x)
{
    x = cast(float,(cast(int,x) - ((x < 0.0f) ? 1 : 0)));
    return cast(int,x);
}
static int
ceili(float x)
{
    if (x < 0) {
        const int t = cast(int,x);
        const float r = x - cast(float,t);
        return (r > 0.0f) ? (t+1): t;
    } else {
        const int i = cast(int,x);
        return (x > i) ? (i+1): i;
    }
}
static float
fround(float x)
{
    int e = 0;
    float y = 0;
    static const float toint = 1.0f/(1.1920928955078125e-07F);
    union {float f; unsigned long i;} u;

    u.f = x;
    e = (u.i >> 23) & 0xff;
    if (e >= 0x7f+23) return x;
    if (u.i >> 31) x = -x;
    if (e < 0x7f-1)
        return 0*u.f;

    y = x + toint - toint - x;
    if (y > 0.5f)
        y = y + x - 1;
    else if (y <= -0.5f)
        y = y + x + 1;
    else y = y + x;
    if (u.i >> 31)
        y = -y;
    return y;
}
static int
roundi(float x)
{
    float f = fround(x);
    return cast(int, f);
}
/* ---------------------------------------------------------------------------
 *                                  Hash
 * --------------------------------------------------------------------------- */
static unsigned
murmur_hash32(const void * key, int len, unsigned seed)
{
    #define ROTL(x,r) ((x) << (r) | ((x) >> (32 - r)))
    union {const unsigned *i; const unsigned char *b;} conv = {0};
    const unsigned char *data = (const unsigned char*)key;
    const int nblocks = len/4;
    unsigned h1 = seed;
    const unsigned c1 = 0xcc9e2d51;
    const unsigned c2 = 0x1b873593;
    const unsigned char *tail;
    const unsigned *blocks;
    unsigned k1;
    int i;

    /* body */
    if (!key) return 0;
    conv.b = (data + nblocks*4);
    blocks = (const unsigned*)conv.i;
    for (i = -nblocks; i; ++i) {
        k1 = blocks[i];
        k1 *= c1;
        k1 = ROTL(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    /* tail */
    tail = (const unsigned char*)(data + nblocks*4);
    k1 = 0;
    switch (len & 3) {
    case 3: k1 ^= (unsigned)(tail[2] << 16); /* fallthrough */
    case 2: k1 ^= (unsigned)(tail[1] << 8u); /* fallthrough */
    case 1: k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL(k1,15);
            k1 *= c2;
            h1 ^= k1;
            break;
    default: break;}

    /* finalization */
    h1 ^= (unsigned)len;
    /* fmix32 */
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    #undef NK_ROTL
    return h1;
}
static uint64_t
murmur_hash64(const void *key, int len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995llu;
    const int r = 47;
    uint64_t h = seed ^ ((uint64_t)len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);
    while(data != end) {
        uint64_t k = *data++;
        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }
    const unsigned char * data2 = (const unsigned char*)data;
    switch(len & 7){
    default: break;
    case 7: h ^= (uint64_t)data2[6] << 48;
    case 6: h ^= (uint64_t)data2[5] << 40;
    case 5: h ^= (uint64_t)data2[4] << 32;
    case 4: h ^= (uint64_t)data2[3] << 24;
    case 3: h ^= (uint64_t)data2[2] << 16;
    case 2: h ^= (uint64_t)data2[1] << 8;
    case 1: h ^= (uint64_t)data2[0]; h *= m;};

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}
static uintptr_t
murmur_hash(const void *key, int len, uintptr_t seed)
{
#if UINTPTR_MAX >= ULLONG_MAX
    return murmur_hash64(key, len, seed);
#else
    return murmur_hash32(key, len, seed);
#endif
}

/* ---------------------------------------------------------------------------
 *                                  UTF-8
 * --------------------------------------------------------------------------- */
#define UTF_SIZ         4
#define UTF_INVALID     0xFFFD

static const unsigned char utfbyte[UTF_SIZ+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ+1] = {0, 0, 0x80, 0x800, 0x10000};
static const long utfmax[UTF_SIZ+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};
#define is_utf8(c) (((c) & 0xC0) != 0x80)

static int
utf8_validate(long *u, int i)
{
    if (!u) return 0;
    if (!between(*u, utfmin[i], utfmax[i]) ||
         between(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i);
    return i;
}
static unsigned
utf8_decode_byte(char c, int *i)
{
    if (!i) return 0;
    for (*i = 0; *i < cntof(utfmask); ++(*i)) {
        if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
            return (unsigned char)(c & ~utfmask[*i]);
    } return 0;
}
static int
utf8_decode(const char *c, long *u, int clen)
{
    unsigned udecoded;
    int i, j, len, type=0;

    if (!c || !u) return 0;
    if (!clen) return 0;
    *u = UTF_INVALID;

    udecoded = utf8_decode_byte(c[0], &len);
    if (!between(len, 1, UTF_SIZ))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf8_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf8_validate(u, len);
    return len;
}
static char
utf8_encode_byte(long u, int i)
{
    return (char)((utfbyte[i]) | ((unsigned char)u & ~utfmask[i]));
}
static int
utf8_encode(long u, char *c, int clen)
{
    int len, i;
    len = utf8_validate(&u, 0);
    if (clen < len || !len || len > UTF_SIZ)
        return 0;

    for (i = len - 1; i != 0; --i) {
        c[i] = utf8_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = utf8_encode_byte(u, len);
    return len;
}
static const char*
utf8_dec(const char *b, const char *e)
{
    while (e > b) {
        char c = *(--e);
        if (is_utf8(c))
            return e;
    } return 0;
}
struct utf8_iter {
    int error;
    long rune;
    int rune_len;
    const char *rune_begin;
    const char *rune_end;
    /* statical */
    const char *next;
    const char *prev;
    const char *eof;
};
static void
utf8_begin(struct utf8_iter *it, const char *str, const char *end)
{
    it->error = 0;
    it->eof = end ? end: str + strlen(str);
    it->prev = it->eof;
    it->next = str;
}
static int
utf8_next(struct utf8_iter *it)
{
    if (it->next >= it->eof) return 0;
    it->rune_begin = it->next;
    it->rune_len = utf8_decode(it->rune_begin, &it->rune, (int)(it->eof - it->rune_begin));
    it->rune_end = it->rune_begin + it->rune_len;
    it->next = (it->rune_end >= it->eof) ? it->eof: it->rune_end;
    if (it->rune == UTF_INVALID) {
        it->error = UTF_INVALID;
        return 0;
    } else return 1;
}
static int
utf8_prev(struct utf8_iter *it)
{
    if (it->prev <= it->next) return 0;
    it->rune_begin = utf8_dec(it->next, it->prev);
    if (!it->rune_begin) return 0;
    it->rune_len = utf8_decode(it->rune_begin, &it->rune, (int)(it->eof - it->rune_begin));
    it->rune_end = it->rune_begin + it->rune_len;
    it->prev = it->rune_begin;
    if (it->rune == UTF_INVALID) {
        it->error = UTF_INVALID;
        return 0;
    } else return 1;
}
static int
utf8_len(const char *str, const char *end)
{
    int n = 0;
    struct utf8_iter it = {0};
    for (utf8_begin(&it, str, end); utf8_next(&it); n++);
    return n;
}
static int
utf8_at(struct utf8_iter *it, const char *str, const char *end, int idx)
{
    memset(it, 0, sizeof(*it));
    if (idx < 0) {
        idx = -idx;
        for (int n = (utf8_begin(it, str, end), 0); utf8_prev(it); n++)
            if (n == idx) return cast(int, it->rune_begin - str);
    } else {
        for (int n = (utf8_begin(it, str, end), 0); utf8_next(it); n++)
            if (n == idx) return cast(int, it->rune_begin - str);
    } return 0;
}
static int
utf8_has(const char *str, long rune)
{
    struct utf8_iter iter = {0};
    for (utf8_begin(&iter, str, 0); utf8_next(&iter); ) {
        if (iter.rune == rune)
            return 1;
    } return 0;
}

/* ---------------------------------------------------------------------------
 *                                  Buffer
 * --------------------------------------------------------------------------- */
#define BUF_ALIGNMENT 16
struct buf_hdr {
    int cnt, cap;
    char buf[1];
};
#define buf__hdr(b) ((struct buf_hdr*)(void*)((char*)(b) - offsetof(struct buf_hdr, buf)))
#define buf__fits(b,n) (buf_cnt(b) + (n) <= buf_cap(b))
#define buf_space(t,n) (sizeof(struct buf_hdr)-1 + sizeof(t) * n + BUF_ALIGNMENT)
#define buf_cnt(b) ((b) ? buf__hdr(b)->cnt: 0)
#define buf_cap(b) ((b) ? abs(buf__hdr(b)->cap): 0)
#define buf_begin(b) ((b) + 0)
#define buf_end(b) ((b) + buf_cnt(b))
#define buf_fit(b,n) (buf__fits((b), (n)) ? 0: ((b) = buf__grow((b),buf_cnt(b)+(n), sizeof(*(b)))))
#define buf__push(b, x) (buf_fit((b),1), (b)[buf__hdr(b)->cnt++] = x)
#define buf_push(b, ...) buf__push(b, (__VA_ARGS__))
#define buf_free(b) ((!(b)) ? 0: (buf__hdr(b)->cap <= 0) ? (b) = 0: (free(buf__hdr(b)), (b) = 0))
#define buf_clear(b) ((b) ? buf__hdr(b)->n = 0 : 0)
#define buf_printf(b, fmt, ...) ((b) = buf__printf((b), (fmt), __VA_ARGS__))

static void*
buf__grow(void *buf, int new_len, int elem_size)
{
    struct buf_hdr *hdr = 0;
    int cap = buf_cap(buf);
    int new_cap = max(2*cap + 1, new_len);
    int new_size = offsetof(struct buf_hdr, buf) + new_cap*elem_size;
    assert(new_len <= new_cap);
    if (!buf) {
        hdr = xmalloc(new_size);
        hdr->cnt = 0;
    } else if (buf__hdr(buf)->cap < 0) {
        hdr = xmalloc(new_size);
        hdr->cnt = buf_cnt(buf);
        memcpy(hdr->buf, buf__hdr(buf)->buf, (size_t)(cap*elem_size));
    } else hdr = realloc(buf__hdr(buf), (size_t)new_size);
    hdr->cap = new_cap;
    return hdr->buf;
}
static void*
buf_fixed(void *buf, int n)
{
    void *u = (char*)buf + BUF_ALIGNMENT + sizeof(struct buf_hdr) - 1;
    void *a = align_down_ptr(u, BUF_ALIGNMENT);
    void *h = (char*)a - sizeof(struct buf_hdr);

    struct buf_hdr *hdr = h;
    assert(isaligned(hdr, alignof(struct buf_hdr)));
    assert(isaligned(a, BUF_ALIGNMENT));
    hdr->cap = -n, hdr->cnt = 0;
    return hdr->buf;
}
static char*
buf__printf(char *buf, const char *fmt, ...)
{
    int cap, n;
    va_list args;
    va_start(args, fmt);
    cap = buf_cap(buf) - buf_cnt(buf);
    n = 1 + vsnprintf(buf_end(buf), (size_t)cap, fmt, args);
    va_end(args);

    if (n > cap) {
        buf_fit(buf, n + buf_cnt(buf));
        va_start(args, fmt);
        int new_cap = buf_cap(buf) - buf_cnt(buf);
        n = 1 + vsnprintf(buf_end(buf), (size_t)new_cap, fmt, args);
        assert(n <= new_cap);
        va_end(args);
    }
    buf__hdr(buf)->cnt += n - 1;
    return buf;
}

/* ---------------------------------------------------------------------------
 *                                  Arena
 * --------------------------------------------------------------------------- */
#define ARENA_ALIGNMENT 8
#define ARENA_BLOCK_CNT 1024
#define ARENA_BLOCK_SIZE (1024*1024)

struct arena {
    char *ptr;
    char *end;
    char **blks;
    char mem[buf_space(char*,ARENA_BLOCK_CNT)];
};
#define arena_push_array(a,t,n) arena_push(a, szof(t) * (n))

static void
arena__grow(struct arena *a, int min_size)
{
    min_size = align_up(min_size, ARENA_ALIGNMENT);
    a->ptr = xmalloc(min_size);
    a->end = a->ptr + min_size;
    buf_push(a->blks, a->ptr);
}
static void*
arena_push(struct arena *a, int size)
{
    char *p = 0;
    if (size > (a->end - a->ptr)) {
        int min_size = max(size, ARENA_BLOCK_SIZE);
        a->blks = (!a->blks) ? buf_fixed(a->mem, ARENA_BLOCK_CNT): a->blks;
        arena__grow(a, min_size);
    }
    p = a->ptr;
    a->ptr = align_up_ptr(p + size, ARENA_ALIGNMENT);
    memset(a->ptr, 0, (size_t)size);
    assert(a->ptr < a->end);
    return p;
}
static char*
arena_printf(struct arena *a, const char *fmt, ...)
{
    int n = 0;
    char *res = 0;
    va_list args;
    va_start(args, fmt);
    n = 1 + vsnprintf(0, 0, fmt, args);
    va_end(args);

    res = arena_push(a, n);
    va_start(args, fmt);
    vsnprintf(res, (size_t)n, fmt, args);
    va_end(args);
    return res;
}
static char*
arena_push_str(struct arena *a, const char *str, const char *end)
{
    end = !end ? str + strlen(str): end;
    char *s = arena_push(a, (int)(end - str) + 1);
    memcpy(s, str, (size_t)(end - str));
    s[end - str] = 0;
    return s;
}
static void
arena_free(struct arena *a)
{
    int i = 0;
    for (i = 0; i < buf_cnt(a->blks); ++i)
        free(a->blks[i]);
    memset(a, 0, sizeof(*a));
}

/* ---------------------------------------------------------------------------
 *                                  Scope
 * --------------------------------------------------------------------------- */
struct scope {
    struct arena *arena;
    char *ptr;
    char *end;
    char *blk;
};
static void
scope_begin(struct scope *s, struct arena *a)
{
    s->ptr = a->ptr;
    s->end = a->end;
    s->arena = a;

    char **at = buf_end(a->blks);
    s->blk = at ? *at: 0;
}
static void
scope_end(struct scope *s)
{
    struct arena *a = s->arena;
    a->ptr = s->ptr;
    a->end = s->end;
    for (char **it = buf_end(a->blks); it != buf_begin(a->blks); it--) {
        if (*it != s->blk) {
            free(*it); *it = 0;
            continue;
        } buf__hdr(a->blks)->cnt = cast(int, buf_end(a->blks) - it);
        return;
    }
}

/* ---------------------------------------------------------------------------
 *                                  Table
 * --------------------------------------------------------------------------- */
#define TBL_MIN_SIZE 32
#define TBL_GROW_FACTOR 2.8f
#define TBL_FULL_PERCENT 0.7f
#define TBL_MIN_GROW_PERCENT 1.3f
#define TBL_DELETED ((uintptr_t)-1)

struct tbl {
    int cnt, cap;
    uintptr_t *keys;
    intptr_t *vals;
};
#define tbl__fits(t, cnt) ((cnt) < (int)((float)(t)->cap * TBL_FULL_PERCENT))
#define tbl__fit(t, n) (tbl__fits(t, n) ? 0: tbl__grow(t, n))
#define tbl_insert(t, key, val) (tbl__fit(t, (t)->n + 1), tbl__put(t, key, val))
#define tbl_remove(t, key) tbl__del(t, key, 0)
#define tbl_free(t) do{free((t)->keys); memset((t), 0, sizeof(*t));}while(0)

static intptr_t
tbl__put(struct tbl *t, uintptr_t key, intptr_t val)
{
    uintptr_t n = cast(uintptr_t, t->cap);
    uintptr_t i = key % n, b = i;
    do {uintptr_t k = t->keys[i];
        if (k && k != TBL_DELETED) continue;
        t->keys[i] = key;
        t->vals[i] = val;
        return ++t->cnt, (intptr_t)i;
    } while ((i = ((i+1) % n)) != b);
    return t->cap;
}
static int
tbl__grow(struct tbl *t, int n)
{
    int i = 0;
    struct tbl old = *t;

    /* allocate new hash table */
    n = max(TBL_MIN_SIZE, (int)((float)n * TBL_MIN_GROW_PERCENT));
    t->cap = max(n, cast(int, TBL_GROW_FACTOR * cast(float, old.cap)));
    t->keys = xmalloc(t->cap * szof(uintptr_t) * 2);
    t->vals = cast(intptr_t*, t->keys + t->cap);
    t->cnt = 0;

    /* rehash old table entries */
    for (i = 0; i < old.cap; ++i)
        if (old.keys[i] && old.keys[i] != TBL_DELETED)
            tbl__put(t, old.keys[i], old.vals[i]);
    free(old.keys);
    return t->cap;
}
static intptr_t
tbl__find(const struct tbl *t, uintptr_t key)
{
    uintptr_t n = cast(uintptr_t, t->cap);
    uintptr_t i = key % n, b = i;
    do {uintptr_t k = t->keys[i];
        if (!k) return t->cap;
        if (k == key) return cast(intptr_t,i);
    } while ((i = ((i+1) % n)) != b);
    return t->cap;
}
static intptr_t
tbl__del(struct tbl *t, uintptr_t key, intptr_t not_found)
{
    if (!t->cnt) return not_found;
    intptr_t i = tbl__find(t, key);
    if (i >= t->cap) return not_found;
    t->cnt--; t->keys[i] = TBL_DELETED;
    return t->vals[i];
}
static intptr_t
tbl_lookup(const struct tbl *t, uintptr_t key, intptr_t not_found)
{
    if (!t->cnt) return not_found;
    intptr_t i = tbl__find(t, key);
    if (i >= t->cap) return not_found;
    return t->vals[i];
}

/* ---------------------------------------------------------------------------
 *                                  String
 * --------------------------------------------------------------------------- */
static int
strscpy(char *d, const char *s, int n)
{
    int slen = (int)strlen(s);
    if (n >= 0) {
        int num = n - 1;
        if (slen < num)
            num = slen;
        memcpy(d,s,(size_t)num);
        d[num] = '\0';
    } return slen;
}
struct str_iter {
    const char *begin;
    const char *end;
    /* statical */
    const char *next;
    const char *eof;
    const char *delims;
    unsigned sep;
};
static void
str_sep(struct str_iter *it, const char *str, const char *end, unsigned sep)
{
    memset(it, 0, sizeof(*it));
    it->eof = (!end) ? str + strlen(str): end;
    it->next = str;
    it->sep = sep;
}
static int
str_sep_next(struct str_iter *it)
{
    if (it->next >= it->eof) return 0;
    it->begin = it->next;

    struct utf8_iter iter = {0};
    for (utf8_begin(&iter, it->next, it->eof); utf8_next(&iter); ) {
        if (iter.rune != it->sep) break;
        it->begin = iter.rune_end;
    }
    it->end = it->begin;
    for (utf8_begin(&iter, it->end, it->eof); utf8_next(&iter); ) {
        if (iter.rune == it->sep) break;
        it->end = iter.rune_end;
    }
    if (it->begin == it->end) return 0;
    it->next = (it->end >= it->eof) ? it->eof: it->end;
    return 1;
}
static void
str_tokens(struct str_iter *it, const char *str, const char *end, const char *delims)
{
    memset(it, 0, sizeof(*it));
    it->eof = (!end) ? str + strlen(str): end;
    it->delims = delims;
    it->next = str;
}
static int
str_token_next(struct str_iter *it)
{
    struct utf8_iter iter = {0};
    if (it->next >= it->eof) return 0;

    it->begin = it->next;
    for (utf8_begin(&iter, it->begin, it->eof); utf8_next(&iter); ) {
        if (!utf8_has(it->delims, iter.rune)) break;
        it->begin = iter.rune_end;
    }
    it->end = it->begin;
    for (utf8_begin(&iter, it->end, it->eof); utf8_next(&iter); ) {
        if (utf8_has(it->delims, iter.rune)) break;
        it->end = iter.rune_end;
    }
    if (it->begin == it->end) return 0;
    it->next = (it->end < it->eof) ? it->end: it->eof;
    return 1;
}

/* ---------------------------------------------------------------------------
 *                                  Path
 * --------------------------------------------------------------------------- */
#ifndef MAX_PATH
  #define MAX_PATH 1024
#endif

struct path_iter {
    const char *begin;
    const char *end;
    /* internal */
    const char *next;
    const char *eof;
};
static void
path_begin(struct path_iter *it, const char *str, const char *end)
{
    memset(it, 0, sizeof(*it));
    it->eof = (!end) ? str + strlen(str): end;
    it->next = str;
}
static int
path_next(struct path_iter *it)
{
    struct utf8_iter iter;
    if (it->next >= it->eof) return 0;

    it->begin = it->next;
    it->end = it->begin;
    for (utf8_begin(&iter, it->end, it->eof); utf8_next(&iter); ) {
        if (iter.rune == '/') break;
        it->end = iter.rune_end;
    }
    if (it->begin == it->end)
        it->next = it->end = it->begin + 1;
    else it->next = (it->end < it->eof) ? it->end + 1: it->eof;
    return 1;
}
static void
path_normalize(char *path)
{
    struct utf8_iter it = {0};
    for (utf8_begin(&it, path, 0); utf8_next(&it); ) {
        if (it.rune == '\\') {
            path[it.rune_begin - path] = '/';
            return;
        }
    }
    if (it.eof != path) {
        /* remove trailing separator */
        utf8_begin(&it, path, 0);
        utf8_prev(&it);
        if (it.rune == '/')
            path[it.rune_begin - path] = 0;
    }
}
static void
path_copy(char *path, const char *src)
{
    strscpy(path, src, MAX_PATH);
    path[MAX_PATH-1] = 0;
}
static void
path_join(char *path, const char *src)
{
    char *ptr = path + strlen(path);
    if (ptr != path && ptr[-1] == '/') ptr--;
    if (*src == '/') src++;
    snprintf(ptr, (size_t)((path + MAX_PATH) - ptr), "/%s", src);
}
static const char*
path_file(char *path)
{
    struct utf8_iter it = {0};
    for (utf8_begin(&it, path, 0); utf8_prev(&it); ) {
        if (it.rune == '/')
            return it.rune_end;
    } return path;
}
static const char*
path_ext(const char *path)
{
    struct utf8_iter it = {0};
    for (utf8_begin(&it, path, 0); utf8_prev(&it); ) {
        if (it.rune == '.')
            return it.rune_end;
    } return "";
}
static void
path_parent(char *parent, const char *path)
{
    struct utf8_iter it = {0};
    strscpy(parent, path, MAX_PATH);
    for (utf8_begin(&it, path, 0); utf8_prev(&it); ) {
        if (it.rune == '/') {
            parent[it.rune_begin - path] = '\0';
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 *                                  Directory
 * --------------------------------------------------------------------------- */
struct dir_iter {
    int valid, err;
    char base[MAX_PATH];
    char name[MAX_PATH];
    int len;
    unsigned is_dir:1;
    void *handle;
};
static int
dir_is_excluded(const struct dir_iter *it)
{
    return it->valid && (strcmp(it->name, ".") == 0 || strcmp(it->name, "..") == 0);
}

#ifdef _MSC_VER /* Windows */

#include <io.h>
#include <errno.h>

static void
path_absolute(char *path)
{
    char rel[MAX_PATH];
    path_copy(rel, path);
    path_normalize(rel);
    _fullpath(path, rel, MAX_PATH);
}
static void
dir_list_free(struct dir_iter *it)
{
    if (!it->valid) return;
    it->valid = 0;
    it->err = 0;
    _findclose((intptr_t)it->handle);
}
static void
dir__update(struct dir_iter *it, int is_done, struct _finddata_t *fileinfo)
{
    it->valid = !is_done;
    it->err = is_done && erno != ENOENT;
    if (is_done) return;

    it->size = fileinfo->size;
    memcpy(it->name, fileinfo->name, sizof(iter->name)-1);
    it->name[MAX_PATH-1] = 0;
    it->is_dir = fileinfo->attrib & _A_SUBDIR;
}
static void
dir_next(struct dir_iter *it)
{
    if (!it->valid) return;
    do {struct _finddata_t fileinfo;
        int res = _findnext((intptr_t)it->handle, &fileinfo);
        dir__update(it, res != 0, &fileinfo);
        if (res != 0) {
            dir_list_free(it);
            return;
        }
    } while (dir_is_excluded(it));
}
static void
dir_begin(struct dir_iter *it, const char *path)
{
    memset(it, 0, sizeof(*it));
    path_copy(it->base, path);
    path_normalize(it->base);

    char filespec[MAX_PATH];
    path_copy(filespec, path);
    path_normalize(filespec);
    path_join(filespec, "*");

    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst(filespec, &fileinfo);
    it->handle = (void*)handle;
    dir__update(it, handle == -1, &fileinfo);
    if (dir_is_excluded(it))
        dir_next(it);
}

#else /* POSIX */

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static void
path_absolute(char *path)
{
    char rel[MAX_PATH];
    path_copy(rel, path);
    path_normalize(rel);
    realpath(rel, path);
}
static void
dir_list_free(struct dir_iter *it)
{
    if (!it->valid) return;
    it->valid = 0;
    it->err = 0;
    closedir(it->handle);
    it->handle = 0;
}
static void
dir_next(struct dir_iter *it)
{
    if (!it->valid) return;
    do {struct dirent *entry = readdir(it->handle);
        if (!entry) {
            dir_list_free(it);
            return;
        }
        path_copy(it->name, entry->d_name);
        it->is_dir = (entry->d_type & DT_DIR) ? 1u: 0u;
    } while (dir_is_excluded(it));
}
static void
dir_begin(struct dir_iter *it, const char *path)
{
    memset(it, 0, sizeof(*it));
    DIR *dir = opendir(path);
    if (!dir) {
        it->valid = 0;
        it->err = 1;
        return;
    }
    it->handle = dir;
    path_copy(it->base, path);
    path_normalize(it->base);
    it->valid = 1;
    dir_next(it);
}
#endif

enum lsdir_flags {
    LS_FILES = 0x01,
    LS_DIRS  = 0x02
};
enum lsdir_option {
    LS_FILES_ONLY = LS_FILES,
    LS_DIRS_ONLY = LS_DIRS,
    LS_FULL = LS_FILES|LS_DIRS
};
static char**
dir_list(const char *filespec, unsigned flags, struct arena *a)
{
    char **buf = 0;
    struct dir_iter it;
    for (dir_begin(&it, filespec); it.valid; dir_next(&it)) {
        if ((it.is_dir && (flags & LS_DIRS)) ||
           (!it.is_dir && (flags & LS_FILES))) {
            char *name = arena_push_str(a, it.name, 0);
            buf_push(buf, name);
        }
    } return buf;
}
