#ifndef __LINUX_PM_RUNTIME_H__
#define __LINUX_PM_RUNTIME_H__

static inline void pm_runtime_put_noidle(struct device *dev) {}

static inline void pm_runtime_get_noresume(struct device *dev) {}
#endif /* __LINUX_PM_RUNTIME_H__ */
