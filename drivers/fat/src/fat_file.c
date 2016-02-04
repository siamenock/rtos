/*
 * file.c
 *
 * Include interface about file operation
 * implementation file.
 *
 * knightray@gmail.com
 * 11/05 2008
 */

#include "fat_file.h"
#include "tffs.h"
#include "common.h"
#include "debug.h"
#include "dir.h"
#include "dirent.h"
#include "fat.h"

#ifndef DBG_FILE
#undef DBG
#define DBG nulldbg
#endif

/* private function declaration. */

static BOOL
_parse_open_mode(
	IN	byte * open_mode,
	OUT	uint32 * popen_mode
);

static void
_file_seek(
	IN	tfile_t * pfile,
	IN	uint32 offset
);

static int32
_initialize_file(
	IN	tffs_t * ptffs,
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry,
	OUT	tfile_t * pfile
);

static int32
_get_next_sec(
	IN	tfile_t * pfile
);

/*----------------------------------------------------------------------------------------------------*/

int32 
TFFS_fopen(
	IN	tffs_handle_t * hfs,
	IN	byte * file_path,
	IN	byte * open_mode,
	OUT	tfile_handle_t ** phfile)
{
	DBG("%s start\n", __FUNCTION__);
	int32 ret;
	tffs_t * ptffs = (tffs_t *)hfs;
	byte * fname, * path;
	byte * dup_file_path;
	tfile_t * pfile;
	tdir_t * pdir;
	tdir_entry_t * pdir_entry;

	/* Start of GurumNetworks modification
	if (!hfs || !file_path || !open_mode || !phfile)
	End of GurumNetworks modification */

	if (!ptffs || !file_path || !open_mode)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	pfile = (tfile_t *)malloc(sizeof(tfile_t));
	pfile->event_id = 0;
	dup_file_path = dup_string(file_path);
	pdir_entry = dirent_malloc();
	fname = (byte *)malloc(DNAME_MAX);
	pfile->secbuf = (ubyte *)malloc(ptffs->pbs->byts_per_sec * ptffs->pbs->sec_per_clus);
	memset(pfile->secbuf, 0, ptffs->pbs->byts_per_sec * ptffs->pbs->sec_per_clus);

	path = dup_file_path;
	if (!divide_path(dup_file_path, fname)) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if (!_parse_open_mode(open_mode, &pfile->open_mode)) {
		ret = ERR_TFFS_INVALID_OPENMODE;
		goto _release;
	}

	if ((dir_init(ptffs, path, &pdir)) != DIR_OK) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if (dirent_find(pdir, fname, pdir_entry) != DIRENTRY_OK) {
		DBG("%s(): can't find file [%s] at [%s]\n", __FUNCTION__, fname, path);
		if (pfile->open_mode == OPENMODE_READONLY) {
			ret = ERR_TFFS_FILE_NOT_EXIST;
			goto _release;
		}

		if (!dirent_init(fname, 0, TRUE, pdir_entry)) {
			ret = ERR_TFFS_INVALID_PATH;
			goto _release;
		}

		if ((ret = dir_append_direntry(pdir, pdir_entry)) != DIR_OK) {
			if (ret == ERR_DIR_NO_FREE_SPACE) {
				ret = ERR_TFFS_NO_FREE_SPACE;
			} else {
				ret = ERR_TFFS_DEVICE_FAIL;
			}

			goto _release;
		}
	}

	ret = _initialize_file(ptffs, pdir, pdir_entry, pfile);
	if (ret == FILE_OK) {
		*phfile = (tfile_handle_t*)pfile;
	}

_release:
	free(fname);
	free(dup_file_path);
	return ret;
}

