#ifndef __LINUX_PCI_IDS_H__
#define __LINUX_PCI_IDS_H__

#define PCI_VENDOR_ID_REALTEK           0x10ec
#define PCI_VENDOR_ID_DLINK             0x1186
#define PCI_VENDOR_ID_AT                0x1259
#define PCI_VENDOR_ID_GIGABYTE          0x1458
#define PCI_VENDOR_ID_LINKSYS           0x1737

#define PCI_ID_ANY	  ~(0)

typedef struct _PCI_ID {
	uint16_t		vendor_id;
	uint16_t		device_id;
	uint16_t		subvendor_id;
	uint16_t		subdevice_id;
	char*			name;
	void*			data;
} PCI_ID;

#endif /* __LINUX_PCI_IDS_H__ */
