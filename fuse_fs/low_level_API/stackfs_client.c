#define _GNU_SOURCE
#define FUSE_USE_VERSION 34

#include <fuse_lowlevel.h>

void stackfs__init(void *userdata, struct fuse_conn_info *conn)
{
}

void stackfs__lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
}
void stackfs__getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
}
void stackfs__readlink(fuse_req_t req, fuse_ino_t ino)
{
}
int stackfs__opendir(const char *, struct fuse_file_info *)
{
    return 0;
}
int stackfs__readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
    return 0;
}
void stackfs__readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
}
void stackfs__releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
}
int stackfs__open(const char *, struct fuse_file_info *)
{
    return 0;
}
int stackfs__release(const char *, struct fuse_file_info *)
{
    return 0;
}
void stackfs__flush(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
}
void stackfs__read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
}
void stackfs__statfs(fuse_req_t req, fuse_ino_t ino)
{
}

static struct fuse_lowlevel_ops stackfs__op =
    {
        .init = stackfs__init,
        .lookup = stackfs__lookup,
        .getattr = stackfs__getattr,
        .readlink = stackfs__readlink,
        .opendir = stackfs__opendir,
        .readdir = stackfs__readdir,
        .readdirplus = stackfs__readdirplus,
        .releasedir = stackfs__releasedir,
        .open = stackfs__open,
        .release = stackfs__release,
        .flush = stackfs__flush,
        .read = stackfs__read,
        .statfs = stackfs__statfs,
};

int main(int argc, char const *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config config;
    struct lo_data lo = {.debug = 0,
                         .writeback = 0};

    int ret = -1;

    /* Don't mask creation mode, kernel already did that */
    umask(0);

    pthread_mutex_init(&lo.mutex, NULL);
    lo.root.next = lo.root.prev = &lo.root;
    lo.root.fd = -1;
    lo.cache = CACHE_NORMAL;

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;
    if (opts.show_help)
    {
        printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        passthrough_ll_help();
        ret = 0;
        goto err_out1;
    }
    else if (opts.show_version)
    {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        goto err_out1;
    }

    if (opts.mountpoint == NULL)
    {
        printf("usage: %s [options] <mountpoint>\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 1;
        goto err_out1;
    }

    if (fuse_opt_parse(&args, &lo, lo_opts, NULL) == -1)
        return 1;

    lo.debug = opts.debug;
    lo.root.refcount = 2;
    if (lo.source)
    {
        struct stat stat;
        int res;

        res = lstat(lo.source, &stat);
        if (res == -1)
        {
            fuse_log(FUSE_LOG_ERR, "failed to stat source (\"%s\"): %m\n",
                     lo.source);
            exit(1);
        }
        if (!S_ISDIR(stat.st_mode))
        {
            fuse_log(FUSE_LOG_ERR, "source is not a directory\n");
            exit(1);
        }
    }
    else
    {
        lo.source = "/";
    }
    if (!lo.timeout_set)
    {
        switch (lo.cache)
        {
        case CACHE_NEVER:
            lo.timeout = 0.0;
            break;

        case CACHE_NORMAL:
            lo.timeout = 1.0;
            break;

        case CACHE_ALWAYS:
            lo.timeout = 86400.0;
            break;
        }
    }
    else if (lo.timeout < 0)
    {
        fuse_log(FUSE_LOG_ERR, "timeout is negative (%lf)\n",
                 lo.timeout);
        exit(1);
    }

    lo.root.fd = open(lo.source, O_PATH);
    if (lo.root.fd == -1)
    {
        fuse_log(FUSE_LOG_ERR, "open(\"%s\", O_PATH): %m\n",
                 lo.source);
        exit(1);
    }

    se = fuse_session_new(&args, &lo_oper, sizeof(lo_oper), &lo);
    if (se == NULL)
        goto err_out1;

    if (fuse_set_signal_handlers(se) != 0)
        goto err_out2;

    if (fuse_session_mount(se, opts.mountpoint) != 0)
        goto err_out3;

    fuse_daemonize(opts.foreground);

    /* Block until ctrl+c or fusermount -u */
    if (opts.singlethread)
        ret = fuse_session_loop(se);
    else
    {
        config.clone_fd = opts.clone_fd;
        config.max_idle_threads = opts.max_idle_threads;
        ret = fuse_session_loop_mt(se, &config);
    }

    fuse_session_unmount(se);
err_out3:
    fuse_remove_signal_handlers(se);
err_out2:
    fuse_session_destroy(se);
err_out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);

    if (lo.root.fd >= 0)
        close(lo.root.fd);

    return ret ? 1 : 0;
    return 0;
}