int32
TFFS_rmfile(
	IN	tffs_handle_t hfs,
	IN	byte * file_path)
{
	int32 ret;
	byte * fname, * path;
	byte * dup_file_path;
	tdir_t * pdir;
	tffs_t * ptffs;
	tdir_entry_t * pdir_entry;

	ret = TFFS_OK;

	if (!hfs || !file_path)
		return ERR_TFFS_INVALID_PARAM;

	ptffs = (tffs_t *)hfs;
	dup_file_path = dup_string(file_path);
	fname = (byte *)malloc(DNAME_MAX);
	pdir_entry = dirent_malloc();

	path = dup_file_path;
	if (!divide_path(dup_file_path, fname)) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if ((dir_init(ptffs, path, &pdir)) != DIR_OK) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if ((ret = dirent_find(pdir, fname, pdir_entry)) == DIRENTRY_OK) {
		if (dirent_get_dir_attr(pdir_entry) & ATTR_DIRECTORY) {
			ret = ERR_TFFS_IS_NOT_A_FILE;
			goto _release;
		}

		if (dir_del_direntry(pdir, fname) != DIR_OK) {
			ret = ERR_TFFS_REMOVE_FILE_FAIL;
			goto _release;
		}

		if (fat_free_clus(pdir->ptffs->pfat, dirent_get_clus(pdir_entry)) != FAT_OK) {
			ret = ERR_TFFS_REMOVE_FILE_FAIL;
			goto _release;
		}

		ret = TFFS_OK;
	} else {
		ret = ERR_TFFS_NO_SUCH_FILE;
		goto _release;
	}

_release:
	free(fname);
	free(dup_file_path);
	dirent_release(pdir_entry);
	dir_destroy(pdir);
	return ret;
}

int32
TFFS_fwrite(
	IN	tfile_handle_t * hfile,
	IN	uint32 buflen,
	IN	ubyte * ptr)
{
	tfile_t * pfile = (tfile_t *)hfile;
	tdir_entry_t * pdir_entry;
	//tdir_t * pdir;
	uint32 write_size;
	uint32 written_size;
	//uint32 file_size;
	int32 ret;

	if (!hfile || !ptr)
		return ERR_TFFS_INVALID_PARAM;

	if (pfile->open_mode == OPENMODE_READONLY)
		return ERR_TFFS_READONLY;

	//pdir = pfile->pdir;
	pdir_entry = pfile->pdir_entry;
//	file_size = dirent_get_file_size(pdir_entry);
	write_size = buflen;
	written_size = 0;
	while (written_size < write_size) {
		uint32 write_once_size;
		boot_sector_t* pbs = pfile->ptffs->pbs;
		write_once_size = min(pbs->byts_per_sec * pbs->sec_per_clus - pfile->cur_sec_offset, write_size - written_size);

		memcpy(pfile->secbuf + pfile->cur_sec_offset, ptr + written_size, write_once_size);
		written_size += write_once_size;
		pfile->cur_sec_offset += write_once_size;

		DBG("%s : cur_sec_offset : %d\n", __FUNCTION__, pfile->cur_sec_offset);
		if (pfile->cur_sec_offset == pbs->byts_per_sec * pbs->sec_per_clus) {
			ret = file_write_sector(pfile);
			if (ret == FILE_OK) {
				if (buflen - written_size > 0) {
					if ((ret = _get_next_sec(pfile)) != FILE_OK) {
						ERR("%s(): get next sector failed at %d with ret = %d\n", __FUNCTION__, __LINE__, ret);
						break;
					}
				}

				pfile->cur_sec_offset = 0;
			} else if (ret == ERR_FILE_NO_FREE_CLUSTER) {
				ret = ERR_TFFS_NO_FREE_SPACE;
				break;
			} else {
				ret = ERR_TFFS_DEVICE_FAIL;
				break;
			}
		}
	}

	if (written_size > 0) {
		dirent_update_wrt_time(pdir_entry);
		dirent_update_wrt_date(pdir_entry);
		pfile->file_size += written_size;
	}

	return written_size;
}

