#include <string.h>
#include <linux/cdev.h>

void cdev_init(struct cdev *cdev, const struct file_operations *fops) {
	memset(cdev, 0, sizeof *cdev);
	//INIT_LIST_HEAD(&cdev->list);
	//kobject_init(&cdev->kobj, &ktype_cdev_default);
	cdev->ops = fops;

}

int cdev_add(struct cdev *p, dev_t dev, unsigned count) {
	p->dev = dev;
	p->count = count;
	//return kobj_map(cdev_map, dev, count, NULL, exact_match, exact_lock, p);
	return 0; 
}

void cdev_del(struct cdev *p) {
	//cdev_unmap(p->dev, p->count);
	//kobject_put(&p->kobj);
}




