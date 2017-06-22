#include <stdio.h>
#include <thread.h>
#include <nic.h>

void ginit(int argc, char** argv) {/* global init code goes here */}

void init(int argc, char** argv) {/* local init code goes here */}

void process(NIC* ni) {/* packet processing code goes here */}

void destroy() {/* local deinit code goes here */}

void gdestroy() {/* global deinit ocde goes here */}

int main(int argc, char** argv) {
	printf("PacketNgin Hello World: Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}

	thread_barrior();
	init(argc, argv);
	thread_barrior();

	thread_barrior();
	destroy();
	thread_barrior();

	if(thread_id() == 0)
		gdestroy();

	return 0;
}
