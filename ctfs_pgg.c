/*****************************
 * 
 * s
 * 
 ****************************/
#include "ctfs_pgg.h"


const uint64_t pgg_size[10] = {
	CT_PGGSIZE_LV0,
	CT_PGGSIZE_LV1,
	CT_PGGSIZE_LV2,
	CT_PGGSIZE_LV3,
	CT_PGGSIZE_LV4,
	CT_PGGSIZE_LV5,
	CT_PGGSIZE_LV6,
	CT_PGGSIZE_LV7,
	CT_PGGSIZE_LV8,
	CT_PGGSIZE_LV9
};

const uint64_t pgg_limit[10] = {
	CT_LIMITSIZE_LV0,
	CT_LIMITSIZE_LV1,
	CT_LIMITSIZE_LV2,
	CT_LIMITSIZE_LV3,
	CT_LIMITSIZE_LV4,
	CT_LIMITSIZE_LV5,
	CT_LIMITSIZE_LV6,
	CT_LIMITSIZE_LV7,
	CT_LIMITSIZE_LV8,
	CT_LIMITSIZE_LV9
};

const uint64_t pgg_subpmd_count_per_pkg[3] = {
	512, 
	64,
	8
};

/* get the level of page group
 * given the size of the file
 * @param[in] size of the file
 * @return level of page group
 */
pgg_level_t pgg_get_lvl(uint64_t size){
	if(size > CT_LIMITSIZE_LV4){
		if(size > CT_LIMITSIZE_LV6){
			if(size > CT_LIMITSIZE_LV7){
				if(size > CT_LIMITSIZE_LV8){
					if(size > CT_LIMITSIZE_LV9){
						return PGG_LVL10;
					}
					else{
						return PGG_LVL9;
					}
				}
				else{
					return PGG_LVL8;
				}
			}
			else{
				return PGG_LVL7;
			}
		}
		else{
			if(size > CT_LIMITSIZE_LV5){
				return PGG_LVL6;
			}
			else{
				return PGG_LVL5;
			}
		}
	}
	else{
		if(size > CT_LIMITSIZE_LV1){
			if(size > CT_LIMITSIZE_LV2){
				if(size > CT_LIMITSIZE_LV3){
					return PGG_LVL4;
				}
				else{
					return PGG_LVL3;
				}
			}
			else{
				return PGG_LVL2;
			}
		}
		else{
			if(size > CT_LIMITSIZE_LV0){
				return PGG_LVL1;
			}
			else{
				return PGG_LVL0;
			}
		}
	}
}

/* report a file allocation 
 * to the allocation protector
 * @param[in] header 
 * @param[in] target
 */
void pgg_alloc_prot_file_add(pgg_header_pt header, relptr_t target){
	int64_t index;
retry:
	index = find_free_bit(&ct_rt.super_blk->alloc_prot_bmp, 
		CT_ALLOC_PROT_SIZE, ct_rt.super_blk->alloc_prot_clock + 1);
	if(index == -1){
		goto retry;
	}
	ct_rt.alloc_prot[index].header = CT_ABS2REL(header);
	ct_rt.alloc_prot[index].target = CT_ABS2REL(target);
	set_bit(&ct_rt.super_blk->alloc_prot_bmp, index);
	ct_rt.super_blk->alloc_prot_clock = index;
	cache_wb_one(&ct_rt.alloc_prot[index]);
	cache_wb_one(&ct_rt.super_blk->alloc_prot_bmp);
}

/* provide the sub_pmd package level
 * for next sub_pmd package
 * allocation. 
 * @return level
 */
pgg_level_t __pgg_sub_pmd_lvl_hint(){
	pgg_level_t ret = ct_rt.super_blk->next_sub_lvl;
	if(ret == PGG_LVL2){
		ct_rt.super_blk->next_sub_lvl = PGG_LVL0;
	}
	else{
		ct_rt.super_blk->next_sub_lvl++;
	}
	return ret;
}

/* build a sub pmd header at given
 * level. update all the sub_pmd_cap
 * in the header group.
 * @param[in] parent, a lvl4 header
 * @param[in] index
 * @param[in] level
 */
