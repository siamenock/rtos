static Map* nodes;

static int cmd_status(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	printf("=== Node status ===\n");
	MapIterator iter;
 	map_iterator_init(&iter, nodes);

	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		printf("%s ", (char*)entry->key);
	}
	printf("\n");

	return 0;
}

static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	static unsigned int next_link_id = 0;
	static unsigned int next_switch_id = 0;

	if(!((argc == 3) || (argc == 4))) { 
		return CMD_STATUS_WRONG_NUMBER;
	}
	
	if(strcmp(argv[1], "-l") == 0) {
		Node* source = (Node*)map_get(nodes, argv[2]);
		Node* destination = (Node*)map_get(nodes, argv[3]);
		
		if(!source) {
			printf("%s doesn't exist\n", argv[2]);
			return 2;
		} 
		
		if(!destination) {
			printf("%s doesn't exist\n", argv[3]);
			return 3;
		}

		char name[16];
		Group* link = link_create(source, destination);

		if(link) {
			sprintf(name, "l%u", next_link_id++);
			map_put(nodes, strdup(name), (void*)link);
		} else {
			printf("Link create failed\n");
			return errno;
		}
	} else if(strcmp(argv[1], "-s") == 0) {
		if(!is_uint8(argv[2])) {
			printf("Port count needs to be number\n");
			return 2;
		}
		
		int port_count = parse_uint8(argv[2]);

		if(port_count > MAX_PORT_SIZE) {
			printf("Too many port count\n");
			return 2;
		}
	
		char name[16];
		Group* s = switch_create(port_count);

		if(s) {
			sprintf(name, "s%d", next_switch_id++);
			map_put(nodes, strdup(name), (void*)s);
		} else {
			printf("Switch create failed\n");
			return errno;
		}
	} else {
		printf("Usage : create [-l | -s] [args ...]\n");
		return 1;
	}

	return 0;
}

static int cmd_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) { 
		return CMD_STATUS_WRONG_NUMBER;
	}

	Group* node = map_get(nodes, argv[1]);
	if(!node) {
		printf("%s doesn't exist\n", argv[1]);
		return 1;
	}

	bool ret;
	switch(node->type) { 
		case NODE_TYPE_LINK : 
			ret = link_destroy(node);

			if(ret == false) { 
				printf("Link %s destroy failed\n", argv[1]);
				return errno;
			} else {
				map_remove(nodes, argv[1]);
				return 0;
			}
			break;
		
		case NODE_TYPE_SWITCH : 
			ret = switch_destroy(node);

			if(ret == false) { 
				printf("Switch %s destroy failed\n", argv[1]);
				return errno;
			} else {
				map_remove(nodes, argv[1]);
				return 0;
			}
			break;

		default : 
			printf("Port cannot be destroyed\n");
			return 1;
	}
}

static int cmd_activate(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) { 
		return CMD_STATUS_WRONG_NUMBER;
	}

	Group* node = map_get(nodes, argv[1]);
	if(!node) {
		printf("%s doesn't exist\n", argv[1]);
		return 1;
	}
	
	switch(node->type) {
		case NODE_TYPE_LINK : 
			node->nodes[0]->is_active = true;
			node->nodes[1]->is_active = true;
			
			printf("Link %s is activated\n", argv[1]); 
			break;
		
	 	case NODE_TYPE_SWITCH :
			node->is_active = true;

			printf("Switch %s is activated\n", argv[1]);
			break;
		
		case NODE_TYPE_NE_PORT :
			node->is_active = true;
			
			printf("Port %s is activated\n", argv[1]);
			break;

		case NODE_TYPE_SWITCH_PORT :
			// not yet 
			break;
	}

	return 0;
}

