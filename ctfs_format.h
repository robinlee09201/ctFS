#ifndef CTFS_FORMAT_H
#define CTFS_FORMAT_H
/* This header contains the layout related
 * structures, macros and functions
 */

#include "ctfs_type.h"

/*****************************************
 * Sub-PMD structures.
 * Store files that is less than 
 * a PMD (2MB).
 * The header always start at 256B from
 * the page start (right after the upper
 * level pgg headers).
 * 
 ****************************************/

struct pgg_subpmd_header{
    pgg_level_t     level;
    uint8_t     bitmap_hint;
    uint16_t    taken;

    relptr_t     parent;
    uint64_t    bitmap[8];
};
typedef struct pgg_subpmd_header pgg_subpmd_header_t;
typedef pgg_subpmd_header_t* pgg_subpmd_header_pt;


#define PGG_SUBPMD_AVB_TEST(cap, lvl)   \
    (cap & (0b01 << lvl))
#define PGG_SUBPMD_PRF_TEST(cap, lvl)   \
    (cap & (0b010000 << lvl))

/*******************************************
 * Page Group Structures
 * 
 *  - 9-level page groups
 * L9   L8  L7  L6  L5   L4   L3 | L2   L1  L0
 * 512G 64G 8G  1G  128M 16M  2M | 256K 32K 4K
 *       Direct Allocate         |   sub-PMD
 * 
 *  - 
 ******************************************/

/* Levels */
#define PGG_LVL0        0
#define PGG_LVL1        1
#define PGG_LVL2        2
#define PGG_LVL3        3
#define PGG_LVL4        4
#define PGG_LVL5        5
#define PGG_LVL6        6
#define PGG_LVL7        7
#define PGG_LVL8        8
#define PGG_LVL9        9
#define PGG_LVL10       10
#define PGG_LVL_NONE    -1


extern const uint64_t pgg_size[10];
extern const uint64_t pgg_limit[10];

#define PGG_STATE_EMPTY     0x00
#define PGG_STATE_FILE      0x01
#define PGG_STATE_SUB       0x02

#define PGG_STATE_INIT      PGG_STATE_SUB

/* conversions */
#define PGG_STATE_LOAD(map,pos)        \
    (0b011 & (map >> (2*(pos))) )
#define PGG_STATE_STORE(map,pos,val)   \
    map = ((((val & 0b011) << (2*(pos)) ) | map ) & ((val << (2*(pos))) | ~(0b011 << (2*(pos)))))

#define PGG_REL2HD_GROUP(addr, level) \
((pgg_hd_group_pt) CT_REL2ABS(addr & (PAGE_MASK << (3 * (level)))))
#define PGG_BIGFILE2INDEX(addr, level) \
((addr >> (12 + 3 * (level))) & 0b0111)
#define PGG_SMALLFILE2INDEX(addr, level) \
((addr >> (12 + 3 * (level))) & (0x01FF >> (3 * (level))) )
/* end of conversions */


/* Header of level 4-9 
 * sub-page Group. 
 * size: 32B
 * The struct is in pmm. 
 */
struct pgg_header {
    /* (Only in L9 header) Pointer to the 
     * next L9 page group. 
     * NULL if it's the last one. 
     */
    relptr_t     next_l9;
    /* Pointer to its parent 
     * (higher level) page group. 
     * If 0, it's in this header.  
     */
    relptr_t     parent_pgg;
    /* Indicating the state of each 
     * lower-level page group 
     * in this page group (2-bit * 8)
     */
    uint16_t    state_map;
    /* Capability Level
     * Indicating the largest 
     * page group it is able 
     * to allocate.
     */
    pgg_level_t     cap_lvl;
    /* Indicating the capability 
     * of sub-PMD allocation. 
     * 4-bit preferred + 4-bit available.
     */
    uint8_t     sub_pmd_cap;
    /* (Only in L4 header) Indicating 
     * the type of each Level-3 page 
     * group (PMD) (2-bit * 8).
     */
    uint16_t    pmd_type;