void __pgg_build_sub_pmd(pgg_header_pt parent, uint8_t index, pgg_level_t level){
	assert(parent->level == PGG_LVL4);
	assert(index < 8);
	assert(level < 3);
	pgg_hd_group_pt parent_group = PGG_HEADER2GROUP(parent);
	pgg_hd_group_pt target_group = (pgg_hd_group_pt)((uint64_t)parent_group + (index * pgg_size[3]));
	pgg_subpmd_header_pt target = &target_group->subpmd_header;
	memset(target, 0, sizeof(pgg_subpmd_header_t));
	target->level = level;
	target->bitmap_hint = 1;
	target->bitmap[0] = 0b01;
	target->taken = 1;
	target->parent = CT_ABS2REL(parent);
	// flush cache
	cache_wb_one(target);

	//handle upper level
	pgg_header_pt header = parent;
	PGG_STATE_STORE(parent->pmd_type, index, level);
	cache_wb_one(parent);
	while(PGG_SUBPMD_PRF_TEST(header->sub_pmd_cap, level) == 0){
#ifdef CTFS_DEBUG
		pgg_header_t hd2 = *header;
#endif
		header->sub_pmd_cap |= 0b010001 << level;
		// flush cache
		cache_wb_one(header);
		if(header->parent_pgg != 0){
#ifdef CTFS_DEBUG
			pgg_header_t par = *header;
#endif
			header = CT_REL2ABS(header->parent_pgg);
			// goto the upper level
#ifdef CTFS_DEBUG
			pgg_header_t hd2 = *header;
#endif
		}
		else{
			assert(header->level == PGG_LVL9);
			break;
		}
	}
}

/* check the availability of current 
 * pgg. if the pgg becomes full, 
 * update the capability of higher level.
 * @param[in] header
 */
void __pgg_update_cap_alloc(pgg_header_pt header){
	pgg_header_pt current_header = header;
	pgg_level_t current_lvl = header->level;
	pgg_level_t max_cap = 0;

	// stop when current header doesn't need to update or max_level tops
	while(current_lvl != 9){
		max_cap = 0;
		pgg_level_t old_cap = current_header->cap_lvl;
		// check the maximum capability level of all 8 blocks (including first sub ppg)
		for(uint16_t i=0; i<8; i++){
			if(PGG_STATE_LOAD(current_header->state_map, i) == PGG_STATE_EMPTY){
				// if any of the 8 blocks is empty, level - 1 is definitely avaliable, no change is required
				return;
			}else if(current_lvl != PGG_LVL4 && PGG_STATE_LOAD(current_header->state_map, i) == PGG_STATE_SUB){
				// if the block is used as sub ppg, we check the capability and pass to parent
				pgg_header_pt child = PGG_GROUP_AT2HEADER(PGG_HEADER2GROUP(current_header), current_header->level - 1, i);
				max_cap = (max_cap > child->cap_lvl)? max_cap : child->cap_lvl;
			}else{
				// if the block is used as data, then we pass
				continue;
			}
		}
#ifdef CTFS_DEBUG
		ctfs_debug_temp = current_header->state_map;
#endif
		if(old_cap == max_cap){
			return;
		}else{
			current_header->cap_lvl = max_cap;
			cache_wb_one(current_header);
		}
#if CTFS_DEBUG > 0
		pgg_header_t h = *current_header;
		pgg_header_t h2 = *(pgg_header_t*)CT_REL2ABS(current_header->parent_pgg);
#endif
		current_header = CT_REL2ABS(current_header->parent_pgg);
		assert(current_lvl == current_header->level - 1);
		current_lvl ++;
	}
}

/* check the availability of current 
 * pgg. if the pgg becomes not full, 
 * update the capability of higher level.
 * @param[in] header
 */
void __pgg_update_cap_dealloc(pgg_header_pt header){
	if(unlikely(header->cap_lvl != header->level - 1)){
		assert(header->cap_lvl < header->level -1);
		pgg_header_pt current_header = header;
		pgg_level_t current_lvl = header->level;
		pgg_level_t min_cap = current_lvl - 1;
		header->cap_lvl = min_cap;
		cache_wb_one(header);

		// stop when current header doesn't need to update or max_level tops
		while(current_lvl != 8){
			current_header = CT_REL2ABS(current_header->parent_pgg);
#ifdef CTFS_DEBUG
			pgg_header_t hd = *current_header;
#endif
			assert(current_lvl + 1 == current_header->level);
			current_lvl ++;

			if(current_header->cap_lvl >= min_cap){
				return;
			}
			current_header->cap_lvl = min_cap;
			cache_wb_one(current_header);
		}
	}
	else{
		return;
	}
}

/* Build new sub page group.
 * 1. fill the header and all the
 *  lower-level headers
 * 2. construct a sub-pmd package 
 *  according to the hint in 
 *  super block.
 * @param[in] header
 * @param[in] parent_header
 * @param[in] level
 */
