#include<stdlib.h>
#include<stdio.h>

void print_data(unsigned char *data, int size) {
	printf("\n");
	for (int i = 0; i < size; i++) {
		if (i != 0 && i % 10 == 0) printf("\n");
		printf("0x%02x ", data[i]);
	}
	printf("\n");
}

int main() {
	int size = 32;
	unsigned char *data = malloc(size);

	print_data(data, size);
	data[3] = 0xff;
	print_data(data, size);
	data[3] = data[3] << 8;
	print_data(data, size);

	printf("\n---\n");

	print_data(data, size);
	data[3] = 0xff;
	print_data(data, size);
	// ((int *) data)[0] = ((int *) data)[0] >> 4;
	((int *) data)[0] = (int) 0x000000ee;
	print_data(data, size);

	printf("\n---\n");

	// Has to be unsigned, otherwise >> would pull in 0xf
	unsigned int foo = 0xff;
	printf("foo = 0x%08x\n", foo);
	foo = foo << 8;
	printf("foo = 0x%08x\n", foo);
	foo = foo << 8;
	printf("foo = 0x%08x\n", foo);
	foo = foo << 8;
	printf("foo = 0x%08x\n", foo);
	foo = (foo << 8) | (foo >> 24);
	printf("foo = 0x%08x\n", foo);

	free(data);
	return 0;
}
