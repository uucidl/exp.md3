static struct cmdsys g_cmdsys;
static struct varsys g_varsys;

/* ---------------------------------------------------------------------------
 *                              Con: Command
 * --------------------------------------------------------------------------- */
static void
cmd_add(struct cmdsys *c, const char *name, cmd_f f)
{
    int id, i = 0;
    struct cmd *cmd;
    for (i = c->cmds; i != MAX_CMDS; i = c->buf[i].next) {
        if (strcmp(c->buf[i].name, name) == 0)
            return;
    }
    if (c->freelist == MAX_CMDS) {
        fprintf(stderr, "MAX_CMDS");
        return;
    }
    id = c->freelist;
    cmd = c->buf + id;

    c->freelist = cmd->next;
    cmd->next = c->cmds;
    c->cmds = id;

    cmd->name = name;
    cmd->exec = f;
}
static void
cmd_remove(struct cmdsys *c, const char *name)
{
    int p = c->cmds, i;
    struct cmd *cmd;
    if (c->cmds == MAX_CMDS)
        return;

    cmd = c->buf + c->cmds;
    if (strcmp(cmd->name, name) == 0) {
        c->cmds = cmd->next;
        cmd->next = c->freelist;
        c->freelist = c->cmds;
        return;
    }
    p = c->cmds, i = cmd->next;
    while (i != MAX_CMDS) {
        cmd = c->buf + i;
        if (strcmp(cmd->name, name) == 0) {
            c->buf[p].next = cmd->next;
            cmd->next = c->freelist;
            c->freelist = i;
            return;
        } p = i;
        i = cmd->next;
    }
}
static void
cmd_run(struct cmdsys *c, int argc, char **argv)
{
    int i = 0;
    if (!argc) return;
    for (i = c->cmds; i != MAX_CMDS; i = c->buf[i].next) {
        struct cmd *cmd = c->buf + i;
        if (strcmp(cmd->name, argv[0]) == 0) {
            cmd->exec(argc, argv);
            return;
        }
    }
}
static void
cmd_exec_str(struct cmdsys *c, const char *cmdline)
{
    struct str_iter line_iter;
    for (str_tokens(&line_iter, cmdline, 0, "\n;"); str_token_next(&line_iter);) {
        char *argv[MAX_CMD_ARGS] = {0};
        char line[MAX_CMD_LINE];
        int argc = 0;

        size_t len = min((size_t)(line_iter.end - line_iter.begin), sizeof(line)-1);
        memcpy(line, line_iter.begin, len);
        line[len] = 0;

        struct str_iter arg_iter;
        for (str_sep(&arg_iter, line, 0, ' '); str_sep_next(&arg_iter);) {
            line[arg_iter.end - line] = 0;
            if (argc >= MAX_CMD_ARGS-1) break;
            argv[argc++] = line + (arg_iter.begin - line);
            arg_iter.next++;
        }
        argv[argc] = 0;
        cmd_run(c, argc, argv);
    }
}
static CMD(cmdlist_f)
{
    int n = 0, i = 0;
    const char *match = 0;
    struct cmdsys *c = &g_cmdsys;
    if (argc > 1) match = argv[1];
    for (i = c->cmds; i != MAX_CMDS; i = c->buf[i].next) {
        struct cmd *cmd = c->buf + i;
        if (!match || strfilter(cmd->name, match)) {
            fprintf(stderr, "%s\n", cmd->name);
            n++;
        }
    }
    fprintf(stdout, "---------------------------");
    fprintf(stdout, "\n%i total cmds\n", i);
    fprintf(stdout, "%i cmd indexes\n", n);
    fprintf(stdout, "---------------------------\n");
}
static void
cmd_exec(const char *filepath)
{
    int sz = 0;
    char *str = 0;
    str = file_load(filepath, &sz);
    if (!str) return;
    cmd_exec_str(&g_cmdsys, str);
    free(str);
}
static CMD(exec_f)
{
    if (argc < 2) {
        printf("%s <filename>: execute a script file \n", argv[0]);
        return;
    }
    cmd_exec(argv[1]);
}
static CMD(echo_f)
{
    for (int i = 1; i < argc; ++i)
        fprintf(stdout, "%s ", argv[i]);
    fprintf(stdout, "\n");
}
static void
cmd_init(struct cmdsys *c)
{
    int i = 0;
    zero(c, szof(*c));
    for (i = 0; i < MAX_CMDS; ++i)
        c->buf[i].next = i + 1;

    c->cmds = MAX_CMDS;
    cmd_add(c, "cmdlist", cmdlist_f);
    cmd_add(c, "exec", exec_f);
    cmd_add(c, "echo", echo_f);
}
/* ---------------------------------------------------------------------------
 *                              Con: Var
 * --------------------------------------------------------------------------- */