int32
TFFS_fread(
	IN	tfile_handle_t* hfile,
	IN	uint32 buflen,
	OUT	ubyte * ptr)
{
	tfile_t * pfile = (tfile_t *)hfile;
	tdir_entry_t * pdir_entry;
	//tdir_t * pdir;
	uint32 read_size;
	uint32 readin_size;
	uint32 file_size;
	int32 ret;

	if (!hfile || !ptr)
		return ERR_TFFS_INVALID_PARAM;

	//pdir = pfile->pdir;
	boot_sector_t* pbs = pfile->ptffs->pbs;
	pdir_entry = pfile->pdir_entry;
	file_size = dirent_get_file_size(pdir_entry);
	read_size = min(buflen, file_size - pfile->cur_fp_offset);
	readin_size = 0;
	while (readin_size < read_size) {
		size_t block_size = pbs->byts_per_sec * pbs->sec_per_clus;
		if (pfile->cur_sec_offset + (read_size - readin_size) >= block_size) {
			memcpy(ptr + readin_size, pfile->secbuf + pfile->cur_sec_offset, block_size - pfile->cur_sec_offset);
			readin_size += block_size - pfile->cur_sec_offset;

			ret = file_read_sector(pfile); 
			if (ret == FILE_OK) {
				pfile->cur_sec_offset = 0;
				continue;
			} else if (ret == ERR_FILE_EOF) {
				DBG("%s(): unexpect file end at %d\n", __FUNCTION__, __LINE__);
				break;
			} else {
				DBG("%s(): read file data sector failed at %d\n", __FUNCTION__, __LINE__);
				ret = ERR_TFFS_DEVICE_FAIL;
				goto _end;
			}
		} else {
			memcpy(ptr + readin_size, pfile->secbuf + pfile->cur_sec_offset, read_size - readin_size);
			pfile->cur_sec_offset += (read_size - readin_size);
			readin_size += (read_size - readin_size);
		}
	}

	if (readin_size > 0) {
		dirent_update_lst_acc_date(pdir_entry);
		pfile->cur_fp_offset += readin_size;
		ret = readin_size;
	} else {
		ret = ERR_TFFS_FILE_EOF;
	}

_end:
	return ret;
}

/* Start of GurumNetworks modification */
typedef struct {
	bool(*callback)(List* blocks, int success, void* context);
	void*		context;
	uint32_t	head_offset;
	tfile_t*	file;
} ReadContext;

static void read_callback(List* blocks, int count, void* context) {
	ReadContext* read_context = context;
	tfile_t* file = read_context->file;
	Cache* cache = file->ptffs->driver->cache;

	int read_count = 0;
	size_t len = 0;
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);
		if(read_count++ < count) {
			// Cache setting
			cache_set(cache, (void*)(uintptr_t)block->sector, block->buffer);
			len += block->size;
		} else {
			list_iterator_remove(&iter);
			free(block);
		}
	}

	file->cur_fp_offset += len;

	BufferBlock* first_block = list_get_first(blocks);
	if(first_block)
		first_block->buffer = (uint8_t*)first_block->buffer - read_context->head_offset;
	
	read_context->callback(blocks, count, read_context->context);
	free(read_context);
}

typedef struct {
	int count;
	void* context;
	List* read_buffers;
} ReadTickContext;

static bool read_tick(void* context) {
	ReadTickContext* read_tick_ctx = context;
	List* read_buffers = read_tick_ctx->read_buffers;
	read_callback(read_buffers, read_tick_ctx->count, read_tick_ctx->context);
	free(read_tick_ctx);

	return false;
}

int32 TFFS_fread_async(tfile_handle_t* hfile, uint32 size, bool(*callback)(List* blocks, int success, void* context), void* context) {
	tfile_t* file = (tfile_t*)hfile;
	tffs_t* tffs = file->ptffs;
	List* read_buffers = tffs->read_buffers;

	if(file->cur_clus == 0)
		return FILE_EOF;

	if(file->open_mode == OPENMODE_APPEND)
		return ERR_TFFS_INVALID_OPENMODE;

	// If there's no space in the buffer
	if(FS_NUMBER_OF_BLOCKS - list_size(read_buffers) <= 0)
		return -FILE_ERR_NOBUFS;

	// Optimize the size that we can handle
	size = min(FS_CACHE_BLOCK * FS_BLOCK_SIZE, size);

	ReadContext* read_context = malloc(sizeof(ReadContext));
	read_context->head_offset = file->cur_sec_offset;
	read_context->callback = callback;
	read_context->context = context;
	read_context->file = file;

	boot_sector_t* pbs = file->ptffs->pbs;
	tdir_entry_t* pdir_entry = file->pdir_entry;
	uint32_t file_size = dirent_get_file_size(pdir_entry);
	uint32_t total_size = min(size, file_size - file->cur_fp_offset);
	uint32_t len = 0;
	int count = 0;
	bool none_cached = false;

	while(len < total_size) {
		BufferBlock* block = malloc(sizeof(BufferBlock));
		if(!block)
			break;
		block->sector = clus2sec(tffs, file->cur_clus) + file->cur_sec;
		block->size = min(FS_BLOCK_SIZE - file->cur_sec_offset, total_size - len);

		// Check if cache has a specific buffer
		block->buffer = cache_get(tffs->driver->cache, (void*)(uintptr_t)block->sector);
		if(!block->buffer)
			none_cached = true;

		// Add blocks into the list to copy into the buffer from callback
		list_add(read_buffers, block);

		len += block->size;
		count++;

		// FIXME: Think about how to sync fat table
		if(fat_get_next_sec(tffs->pfat, &file->cur_clus, &file->cur_sec));
	}

	if(none_cached) {
		DiskDriver* disk_driver = tffs->driver->driver;
		disk_driver->read_async(disk_driver, read_buffers, pbs->sec_per_clus, read_callback, read_context);
	} else {
		ReadTickContext* read_tick_ctx = malloc(sizeof(ReadTickContext));
		read_tick_ctx->count = count;
		read_tick_ctx->context = read_context;
		read_tick_ctx->read_buffers = read_buffers;

		event_busy_add(read_tick, read_tick_ctx);
	}

	return TFFS_OK;
}

