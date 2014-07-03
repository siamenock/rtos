struct RPC_Message {
	unsigned hyper	vmid;
	unsigned int	coreid;
	unsigned int	fd;
	
	opaque		message<>;
};

program CALLBACK {
	version CALLBACK_APPLE {
		void CALLBACK_NULL(void) = 0;
		
		void STDIO(RPC_Message message) = 1;
	} = 1;
} = 0x20000101;
