/*
 * fat.c
 *
 * Include functions operation FAT.
 * implementation file.
 *
 * knightray@gmail.com
 * 10/29 2008
 */

#include "fat.h"
#include "tffs.h"
#include "debug.h"

#ifndef DBG_FAT
#undef DBG
#define DBG nulldbg
#endif

/* Internal routines declaration. */

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus
);

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val
);

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat
);

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

/*----------------------------------------------------------------------------------------------------*/

tfat_t *
fat_init(
	IN	tffs_t * ptffs)
{
	tfat_t * pfat;

#ifdef _KERNEL_
	ASSERT(ptffs);
#endif
	pfat = (tfat_t *)malloc(sizeof(tfat_t));
	pfat->ptffs = ptffs;
    	pfat->last_free_clus = 0;
    	pfat->secbuf = (ubyte *)malloc(ptffs->pbs->byts_per_sec);

	return pfat;
}

void
fat_destroy(
	IN	tfat_t * pfat)
{
	free(pfat->secbuf);
	free(pfat);
}

int32
fat_get_next_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 next_clus;

#ifdef _KERNEL_
	ASSERT(pfat);
#endif
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, clus)))
		return ERR_FAT_DEVICE_FAIL;

	next_clus = _get_fat_entry(pfat, clus);
	if (_is_entry_eof(pfat, next_clus))
		return ERR_FAT_LAST_CLUS;

	return next_clus;
}

BOOL
fat_get_next_sec(
	IN	tfat_t * pfat,
	OUT	uint32 * pcur_clus,
	OUT uint32 * pcur_sec)
{
	tffs_t * ptffs = pfat->ptffs;
	int32 next_clus;

	DBG("%s(): get next sector for cluster 0x%x\n", __FUNCTION__, *pcur_clus);
	if (*pcur_clus == ROOT_DIR_CLUS_FAT16 && (ptffs->fat_type == FT_FAT12 || ptffs->fat_type == FT_FAT16)) {
		if (*pcur_sec + 1 < ptffs->root_dir_sectors) {
			(*pcur_sec)++;
		} else {
			return FALSE;
		}
	} else if (*pcur_sec + ptffs->pbs->sec_per_clus < ptffs->pbs->sec_per_clus) {
		(*pcur_sec) += ptffs->pbs->sec_per_clus;
	} else {
		next_clus = fat_get_next_clus(pfat, *pcur_clus);
		if (next_clus == ERR_FAT_LAST_CLUS)
			return FALSE;

		DBG("%s(): next_clus = 0x%x\n", __FUNCTION__, next_clus);
		*pcur_clus = next_clus;
		*pcur_sec = 0;
	}

	return TRUE;
}

int32
fat_malloc_clus(
	IN	tfat_t * pfat,
	IN	uint32 cur_clus,
	OUT	uint32 * pnew_clus)
{
	uint32 new_clus;
	int32 ret;

#ifdef _KERNEL_	
	ASSERT(pfat && pnew_clus);
#endif
	if (cur_clus == FAT_INVALID_CLUS) {
		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;
	} else {
		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;

		if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;

		_set_fat_entry(pfat, cur_clus, new_clus);

		DBG("%s() : write_fat_sector = %d \n", __FUNCTION__, _clus2fatsec(pfat, cur_clus));
		if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;
	}

	*pnew_clus = new_clus;
	DBG("%s(): new clus = %d\n", __FUNCTION__, new_clus);
	return FAT_OK;
}

int32
fat_free_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 last_clus;
	uint32 cur_clus;

#ifdef _KERNEL_
	ASSERT(pfat);
#endif
	
	last_clus = clus;
	cur_clus = clus;

	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		if (_clus2fatsec(pfat, cur_clus) != _clus2fatsec(pfat, last_clus)) {
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
				return ERR_FAT_DEVICE_FAIL;

			if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
				return ERR_FAT_DEVICE_FAIL;
		}

		last_clus = cur_clus;
		cur_clus = _get_fat_entry(pfat, last_clus);
		if (_is_entry_eof(pfat, cur_clus)) {
			_set_fat_entry(pfat, last_clus, 0);
			break;
		}
		
		_set_fat_entry(pfat, last_clus, 0);
	}

	if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
		return ERR_FAT_DEVICE_FAIL;

	return FAT_OK;
}