typedef struct {
	void(*callback)(void* buffer, int len, void* context);
	void*			usr_buffer;
	List*			blocks;
	void*			context;
	FileSystemDriver*	driver;
} FragContext;

static bool write_tick(void* context) {
	DBG("%s\n", __FUNCTION__);
	FragContext* frag_context = context;
	FileSystemDriver* driver = frag_context->driver;
	tffs_t* tffs = (tffs_t*)driver->priv;
	Cache* cache = driver->cache;
	List* write_buffers = tffs->write_buffers;

	size_t len = 0;
	ListIterator iter;
	list_iterator_init(&iter, frag_context->blocks);
	while(list_iterator_has_next(&iter)) {
		BufferBlock* block = list_iterator_next(&iter);
		list_iterator_remove(&iter);

		// Move clean blocks into write buffers
		list_add(write_buffers, block);

		// Cache update
		void* data = cache_get(cache, (void*)(uintptr_t)block->sector);
		if(data)
			memcpy(data, block->buffer, tffs->pbs->byts_per_sec * tffs->pbs->sec_per_clus);

		len += block->size;
		tffs->undone--;
	}

	list_destroy(frag_context->blocks);

	// Callback
	frag_context->callback(frag_context->usr_buffer, len, frag_context->context);
	free(frag_context);
	return false;
}

typedef struct {
	void(*callback)(List* blocks, int count, void* context);
	void*			context;
	FileSystemDriver*	driver;
} SyncContext;

// Flush the write buffer and sync with the disk
static bool sync_tick(void* context) {
	DBG("%s\n", __FUNCTION__);
	if(!context)
		return false;

	SyncContext* sync_context = context;
	FileSystemDriver* driver = sync_context->driver;
	tffs_t* tffs = (tffs_t*)driver->priv;
	List* write_buffers = tffs->write_buffers;
	DiskDriver* disk_driver = driver->driver;

	if(list_size(write_buffers) <= 0) {
		if(sync_context->callback)
			sync_context->callback(NULL, 0, sync_context->context);

		free(sync_context);
		return false;
	}

	if(disk_driver->write_async(disk_driver, write_buffers, tffs->pbs->sec_per_clus, sync_context->callback, sync_context->context) >= 0) {
		free(sync_context);
		return false;
	}

	return true;
}

typedef struct {
	void(*callback)(void* buffer, int len, void* context);
	void(*sync_callback)(int errno, void* context2);
	void*		context;
	void*		sync_context;
	tfile_handle_t*	file;
	void*		buffer;
	size_t		size;
} LazyWriteContext;

typedef struct {
	void(*callback)(int errno, void* context);
	void*		context;
	tfile_t*	file;
} SyncedContext;