void __pgg_new_subpgg(pgg_header_pt header, pgg_header_pt parent_header, pgg_level_t level){
	assert(level <= 9);
	pgg_hd_group_pt group = PGG_HEADER2GROUP(header);
	/* create sub-pmd package */
	/* Fill the headers */
	pgg_header_pt hd = NULL;
	for(uint16_t i=level; i>3; i-- ){
		hd = &group->header[9-i];
		memset(hd, 0, sizeof(pgg_header_t));
		hd->parent_pgg = (i == 9) ? 0 : CT_ABS2REL(&group->header[9-i-1]);
		hd->state_map = PGG_STATE_INIT;
		hd->cap_lvl = i - 1;
		hd->level = i;    
		cache_wb_one(hd);
	}
	/* create sub-pmd package */
	assert(hd->level == PGG_LVL4);
	header->parent_pgg = CT_ABS2REL(parent_header);
	__pgg_build_sub_pmd(hd, 0, __pgg_sub_pmd_lvl_hint());
	assert(header->level == level);
	
	// flush cache
	cache_wb_one(header);
	
}

/* Allocate a lvl3 and upper file
 * It's a recursive function
 * which will return at the level
 * it requests. 
 * @param[in]   level
 * @param[in]   header
 * @return      relative pointer to the file
 */
relptr_t __pgg_allocate_big(pgg_level_t level, pgg_header_pt header){
	if(level +1 == header->level){
		// Just find an empty spot.
		uint16_t map = header->state_map;
		for(uint16_t i=1; i<8; i++){
			if(PGG_STATE_LOAD(map, i) == PGG_STATE_EMPTY){
				relptr_t ret = CT_ABS2REL((uint64_t)header + (pgg_size[level] * i));
				// report to allocation protector
				// pgg_alloc_prot_file_add(header, ret);
				PGG_STATE_STORE(header->state_map, i, PGG_STATE_FILE);
#ifdef CTFS_DEBUG
				ctfs_debug_temp  = header->state_map;
#endif			
				__pgg_update_cap_alloc(header);
				return ret;
			}
		}
		/*********BUG!!!!!!********/
	}
	else{
		uint16_t map = header->state_map;
		for(uint16_t i=0; i<8; i++){
			if(PGG_STATE_LOAD(map, i) == PGG_STATE_SUB){
				pgg_header_pt child = PGG_GROUP_AT2HEADER(PGG_HEADER2GROUP(header),header->level-1, i);
				assert(child->level == header->level - 1);
#ifdef CTFS_DEBUG
				pgg_hd_group_t gp2= *PGG_HEADER2GROUP(header);
				pgg_header_t hd2 = *header;
				pgg_header_t chd = *child;
#endif
				if(child->cap_lvl < level){
					continue;
				}

				return __pgg_allocate_big(level, child);
			}
		}
		// no existing sub pgg available. create a new one.
		for(uint16_t i=1; i<8; i++){
			if(PGG_STATE_LOAD(map, i) == PGG_STATE_EMPTY){
				pgg_level_t child_lvl = header->level - 1;
				pgg_header_pt child = PGG_GROUP_AT2HEADER(PGG_HEADER2GROUP(header),child_lvl, i);
				__pgg_new_subpgg(child, header, child_lvl);
				assert(child->level == header->level - 1);
				header->state_map |= PGG_STATE_SUB << (2*i);
				cache_wb_one(header);
				__pgg_update_cap_alloc(header);
				return __pgg_allocate_big(level, child);
			}
		}
		/*********BUG!!!!!!********/
	}
	// pgg_hd_group_t gp = *PGG_HEADER2GROUP(header);
	assert(0);
	return 0;
}

/* helper for __pgg_allocate_small
 * alocate a sub_pmd file
 * given a subpmd header
 * @param[in]   level
 * @param[in]   header
 * @return      relative pointer to the target
 */
relptr_t __pgg_sub_pmd_alloc(pgg_level_t level, pgg_subpmd_header_pt header){
	assert(level < PGG_LVL3);
	assert(level == header->level);
	assert(header->taken < pgg_subpmd_count_per_pkg[level]);
	int64_t target = (level == PGG_LVL2) ? find_free_bit_tiny(header->bitmap, pgg_subpmd_count_per_pkg[level]) :
	find_free_bit(header->bitmap, pgg_subpmd_count_per_pkg[level], header->bitmap_hint);

	assert(target != -1);
	relptr_t ret = CT_ABS2REL((uint64_t)header + (pgg_size[level] * target));
	set_bit(header->bitmap, (size_t)target);
	header->bitmap_hint = (uint8_t)target;
	// pgg_alloc_prot_file_add(header, ret);
	// !!!!Need rethink above! !!!!!!!!!!!!!
	// How to protect sub_pmd allocation
	header->taken ++;
#ifdef DAX_DEBUGGING
		pgg_subpmd_header_t gr = *header;
#endif
	cache_wb_one(header);
	return ret;
}

