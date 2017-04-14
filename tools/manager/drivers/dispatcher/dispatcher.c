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
#include <linux/ip.h>
#include <linux/if_vlan.h>
#include <linux/etherdevice.h>

#include "dispatcher.h"
//TODO
#include <util/map.h>
#include "driver/nic.h"
#include "core/include/util/map.h"
#include "vnic.h"

#define ETHER_MULTICAST	0xffffffffff

static struct task_struct *dispatcher_daemon;
static spinlock_t work_lock;
static struct list_head work_list;

rx_handler_result_t dispatcher_handle_frame(struct sk_buff** pskb);

struct dispatcher_work* alloc_dispatcher_work(dispatcher_work_fn_t fn,
		struct net_device *dev, void* data)
{
	struct dispatcher_work *work = kmalloc(sizeof(struct dispatcher_work),
			GFP_KERNEL);
	if (!work)
		return NULL;

	INIT_LIST_HEAD(&work->node);
	work->fn = fn;
	work->data = data;
	work->dev = dev;

	return work;
}

static struct dispatcher_work* dispatcher_work_by_netdev(struct net_device *dev)
{
	struct dispatcher_work *pos;

	list_for_each_entry(pos, &work_list, node) {
		if (dev == pos->dev)
			return pos;
	}

	return NULL;
}

static inline void dispatcher_work_enqueue(struct dispatcher_work *work)
{
	struct net_device *dev = work->dev;
	struct napi_struct *napi;

	list_for_each_entry(napi, &dev->napi_list, dev_list) {
		test_and_set_bit(NAPI_STATE_NPSVC, &napi->state);
	}

	spin_lock(&work_lock);
	list_add_tail(&work->node, &work_list);
	spin_unlock(&work_lock);

	rtnl_lock();
	if (netdev_rx_handler_register(dev,
			dispatcher_handle_frame, work->data) < 0)
		printk("Failed to register rx_handler\n");
	else
		printk("Register net_device handler %s %p\n", dev->name, dev);

	dev_set_promiscuity(dev, 1);
	printk("Set promiscuity of Network Device\n");
	rtnl_unlock();
}

static inline void dispatcher_work_dequeue(struct dispatcher_work *work)
{
	rtnl_lock();
	struct net_device *dev = work->dev;
	netdev_rx_handler_unregister(dev);
	printk("Unregister net_device handler\n");
	dev_set_promiscuity(dev, 1);
	printk("Unset promiscuity of Network Device\n");
	rtnl_unlock();

	spin_lock(&work_lock);
	list_del(&work->node);
	spin_unlock(&work_lock);

	struct napi_struct *napi;
	list_for_each_entry(napi, &dev->napi_list, dev_list) {
		clear_bit(NAPI_STATE_NPSVC, &napi->state);
		clear_bit(NAPI_STATE_SCHED, &napi->state);
	}
}

static inline void dispatcher_work_queue_flush(void)
{
	//unsigned long flags;
	struct dispatcher_work *pos;
	struct dispatcher_work *tmp;

	list_for_each_entry_safe(pos, tmp, &work_list, node) {
		rtnl_lock();
		struct net_device *dev = pos->dev;
		netdev_rx_handler_unregister(dev);
		printk("Unregister net_device handler\n");
		dev_set_promiscuity(dev, -1);
		printk("Unset promiscuity of Network Device\n");
		rtnl_unlock();

		struct napi_struct *napi;
		list_for_each_entry(napi, &dev->napi_list, dev_list) {
			clear_bit(NAPI_STATE_NPSVC, &napi->state);
			clear_bit(NAPI_STATE_SCHED, &napi->state);
		}

		spin_lock_irq(&work_lock);
		list_del(&pos->node);
		spin_unlock_irq(&work_lock);

		kfree(pos);
	}
}