static int cmd_deactivate(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) { 
		return CMD_STATUS_WRONG_NUMBER;
	}

	Group* node = map_get(nodes, argv[1]);

	if(!node) {
		printf("%s doesn't exist\n", argv[1]);
		return 1;
	}

	switch(node->type) {
		case NODE_TYPE_LINK : 
			node->nodes[0]->is_active = false;
			node->nodes[1]->is_active = false;
			
			printf("Link %s is deactivated\n", argv[1]); 
			break;
		
	 	case NODE_TYPE_SWITCH :
			node->is_active = false;

			printf("Switch %s is deactivated\n", argv[1]);
			break;
		
		case NODE_TYPE_NE_PORT :
			node->is_active = false;
			
			printf("Port %s is deactivated\n", argv[1]);
			break;

		case NODE_TYPE_SWITCH_PORT :
			// not yet 
			break;
	}

	return 0;
}

static int cmd_set(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) { 
		return CMD_STATUS_WRONG_NUMBER;
	}

	Group* node = map_get(nodes, argv[1]);
	if(!node) {
		printf("%s doesn't exist\n", argv[1]);
		return 1;
	}
	
	switch(node->type) {
		case NODE_TYPE_LINK : 
			for(int i = 2; i < argc; i++) {
				if(strcmp(argv[i], "band:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("bandwidth must be uint64\n");
						return i;
					}
					// TODO : setting for each line
					((Line*)node->nodes[0])->bandwidth = parse_uint64(argv[i]);
					((Line*)node->nodes[1])->bandwidth = parse_uint64(argv[i]);
				} else if(strcmp(argv[i], "error:") == 0) {
					// TODO : double ?
					i++;
					if(!is_uint16(argv[i])) {
						printf("error rate must be double\n");
						return i;
					}
				} else if(strcmp(argv[i], "jitter:") == 0) {
					// TODO : double ?	
					i++;
					if(!is_uint16(argv[i])) {
						printf("jitter must be uint16\n");
						return i;
					}
				} else if(strcmp(argv[i], "latency:") == 0) {
					i++;
					if(!is_uint64(argv[i])) {
						printf("latency must be uint64\n");
						return i;
					}
					((Line*)node->nodes[0])->latency = parse_uint64(argv[i]);
					((Line*)node->nodes[1])->latency = parse_uint64(argv[i]);
				} 
			}
			break;
		case NODE_TYPE_SWITCH :
			break;
		case NODE_TYPE_NE_PORT : 
			break;
		default : 
			printf("cannot get information from %s\n", argv[1]);
			break;
	}
		// not yet	
	
	return 0;
}

static int cmd_get(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) { 
		return CMD_STATUS_WRONG_NUMBER;
	}

	Group* link = map_get(nodes, argv[1]);

	if(!link) {
		printf("%s doesn't exist\n", argv[1]);
		return 1;
	}

	if(link->type != NODE_TYPE_LINK) {
		printf("%s is not a link\n", argv[1]);
		return 1;
	}

	printf("=== Link %s status ===\n", argv[1]);
	printf("			line 1			line 2\n");
	printf("active		:	%s		%s\n", 
			((Line*)link->nodes[0])->is_active ? "activated": "deactivated", 
			((Line*)link->nodes[1])->is_active ? "activated": "deactivated");
	printf("bandwidth 	:	%lu			%lu\n", 
			((Line*)link->nodes[0])->bandwidth, 
			((Line*)link->nodes[1])->bandwidth);
	printf("latency 	:	%lu			%lu\n", 
			((Line*)link->nodes[0])->latency, 
			((Line*)link->nodes[1])->latency);
	printf("jitter 		:	%.3f			%.3f\n", 
			((Line*)link->nodes[0])->jitter, 
			((Line*)link->nodes[1])->jitter);
	printf("error rate	:	%.3f			%.3f\n", 
			((Line*)link->nodes[0])->error_rate, 
			((Line*)link->nodes[1])->error_rate);

	return 0;
}

	/*
	nodes = map_create(255, map_string_hash, map_string_equals, NULL);

	// port count setting 
	static int ne_port_count = 32; // default 

	if(_argc > 2) {
		if(strcmp(_argv[1], "-p") == 0) {
			if(!is_uint8(_argv[2])) {
				printf("Port count needs to be number\n");
				return 1;
			} else {
				ne_port_count = parse_uint8(_argv[2]);
				
				if(ne_port_count > MAX_PORT_SIZE) {
					printf("Too many port count\n");
					return 1;
				}
			}
		}
	}

	
	// create network emulator port
	*/
