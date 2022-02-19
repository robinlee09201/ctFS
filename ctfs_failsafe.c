#include "ctfs_failsafe.h"

uint64_t find_free_frame() {
	uint64_t ret = ct_rt.failsafe_clock;
	while(ct_rt.failsafe_frame[ret].stage != 0){
		ret++;
		if(ret >= CT_FAILSAFE_NFRAMES){
			ret = 0;
		}
	}
	ct_rt.failsafe_clock = (ret + 1 >= CT_FAILSAFE_NFRAMES) ? 0 :ret + 1;
	return ret;
}


/***********************************************
 * File Create:
 * 1. Allocate an inode
 * 2. Allocate a pgg
 * 3. Clear pgg's parents
 * 4. Fill the parent inode's dirent 
 **********************************************/

failsafe_frame_t * failsafe_file_create_1(index_t inode, index_t parent_inode){
	uint64_t ret = find_free_frame();
	ct_rt.failsafe_frame[ret].op = FAILSAFE_FILE_CREATE;
	ct_rt.failsafe_frame[ret].inode_num = inode;
	ct_rt.failsafe_frame[ret].parent_inode_num = parent_inode;
	ct_rt.failsafe_frame[ret].stage = 1;
	return &ct_rt.failsafe_frame[ret];
}

void failsafe_file_create_2(failsafe_frame_t * frame, relptr_t pgg_header){
	frame->pgg_header = pgg_header;
	frame->stage = 2;
}

void failsafe_file_create_3(failsafe_frame_t * frame){
	frame->stage = 3;
}

void failsafe_file_create_4(failsafe_frame_t * frame){
	frame->stage = 0;
}


/***********************************************
 * File Delete:
 * 1. Unlink from dirent
 * 2. Deallocate inode
 * 3. Deallocate pgg
 * 4. Clear pgg's parents 
 **********************************************/

failsafe_frame_t * failsafe_file_delete_1(index_t inode, index_t parent_inode, relptr_t pgg_header){
	uint64_t ret = find_free_frame();
	ct_rt.failsafe_frame[ret].op = FAILSAFE_FILE_DELETE;
	ct_rt.failsafe_frame[ret].inode_num = inode;
	ct_rt.failsafe_frame[ret].parent_inode_num = parent_inode;
	ct_rt.failsafe_frame[ret].pgg_header = pgg_header;
	ct_rt.failsafe_frame[ret].stage = 1;
	return &ct_rt.failsafe_frame[ret];
}

void failsafe_file_delete_2(failsafe_frame_t * frame){
	frame->stage = 2;
}

void failsafe_file_delete_3(failsafe_frame_t * frame){
	frame->stage = 3;
}

void failsafe_file_delete_4(failsafe_frame_t * frame){
	frame->stage = 0;
}

/***********************************************
 * Inode modify:
 * 1. Begin
 * 2. End
 **********************************************/

failsafe_frame_t * failsafe_inode_mod_1(index_t inode){
	uint64_t ret = find_free_frame();
	ct_rt.failsafe_frame[ret].op = FAILSAFE_INODE_MOD;
	ct_rt.failsafe_frame[ret].inode_num = inode;
	ct_rt.failsafe_frame[ret].stage = 1;
	return &ct_rt.failsafe_frame[ret];
}

void failsafe_inode_mod_2(failsafe_frame_t * frame){
	frame->stage = 0;
}

/***********************************************
 * PGG upgrade:
 * 1. Find new home
 * 2. pswap
 * 3. Record the change in inode
 **********************************************/

failsafe_frame_t * failsafe_pgg_upgrade_1(index_t inode, relptr_t pgg_header){
	uint64_t ret = find_free_frame();
	ct_rt.failsafe_frame[ret].op = FAILSAFE_PGG_UPGRADE;
	ct_rt.failsafe_frame[ret].inode_num = inode;
	ct_rt.failsafe_frame[ret].pgg_header = pgg_header;
	ct_rt.failsafe_frame[ret].stage = 1;
	return &ct_rt.failsafe_frame[ret];
}

void failsafe_pgg_upgrad_3(failsafe_frame_t * frame){
	frame->stage = 0;
}

/***********************************************
 * Strict write allocate:
 * 1. Find a pgg
 * 2. Copy and pswap
 * 3. Deallocate
 **********************************************/

failsafe_frame_t * failsafe_write_alloc_1(relptr_t pgg_header){
	uint64_t ret = find_free_frame();
	ct_rt.failsafe_frame[ret].op = FAILSAFE_WRITE_ALLOC;
	ct_rt.failsafe_frame[ret].pgg_header = pgg_header;
	ct_rt.failsafe_frame[ret].stage = 1;
	return &ct_rt.failsafe_frame[ret];
}

void failsafe_write_alloc_3(failsafe_frame_t * frame){
	frame->stage = 0;
}
