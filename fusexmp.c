/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#ifndef HAVE_UTIMENSAT
#define HAVE_UTIMENSAT
#endif

#ifndef HAVE_POSIX_FALLOCATE
#define HAVE_POSIX_FALLOCATE
#endif 

#ifndef HAVE_SETXATTR
#define HAVE_SETXATTR
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <linux/limits.h>


static struct {
  char driveA[512];
  char driveB[512];
} global_context;

static int xmp_getattr(const char *path, struct stat *stbuf)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);

  fprintf(stdout, "getattr: %s\n", fullpath);

	res = lstat(fullpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, mask) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, mask) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);

  fprintf(stdout, "access: %s\n", fullpath);

	res = access(fullpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  
  fprintf(stdout, "readlink: %s\n", fullpath);

	res = readlink(fullpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];

	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(opendir(tmpa) != NULL) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(opendir(tmpb) != NULL) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);
  fprintf(stdout, "readdir: %s\n", fullpath);

	dp = opendir(fullpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;

    fprintf(stdout, "directory: %s\n", de->d_name);

		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */

    fprintf(stdout, "mknod: %s\n", fullpath);

    if (S_ISREG(mode)) {
      res = open(fullpath, O_CREAT | O_EXCL | O_WRONLY, mode);
      if (res >= 0)
        res = close(res);
    } else if (S_ISFIFO(mode))
      res = mkfifo(fullpath, mode);
    else
      res = mknod(fullpath, mode, rdev);
    if (res == -1)
      return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  printf("%s\n",tmpa);
  printf("%s\n",tmpb);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else { 
  sprintf(fullpath, "%s%s", rand () % 2 == 0 ? global_context.driveA : global_context.driveB, path);
  }
    fprintf(stdout, "mkdir: %s\n", fullpath);

    res = mkdir(fullpath, mode);
    if (res == -1)
      return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  
    fprintf(stdout, "unlink: %s\n", fullpath);
    res = unlink(fullpath);
    if (res == -1)
      return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
	int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);

    fprintf(stdout, "rmdir: %s\n", fullpath);
    res = rmdir(fullpath);
    if (res == -1)
      return -errno;

  return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
  char read_fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  char write_fullpath[PATH_MAX];
  int res;
 
  sprintf(tmpa, "%s%s", global_context.driveA, from);
  sprintf(tmpb, "%s%s", global_context.driveB, from);
  if(access(tmpa, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveA, from);
   sprintf(write_fullpath, "%s%s", global_context.driveA, to);
  } else if(access(tmpb, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveB, from);
   sprintf(write_fullpath, "%s%s", global_context.driveB, to);
  } else { 
   sprintf(read_fullpath, "%s", from);
   sprintf(write_fullpath, "%s", to);
  }
  
  res = symlink(read_fullpath, write_fullpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rename(const char *from, const char *to)
{
  char read_fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  char write_fullpath[PATH_MAX];
  int res;
 
  sprintf(tmpa, "%s%s", global_context.driveA, from);
  sprintf(tmpb, "%s%s", global_context.driveB, from);
  if(access(tmpa, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveA, from);
   sprintf(write_fullpath, "%s%s", global_context.driveA, to);
  } else if(access(tmpb, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveB, from);
   sprintf(write_fullpath, "%s%s", global_context.driveB, to);
  } else { 
   sprintf(read_fullpath, "%s", from);
   sprintf(write_fullpath, "%s", to);
  }
  
  res = rename(read_fullpath, write_fullpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_link(const char *from, const char *to)
{
  char read_fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  char write_fullpath[PATH_MAX];
  int res;
 
  sprintf(tmpa, "%s%s", global_context.driveA, from);
  sprintf(tmpb, "%s%s", global_context.driveB, from);
  if(access(tmpa, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveA, from);
   sprintf(write_fullpath, "%s%s", global_context.driveA, to);
  } else if(access(tmpb, F_OK) == 0) {
   sprintf(read_fullpath, "%s%s", global_context.driveB, from);
   sprintf(write_fullpath, "%s%s", global_context.driveB, to);
  } else { 
   sprintf(read_fullpath, "%s", from);
   sprintf(write_fullpath, "%s", to);
  }
  
  res = link(read_fullpath, write_fullpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  res = chmod(fullpath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  res = lchown(fullpath, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  res = truncate(fullpath, size);
  if (res == -1)
    return -errno;

  return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);
    
  /* don't use utime/utimes since they follow symlinks */
  res = utimensat(0, fullpath, ts, AT_SYMLINK_NOFOLLOW);
  if (res == -1)
    return -errno;

  return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);
 
  fprintf(stdout, "open: %s\n", fullpath);

  res = open(fullpath, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int fd;
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "read: %s\n", fullpath);

  (void) fi;
  fd = open(fullpath, O_RDONLY);
  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);
  return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
    off_t offset, struct fuse_file_info *fi)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int fd;
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  (void) fi;

  fprintf(stdout, "write: %s\n", fullpath);

  fd = open(fullpath, O_WRONLY);
  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);

  return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int res;
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "statfs: %s\n", fullpath);

  res = statvfs(fullpath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) fi;
  return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
    struct fuse_file_info *fi)
{
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
    off_t offset, off_t length, struct fuse_file_info *fi)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  int fd;
  int res;
  
  (void) fi;
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  if (mode)
    return -EOPNOTSUPP;

    fprintf(stdout, "fullpath: %s\n", fullpath);

    fd = open(fullpath, O_WRONLY);
    if (fd == -1)
      return -errno;

    res = -posix_fallocate(fd, offset, length);

    close(fd);

  return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
    size_t size, int flags)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];

  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "setxattr: %s\n", fullpath);
  int res = lsetxattr(fullpath, name, value, size, flags);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
    size_t size)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
   
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "getxattr: %s\n", fullpath);
  int res = lgetxattr(fullpath, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "listxattr: %s\n", fullpath);
  int res = llistxattr(fullpath, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
  char fullpath[PATH_MAX], tmpa[PATH_MAX], tmpb[PATH_MAX];
  
  sprintf(tmpa, "%s%s", global_context.driveA, path);
  sprintf(tmpb, "%s%s", global_context.driveB, path);
  if(access(tmpa, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveA, path);
  else if(access(tmpb, F_OK) == 0) sprintf(fullpath, "%s%s", global_context.driveB, path);
  else sprintf(fullpath, "%s", path);

  fprintf(stdout, "removexattr: %s\n", fullpath);
  int res = lremovexattr(fullpath, name);
  if (res == -1)
    return -errno;

  return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
  .getattr	= xmp_getattr,
  .access		= xmp_access,
  .readlink	= xmp_readlink,
  .readdir	= xmp_readdir,
  .mknod		= xmp_mknod,
  .mkdir		= xmp_mkdir,
  .symlink	= xmp_symlink,
  .unlink		= xmp_unlink,
  .rmdir		= xmp_rmdir,
  .rename		= xmp_rename,
  .link		= xmp_link,
  .chmod		= xmp_chmod,
  .chown		= xmp_chown,
  .truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
  .utimens	= xmp_utimens,
#endif
  .open		= xmp_open,
  .read		= xmp_read,
  .write		= xmp_write,
  .statfs		= xmp_statfs,
  .release	= xmp_release,
  .fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
  .fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
  .setxattr	= xmp_setxattr,
  .getxattr	= xmp_getxattr,
  .listxattr	= xmp_listxattr,
  .removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
  if (argc < 4) {
    fprintf(stderr, "usage: ./myfs <mount-point> <drive-A> <drive-B>\n");
    exit(1);
  }

  strcpy(global_context.driveA, argv[--argc]);
  strcpy(global_context.driveB, argv[--argc]);

  srand(time(NULL));

  umask(0);
  return fuse_main(argc, argv, &xmp_oper, NULL);
}
