#define _GNU_SOURCE
#define FUSE_USE_VERSION 34

#include <fuse_lowlevel.h>

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
    return 0;
}