static int dispatcherd(void *data)
{
	struct dispatcher_work *work = NULL;
	struct task_struct *task = (struct task_struct *)data;
	unsigned uninitialized_var(seq);
	struct list_head *pos;

	printk("PacketNgin dispatcher daemon created\n");
	use_mm(task->mm);
	//set_user_nice(current, -20);

	for (;;) {
		if (kthread_should_stop()) {
			printk("Stop dispatcher daemon\n");
			__set_current_state(TASK_RUNNING);
			break;
		}

		spin_lock_irq(&work_lock);
		if (!list_empty(&work_list)) {
			list_for_each(pos, &work_list) {
				work = list_entry(pos, struct dispatcher_work, node);
				work->fn(work);
			}
		}

		spin_unlock_irq(&work_lock);
		schedule();
	}

	printk("PacketNgin dispatcher deamon destroyed\n");
	unuse_mm(task->mm);
	return 0;
}

static int dispatcherd_start(struct task_struct *manager_task)
{
	int err;

	spin_lock_init(&work_lock);
	INIT_LIST_HEAD(&work_list);

	dispatcher_daemon = kthread_create(dispatcherd, manager_task,
			"dispatcherd-%d", current->pid);
	if (IS_ERR(dispatcher_daemon)) {
		err = PTR_ERR(dispatcher_daemon);
		goto err;
	}

	wake_up_process(dispatcher_daemon);

	return 0;
err:
	return err;
}

static void dispatcherd_stop(void)
{
	dispatcher_work_queue_flush();
	if (dispatcher_daemon) {
		printk("Kthread stop \n");
		kthread_stop(dispatcher_daemon);
		dispatcher_daemon = NULL;
	}
}

static inline struct task_struct* manager_task(pid_t pid)
{
	return pid_task(find_vpid(pid), PIDTYPE_PID);
}

rx_handler_result_t dispatcher_handle_frame(struct sk_buff** pskb)
{
	struct sk_buff *skb = *pskb;

	if (unlikely(skb->pkt_type == PACKET_LOOPBACK))
		return RX_HANDLER_PASS;

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source)) {
		kfree_skb(skb);
		return RX_HANDLER_CONSUMED;
	}

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return RX_HANDLER_CONSUMED;

	struct ethhdr *eth = (struct ethhdr*)skb_mac_header(skb);
	printk("Penguin: src mac %pM, dst mac %pM\n", eth->h_source, eth->h_dest);

	//WARN_ONCE(current->pid != dispatcher_daemon->pid,
	//	"Current PID: %d, Worker PID: %d\n", current->pid, dispatcher_daemon->pid);

	if(current->pid != dispatcher_daemon->pid) {
		printk("Current PID: %d, Worker PID: %d\n", current->pid, dispatcher_daemon->pid);
		return RX_HANDLER_PASS;
	}

	skb_linearize(skb);

	NICDevice* nic_device = rcu_dereference(skb->dev->rx_handler_data);

	BUG_ON(!nic_device);

	//TODO map_get vnic from vnics

	if(1) {
		MapIterator iter;
		map_iterator_init(&iter, nic_device->vnics);
		while(map_iterator_has_next(&iter)) {
			VNIC* vnic = (VNIC*)map_iterator_next(&iter);
			vnic_process_input(vnic, (void*)eth, ETH_HLEN + skb->len, NULL, 0);
		}
		//kfree_skb(skb);
	} else {
		VNIC* vnic = map_get(nic_device->vnics, eth->h_dest);
		if(vnic) {
			vnic_process_input(vnic, (void*)eth, ETH_HLEN + skb->len, NULL, 0);
			kfree_skb(skb);
			return RX_HANDLER_CONSUMED;
		}
	}

	return RX_HANDLER_PASS;
}

static int dispatcher_open(struct inode *inode, struct file *f)
{
	struct task_struct *task = manager_task(current->pid);
	if (!task) {
		printk("Could not find manager task\n");
		return -1;
	}

	if (dispatcherd_start(task) < 0) {
		printk("Failed to start PacketNgin dispatcher daemon\n");
		return -1;
	}

	printk("PacketNgin manager associated with disptacher\n");
	return 0;
}

static int dispatcher_release(struct inode *inode, struct file *f)
{
	dispatcherd_stop();
	printk("PacketNgin manager unassociated with disptacher\n");
	return 0;
}