    pgg_level_t level;
    /* padding. 32B - 25B = 7B */
    char        padding[7];
};
typedef struct pgg_header pgg_header_t;
typedef pgg_header_t* pgg_header_pt;

/* The header group
 * in each sub-page group.
 * It contains 6 pgg_header in rescending order
 * i.e. from level 9 to level 4
 * and an additional jounal and control area.
 * The struct shall be 256B in size.
 * 32B * 6 + jounal + others and padding.
 * This struct is in pmm.
 */
struct pgg_hd_group{
    struct pgg_header   header[6];
    
    //TODO: Journal

    /* Padding. 256B - 6 * 32B = 64B */
    char                padding[64];

    struct  pgg_subpmd_header  subpmd_header;
};
typedef struct pgg_hd_group pgg_hd_group_t;
typedef pgg_hd_group_t* pgg_hd_group_pt;

#define PGG_HEADER2GROUP(header)        \
    ((pgg_hd_group_pt)(((uint64_t)header) & PAGE_MASK))
/* get the nth group
 * of the current group
 * index = n
 * level is the lower level
 */
#define PGG_GROUP_AT(group,level,index)   \
    ( (pgg_hd_group_pt)((uint64_t)group + (index) * pgg_size[level]) )
/* get the header of this group */
#define PGG_GROUP2HEADER(group,level) \
    ((pgg_header_pt) (&group->header[9-(level)]))

#define PGG_GROUP_AT2HEADER(group,level,index)  \
    PGG_GROUP2HEADER(PGG_GROUP_AT(group, level, index), level)

/*******************************************
 * Super page group structure
 * The first pgg in a ctfs.
 * It has 3 components: 
 *  - Super block for all metadata (1 page)
 *  - inode bitmap  (512KB - 512MB)
 *  - inode table   (512MB - 512GB)
 * Total supported inodes: 
 *      (512M - 512K) * 8 = 4,290,772,992
 * 
 * In super block, it has the following: 
 * metadata structure (0B - 512B)
 * metadata mirror    (512B - 1k)
 * allocation protector (1K - 2K)
 * lvl9 pgg bit map   (2K - 4K)
 ******************************************/

/* Super block
 * This struct is in pmm.
 * Size shall be less than 512B.
 */
struct ct_super_blk{
    // 64-bit
    char        magic[8];
    relptr_t    first_pgg;
    size_t      page_used;
    size_t      page_total;
    uint64_t    alloc_prot_bmp;
    uint64_t    lvl9_bmp;
    uint64_t    root_inode;

    uint64_t    padding[1];
    // start of inode, kept in one cacheline
    size_t      inode_used;
    size_t      inode_bmp_touched;
    uint64_t    inode_hint;
    // end of inode
    // 32-bit

    // 16-bit

    // 8-bit
    uint8_t     alloc_prot_clock;
    pgg_level_t     next_sub_lvl; 
};
typedef struct ct_super_blk ct_super_blk_t;
typedef ct_super_blk_t* ct_super_blk_pt;

/* allocation protector
 * proctect the allocation operations
 * from leaking.
 * if target==NULL, it's sub_pmd header
 * allocation. 
 * else, it's file allocation. 
 * Need to find whether it's sub_pmd file
 * or not by check the header address. 
 * This struct is in pmm.
 */
struct ct_alloc_prot{
    relptr_t         header;
    relptr_t         target;
};
typedef struct ct_alloc_prot ct_alloc_prot_t;
typedef ct_alloc_prot_t* ct_alloc_prot_pt;

#define CT_ALLOC_PROT_PTR   ((ct_alloc_prot_pt)(((void*)(ct_rt.first_pgg)) + 1024))
#define CT_LVL9_BMP_PTR     ((uint64_t*)(((void*)(ct_rt.first_pgg)) + 2048))


#endif