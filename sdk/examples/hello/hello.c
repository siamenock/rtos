int main(int argc, char** argv) {
	while(1) {
		char* video = (char*)0xb8000;
		*video++ = 'H';
		*video++ = 0x07;
		*video++ = 'e';
		*video++ = 0x07;
		*video++ = 'l';
		*video++ = 0x07;
		*video++ = 'l';
		*video++ = 0x07;
		*video++ = 'o';
		*video++ = 0x07;
	}
}
