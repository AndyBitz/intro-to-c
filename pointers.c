#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>

void print_line(uint8_t *line) {
	for (uint8_t i = 0; i < 0x0f; i++) {
		printf("0x%02x ", line[i]);
	}
	printf("\n");
}

int main() {
	uint8_t *data = malloc(0xff);

	for (uint8_t i = 0; i < 0xff; i++) {
		data[i] = i;
	}

	for (uint8_t i = 0; i < 0xff; i++) {
		if (i != 0 && i % (0x0f + 0) == 0) printf("\n");
		printf("0x%02x ", data[i]);
	}
	printf("\n\n");

	// Will start at 0x0f, it will just add 0x0f to the original offset
	uint8_t *sub1 = ((uint8_t *) data) + 0x0f;
	print_line(sub1);

	// Will start at 0x0c because it multiplies 4 bytes (32 bits) by 0x0f
	uint8_t *sub2 = ((uint32_t *) data) + 0x0f;
	print_line(sub2);

	uint8_t *sub3 = ((uint8_t *) data) + 2 * 0xf;
	print_line(sub3);

	for (int i = 0; i < 0xf; i++) {
		sub3[i] = sub3[i] ^ 0xff;
	}

	print_line(sub3);

	free(data);

	return 0;
}
