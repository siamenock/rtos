#ifndef __DRIVER_CHAROUT__
#define __DRIVER_CHAROUT__

typedef struct {
	char*	buf;
	int	len;
	int	is_capture;
	int	is_render;
} CharOutInit;

typedef struct {
	int	(*init)(void* device, void* data);
	void	(*destroy)(int id);
	int	(*write)(int id, const char* buf, int len);
	int	(*scroll)(int id, int lines);
	void	(*set_cursor)(int id, int rows, int cols);
	void	(*get_cursor)(int id, int* rows, int* cols);
	void	(*set_render)(int id, int is_render);
	int	(*is_render)(int id);
} CharOut;

#endif /* __DRIVER_CHAROUT__ */
