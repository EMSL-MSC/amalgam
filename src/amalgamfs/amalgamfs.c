/*
  Amalgamfs: Amalgam posix filesystem
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2009-2010  Kevin Fox <Kevin.Fox@pnl.gov>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.
*/

#define _GNU_SOURCE
#include <glib.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <switchuser.h>
#include <malloc.h>

#include <config.h>
#include "amalgamfs.h"
#include "tar.h"

static amalgamfs_config global_config = {};
static const char amalgamfs_version_string[]= "Version: " VERSION "-" SVNVER "\nBranch: " SVNBRANCH "\n";

typedef struct
{
	off_t offset;
	int dentry_real;
	int dentry_valid;
	struct dirent dentry;
	DIR *dir;
	char *data;
	size_t data_size;
	int fh;
} amalgamfs_fd;

//#include "debug.h"

#define EXTENSION ".amalgam"
#define TAR_EXTENSION ".tar"

static int root_fh = -1;

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int amalgamfs_getattr(const char *path, struct stat *st)
{
	char *file_path;
	guint64 size;
	int fd = 0;
	int len;
	int res;
	char *slash;
	for(; *path == '/'; path++);
	if(*path == '\0')
		path = ".";
	res = fstatat(root_fh, path, st, AT_SYMLINK_NOFOLLOW);
	len = strlen(path);
fprintf(stderr, "WAAAA %d %d\n\n\n", res, errno);
//FIXME bug in buhfs? ENOTDIR?
	if(res != 0 && (errno == ENOENT || errno == ENOTDIR) && len > strlen(TAR_EXTENSION) && !strcmp(path + len - strlen(TAR_EXTENSION), TAR_EXTENSION))
	{
//FIXME failure
		slash = strrchr(path, '/');
		file_path = malloc(slash - path + 1);
		strncpy(file_path, path, slash - path);
		file_path[slash - path] = '\0';
		res = fstatat(root_fh, file_path, st, AT_SYMLINK_NOFOLLOW);
		st->st_size = 0;
		fprintf(stderr, "Opening %s %d\n\n\n", path, root_fh);
		fd = openat(root_fh, file_path, O_RDONLY);
		if(fd < 0)
		{
			res = -errno;
		}
		else
		{
	//FIXME on failure?
			read(fd, &size, sizeof(guint64));
			st->st_size = GUINT64_FROM_LE(size);
			close(fd);
		}
		free(file_path);
	}
	else if(res == 0)
	{
		if(len > strlen(EXTENSION) && !strcmp(path + len - strlen(EXTENSION), EXTENSION))
		{
			st->st_nlink = 2;
			st->st_mode &= ~S_IFREG;
			st->st_mode |= S_IFDIR | S_IXUSR;
			if(st->st_mode & S_IRGRP)
			{
				st->st_mode |= S_IXGRP;
			}
		}
	}
	if(res == -1)
		res = -errno;
	return res;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
static int amalgamfs_readlink(const char *path, char *link, size_t size)
{
	int res;
	*link = '\0';
	for(; *path == '/'; path++);
	if(*path == '\0')
		path = ".";
	res = readlinkat(root_fh, path, link, size - 1);
	if(res == -1)
		res = -errno;
	else
		link[res] = '\0';
	return res;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
static int amalgamfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	return -EROFS;
}

/** Create a directory 
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
static int amalgamfs_mkdir(const char *path, mode_t mode)
{
	return -EROFS;
}

/** Remove a file */
static int amalgamfs_unlink(const char *path)
{
	return -EROFS;
}

/** Remove a directory */
static int amalgamfs_rmdir(const char *path)
{
	return -EROFS;
}

/** Create a symbolic link */
static int amalgamfs_symlink(const char *contents, const char *path)
{
	return -EROFS;
}

/** Rename a file */
static int amalgamfs_rename(const char *path, const char *new_path)
{
	return -EROFS;
}

/** Create a hard link to a file */
static int amalgamfs_link(const char *path, const char *new_path)
{
	return -EROFS;
}

/** Change the permission bits of a file */
static int amalgamfs_chmod(const char *path, mode_t mode)
{
	return -EROFS;
}

/** Change the owner and group of a file */
static int amalgamfs_chown(const char *path, uid_t uid, gid_t gid)
{
	return -EROFS;
}

/** Change the size of a file */
static int amalgamfs_truncate(const char *path, off_t offset)
{
	return -EROFS;
}

/** File open operation
 *
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * Changed in version 2.2
 */
static int amalgamfs_open(const char *path, struct fuse_file_info *fi)
{
	struct stat st;
	amalgamfs_fd *fd;
	int len;
	int res;
	char *file_path;
	char *slash;
	for(; *path == '/'; path++);
	if(*path == '\0')
		path = ".";
	fd = (amalgamfs_fd*)malloc(sizeof(amalgamfs_fd));
//FIXME error check.
	res = openat(root_fh, path, fi->flags);
//FIXME BUHFS?
	if(res == -1 && (errno == ENOENT || errno == ENOTDIR))
	{
		len = strlen(path);
		if(res != 0 && (errno == ENOENT || errno == ENOTDIR) && len > strlen(TAR_EXTENSION) && !strcmp(path + len - strlen(TAR_EXTENSION), TAR_EXTENSION))
		{
//FIXME failure
			slash = strrchr(path, '/');
			file_path = malloc(slash - path + 1);
			strncpy(file_path, path, slash - path);
			file_path[slash - path] = '\0';
			res = openat(root_fh, file_path, fi->flags);
			fprintf(stderr, "WARK! %s %d\n\n\n", file_path, res);
			if(res >= 0)
			{
//FIXME error check but be very careful of res
				fstat(res, &st);
				fd->data_size = st.st_size;
//FIXME error check.
				fd->data = (char*)mmap(NULL, fd->data_size, PROT_READ, MAP_PRIVATE, res, 0);
				if(fd->data == MAP_FAILED)
				{
fprintf(stderr, "MAP FAILED!\n");
				}
			}
			free(file_path);
		}
	}
	if(res == -1)
	{
		free(fd);
		res = -errno;
	}
	else
	{
		fi->fh = (long)fd;
		res = 0;
	}
	return res;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
static int amalgamfs_read(const char *path, char *buffer, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
	int res;
	amalgamfs_fd *fd = (amalgamfs_fd*)(fi->fh);
	if(fd->data)
	{
		fprintf(stderr, "GENREAD! %ld %ld\n\n\n", size, offset);
		res = amalgam_archive_read(fd->data, buffer, size, offset, AMALGAM_READ_TAR);
	}
	else
	{
		res = pread(fd->fh, buffer, size, offset);
		if(res == -1)
			res = -errno;
	}
	return res;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
static int amalgamfs_write(const char *path, const char *buffer, size_t size, off_t offset,
	      struct fuse_file_info *fi)
{
	return -EROFS;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
static int amalgamfs_statfs(const char *path, struct statvfs *st)
{
	int res;
//FIXME, prefix.
	res = statvfs(path, st);
	if(res == -1)
		res = -errno;
	return res;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().	This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.	It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
static int amalgamfs_flush(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
static int amalgamfs_release(const char *path, struct fuse_file_info *fi)
{
	int res = 0;
	amalgamfs_fd *fd = (amalgamfs_fd*)(fi->fh);
	if(fd)
	{
		if(fd->data)
		{
			res = munmap(fd->data, fd->data_size);
		}
//FIXME capture error better.
		res = close(fd->fh);
		if(res == -1)
		res = -errno;
	}
	return res;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
static int amalgamfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	return 0;
}

/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * Introduced in version 2.3
 */
static int amalgamfs_opendir(const char *path, struct fuse_file_info *fi)
{
	struct stat st;
	amalgamfs_fd *fd;
	int res;
	int len;
	fd = (amalgamfs_fd*)malloc(sizeof(amalgamfs_fd));
	if(!fd)
		res = -ENOMEM;
	else
	{
		for(; *path == '/'; path++);
		if(*path == '\0')
			path = ".";
		fi->fh = (long)fd;
		res = fstatat(root_fh, path, &st, AT_SYMLINK_NOFOLLOW);
		len = strlen(path);
		if(S_ISREG(st.st_mode) && len > strlen(EXTENSION) && !strcmp(path + strlen(path) - strlen(EXTENSION), EXTENSION))
		{
			fd->dentry_real = 0;
		}
		else
		{
			fd->dentry_real = 1;
			res = openat(root_fh, path, fi->flags);
			if(res == -1)
			{
				res = -errno;
				free(fd);
			}
			else
			{
				fd->offset = 0;
				fd->dentry_valid = 0;
				fd->dir = fdopendir(res);
				res = 0;
				if(!fd->dir)
				{
					res = -errno;
					close(res);
					free(fd);
				}
			}
		}
	}
	return res;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
static int amalgamfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{
	struct dirent *dentry;
	amalgamfs_fd *fd = (amalgamfs_fd*)(fi->fh);
	int len = 0;
	int res;
	struct stat s;
	char *slash;
	char *str;
	if(fd->dentry_real == 0)
	{
		slash = strrchr(path, '/');
		if(!slash)
		{
			return -EINVAL;
		}
		slash++;
		len = strlen(slash);
		if(!(len > strlen(EXTENSION) && !strcmp(slash + len - strlen(EXTENSION), EXTENSION)))
		{
			return -EINVAL;
		}
		str = (char *)malloc(len - strlen(EXTENSION) + strlen(TAR_EXTENSION));
		if(!str)
		{
			return -ENOMEM;
		}
		strcpy(str, slash);
		strcpy(str + len - strlen(EXTENSION), TAR_EXTENSION);
		filler(buffer, ".", NULL, 0);
		filler(buffer, "..", NULL, 0);
		filler(buffer, str, NULL, 0);
		free(str);
		return 0;
	}
	if(offset != fd->offset)
	{
			seekdir(fd->dir, offset);
			fd->offset = offset;
			fd->dentry_valid = 0;
	}
	while(1)
	{
		if(!fd->dentry_valid)
		{
			res = readdir_r(fd->dir, &(fd->dentry), &dentry);
			if(res != 0 || !dentry)
			{
				res = -res;
				break;
			}
		}
		s.st_mode = DTTOIF(fd->dentry.d_type);
		s.st_ino = fd->dentry.d_ino;
		len = strlen(fd->dentry.d_name);
		fprintf(stderr, "Foo: %s %d %d\n\n\n", fd->dentry.d_name, S_ISREG(s.st_mode), len);
		if(S_ISREG(s.st_mode) && len > strlen(EXTENSION) && !strcmp(fd->dentry.d_name + len - strlen(EXTENSION), EXTENSION))
		{
			s.st_mode &= ~S_IFREG;
			s.st_mode |= S_IFDIR;
		}
		if(filler(buffer, fd->dentry.d_name, &s, fd->dentry.d_off))
			break;
		fd->offset = fd->dentry.d_off;
		fd->dentry_valid = 0;
	}
	return res;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
static int amalgamfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	amalgamfs_fd *fd = (amalgamfs_fd*)(fi->fh);
	int res;
	if(fd && fd->dentry_real)
	{
		if(fd->dir)
		{
			closedir(fd->dir);
		}
		free(fd);
		fi->fh = 0;
	}
	return res;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
static void *amalgamfs_init(struct fuse_conn_info *conn)
{
	char *path = "/srv/amalgamfs/data";
	int res;
	if(global_config.workdir)
	{
		root_fh = open(global_config.workdir, O_RDONLY);
	}
	else
	{
		root_fh = open("/srv/amalgamfs/working", O_RDONLY);
	}
	if(root_fh == -1)
	{
		fprintf(stderr, "Failed to open root\n");
		exit(-1);
	}
	if(global_config.datadir)
	{
		path = global_config.datadir;
	}
	res = chdir(path);
	if(res)
	{
		fprintf(stderr, "Failed to chdir.");
		exit(-1);
	}
	return NULL;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
static void amalgamfs_destroy(void *data)
{
	close(root_fh);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
static int amalgamfs_access(const char *path, int mask)
{
	int res;
	for(; *path == '/'; path++);
	if(*path == '\0')
		path = ".";
	res = faccessat(root_fh, path, mask, 0);
	if(res == -1)
		res = -errno;
	return res;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
static int amalgamfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	return -EROFS;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
static int amalgamfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	return -EROFS;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
static int amalgamfs_fgetattr(const char *path, struct stat *st, struct fuse_file_info *fi)
{
//FIXME make better.
	return amalgamfs_getattr(path, st);
}

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * Introduced in version 2.6
 */
static int amalgamfs_utimens(const char *path, const struct timespec tv[2])
{
	return -EROFS;
}

static int amalgamfs_getxattr(const char *path, const char *name, char *value, size_t vlen)
{
	int res = -ENOTSUP;
	return res;
}

static int amalgamfs_setxattr(const char *path, const char *name, const char *value, size_t vlen, int flags)
{
	int res = -ENOTSUP;
	return res;
}

int amalgamfs_listxattr(const char *path, char *buffer, size_t size)
{
	int res = -ENOTSUP;
	return res;
}

struct fuse_operations amalgamfs_operations = {
	.getattr = amalgamfs_getattr,
	.readlink = amalgamfs_readlink,
	.mknod = amalgamfs_mknod,
	.mkdir = amalgamfs_mkdir,
	.unlink = amalgamfs_unlink,
	.rmdir = amalgamfs_rmdir,
	.symlink = amalgamfs_symlink,
	.rename = amalgamfs_rename,
	.link = amalgamfs_link,
	.chmod = amalgamfs_chmod,
	.chown = amalgamfs_chown,
	.truncate = amalgamfs_truncate,
	.open = amalgamfs_open,
	.read = amalgamfs_read,
	.write = amalgamfs_write,
	.statfs = amalgamfs_statfs,
	.flush = amalgamfs_flush,
	.release = amalgamfs_release,
	.fsync = amalgamfs_fsync,
	.opendir = amalgamfs_opendir,
	.readdir = amalgamfs_readdir,
	.releasedir = amalgamfs_releasedir,
	.init = amalgamfs_init,
	.destroy = amalgamfs_destroy,
	.access = amalgamfs_access,
	.create = amalgamfs_create,
	.ftruncate = amalgamfs_ftruncate,
	.fgetattr = amalgamfs_fgetattr,
	.utimens = amalgamfs_utimens,
	.getxattr = amalgamfs_getxattr,
	.setxattr = amalgamfs_setxattr,
	.listxattr = amalgamfs_listxattr,
};

/*
FIXME Copy/pasted from debug.c
*/
int debug_super_loglevel;

void *d_specific_get()
{
	return NULL;
}
/*
FIXME Copy/pasted from buhfs.
*/
typedef enum
{
	TARFS_OPTKEY_VERSION,
	TARFS_OPTKEY_HELP,
} amalgamfs_opts_keytype;

int amalgamfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	//d_enter();
	switch(key)
	{
		case TARFS_OPTKEY_VERSION:
			printf("%s", amalgamfs_version_string);
			exit(0);
		case TARFS_OPTKEY_HELP:
			fprintf(stderr, "Usage: %s mountpoint [options]\n\n", outargs->argv[0]);
			fprintf(stderr, "Amalgamfs Options:\n"
					"    --help                 show this info.\n"
					"    --version              display the version.\n"
					"    -o datadir             specify a subdirectory where raw data is read from.\n"
					"    -o workdir             specify a subdirectory where amalgam files are read from.\n"
					"    -o switchuser          specify a unix user to run this file system as.\n"
					"\n");
			fuse_opt_add_arg(outargs, "-ho");
			fuse_main(outargs->argc, outargs->argv, &amalgamfs_operations, NULL);
			//d_exit();
			exit(1);
	}
	//d_return(1);
	return(1);
}

struct fuse_opt global_amalgamfs_opts[] =
{
#define AMALGAMFS_OPT(target, name, value) { target, offsetof(amalgamfs_config, name), value }
	AMALGAMFS_OPT("switchuser=%s" , switchuser, 0),
	AMALGAMFS_OPT("datadir=%s" , datadir, 0),
	AMALGAMFS_OPT("workdir=%s" , workdir, 0),
	FUSE_OPT_KEY("--help" , TARFS_OPTKEY_HELP),
	FUSE_OPT_KEY("-h" , TARFS_OPTKEY_HELP),
	FUSE_OPT_KEY("--version" , TARFS_OPTKEY_VERSION),
	FUSE_OPT_KEY("-V" , TARFS_OPTKEY_VERSION),
	FUSE_OPT_END
};

amalgamfs_config *amalgamfs_config_get()
{
	return &(global_config);
}