static void synced_callback(List* blocks, int len, void* context) {
	DBG("%s\n", __FUNCTION__);
	// Call the sync callback
	SyncedContext* synced_context = context;

	// If there are another write operations in the wating list. Do it now.
	tfile_t* file = synced_context->file;

	if(len > 0) {
//		tdir_entry_t * pdir_entry = file->pdir_entry;
//		dirent_update_wrt_time(pdir_entry);
//		dirent_update_wrt_date(pdir_entry);
		file->file_size += len;
	}

	synced_context->file->event_id = 0;
	if(synced_context->callback)
		synced_context->callback(len, synced_context->context);

	free(synced_context);

	tffs_t* tffs = file->ptffs;
	FileSystemDriver* driver = tffs->driver;

	DBG("%s : lazy write fire\n", __FUNCTION__);
	List* wait_lists = tffs->wait_lists;
	ListIterator iter;
	list_iterator_init(&iter, wait_lists);
	while(list_iterator_has_next(&iter)) {
		LazyWriteContext* lazy_write = list_iterator_next(&iter);
		
		if(!lazy_write) {
			list_iterator_remove(&iter);
			continue;
		}

		// If write operation is done well, free the context and remove from the list
		int ret = driver->write_async(driver, lazy_write->file, lazy_write->buffer, lazy_write->size, lazy_write->callback, lazy_write->context, lazy_write->sync_callback, lazy_write->sync_context);
		if(ret > 0) {
		} else {
			if(lazy_write->callback)
				lazy_write->callback(lazy_write->buffer, ret, lazy_write->context);
		}

		free(lazy_write);
		list_iterator_remove(&iter);
	}
}

void free_lazy(tffs_t* tffs) {
	DBG("%s\n", __FUNCTION__);
	List* wait_lists = tffs->wait_lists;
	ListIterator iter;
	list_iterator_init(&iter, wait_lists);
	while(list_iterator_has_next(&iter)) {
		LazyWriteContext* lazy_write = list_iterator_next(&iter);
		list_iterator_remove(&iter);
		if(lazy_write)
			free(lazy_write);
	}
}

