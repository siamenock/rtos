#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

static void help() {
	printf("Usage: create [Core Option] [Memory Option] [Storage Option]\
			[NIC Option] [Arguments Option] \n");
}

int main(int argc, char *argv[]) {
	int opt; 
	int core = 1;
	int memory = 0x1000000;		// 16MB
	int storage = 0x1000000;	// 16MB
	int nic = 0;
	char* args = NULL;

	int area = -1, perimeter = -1, breadth = -1, length =-1;

	static struct option long_options[] = {
		{"core", required_argument, 0, 'c' },
		{"memory", required_argument, 0, 'm' },
		{"storage", required_argument, 0, 's' },
		{"nic",	required_argument, 0, 'n' },
		{"args", required_argument, 0, 'a' },
		{ 0, 0, 0, 0 }
	};

	enum {
		RO_OPT = 0,
		RW_OPT,
		NAME_OPT
	};
	
	char* const token[] = {
		[RO_OPT]   = "ro",
		[RW_OPT]   = "rw",
		[NAME_OPT] = "name",
		NULL
	};

	char *subopts;
	char *value;

	int readonly = 0;
	int readwrite = 0;
	char *name = NULL;
	int error = 0;

	int long_index = 0;
	while((opt = getopt_long(argc, argv,"c:m:s:n:a:", 
					long_options, &long_index)) != -1) {
		switch(opt) {
			case 'c' : 
				core = atoi(optarg);
				break;
			case 'm' : 
				memory = atol(optarg);
				break;
			case 's' : 
				storage = atol(optarg); 
				break;
			case 'n' : 
				subopts = optarg;
				while(*subopts != '\0') {

					switch(getsubopt(&subopts, token, &value)) {
						case RO_OPT:
							readonly = 1;
							break;

						case RW_OPT:
							readwrite = 1;
							break;

						case NAME_OPT:
							if (value == NULL) {
								fprintf(stderr, "Missing value for "
										"suboption '%s'\n", token[NAME_OPT]);
								help();
								exit(EXIT_FAILURE);
							}

							name = value;
							break;

						default:
							fprintf(stderr, "No match found "
									"for token: /%s/\n", value);
							help();
							exit(EXIT_FAILURE);
							break;
					}
				}

				nic++;
				break;
			case 'a' :
				args = optarg;
				break;
			default: 
				help(); 
				exit(EXIT_FAILURE);
		}
	}


	printf("Core : %x, Memory : %x, Storage : %x, Nic: %d, Args: %s Readonly : %d Name : %s Readwrite : %d\n", core, memory, storage, nic, args, readonly, name, readwrite);

	return 0;
}