/*static void poll_one_napi(struct napi_struct *napi)*/
/*{*/
	/*int work = 0;*/

	/* net_rx_action's ->poll() invocations and our's are
	 * synchronized by this test which is only made while
	 * holding the napi->poll_lock.
	 */
	/*if (!test_bit(NAPI_STATE_SCHED, &napi->state))*/
		/*return;*/

	/*
	 *static int index = 0;
	 *if(net_ratelimit())
	 *        printk("Polling Loop index :%d \n", index++);
	 */

	/* If we set this bit but see that it has already been set,
	 * that indicates that napi has been disabled and we need
	 * to abort this operation
	 */
	/*
	 *if (test_and_set_bit(NAPI_STATE_NPSVC, &napi->state))
	 *        return;
	 */

	/* We explicilty pass the polling call a budget of 0 to
	 * indicate that we are clearing the Tx path only.
	 */
	/*work = napi->poll(napi, 1);*/
	/*//WARN_ONCE(work, "%pF exceeded budget in poll\n", napi->poll);*/
	/*//trace_napi_poll(napi);*/

	/*//clear_bit(NAPI_STATE_NPSVC, &napi->state);*/
/*}*/

/*static void poll_napi(struct net_device *dev)*/
/*{*/
	/*struct napi_struct *napi;*/

	/*list_for_each_entry(napi, &dev->napi_list, dev_list) {*/
		/*
		 *if (napi->poll_owner != smp_processor_id() &&
		 *                spin_trylock(&napi->poll_lock)) {
		 */
		/*//poll_one_napi(napi);*/
		/*if (!test_bit(NAPI_STATE_SCHED, &napi->state))*/
			/*return;*/

		/*napi->poll(napi, 1);*/
		/*
		 *        spin_unlock(&napi->poll_lock);
		 *}
		 */
	/*}*/
/*}*/

/*static void netpoll_poll_dev(struct net_device *dev)*/
/*{*/
	/*const struct net_device_ops *ops;*/
	/*//struct netpoll_info *ni = rcu_dereference_bh(dev->npinfo);*/

	/* Don't do any rx activity if the dev_lock mutex is held
	 * the dev_open/close paths use this to block netpoll activity
	 * while changing device state
	 */
	/*
	 *if (down_trylock(&ni->dev_lock))
	 *        return;
	 */

	/*if (!netif_running(dev)) {*/
		/*//up(&ni->dev_lock);*/
		/*return;*/
	/*}*/

	/*ops = dev->netdev_ops;*/
	/*if (!ops->ndo_poll_controller) {*/
		/*//up(&ni->dev_lock);*/
		/*return;*/
	/*}*/

	/*[> Process pending work on NIC <]*/
	/*ops->ndo_poll_controller(dev);*/

	/*//poll_napi(dev);*/
	/*struct napi_struct *napi;*/
	/*list_for_each_entry(napi, &dev->napi_list, dev_list) {*/
		/*if (!test_bit(NAPI_STATE_SCHED, &napi->state))*/
			/*return;*/

		/*napi->poll(napi, 1);*/
	/*}*/

	/*//up(&ni->dev_lock);*/

	/*//zap_completion_queue();*/
/*}*/

/*static int netpoll_start_xmit(struct sk_buff *skb, struct net_device *dev)*/
/*{*/
	/*int status = NETDEV_TX_OK;*/
	/*netdev_features_t features;*/

	/*features = netif_skb_features(skb);*/

	/*if (skb_vlan_tag_present(skb) &&*/
	    /*!vlan_hw_offload_capable(features, skb->vlan_proto)) {*/
		/*skb = __vlan_hwaccel_push_inside(skb);*/
		/*if (unlikely(!skb)) {*/
			/* This is actually a packet drop, but we
			 * don't want the code that calls this
			 * function to try and operate on a NULL skb.
			 */
			/*goto out;*/
		/*}*/
	/*}*/

	/*status = __netdev_start_xmit(dev->netdev_ops, skb, dev, false);*/

/*out:*/
	/*return status;*/
/*}*/

