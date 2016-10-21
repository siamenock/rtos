/* 
 * PacketNgin network accelerator driver 
 **/

#include <linux/compat.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/kthread.h>
#include <linux/mmu_context.h>
#include <linux/wait.h>
#include <linux/virtio.h>
#include <linux/rtnetlink.h>

#include "map.h"
#include "dispatcher.h"

static struct task_struct *worker;
static spinlock_t work_lock;

static struct list_head work_list;
static struct net_device *dispatcher_netdev;

bool dispatcher_enabled = false;

/* TODO: Will be removed. Configuration variable. It should be allocated by manager */
Map* nics;
void* __malloc_pool;

struct dispatcher_work* alloc_dispatcher_work(dispatcher_work_fn_t fn, void* data)
{
	struct dispatcher_work *work = kmalloc(sizeof(struct dispatcher_work), GFP_KERNEL);
	if (!work)
		return NULL;

	INIT_LIST_HEAD(&work->node);
	work->fn = fn;
	work->data = data;
	work->dev = dispatcher_netdev;

	return work;
}

static inline void dispatcher_work_queue(struct dispatcher_work *work)
{
	unsigned long flags;

	spin_lock_irqsave(&work_lock, flags);
	if (list_empty(&work->node)) {
		list_add_tail(&work->node, &work_list);
		wake_up_process(worker);
	}

	spin_unlock_irqrestore(&work_lock, flags);
}

static inline void dispatcher_work_queue_flush(void)
{
	unsigned long flags;
	struct dispatcher_work *pos;
	struct dispatcher_work *tmp;

	spin_lock_irqsave(&work_lock, flags);

	list_for_each_entry_safe(pos, tmp, &work_list, node) {
		list_del(&pos->node);
		kfree(pos);
	}

	spin_unlock_irqrestore(&work_lock, flags);
}

static int dispatcher_worker(void *data)
{
	struct dispatcher_work *work = NULL;
	struct task_struct *task = (struct task_struct *)data;
	unsigned uninitialized_var(seq);
	struct list_head *pos;

	printk("PacketNgin worker thread created\n");
	use_mm(task->mm);
	set_user_nice(current, -20);

	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);

		spin_lock_irq(&work_lock);

		if (kthread_should_stop()) {
			printk("Thread stopped called\n");
			spin_unlock_irq(&work_lock);
			__set_current_state(TASK_RUNNING);
			break;
		}
		
		if (!list_empty(&work_list)) {
			set_current_state(TASK_RUNNING);
			list_for_each(pos, &work_list) {
				work = list_entry(pos, struct dispatcher_work, node);
				work->fn(work);
			}
		}

		spin_unlock_irq(&work_lock);
		set_current_state(TASK_RUNNING);
		schedule();
	}
	
	printk("PacketNgin worker thread destroyed\n");
	unuse_mm(task->mm);
	return 0;
}

static int dispatcher_worker_start(struct task_struct *manager_task)
{
	int err;

	spin_lock_init(&work_lock);
	INIT_LIST_HEAD(&work_list);

	worker = kthread_create(dispatcher_worker, manager_task, "dispatcher-%d", current->pid);
	if (IS_ERR(worker)) {
		err = PTR_ERR(worker);
		goto err;
	}

	wake_up_process(worker);

	return 0;
err:
	return err;
}

static void dispatcher_worker_stop(void)
{
	if (worker) {
		kthread_stop(worker);
		worker = NULL;
	}
}

static inline struct task_struct* manager_task(pid_t pid)
{
	return pid_task(find_vpid(pid), PIDTYPE_PID);
}	

static int dispatcher_open(struct inode *inode, struct file *f)
{
	return 0;
}

static int dispatcher_release(struct inode *inode, struct file *f)
{
	return 0;
}

static long dispatcher_ioctl(struct file *f, unsigned int ioctl,
		unsigned long arg)
{
	struct dispatcher_work *rx_work;
	struct dispatcher_work *tx_work;
	struct task_struct *task;
	void __user *argp = (void __user *)arg;
	pid_t pid = (pid_t __user)arg;

	switch (ioctl) {
		case DISPATCHER_SET_MANAGER:
			printk("Set manager on disptacher worker\n");
			task = manager_task(pid);
			if (!task) {
				printk("Could not find manager task\n");
				break;
			}

			if (dispatcher_worker_start(task) < 0) {
				printk("Failed to start PacketNgin worker\n");
				break;
			}

			printk("Set promiscuity of Network Device\n");
			rtnl_lock();
			dev_set_promiscuity(dispatcher_netdev, 1);
			rtnl_unlock();
			return 0;

		case DISPATCHER_UNSET_MANAGER:
			printk("Unset manager on disptacher worker\n");
			dispatcher_enabled = false;

			dispatcher_work_queue_flush();	
			dispatcher_netdev->netdev_ops->ndo_open(dispatcher_netdev);

			dispatcher_worker_stop();

			printk("Unset promiscuity of Network Device\n");
			rtnl_lock();
			dev_set_promiscuity(dispatcher_netdev, -1);
			rtnl_unlock();
			return 0;

		case DISPATCHER_REGISTER_NIC:
			printk("Register NIC on dispatcher device\n");
			nics = (Map *)argp;	

			/* Virtio specific code */
			void dispatcher_rx(void *data);
			void dispatcher_tx(void *data);
			rx_work = alloc_dispatcher_work(dispatcher_rx, NULL);
			tx_work = alloc_dispatcher_work(dispatcher_tx, NULL);

			if(!tx_work || !rx_work) {
				printk("Failed to alloc work\n");
				return -ENOMEM;
			}

			dispatcher_netdev->netdev_ops->ndo_stop(dispatcher_netdev);

			dispatcher_work_queue(rx_work);
			dispatcher_work_queue(tx_work);

			dispatcher_enabled = true;
			return 0;

		case DISPATCHER_UNREGISTER_NIC:
			printk("Unregister NIC on dispatcher device\n");
			dispatcher_enabled = false;

			dispatcher_work_queue_flush();	
			dispatcher_netdev->netdev_ops->ndo_open(dispatcher_netdev);
			return 0;
		default:
			printk("IOCTL %d does not supported\n", ioctl);
			return -EFAULT;
	}

	return -EFAULT;
}

static const struct file_operations dispatcher_fops = {
	.owner          = THIS_MODULE,
	.release        = dispatcher_release,
	.unlocked_ioctl = dispatcher_ioctl,
	.open           = dispatcher_open,
	.llseek		= noop_llseek,
};

struct miscdevice dispatcher_misc = {
	MISC_DYNAMIC_MINOR,
	"dispatcher",
	&dispatcher_fops,
};

int dispatcher_init(struct net_device* dev)
{
	printk("PacketNgin network dispatcher initialized\n");
	dispatcher_netdev = dev;
	return misc_register(&dispatcher_misc);
}

void dispatcher_exit(void)
{
	printk("PacketNgin network dispatcher terminated\n");
	dispatcher_worker_stop();
	misc_deregister(&dispatcher_misc);
}
