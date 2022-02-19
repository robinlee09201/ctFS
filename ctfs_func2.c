#include "ctfs.h"
#include "ctfs_pgg.h"
#include "ctfs_runtime.h"
// #define TEST_DRAM
#define FLUSH_ALIGN (uint64_t)64
#define ALIGN_MASK	(FLUSH_ALIGN - 1)

int ctfs_stat(const char *pathname, struct stat *buf){
    return ctfs_lstat (pathname, buf);
}

int ctfs_fstat(int fd, struct stat *buf){
    if(unlikely(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL)){
		ct_rt.errorn = EBADF;
		return -1;
	}
    if(unlikely(ct_rt.fd[fd].flags & O_WRONLY)){
		ct_rt.errorn = EBADF;
		return -1;
	}
    if(unlikely(buf == NULL)){
        ct_rt.errorn = EFAULT;
        return -1;
    }
    dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);

    ino_t inode_n = ct_rt.fd[fd].inode->i_number;
    ct_inode_pt c;
    inode_rt_lock(inode_n);

    c = &(ct_rt.inode_start[inode_n]);
    buf->st_ino = c->i_number;
    buf->st_mode = c->i_mode;
    buf->st_nlink = c->i_nlink;
    buf->st_uid = c->i_uid;
    buf->st_gid = c->i_gid;
    buf->st_size = c->i_size;
    buf->st_blksize = 4096;

    buf->st_atim = c->i_atim;
    buf->st_mtim = c->i_mtim;
    buf->st_ctim = c->i_ctim;
    dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
    inode_rt_unlock(inode_n);
    return 0;
}

int ctfs_lstat (const char *pathname, struct stat *buf){
    if(unlikely(buf == NULL)){
        ct_rt.errorn = EFAULT;
        return -1;
    }

    int res;
	ct_inode_pt c;
	ct_inode_frame_t frame = {.flag = 0, .path = pathname};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);

    if(*pathname != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
    res = inode_path2inode(&frame);
    if(res){
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        ct_rt.errorn = res;
        return -1;
    }

    c = frame.current;
    buf->st_ino = c->i_number;
    buf->st_mode = c->i_mode;
    buf->st_nlink = c->i_nlink;
    buf->st_uid = c->i_uid;
    buf->st_gid = c->i_gid;
    buf->st_size = c->i_size;
    buf->st_blksize = 4096;

    buf->st_atim = c->i_atim;
    buf->st_mtim = c->i_mtim;
    buf->st_ctim = c->i_ctim;

    dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
    inode_rt_unlock(frame.current->i_number);
    return 0;
}

// note:
// need to get the parent if current doesn't exist
// TODO HERE
int ctfs_link(const char *oldpath, const char *newpath){
    if(oldpath == NULL || *oldpath == '\0'){
        ct_rt.errorn = EPERM;
        return -1;
    }

    int res;
	ct_inode_pt c_old;
    ct_inode_frame_t old_frame = {.flag = 0, .path = oldpath};
	ct_inode_frame_t new_frame = {.flag = CT_INODE_FRAME_PARENT, .path = newpath};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);

    // get the inode and its info for old path
    if(*oldpath != '/'){
		// start from the current dir
		old_frame.inode_start = ct_rt.current_dir;
	}
    res = inode_path2inode(&old_frame);
    if(res != 0){
        ct_rt.errorn = res;
        return -1;
    }
    c_old = old_frame.current;
    if((c_old->i_mode & S_IFMT) == S_IFDIR){
        ct_rt.errorn = EPERM;
        return -1;
    }

    // now deal with the new path
    if(*newpath != '/'){
		// start from the current dir
		new_frame.inode_start = ct_rt.current_dir;
	}
    res = inode_path2inode(&new_frame);
    if(res != 0){  
        ct_rt.errorn = res;
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        return -1;
    }
    if(new_frame.parent == NULL || (new_frame.parent->i_mode & S_IFMT) == S_IFDIR ){
        ct_rt.errorn = ENOTDIR;
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        return -1;
    }
    
    // TODO HERE

    // inode_rt_unlock(frame.current->i_number);
    // inode_rt_unlock(frame.current->i_number);

    return -1;
}

int ctfs_unlink (const char *pathname){
    if(pathname == NULL || *pathname == '\0'){
        ct_rt.errorn = EPERM;
        return -1;
    }

    int res;
	ct_inode_pt c;
    ct_inode_pt parent_c;
	ct_inode_frame_t frame = {.flag = CT_INODE_FRAME_PARENT, .path = pathname};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);

    if(*pathname != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
    res = inode_path2inode(&frame);
    if(res != 0){
        ct_rt.errorn = res;
        return -1;
    }

    c = frame.current;
    parent_c = frame.parent;
    if(parent_c != NULL){
        parent_c->i_ndirent --;
    }
    // free the dirent of current pathname
    ct_dirent_pt current_dirent = frame.dirent;
    current_dirent->d_ino = 0;

    if(c->i_nlink == 1){
        inode_dealloc(c->i_number);
        if(c->i_level != PGG_LVL_NONE){
            pgg_deallocate(c->i_level, c->i_block);
        }
    }else{
        ct_time_stamp(&(c->i_mtim));
        c->i_nlink --;
    }

    inode_rt_unlock(frame.current->i_number);
    if((frame.flag & CT_INODE_FRAME_SAME_INODE_LOCK) == 0){
        inode_rt_unlock(frame.parent->i_number);
    }
    dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
    return 0;
}

int ctfs_rmdir(const char *pathname){
    if(pathname == NULL || *pathname == '\0'){
        ct_rt.errorn = EPERM;
        return -1;
    }

    int res;
	ct_inode_pt c, parent_c;
	ct_inode_frame_t frame = {.flag = CT_INODE_FRAME_PARENT, .path = pathname};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);

    if(*pathname != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
    res = inode_path2inode(&frame);
    if(res != 0){
        ct_rt.errorn = res;
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        return -1;
    }

    c = frame.current;
    if((c->i_mode & S_IFMT) != S_IFDIR){
        ct_rt.errorn = ENOTDIR;
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        return -1;
    }

    if(c->i_ndirent != 2){
        ct_rt.errorn = ENOTEMPTY;
        inode_rt_unlock(frame.current->i_number);
        if((frame.flag & CT_INODE_FRAME_SAME_INODE_LOCK) == 0){
            inode_rt_unlock(frame.parent->i_number);
        }
        dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
        return -1;
    }

    parent_c = frame.parent;
    if(parent_c != NULL){
        parent_c->i_ndirent --;
    }
    inode_dealloc(c->i_number);
    frame.dirent->d_ino = 0;
    
    inode_rt_unlock(frame.current->i_number);
    if((frame.flag & CT_INODE_FRAME_SAME_INODE_LOCK) == 0){
        inode_rt_unlock(frame.parent->i_number);
    }
    dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
    return 0;
}