/*[> call with IRQ disabled <]*/
/*void netpoll_send_skb_on_dev(struct sk_buff *skb, struct net_device *dev)*/
/*{*/
	/*int status = NETDEV_TX_BUSY;*/
	/*unsigned long tries;*/
	/*[> It is up to the caller to keep npinfo alive. <]*/
	/*//struct netpoll_info *npinfo;*/

	/*WARN_ON_ONCE(!irqs_disabled());*/

	/*//npinfo = rcu_dereference_bh(np->dev->npinfo);*/
	/*if (!netif_running(dev) || !netif_device_present(dev)) {*/
		/*dev_kfree_skb_irq(skb);*/
		/*return;*/
	/*}*/

	/*[> don't get messages out of order, and no recursion <]*/
	/*//if (skb_queue_len(&npinfo->txq) == 0 && !netpoll_owner_active(dev)) {*/
		/*//struct netdev_queue *txq;*/

		/*//txq = netdev_pick_tx(dev, skb, NULL);*/

		/*[> try until next clock tick <]*/
		/*for (tries = jiffies_to_usecs(1) / 50;*/
		     /*tries > 0; --tries) {*/
			/*//if (HARD_TX_TRYLOCK(dev, txq)) {*/
				/*//if (!netif_xmit_stopped(txq))*/
			/*//status = netpoll_start_xmit(skb, dev);*/
			/*int status = NETDEV_TX_OK;*/
			/*netdev_features_t features;*/

			/*features = netif_skb_features(skb);*/

			/*if (skb_vlan_tag_present(skb) &&*/
					/*!vlan_hw_offload_capable(features, skb->vlan_proto)) {*/
				/*skb = __vlan_hwaccel_push_inside(skb);*/
				/*if (unlikely(!skb)) {*/
					/* This is actually a packet drop, but we
					 * don't want the code that calls this
					 * function to try and operate on a NULL skb.
					 */
					/*goto out;*/
				/*}*/
			/*}*/

			/*status = __netdev_start_xmit(dev->netdev_ops, skb, dev, false);*/

/*out:*/

			/*//	HARD_TX_UNLOCK(dev, txq);*/

			/*if (status == NETDEV_TX_OK)*/
					/*break;*/

			/*//}*/

			/*[> tickle device maybe there is some cleanup <]*/
			/*//netpoll_poll_dev(np->dev);*/

			/*udelay(50);*/
		/*}*/

		/*WARN_ONCE(!irqs_disabled(),*/
			/*"netpoll_send_skb_on_dev(): %s enabled interrupts in poll (%pF)\n",*/
			/*dev->name, dev->netdev_ops->ndo_start_xmit);*/

	/*//}*/

	/*//if (status != NETDEV_TX_OK) {*/
	/*//	skb_queue_tail(&npinfo->txq, skb);*/
	/*//	schedule_delayed_work(&npinfo->tx_work,0);*/
	/*//}*/
/*}*/

static struct sk_buff* convert_to_skb(struct net_device* dev, Packet* packet)
{
	void* buf = packet->buffer + packet->start;
	unsigned int len = packet->end - packet->start;

	struct sk_buff* skb = netdev_alloc_skb_ip_align(dev, len);
	if (unlikely(!skb))
		return NULL;

	skb_put(skb, len);
	memcpy(skb->data, buf, len);

	printk("NIC free in convert to skb\n");
	nic_free(packet);
	return skb;
}

static inline void dispatcher_tx(struct net_device *dev)
{
	struct sk_buff* skb;
	Packet* packet;

	/*
	 *WARN_ONCE(current->pid != dispatcher_daemon->pid,
	 *        "Current PID: %d, Worker PID: %d\n", current->pid, dispatcher_daemon->pid);
	 */

	NICDevice* nic_device = rcu_dereference(dev->rx_handler_data);
	BUG_ON(!nic_device);

	/*
	 *unsigned long flags;
	 *local_irq_save(flags);
	 */

	//TODO map_iterator

	MapIterator iter;
	map_iterator_init(&iter, nic_device->vnics);
	while(map_iterator_has_next(&iter)) {
		VNIC* vnic = (VNIC*)map_iterator_next(&iter);

		while((packet = vnic_process_output(vnic)) != NULL) {
			printk("Fast path %p\n", packet);
			skb = convert_to_skb(dev, packet);
			if(!skb) {
				printk("Failed to convert skb\n");
				vnic_free(packet);
				continue;
			}

			//netpoll_send_skb_on_dev(skb, dev);
			if (!netif_running(dev) || !netif_device_present(dev)) {
				dev_kfree_skb_irq(skb);
				printk("not running\n");
				continue;
			}

			netdev_features_t features;

			features = netif_skb_features(skb);

			if (skb_vlan_tag_present(skb) &&
					!vlan_hw_offload_capable(features, skb->vlan_proto)) {
				skb = __vlan_hwaccel_push_inside(skb);
				if (unlikely(!skb)) {
					printk("skb_vlan_tag\n");
					continue;
				}
			}

			printk("__netdev_start_xmit\n");
			if (current->pid != dispatcher_daemon->pid) {
				printk("Current PID: %d, Worker PID: %d\n", current->pid, dispatcher_daemon->pid);
				return ;
			}
			__netdev_start_xmit(dev->netdev_ops, skb, dev, false);
		}
	}
	//local_irq_restore(flags);
}

