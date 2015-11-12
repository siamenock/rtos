#ifndef _VIRTIO_CONFIG_H_
#define _VIRTIO_CONFIG_H_

/* Status byte for guest to report progress, and synchronize features. */
/* We have seen device and processed generic fields (VIRTIO_CONFIG_F_VIRTIO) */
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
/* We have found a driver for the device. */
#define VIRTIO_CONFIG_S_DRIVER		2
/* Driver has used its parts of the config, and is happy */
#define VIRTIO_CONFIG_S_DRIVER_OK	4
/* Driver has finished configuring features */
#define VIRTIO_CONFIG_S_FEATURES_OK	8
/* We've given up on this device. */
#define VIRTIO_CONFIG_S_FAILED		0x80

/* Some virtio feature bits (currently bits 28 through 32) are reserved for the
 * transport being used (eg. virtio_ring), the rest are per-device feature
 * bits. */
#define VIRTIO_TRANSPORT_F_START	28
#define VIRTIO_TRANSPORT_F_END		33

/* Do we get callbacks when the ring is completely used, even if we've
 * suppressed them? */
#define VIRTIO_F_NOTIFY_ON_EMPTY	24

/* Can the device handle any descriptor layout? */
#define VIRTIO_F_ANY_LAYOUT		27

/* v1.0 compliant. */
#define VIRTIO_F_VERSION_1		32

/*
struct virtio_config_ops {
	void (*get)(struct virtio_device *vdev, uint32_t offset, void *buf, uint32_t len);
	void (*set)(struct virtio_device *vdev, uint32_t offset, const void *buf, uint32_t len);
	uint8_t (*get_status)(struct virtio_device *vdev);
	void (*set_status)(struct virtio_device *vdev, uint8_t status);
	void (*reset)(struct virtio_device *vdev);
	struct virtqueue* (*find_vq)(struct virtio_device *vdev, uint32_t index, void (*callback)(struct virtqueue *));
	void (*del_vq)(struct virtqueue *vq);
	uint32_t (*get_features)(struct virtio_device *vdev);
	void (*finalize_features)(struct virtio_device *vdev);
};
*/


#endif /* _VIRTIO_CONFIG_H_ */