/*----------------------------------------------------------------------------------------------------*/

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	tffs_t * ptffs = pfat->ptffs;
	boot_sector_t* pbs = ptffs->pbs;
	int16 entry_len;
	void * pclus;
	uint32 entry_val = 0;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + (clus * entry_len / 8) % (pbs->byts_per_sec);
	
	if (ptffs->fat_type == FT_FAT12) {
		DBG("%s : we don't support FAT12\n", __FUNCTION__);
		uint16 fat_entry;
		fat_entry = *((uint16 *)pclus);
		if (clus & 1) {
			entry_val = fat_entry >> 4;
		} else {
			entry_val = fat_entry & 0x0FFF;
		}
	} else if (ptffs->fat_type == FT_FAT16) {
		DBG("%s : we don't support FAT16\n", __FUNCTION__);
		entry_val = *((uint16 *)pclus);
	} else if (ptffs->fat_type == FT_FAT32) {
		entry_val = *((uint32 *)pclus) & 0x0FFFFFFF;
	}

	return entry_val;
}

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;
	int16 entry_len;
	void * pclus;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + ((clus * entry_len / 8) % ptffs->pbs->byts_per_sec);
	
	if (ptffs->fat_type == FT_FAT12) {
		uint16 fat12_entry_val = entry_val & 0xFFFF;
		if (clus & 1) {
			fat12_entry_val = fat12_entry_val << 4;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0x000F;
		} else {
			fat12_entry_val = fat12_entry_val & 0x0FFF;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0xF000;
		}

		*((uint16 *)pclus) = *((uint16 *)pclus) | fat12_entry_val;
	} else if (ptffs->fat_type == FT_FAT16) {
		*((uint16 *)pclus) = entry_val & 0xFFFF;
	} else if (ptffs->fat_type == FT_FAT32) {
		*((uint32 *)pclus) = *((uint32 *)pclus) & 0xF0000000;
		*((uint32 *)pclus) = *((uint32 *)pclus) | (entry_val & 0x0FFFFFFF);
	}
}

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat)
{
	tffs_t * ptffs = pfat->ptffs;
	uint16 entry_len = 32;

	switch (ptffs->fat_type) {
	case FT_FAT12:
		entry_len = 12;
		break;
	case FT_FAT16:
		entry_len = 16;
		break;
	case FT_FAT32:
		entry_len = 32;
		break;
	default:;
#ifdef _KERNEL_
		ASSERT(0);
#endif
	}

	return entry_len;
}

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	tffs_t * ptffs = pfat->ptffs;

	/* Start of GurumNetworks modification */
	boot_sector_t* pbs = ptffs->pbs;
	memcpy(pfat->secbuf, ptffs->fat + (fat_sec - ptffs->first_lba - pbs->resvd_sec_cnt) * pbs->byts_per_sec, pbs->byts_per_sec);
	pfat->cur_fat_sec = fat_sec;
	/* End of GurumNetworks modification */

	/* Start of GurumNetworks modification
	//if (fat_sec == cur_fat_sec) 
	//	return TRUE;

	if (HAI_readsector(ptffs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
		return FALSE;
	}

	if (ptffs->fat_type == FT_FAT12) {
	// This cluster access spans a sector boundary in the FAT      
   	// There are a number of strategies to handling this. The      
    	// easiest is to always load FAT sectors into memory           
    	// in pairs if the volume is FAT12 (if you want to load        
    	// FAT sector N, you also load FAT sector N+1 immediately      
    	// following it in memory unless sector N is the last FAT      
    	// sector). It is assumed that this is the strategy used here  
    	// which makes this if test for a sector boundary span         
    	// unnecessary.                                                

		if (HAI_readsector(ptffs->hdev, fat_sec + 1, 
				pfat->secbuf + ptffs->pbs->byts_per_sec) != HAI_OK) {
			return FALSE;
		}
		printf("We don't support FAT12\n");
	}

	pfat->cur_fat_sec = fat_sec;
	End of GurumNetworks modification */
	return TRUE;
}

void fat_sync_callback(List* blocks, int count, void* context) {
	tffs_t* tffs = context;

	list_destroy(blocks);

	tffs->event_id = 0;
}