int TFFS_fwrite_async(tfile_handle_t* hfile, void* buffer, uint32 size, void(*callback)(void* buffer, int success, void* context), void* context, void(*sync_callback)(int errno, void* context), void* context2) {
	DBG("%s\n", __FUNCTION__);
	tfile_t* file = (tfile_t*)hfile;

	if(!hfile | !buffer)
		return ERR_TFFS_INVALID_PARAM;

	if(file->open_mode == OPENMODE_READONLY)
		return ERR_TFFS_READONLY;

	tffs_t* tffs = file->ptffs;
	boot_sector_t* pbs = tffs->pbs;
	FileSystemDriver* driver = tffs->driver;
	List* write_buffers = tffs->write_buffers;
	List* wait_lists = tffs->wait_lists;

	// Temporarily set the maximum size : FS_BLOCK_SIZE(4K) * 2560 : 10M
	size_t block_size = pbs->byts_per_sec * pbs->sec_per_clus;
	size_t total_size = min(size, block_size * FS_WRITE_BUF_SIZE);

	// If write cache buffer is full
	if((FS_WRITE_BUF_SIZE - list_size(write_buffers) - tffs->undone) < (total_size / block_size)) {
		LazyWriteContext* lazy_write = malloc(sizeof(LazyWriteContext));
		if(!lazy_write)
			return ERR_TFFS_NO_FREE_SPACE;

		lazy_write->callback = callback;
		lazy_write->context = context;
		lazy_write->sync_callback = sync_callback;
		lazy_write->sync_context = context2;
		lazy_write->file = hfile;
		lazy_write->buffer = buffer;
		lazy_write->size = total_size;

		// Add context information into the list
		list_add(wait_lists, lazy_write);

		// Update the period of timer event to fire
		event_timer_update(file->event_id, 0);

		return TFFS_OK;
	} else if(file->event_id == 0) {	// If there's no timer event registered yet
		SyncedContext* synced_context = malloc(sizeof(SyncContext));
		synced_context->callback = sync_callback;
		synced_context->context = context2;
		synced_context->file = file;

		SyncContext* sync_context = malloc(sizeof(SyncContext));
		sync_context->driver = driver;
		sync_context->callback = synced_callback;
		sync_context->context = synced_context;

		file->event_id = event_timer_add(sync_tick, sync_context, 10000, 10000);
	} else {
		event_timer_update(file->event_id, 10000);
	}

	int written_size = 0;
	FragContext* frag_context = malloc(sizeof(FragContext));
	if(!frag_context) {
		DBG("%s : malloc error\n", __FUNCTION__);
		free_lazy(tffs);
		return ERR_FAT_DEVICE_FAIL;
	}

	List* blocks = list_create(NULL);
	if(!blocks) {
		DBG("%s : malloc error\n", __FUNCTION__);
		free(frag_context);
		free_lazy(tffs);
		return ERR_FAT_DEVICE_FAIL;
	}

	while(written_size < total_size) {
		BufferBlock* block = malloc(sizeof(BufferBlock));
		if(!block) {
			DBG("%s : malloc error\n", __FUNCTION__);
			free(frag_context);
			list_destroy(blocks);
			free_lazy(tffs);
			return ERR_FAT_DEVICE_FAIL;
		}

		if(file->cur_clus == 0) {
			int fatret;
			uint32 new_clus;
			if((fatret = fat_malloc_clus(tffs->pfat, FAT_INVALID_CLUS, &new_clus)) == FAT_OK) {
				DBG("%s: new_clus = %d\n", __FUNCTION__, new_clus);
				dirent_set_clus(file->pdir_entry, new_clus);
				file->cur_clus = new_clus;
				file->cur_sec = 0;
			} else {
				DBG("%s: ERR_FAT_NO_FREE_CLUSTER\n", __FUNCTION__);
				free(block);
				free(frag_context);
				tffs->undone -= list_size(blocks);
				list_destroy(blocks);
				return fatret;
			} 
		}

		block->sector = clus2sec(tffs, file->cur_clus) + file->cur_sec;
		block->size = min(block_size - file->cur_sec_offset, total_size - written_size);
		block->buffer = buffer + written_size;

		list_add(blocks, block);
		tffs->undone++;

		memcpy(file->secbuf + file->cur_sec_offset, buffer + written_size, block->size);
		file->cur_sec_offset += block->size;
		written_size += block->size;

		if(file->cur_sec_offset == block_size) {
			file->cur_sec_offset = 0;
			if(written_size < total_size) {
				if(_get_next_sec(file) == ERR_FILE_NO_FREE_CLUSTER) {
					free(frag_context);
					tffs->undone -= list_size(blocks);
					list_destroy(blocks);
					return ERR_FILE_NO_FREE_CLUSTER;
				}
			}
		}
	}

	frag_context->callback = callback;
	frag_context->context = context;
	frag_context->usr_buffer = buffer;
	frag_context->blocks = blocks;
	frag_context->driver = driver;

	// Need to add event to prevent an infinite cycle
	event_busy_add(write_tick, frag_context);

	return TFFS_OK;
}
/* End of GurumNetworks modification */

