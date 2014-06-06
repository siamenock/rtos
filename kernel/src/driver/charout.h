#ifndef __DRIVER_CHAROUT__
#define __DRIVER_CHAROUT__

typedef struct {
	int	(*init)(void* device, void* data);
	void	(*destroy)(int id);
	int	(*write)(int id, const char* buf, int len);
	int	(*scroll)(int id, int lines);
	void	(*set_buffer)(int id, char* buf, int len);
	void	(*set_cursor)(int id, int rows, int cols);
	void	(*get_cursor)(int id, int* rows, int* cols);
} CharOut;

#endif /* __DRIVER_CHAROUT__ */
