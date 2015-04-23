#ifndef __LINUX_SCATTERLIST_H__
#define __LINUX_SCATTERLIST_H__

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bug.h>

struct scatterlist {
	unsigned long   page_link;
	unsigned int    offset;
	unsigned int    length;
	dma_addr_t      dma_address;
};

void sg_init_table(struct scatterlist *, unsigned int);

static inline void sg_set_buf(struct scatterlist *sg, const void *buf,
			      unsigned int buflen)
{
#ifdef CONFIG_DEBUG_SG
	BUG_ON(!virt_addr_valid(buf));
#endif
//	sg_set_page(sg, virt_to_page(buf), buflen, offset_in_page(buf));
}

static inline void sg_set_page(struct scatterlist *sg, struct page *page,
			       unsigned int len, unsigned int offset)
{
//	sg_assign_page(sg, page);
//	sg->offset = offset;
//	sg->length = len;
}


#endif /* __LINUX_SCATTERLIST_H__ */