int32
TFFS_fclose(
	IN	tfile_handle_t* hfile)
{
	DBG("%s\n", __FUNCTION__);
	int32 ret;
	tfile_t * pfile = (tfile_t *)hfile;

	if (!hfile)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	if (file_write_sector(pfile) != FILE_OK) {
		ret = ERR_TFFS_DEVICE_FAIL;
	}

	dirent_set_file_size(pfile->pdir_entry, pfile->file_size);
	if(dir_update_direntry(pfile->pdir, pfile->pdir_entry) != DIR_OK) {
		ret = ERR_TFFS_DEVICE_FAIL;
	}

	dir_destroy(pfile->pdir);
	dirent_release(pfile->pdir_entry);
	free(pfile->secbuf);
	free(pfile);
	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

int32
file_read_sector(
	IN	tfile_t * pfile)
{
	int32 ret = FILE_OK;
	tffs_t * ptffs = pfile->ptffs;
	
	if (pfile->cur_clus == 0) {
		ret = ERR_FILE_EOF;
	} else {
		if (fat_get_next_sec(ptffs->pfat, &pfile->cur_clus, &pfile->cur_sec)) {
			/* Start of GurumNetworks modification
			if (cache_readsector(ptffs->pcache, clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec, pfile->secbuf) == CACHE_OK) {
			End of GurumNetworks modification */

			size_t size = ptffs->pbs->sec_per_clus * ptffs->pbs->byts_per_sec;
			uint32_t sector = clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec;
			void* data = cache_get(ptffs->driver->cache, (void*)(uintptr_t)sector);
			if(!data) {
				data = gmalloc(size);
				DiskDriver* disk_driver = ptffs->driver->driver;
				if(disk_driver->read(disk_driver, sector, ptffs->pbs->sec_per_clus, data) >= 0) {

				} else {
					gfree(data);
					return ERR_FILE_DEVICE_FAIL;
				}

				if(!cache_set(ptffs->driver->cache, (void*)(uintptr_t)sector, data)) {
					gfree(data);
					return ERR_FILE_DEVICE_FAIL;
				}
			}

			memcpy(pfile->secbuf, data, size);
		} else {
			return ERR_FILE_EOF;
		}
	}

	return ret;
}

int32 file_write_sector(IN tfile_t * pfile)
{
	int32 ret;
	tffs_t * ptffs = pfile->ptffs;
	tdir_entry_t * pdir_entry = pfile->pdir_entry;
	uint32 new_clus;

	ret = FILE_OK;

	if (pfile->cur_clus == 0) {
		int32 fatret;
		if ((fatret = fat_malloc_clus(ptffs->pfat, FAT_INVALID_CLUS, &new_clus)) == FAT_OK) {
			DBG("%s: new_clus = %d\n", __FUNCTION__, new_clus);
			dirent_set_clus(pdir_entry, new_clus);
			pfile->cur_clus = new_clus;
			pfile->cur_sec = 0;
		} else if (fatret == ERR_FAT_NO_FREE_CLUSTER) {
			return ERR_FILE_NO_FREE_CLUSTER;
		} else {
			return ERR_FILE_DEVICE_FAIL;
		}
	}

	if (ret == FILE_OK) {
		/* Start of GurumNetworks modification
		if (cache_writesector(ptffs->pcache, clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec, pfile->secbuf) != CACHE_OK) {
			ret = ERR_FILE_DEVICE_FAIL;
		}
		End of GurumNetworks modification */

		size_t size = ptffs->pbs->byts_per_sec * ptffs->pbs->sec_per_clus;
		uint32_t sector = clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec;
		DiskDriver* disk_driver = ptffs->driver->driver;
		void* temp = gmalloc(size);
		memcpy(temp, pfile->secbuf, size);
		if(disk_driver->write(disk_driver, sector, ptffs->pbs->sec_per_clus, temp) < 0) {
			gfree(temp);
			return ERR_FILE_DEVICE_FAIL;
		} else {
			void* data = cache_get(ptffs->driver->cache, (void*)(uintptr_t)sector);
			if(!data) {
				if(!cache_set(ptffs->driver->cache, (void*)(uintptr_t)sector, temp)) {
					gfree(temp);
					return ERR_FILE_DEVICE_FAIL;
				}
			} else {
				memcpy(data, pfile->secbuf, size);
				gfree(temp);
			}
		}
	}

	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

static int32
_initialize_file(
	IN	tffs_t * ptffs,
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry,
	OUT	tfile_t * pfile)
{
	int32 ret;
	pfile->ptffs = ptffs;
	pfile->pdir = pdir;
	pfile->pdir_entry = pdir_entry;
	ret = FILE_OK;

	pfile->start_clus = dirent_get_clus(pdir_entry);
	pfile->file_size = dirent_get_file_size(pdir_entry);
	pfile->cur_clus = pfile->start_clus;
	pfile->cur_sec = 0;
	pfile->cur_sec_offset = 0;
	pfile->cur_fp_offset = 0;

	if (pfile->open_mode == OPENMODE_APPEND) {
		pfile->cur_fp_offset = dirent_get_file_size(pdir_entry);
		_file_seek(pfile, pfile->cur_fp_offset);
		if (pfile->cur_clus != 0) {
			/* Start of GurumNetworks modification
			if (cache_readsector(ptffs->pcache, clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec, pfile->secbuf) != CACHE_OK) {
				ret = ERR_TFFS_DEVICE_FAIL;
			}
			End of GurumNetworks modification */

			size_t size = ptffs->pbs->sec_per_clus * ptffs->pbs->byts_per_sec;
			uint32_t sector = clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec;
			void* data = cache_get(ptffs->driver->cache, (void*)(uintptr_t)sector);
			if(!data) {
				data = gmalloc(size);
				DiskDriver* disk_driver = ptffs->driver->driver;
				if(disk_driver->read(disk_driver, sector, ptffs->pbs->sec_per_clus, data) < 0) {
					gfree(data);
					ret = ERR_TFFS_DEVICE_FAIL;
				}

				if(!cache_set(ptffs->driver->cache, (void*)(uintptr_t)sector, data)) {
					gfree(data);
					ret = ERR_TFFS_DEVICE_FAIL;
				}
			}

			memcpy(pfile->secbuf, data, size);
		}
	} else if (pfile->open_mode == OPENMODE_WRITE) {
		if (pfile->start_clus != 0) {
			if (fat_free_clus(ptffs->pfat, pfile->start_clus) != FAT_OK) {
				ERR("%s(): %d fat_free_clus failed.\n", __FUNCTION__, __LINE__);
				ret = ERR_TFFS_FAT;
			}
		}

		dirent_set_file_size(pdir_entry , 0);
		dirent_set_clus(pdir_entry, 0);

		pfile->file_size = 0;
		pfile->cur_clus = 0;
		pfile->cur_sec = 0;
		pfile->cur_sec_offset = 0;
		pfile->cur_fp_offset = 0;
	} else {
		if (pfile->cur_clus != 0) {
			/* Start of GurumNetworks modification
			if (cache_readsector(ptffs->pcache, clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec, pfile->secbuf) != CACHE_OK) {
			End of GurumNetworks modification */

			size_t size = ptffs->pbs->sec_per_clus * ptffs->pbs->byts_per_sec;
			uint32_t sector = clus2sec(ptffs, pfile->cur_clus) + pfile->cur_sec;
			void* data = cache_get(ptffs->driver->cache, (void*)(uintptr_t)sector);
			if(!data) {
				data = gmalloc(size);
				DiskDriver* disk_driver = ptffs->driver->driver;
				if(disk_driver->read(disk_driver, sector, ptffs->pbs->sec_per_clus, data) < 0) {
					gfree(data);
					ret = ERR_TFFS_DEVICE_FAIL;
				}

				if(!cache_set(ptffs->driver->cache, (void*)(uintptr_t)sector, data)) {
					gfree(data);
					ret = ERR_TFFS_DEVICE_FAIL;
				}
			}
			memcpy(pfile->secbuf, data, size);
		}
	}

	return ret;
}

static void
_file_seek(
	IN	tfile_t * pfile,
	IN	uint32 offset)
{
	int32 cur_offset;
	boot_sector_t* pbs = pfile->ptffs->pbs;

	cur_offset = offset;
	while (cur_offset - pbs->byts_per_sec * pbs->sec_per_clus  > 0 && fat_get_next_sec(pfile->ptffs->pfat, &pfile->cur_clus, &pfile->cur_sec)) {
		cur_offset -= pfile->ptffs->pbs->byts_per_sec * pbs->sec_per_clus;
	}
	pfile->cur_sec_offset = cur_offset;
}

static int32
_get_next_sec(
	IN	tfile_t * pfile)
{
	int32 ret;
	tffs_t * ptffs = pfile->ptffs;
	uint32 new_clus;

	ret = FILE_OK;

	if (pfile->cur_sec + ptffs->pbs->sec_per_clus < ptffs->pbs->sec_per_clus) {
		pfile->cur_sec += ptffs->pbs->sec_per_clus;
	} else {
		int32 fatret;

		if ((fatret = fat_malloc_clus(ptffs->pfat, pfile->cur_clus, &new_clus)) == FAT_OK) {
			pfile->cur_clus = new_clus;
			pfile->cur_sec = 0;
		} else if (fatret == ERR_FAT_NO_FREE_CLUSTER) {
			ret = ERR_FILE_NO_FREE_CLUSTER;
		} else {
			ret = ERR_FILE_DEVICE_FAIL;
		}
	}

	return ret;
}

static BOOL
_parse_open_mode(
	IN	byte * open_mode,
	OUT	uint32 * popen_mode)
{
	if (!strcmp(open_mode, "r")) {
		*popen_mode = OPENMODE_READONLY;
	} else if (!strcmp(open_mode, "w")) {
		*popen_mode = OPENMODE_WRITE;
	} else if (!strcmp(open_mode, "a")) {
		*popen_mode = OPENMODE_APPEND;
	} else {
		return FALSE;
	}

	return TRUE;
}

