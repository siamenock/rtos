const RPC_MAX_NIC_COUNT	= 32;
const RPC_MAX_ARGS_SIZE	= 4096;
const RPC_MAX_URL_SIZE	= 2048;
const RPC_MAX_MD_SIZE	= 32;
const RPC_MAX_VM_COUNT	= 128;
const RPC_MAX_CALLBACK_COUNT	= 256;

enum RPC_VMStatus {
	RPC_NONE = 0,
	RPC_START = 1,
	RPC_PAUSE = 2,
	RPC_RESUME = 3,
	RPC_STOP = 4
};

enum RPC_MessageDigestType {
	RPC_MD5 = 1
};

struct RPC_NIC {
	unsigned hyper	mac;
	unsigned int	port;
	unsigned int	input_buffer_size;
	unsigned int	output_buffer_size;
	unsigned hyper	input_bandwidth;
	unsigned hyper	output_bandwidth;
	unsigned int	pool_size;
};

struct RPC_VM {
	unsigned int	core_num;
	unsigned int	memory_size;
	unsigned int	storage_size;
	RPC_NIC		nics<RPC_MAX_NIC_COUNT>;
	opaque		args<RPC_MAX_ARGS_SIZE>;
};

struct RPC_Address {
	unsigned hyper	ip;
	unsigned int	port;
};

typedef opaque RPC_Digest<RPC_MAX_MD_SIZE>;
typedef RPC_Address RPC_Addresses<RPC_MAX_CALLBACK_COUNT>;
typedef unsigned hyper RPC_VMList<RPC_MAX_VM_COUNT>;

program MANAGER {
	version MANAGER_APPLE {
		void MANAGER_NULL(void) = 0;
		
		unsigned hyper VM_CREATE(RPC_VM vm) = 1;
		RPC_VM VM_GET(unsigned hyper vmid) = 2;
		bool VM_SET(unsigned hyper vmid, RPC_VM vm) = 3;
		bool VM_DELETE(unsigned hyper vmid) = 4;
		RPC_VMList VM_LIST() = 16;
		
		bool STATUS_SET(unsigned hyper vmid, RPC_VMStatus status) = 5;
		RPC_VMStatus STATUS_GET(unsigned hyper vmid) = 6;
		
		RPC_Digest STORAGE_DIGEST(unsigned hyper vmid, RPC_MessageDigestType type, unsigned hyper size) = 12;
		
		bool CALLBACK_ADD(RPC_Address addr) = 13;
		bool CALLBACK_REMOVE(RPC_Address addr) = 14;
		RPC_Addresses CALLBACK_LIST() = 15;
	} = 1;
} = 0x20000100;
