#ifndef CTFS_FAILSAFE_H
#define CTFS_FAILSAFE_H
#include "ctfs_runtime.h"

typedef enum failsafe_op {
	FAILSAFE_FILE_CREATE,
	FAILSAFE_FILE_DELETE,
	FAILSAFE_INODE_MOD,
	FAILSAFE_PGG_UPGRADE,
	FAILSAFE_PGG_DOWNGRADE,
	FAILSAFE_WRITE_ALLOC,
	FAILSAFE_SUPER_MOD
} failsafe_op_t;

typedef struct failsafe_frame {
	failsafe_op_t op;
	uint8_t stage;
	relptr_t pgg_header;
	index_t inode_num;
	index_t parent_inode_num;
} failsafe_frame_t;

failsafe_frame_t * failsafe_file_create_1(index_t inode, index_t parent_inode);

void failsafe_file_create_2(failsafe_frame_t * frame, relptr_t pgg_header);

void failsafe_file_create_3(failsafe_frame_t * frame);

void failsafe_file_create_4(failsafe_frame_t * frame);

failsafe_frame_t * failsafe_file_delete_1(index_t inode, index_t parent_inode, relptr_t pgg_header);

void failsafe_file_delete_2(failsafe_frame_t * frame);

void failsafe_file_delete_3(failsafe_frame_t * frame);

void failsafe_file_delete_4(failsafe_frame_t * frame);

failsafe_frame_t * failsafe_inode_mod_1(index_t inode);

void failsafe_inode_mod_2(failsafe_frame_t * frame);

failsafe_frame_t * failsafe_pgg_upgrade_1(index_t inode, relptr_t pgg_header);

void failsafe_pgg_upgrad_3(failsafe_frame_t * frame);

failsafe_frame_t * failsafe_write_alloc_1(relptr_t pgg_header);

void failsafe_write_alloc_3(failsafe_frame_t * frame);

#endif