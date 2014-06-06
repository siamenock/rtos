# REST/JSON Representation
Authentication: use HTTP authentication
ACL: To be done
User management: To be done

MAC accress notation
	00:11:22:33:44:55
	00:00:00:00:00:00 means dynamic allocation

HTTP Error
	400: JSON syntax error
	401: Permission denied
	410: There is no such resource
	507: Not enough resource to allocate
	509: The resource already allocated
	500: Unknown(Internal) error

# Implementation status
| Function		| Status|
|-----------------------|-------|
| Create VM		| O	|
| Get VM status		| O	|
| Update VM spec	|	|
| Delete VM		| O	|
| VM power start	| O	|
| VM power stop		| O	|
| VM power puase	|	|
| VM power resume	|	|
| VM power get		| O	|
| VM storage up		| O	|
| VM storage down	| O	|
| VM storage del	| O	|
| Thread input		| O	|
| Thread output		| O	|
| Thread error		| O	|


## VM management
/vm - Manage VM
	Post - Create VM
		Request
			{
				core_num: number
				memroy_size: number
				storage_size: number
				nics: {
					mac: string
					input_buffer_size: number
					output_buffer_size: number
					input_bandwidth: number
					output_bandwidth: number
					pool_size: number
				}[]
				args: string[]
			}
		
		Response
			pid: number
		
		Error: 400, 401, 507, 509
		
/vm/[pid:number]
	Get - Get VM status
		Request
			?q=cpu,memory,storage,nic[00:11:22:33:44:55],nic[00:55:44:33:22:11]
		
		Response
			{
				cpu: {
					usage: number
					total: number
				}[]
				memory: {
					heap: {
						usage: number
						total: number
					}[]
					stack: {
						usage: number
						total: number
					}[]
					global_heap: {
						usage: number
						total: number
					}
				}
				nic: {
					input: {
						buffer: {
							usage: number
							total: number
						}
						bandwidth: {
							usage: number
							total: number
						}
						bps: number
						pps: number
						dbps: number
						dpps: number
					}
					output: {
						buffer: {
							usage: number
							total: number
						}
						bandwidth: {
							usage: number
							total: number
						}
						bps: number
						pps: number
						dbps: number
						dpps: number
					}
					pool: {
						usage: number
						total: number
					}
				}[]
			}
		
		Error: 401, 410
	
	Put - Update VM spec dynamically (TODO: To be done)
		Request
			{
				TODO: Fill it
				status: string - "stop", "pause", "start"
			}
		
		Response
			result: boolean - Always true
		
		Error: 400, 401, 410
		
	Delete - Stop and Delete VM
		Response
			result: boolean - Always true
		
		Error: 401, 410

## VM power management
/vm/[pid:number]/power
	Put - Power on, pause, off
		Request
			"stop", "pause", "start" or "resume"
		
		Response
			result: boolean - True when status is changed.
			or error code: int
		
		Error: 400, 401, 410, 500
	
	Get - Get power status
		Response
			"stopped", "paused" or "started"
		
		Error: 410

## VM storage management
/vm/[pid:number]/storage - Manage VM's storage image
	/ Post - Transfer image file
		Request - Content-Type: "application/octet-stream"
		
		Response
			md5 checksum string in hexical encoding
		
		Error: 401, 410, 507
		
	/ Get - Download image file
		Response - Content-Type: "application/octet-stream"
			Binary stream
		
		Error: 410
	
	/ Delete - Clear storage
		Response
			true
		
		Error: 410

## Thread I/O/E
// Standard I/O protocol (all numeric value is represented in hexadecimal with ascii code)
	[I/O number]\r\n	- Standard input: 0, output: 1, error: 2
	[core number]\r\n
	[size]\r\n
	[array of text]

/vm/[pid:number]/stdio - Core's standard I/O/E stream
	/ Post - Send and receive I/O/E stream
		Request - Content-Type: "application/octet-stream"
		
		Error: 401, 410
		
		Response - Content-Type: "application/octet-stream"
		
		Error: 401, 410