/* sub_pmd file allocation
 * three scenario:
 * 1. found a preferred pgg
 * 2. found an empty lvl3 pgg
 * 3. found an empty upper pgg
 * @param[in]   level
 * @param[in]   header
 * @return      relative pointer to the target
 */
relptr_t __pgg_allocate_small(pgg_level_t level, pgg_header_pt header){
	assert(level < PGG_LVL3);
	pgg_hd_group_pt group;
	pgg_header_pt child;
	pgg_subpmd_header_pt target = NULL;
	int found;
	if(PGG_SUBPMD_PRF_TEST(header->sub_pmd_cap, level)){
		// case: preferred
		while(1){
			// iterate each layer
			found = 0;
			group = PGG_HEADER2GROUP(header);
#ifdef DAX_DEBUGGING
		pgg_hd_group_t gr = *group;
#endif
			for(uint16_t i=0; i<8; i++){
				if(PGG_STATE_LOAD(header->state_map, i) == PGG_STATE_SUB){
					child = PGG_GROUP_AT2HEADER(group, header->level - 1, i);
					if(PGG_SUBPMD_PRF_TEST(child->sub_pmd_cap, level)){
						header = child;
						found = 1;
						break;                        
					}
				}
			}
			assert(found);
			if(header->level == PGG_LVL4){
				break;
			}
		}
		for(uint16_t i=0; i<8; i++){
			if(PGG_STATE_LOAD(header->state_map, i) && 
			PGG_STATE_LOAD(header->pmd_type, i) == level){
				group = PGG_HEADER2GROUP(header);
				group = PGG_GROUP_AT(group, PGG_LVL3, i);
				target = &group->subpmd_header;
				break;
			}
		}
#ifdef DAX_DEBUGGING
		pgg_hd_group_t gr = *group;
#endif
		assert(target!=NULL);
		// Now we have the target header
	}
	else if(header->cap_lvl >= PGG_LVL3){
		// case: available. Should find one lvl3
		while(1){
			// iterate each layer
			found = 0;
			group = PGG_HEADER2GROUP(header);
#ifdef DAX_DEBUGGING
		pgg_hd_group_t gr = *group;
#endif
			for(uint16_t i=0; i<8; i++){
				if(PGG_STATE_LOAD(header->state_map, i) == PGG_STATE_SUB){
					child = PGG_GROUP_AT2HEADER(group, header->level - 1, i);
					if(child->cap_lvl >= PGG_LVL3){
						header = child;
						found = 1;
						break;
					}
				}
			}
			assert(found);
#ifdef DAX_DEBUGGING
		gr = *group;
#endif
			if(header->level == PGG_LVL4){
				break;
			}
		}
		//now we have a lvl4 that contains an empty pgg
		for(uint16_t i=1; i<8; i++){
			if(PGG_STATE_LOAD(header->state_map, i) == PGG_STATE_EMPTY){
				__pgg_build_sub_pmd(header, i, level);
				group = PGG_HEADER2GROUP(header);
				group = PGG_GROUP_AT(group, PGG_LVL3, i);
				target = &group->subpmd_header;
				break;
			}
		}
		assert(target!=NULL);
	}
	else{
		return 0;
	}
	// Target header is ready here
	relptr_t ret = __pgg_sub_pmd_alloc(level, target);
	if(target->taken == pgg_subpmd_count_per_pkg[level]){
		//need update cap
		pgg_header_pt current;
		pgg_hd_group_pt hdg;
		uint16_t need_act = 1;
		hdg = PGG_REL2HD_GROUP(ret, PGG_LVL4);
		current = &hdg->header[9 - PGG_LVL4];
		assert(current->cap_lvl >= PGG_LVL3);
		// check for availiblity
		// TBD below
		for(uint16_t i = 0; i < 8; i++){
			if(PGG_STATE_LOAD(current->state_map, i) == PGG_STATE_SUB){
				pgg_subpmd_header_pt sp_header;
				sp_header = &PGG_GROUP_AT(hdg, PGG_LVL3, i)->subpmd_header;
				assert(PGG_STATE_LOAD(current->pmd_type, i) == sp_header->level);
				if(sp_header->level == level && sp_header->taken != pgg_subpmd_count_per_pkg[level]){
					// still space left
					// stay same
					need_act = 0;
					break;
				}
			}
		}
		if(need_act){
			// clear bits
			current->sub_pmd_cap &= ~(0b010001 << level);
			cache_wb_one(current);
			for(pgg_level_t lvl = 5; lvl <= 9; lvl++){
				hdg = PGG_REL2HD_GROUP(ret, lvl);
				for(uint16_t i=0; i<8; i++){
					current = &PGG_GROUP_AT(hdg, lvl - 1, i)->header[9-(lvl-1)];
					assert(current->level == lvl - 1);
					// TBD
				}
			}
		}
		
		
	}
	return ret;
}