static unsigned long
hash(const char *str, unsigned long x)
{
    int i = 0;
    int len = cast(int, strlen(str));
    unsigned long h = x;
    for(i = 0; i < len; ++i)
        h = 65599lu * h + cast(unsigned char, str[i]);
    return h^(h>>16);
}
static void
var_insert(struct varsys *v, const struct var *var)
{
    unsigned long id = hash(var->name, 0);
    unsigned long idx = id & MAX_VAR_MASK, begin = idx;
    do {unsigned long key = v->keys[idx];
        if (key) continue;
        v->keys[idx] = id;
        v->vars[idx] = var;
        v->cnt++; return;
    } while ((idx = ((idx+1) & MAX_VAR_MASK)) != begin);
}
static const struct var*
var_find(struct varsys *v, const char *name)
{
    unsigned long key, id = hash(name, 0);
    unsigned idx = id & MAX_VAR_MASK, begin = idx;
    do {if (!(key = v->keys[idx])) return 0;
        if (key == id) return v->vars[idx];
    } while ((idx = ((idx+1) & MAX_VAR_MASK)) != begin);
    return 0;
}
static void
var_set(struct varsys *v, const char *name, const char *value)
{
    const struct var *var = var_find(v, name);
    if (!var) return;
    switch (var->type) {
    case VAR_STRING: {
        strscpy(var->val.s.buf, value, var->val.s.len);
    } break;
    case VAR_INT: {
        char *ep = NULL;
        long n = strtol(value, &ep, 10);
        if (*ep != '\0' || ep == value) return;
        if (n < INT_MIN || n > INT_MAX) return;
        *var->val.i.val = cast(int, n);
    } break;
    case VAR_FLOAT: {
        char *ep = NULL;
        double d = strtod(value, &ep);
        if (*ep != '\0' || ep == value) return;
        *var->val.f.val = cast(float, d);
    } break;
    case VAR_ENUM: {
        char *ep = NULL;
        long n = strtol(value, &ep, 10);
        if (*ep != '\0' || ep == value) {
            for (int i = 0; i < var->val.e.num; ++i) {
                if (strcmp(var->val.e.options[i], value) == 0) {
                    *var->val.e.sel = i;
                    return;
                }
            }
        } else *var->val.e.sel = clamp(0, (int)n, var->val.e.num-1);
    } break;}
}
static CMD(varlist_f)
{
    int x = 0, n = 0, i = 0;
    const char *match = 0;
    if (argc > 1) match = argv[1];

    for (i = 0; i < MAX_VAR_HASH; ++i) {
        const struct var *var = 0;
        if (!g_varsys.vars[i]) continue;
        var = g_varsys.vars[i]; n++;
        if (match && !strfilter(var->name, match))
            continue;

        switch (var->type) {
        case VAR_INT: fprintf(stdout, "%s \"%d\" ", var->name, *var->val.i.val);
        case VAR_FLOAT: fprintf(stdout, "%s \"%.2f\" ", var->name, *var->val.f.val);
        case VAR_STRING: {
            fprintf(stdout, "%s \"%s\" ", var->name, var->val.s.buf);
        } break;
        case VAR_ENUM: {
            int sel = *var->val.e.sel;
            fprintf(stdout, "%s \"%s\" ", var->name, var->val.e.options[sel]);
        } break;}

        if (var->flags & VAR_ROM)
            fprintf(stdout, "R");
        if (var->flags & VAR_ARCHIVE)
            fprintf(stdout, "A");
        fprintf(stdout, "\n");
        x++;
    }
    fprintf(stdout, "---------------------------\n");
    fprintf(stdout, "%i total vars\n", n);
    fprintf(stdout, "%i var indexes\n", x);
    fprintf(stdout, "---------------------------\n");
}
static CMD(var_set_f)
{
    if (argc < 3) {
        fprintf(stdout, "usage: set <name> <value>\n");
        return;
    }
    var_set(&g_varsys, argv[1], argv[2]);
}
static void
var_init(struct varsys *v)
{
    zero(v, szof(*v));
    cmd_add(&g_cmdsys, "varlist", varlist_f);
    cmd_add(&g_cmdsys, "set", var_set_f);
}
