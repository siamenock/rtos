void smbios_parse(SMBIOS_Parser* parser) {
	bool is_SMBIOS(uint8_t* p) {
		if(p[0] == '_' && p[1] == 'S' && p[2] == 'M' && p[3] == '_') {
			uint8_t len = p[5];
			uint8_t checksum = 0;
			for(int i = 0; i < len; i++)
				checksum += p[i];

			return checksum == 0;
		}
		return false;
	}
	
	SMBIOS_EntryPoint* find_SMBIOS() {
		uint8_t* p = (uint8_t*)0xf0000;
		while(p < (uint8_t*)0x100000) {
			if(is_SMBIOS(p))
				return (SMBIOS_EntryPoint*)p;
			
			p += 16;
		}
		
		return NULL;
	}
	
	SMBIOS_EntryPoint* smbios = find_SMBIOS();
	if(!smbios || (parser->parse_entrypoint && !parser->parse_entrypoint(smbios)))
		return;
	
	SMBIOS_Header* header = (SMBIOS_Header*)(uintptr_t)smbios->table_address;
	for(int i = 0; i < smbios->count; i++) {
		switch(header->type) {
			case 4:
				if(parser->parse_processor && !parser->parse_processor((SMBIOS_ProcessorInfo*)header))
					return;
				break;
		}
		
		uint16_t* eos = (void*)header + header->length;
		while(*eos != 0)
			eos = (void*)eos + 1;
		
		header = (void*)(eos + 1);
	}
}