bool fat_sync(void* context) {
	tffs_t* tffs = context;
	boot_sector_t* pbs = tffs->pbs;
	DiskDriver* disk_driver = tffs->driver->driver;
	
	ListIterator iter;
	list_iterator_init(&iter, tffs->fat_sync);
	List* write_blocks = list_create(NULL);
	if(!write_blocks) {
			DBG("%s(): list creation error\n", __FUNCTION__);
			return true;
	}

	while(list_iterator_has_next(&iter)) {
		void* data = list_iterator_next(&iter);

		BufferBlock* block = (BufferBlock*)malloc(sizeof(BufferBlock));
		if(!block) {
			DBG("%s(): malloc error\n", __FUNCTION__);
			list_destroy(write_blocks);
			return true;
		}

		void* buffer = gmalloc(pbs->byts_per_sec);
		if(!buffer) {
			DBG("%s(): gmalloc error\n", __FUNCTION__);
			list_destroy(write_blocks);
			free(block);
			return true;
		}

		memcpy(buffer, tffs->fat + ((uint32_t)(uint64_t)data - tffs->first_lba - pbs->resvd_sec_cnt) * pbs->byts_per_sec, pbs->byts_per_sec);

		block->sector = (uint32_t)(uint64_t)data;
		block->buffer = buffer;
		block->size = pbs->byts_per_sec;

		list_add(write_blocks, block);

		list_iterator_remove(&iter);
	}

	disk_driver->write_async(disk_driver, write_blocks, 1, fat_sync_callback, tffs);

	return false;
}

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	tffs_t * ptffs = pfat->ptffs;

	/* Start of GurumNetworks modification */
	boot_sector_t* pbs = ptffs->pbs;
	memcpy(ptffs->fat + (fat_sec - ptffs->first_lba - pbs->resvd_sec_cnt) * pbs->byts_per_sec, pfat->secbuf, pbs->byts_per_sec);

	if(ptffs->event_id == 0)
		ptffs->event_id = event_timer_add(fat_sync, ptffs, 100000, 100000);
	else if(list_size(ptffs->fat_sync) >= 50)
		event_timer_update(ptffs->event_id, 0);
	else
		event_timer_update(ptffs->event_id, 100000);

	ListIterator iter;
	list_iterator_init(&iter, ptffs->fat_sync);
	while(list_iterator_has_next(&iter)) {
		void* data = list_iterator_next(&iter);

		if((uint32_t)(uint64_t)data == fat_sec)
			return TRUE;
	}

	list_add(ptffs->fat_sync, (void*)(uint64_t)fat_sec);

	/* End of GurumNetworks modification */

	/* Start of GurumNetworks modification
	if (HAI_writesector(ptffs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
		return FALSE;
	}

	if (ptffs->fat_type == FT_FAT12) {
		if (HAI_writesector(ptffs->hdev, fat_sec + 1, pfat->secbuf + ptffs->pbs->byts_per_sec) != HAI_OK) {
			return FALSE;
		}
	}
	End of GurumNetworks modification */

	return TRUE;
}

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	boot_sector_t* pbs = pfat->ptffs->pbs;
	return pfat->ptffs->sec_fat + clus * _get_fat_entry_len(pfat) / 8 / pbs->byts_per_sec;
}

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;

	if (ptffs->fat_type == FT_FAT12) {
		return !(entry_val & 0x0FFF);
	}
	else if (ptffs->fat_type == FT_FAT16) {
		return !(entry_val & 0xFFFF);
	}
	else if (ptffs->fat_type == FT_FAT32) {
		return !(entry_val & 0x0FFFFFFF);
	}

#ifdef _KERNEL_
	ASSERT(0);
#endif

	return FALSE;
}

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;

	if (ptffs->fat_type == FT_FAT12) {
		return (entry_val & 0x0FFF) > 0x0FF8;
	}
	else if (ptffs->fat_type == FT_FAT16) {
		return (entry_val & 0xFFFF) > 0xFFF8;
	}
	else if (ptffs->fat_type == FT_FAT32) {
		return (entry_val & 0x0FFFFFFF) >= 0x0FFFFFF8;
	}

#ifdef _KERNEL_
	ASSERT(0);
#endif
	return FALSE;
}

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus)
{
	tffs_t * ptffs = pfat->ptffs;
	uint32 cur_clus;
	int32 ret;
	
	ret = FAT_OK;
	cur_clus = pfat->last_free_clus;

	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, pfat->last_free_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		uint32 entry_val;

		entry_val = _get_fat_entry(pfat, cur_clus);
		if (_is_entry_free(pfat, entry_val)) {

			_set_fat_entry(pfat, cur_clus, 0x0FFFFFFF);
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
			}

			*pfree_clus = cur_clus;
			pfat->last_free_clus = cur_clus;
			break;
		}

		cur_clus++;
		if (cur_clus > ptffs->total_clusters) {
			cur_clus = 0;
		}

		if (cur_clus == pfat->last_free_clus) {
			ret = ERR_FAT_NO_FREE_CLUSTER;
			break;
		}

		if (_clus2fatsec(pfat, cur_clus - 1) != _clus2fatsec(pfat, cur_clus)) {
			if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
			}
		}
	}

	return ret;
}

