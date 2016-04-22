#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <thread.h>
#include <openssl/des.h>

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

void destroy() {
}

void gdestroy() {
}

char * Encrypt( char *Key, char *Msg, int size) {
        static char*    Res;
        int             n=0;
        DES_cblock      Key2;
        DES_key_schedule schedule;
 
        Res = ( char * ) malloc( size );
 
        /* Prepare the key for use with DES_cfb64_encrypt */
        memcpy( Key2, Key,8);
        DES_set_odd_parity( &Key2 );
        DES_set_key_checked( &Key2, &schedule );
 
        /* Encryption occurs here */
        DES_cfb64_encrypt( ( unsigned char * ) Msg, ( unsigned char * ) Res,
                           size, &schedule, &Key2, &n, DES_ENCRYPT );
 
         return (Res);
}
 
char * Decrypt( char *Key, char *Msg, int size) {
        static char*    Res;
        int             n=0;
 
        DES_cblock      Key2;
        DES_key_schedule schedule;
 
        Res = ( char * ) malloc( size );
 
        /* Prepare the key for use with DES_cfb64_encrypt */
        memcpy( Key2, Key,8);
        DES_set_odd_parity( &Key2 );
        DES_set_key_checked( &Key2, &schedule );
 
        /* Decryption occurs here */
        DES_cfb64_encrypt( ( unsigned char * ) Msg, ( unsigned char * ) Res,
                           size, &schedule, &Key2, &n, DES_DECRYPT );
 
        return (Res);
 
}

int main(int argc, char** argv) {
	printf("Test %d\n", argc);
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	char key[] = "password";
	char message[] = "Hello World";
	
	printf("Plain text: ");
	for(int i = 0; i < sizeof(message); i++) {
		printf("%02x", message[i]);
	}
	printf("\n");
	
	char* encrypted = Encrypt(key, message, sizeof(message));
	printf("Encry text: ");
	for(int i = 0; i < sizeof(message); i++) {
		printf("%02x", encrypted[i]);
	}
	printf("\n");
	
	char* decrypted = Decrypt(key, encrypted, sizeof(message));
	printf("Decry text: ");
	for(int i = 0; i < sizeof(message); i++) {
		printf("%02x", decrypted[i]);
	}
	printf("\n");
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