static inline void dispatcher_rx(struct net_device *dev) {
	/*
	 *unsigned long flags;
	 *local_irq_save(flags);
	 */

	//const struct net_device_ops *ops;
	if (!netif_running(dev))
		return;

	//ops = dev->netdev_ops;
	//if (!ops->ndo_poll_controller)
	//	return;

	/* Process pending work on NIC */
	//ops->ndo_poll_controller(dev);

	struct napi_struct *napi;
	list_for_each_entry(napi, &dev->napi_list, dev_list) {
		if (!test_bit(NAPI_STATE_SCHED, &napi->state))
			return;

		napi->poll(napi, 1);
	}

//	local_irq_restore(flags);
}

static void dispatcher_worker(void* data) {
	struct dispatcher_work *work = data;
	struct net_device *dev = work->dev;

	dispatcher_rx(dev);
	dispatcher_tx(dev);
}

static long dispatcher_ioctl(struct file *f, unsigned int ioctl,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	struct dispatcher_work *work;
	struct net_device *dev;
	NICDevice* nic_device;

	switch (ioctl) {
		case DISPATCHER_REGISTER_NIC:
			printk("Register NIC on dispatcher device %p\n", argp);
			nic_device = (NICDevice *)argp;
			dev = __dev_get_by_name(&init_net, nic_device->name);
			if (!dev) {
				printk("Failed to find net_device for %s\n", nic_device->name);
				return -EINVAL;
			}

			work = alloc_dispatcher_work(dispatcher_worker, dev, (void *)nic_device);
			if (!work) {
				printk("Failed to alloc work\n");
				return -ENOMEM;
			}

			dispatcher_work_enqueue(work);

			return 0;

		case DISPATCHER_UNREGISTER_NIC:
			printk("Unregister NIC on dispatcher device\n");
			nic_device = (NICDevice *)argp;

			dev = __dev_get_by_name(&init_net, nic_device->name);
			if(!dev) {
				printk("Failed to find net_device for %s\n", nic_device->name);
				return -EINVAL;
			}

			work = dispatcher_work_by_netdev(dev);
			if (!work) {
				printk("Failed to find work associated with %s\n", nic_device->name);
				return -ENOMEM;
			}

			dispatcher_work_dequeue(work);

			return 0;
		case DISPATCHER_CREATE_VNIC:
			base, size
			vnic_create();
			return;

		case DISPATCHER_DESTROY_VNIC:
			vnic_destroy();
			return;

		case DISPATCHER_UPDATE_VNIC:
			return;
		case DISPATCHER_GET_VNIC:
			return

		case VNIC_REGIS_:
			return;
			
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

int dispatcher_init()
{
	return misc_register(&dispatcher_misc);
}

void dispatcher_exit(void)
{
	misc_deregister(&dispatcher_misc);
}

static int __init init(void)
{
	printk("PacketNgin network dispatcher initialized\n");
	dispatcher_init();
	return 0;
}

static void __exit fini(void)
{
	printk("PacketNgin network dispatcher terminated\n");
	dispatcher_exit();
}

module_init(init);
module_exit(fini);

MODULE_DESCRIPTION("PacketNgin dispatcher");
MODULE_LICENSE("GPL");
