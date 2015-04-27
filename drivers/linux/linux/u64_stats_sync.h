#ifndef __LINUX_U64_STATS_SYNC_H__
#define __LINUX_U64_STATS_SYNC_H__

struct u64_stats_sync {
};

static inline void u64_stats_update_begin(struct u64_stats_sync *syncp)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	write_seqcount_begin(&syncp->seq);
#endif
}

static inline void u64_stats_update_end(struct u64_stats_sync *syncp)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	write_seqcount_end(&syncp->seq);
#endif
}

#endif /* __LINUX_U64_STATS_SYNC_H__ */
