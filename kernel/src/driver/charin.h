#ifndef __DRIVER_CHARIN__
#define __DRIVER_CHARIN__

typedef void(*CharInCallback)();

typedef struct {
	int	(*init)(void* device, void* data);
	void	(*destroy)(int id);
	void	(*set_callback)(int id, CharInCallback callback);
} CharIn;

#endif /* __DRIVER_CHARIN__ */
