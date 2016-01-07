
typedef struct {
	uint8_t		anchor[4];
	uint8_t		checksum;
	uint8_t		length;
	uint8_t		major;
	uint8_t		minor;
	uint16_t	max_structure_size;
	uint8_t		revision;
	uint8_t		formatted_area[5];
	uint8_t		endpoint_string[5];
	uint8_t		checksum2;
	uint16_t	table_length;
	uint32_t	table_address;
	uint16_t	count;
	uint8_t		bcd_revision;
} __attribute__((packed)) SMBIOS_EntryPoint;

typedef struct {
	uint8_t		type;
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_Header;

typedef struct {
	uint8_t		type;		// 0
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_BIOSInfo;

typedef struct {
	uint8_t		type;		// 1
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_SystemInfo;

typedef struct {
	uint8_t		type;		// 2
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_BaseboardInfo;

typedef struct {
	uint8_t		type;		// 3
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_ChassisInfo;

typedef struct {
	uint8_t		type;		// 4
	uint8_t		length;
	uint16_t	handle;

	uint8_t		socket;
	uint8_t		processor_type;
	uint8_t		processor_family;
	uint8_t		processor_manufacturer;
	uint64_t	processor_id;
	uint8_t		processor_version;
	uint8_t		voltage;
	uint16_t	external_clock;
	uint16_t	max_speed;
	uint16_t	current_speed;
	uint8_t		status;
	uint8_t		processor_upgrade;
	uint16_t	l1_cache_handle;
	uint16_t	l2_cache_handle;
	uint16_t	l3_cache_handle;
	uint8_t		serial_number;
	uint8_t		asset_tag;
	uint8_t		part_number;
	uint8_t		core_count;
	uint8_t		core_enabled;
	uint8_t		thread_count;
	uint16_t	processor_characteristics;
	uint16_t	processor_family2;
	uint16_t	core_count2;
	uint16_t	core_enabled2;
	uint16_t	thread_count2;
} __attribute__((packed)) SMBIOS_ProcessorInfo;

typedef struct {
	uint8_t		type;		// 7
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_CacheInfo;

typedef struct {
	uint8_t		type;		// 8
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_PortInfo;

typedef struct {
	uint8_t		type;		// 9
	uint8_t		length;
	uint16_t	handle;
} __attribute__((packed)) SMBIOS_SlotsInfo;

typedef struct {
	bool(*parse_entrypoint)(SMBIOS_EntryPoint* entry);
	bool(*parse_processor)(SMBIOS_ProcessorInfo* info);
} SMBIOS_Parser;

void smbios_parse(SMBIOS_Parser* parser);
