#define MAX_VAR 1024
#define MAX_VAR_STR 16
#define MAX_VAR_HASH_SHIFT 13
#define MAX_VAR_HASH (1 << MAX_VAR_HASH_SHIFT)
#define MAX_VAR_MASK (MAX_VAR_HASH-1)

enum var_flags {
    VAR_ROM     = 0x01,
    VAR_ARCHIVE = 0x02
};
enum var_type {
    VAR_INT,
    VAR_FLOAT,
    VAR_STRING,
    VAR_ENUM
};
struct property_float {
    float min;
    float max;
    float *val;
};
struct property_int {
    int min;
    int max;
    int *val;
};
struct property_str {
    char *buf;
    int len;
};
struct property_enum {
    const char **options;
    int num;
    int *sel;
};
union property {
    struct property_float f;
    struct property_int i;
    struct property_enum e;
    struct property_str s;
};
struct var {
    enum var_type type;
    unsigned flags;
    const char *name;
    const char *print;
    const char *help;
    union property val;
};
struct varsys {
    const struct var *vars[MAX_VAR_HASH];
    unsigned long keys[MAX_VAR_HASH];
    int cnt;
};

#define MAX_CMDS 1024
#define MAX_CMD_ARGS 32
#define MAX_CMD_LINE (2*1024)

#define CMD(n) void n(int argc, char **argv)
typedef CMD((*cmd_f));

struct cmd {
    const char *name;
    cmd_f exec;
    int next;
};
struct cmdsys {
    int cmds, freelist;
    struct cmd buf[MAX_CMDS];
    int cnt;
};