relptr_t pgg_allocate(pgg_level_t level){
	relptr_t ret;
	bitlock_acquire(&ct_rt.pgg_lock, 0);
	if(level <= PGG_LVL2){
		ret = __pgg_allocate_small(level, &ct_rt.first_pgg->header[0]);
#if CTFS_DEBUG > 0
		printf("\tallocated lvl %d @0x%lx\n", ret);
#endif
		bitlock_release(&ct_rt.pgg_lock, 0);
		return ret;
	}
	else if(level > PGG_LVL2 && level <= PGG_LVL9){
		ret = __pgg_allocate_big(level, &ct_rt.first_pgg->header[0]) & PAGE_MASK;
#if CTFS_DEBUG > 0
		printf("\tallocated lvl %d @0x%lx\n", level, ret);
#endif
		bitlock_release(&ct_rt.pgg_lock, 0);
		return ret;
	}
	else{
		// TBD: file > 512G
		bitlock_release(&ct_rt.pgg_lock, 0);
	}
	bitlock_release(&ct_rt.pgg_lock, 0);
	return 0;
}

/* deallocate one file
 * @param[in] level
 * @param[in] target
 */
void pgg_deallocate(pgg_level_t level, relptr_t target){
	bitlock_acquire(&ct_rt.pgg_lock, 0);
	if(level > PGG_LVL2){
		// PMD and above
		pgg_header_pt header = &PGG_REL2HD_GROUP(target, level + 1) ->header[9 - (level + 1)];
#ifdef CTFS_DEBUG
		pgg_hd_group_t grp = *PGG_REL2HD_GROUP(target, level + 1);
		pgg_header_t hdr = *header;
		printf("------deallocated lvl %d @0x%lx\n", level, target);
#endif
		assert(header->level == level + 1);
		uint8_t index = PGG_BIGFILE2INDEX(target, level);
#ifdef CTFS_DEBUG
		ctfs_debug_temp  = header->state_map;
#endif
		assert(PGG_STATE_LOAD(header->state_map, index) == PGG_STATE_FILE);
		PGG_STATE_STORE(header->state_map, index, PGG_STATE_EMPTY);

		// check if it was full
		__pgg_update_cap_dealloc(header);
	}
	else{
		// sub PMD
		pgg_subpmd_header_pt header = &PGG_REL2HD_GROUP(target, PGG_LVL3)->subpmd_header;
		uint16_t index = PGG_SMALLFILE2INDEX(target, header->level);
		clear_bit(header->bitmap, index);
		if(index < header->bitmap_hint){
			header->bitmap_hint = index;
		}
		header->taken --;
		cache_wb_one(header);
		pgg_hd_group_pt hdg = PGG_REL2HD_GROUP(target, PGG_LVL4);
		// uint8_t hd_index = PGG_BIGFILE2INDEX(target, PGG_LVL3);
		if(hdg->header[5].cap_lvl < PGG_LVL3){
			// it was full
			assert(PGG_SUBPMD_PRF_TEST(hdg->header[5].sub_pmd_cap, level) == 0);
			for(pgg_level_t lvl = PGG_LVL4; lvl < PGG_LVL10; lvl++){
				pgg_header_pt current = &PGG_REL2HD_GROUP(target, lvl)->header[lvl];
				assert(current->level == lvl);
				if(current->cap_lvl >= PGG_LVL3){
					assert(PGG_SUBPMD_PRF_TEST(current->sub_pmd_cap, level) != 0);
					break;
				}
				else{
					assert(PGG_SUBPMD_PRF_TEST(current->sub_pmd_cap, level) == 0);
					current->sub_pmd_cap |= 0b010001 << level;
					cache_wb_one(current);
				}
			}
		}
	}
	bitlock_release(&ct_rt.pgg_lock, 0);
}

/* Called for mkfs
 * After super block
 * is initialized.
 * @return the root directory's block ptr 
 */
relptr_t pgg_mkfs(){
	pgg_hd_group_pt hdg = ct_rt.first_pgg;
	pgg_header_pt current;
	current = &hdg->header[0];
	__pgg_new_subpgg(current, current, PGG_LVL9);
#ifdef CTFS_HACK
	return __pgg_allocate_big(PGG_LVL3, current);
#else
	return __pgg_allocate_small(PGG_LVL0, current);
#endif
}
