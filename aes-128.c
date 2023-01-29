#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<string.h>

#define ROTL8(x,shift) ((uint8_t) ((x) << (shift)) | ((x) >> (8 - (shift))))

// See: https://github.com/spotify/linux/blob/master/include/linux/bitops.h#L83
uint32_t ror32(uint32_t word, unsigned int shift) {
	return (word >> shift) | (word << (32 - shift));
}

// See: https://github.com/torvalds/linux/blob/master/lib/crypto/aes.c#L91
uint32_t mul_by_x(uint32_t w) {
	uint32_t x = w & 0x7f7f7f7f;
	uint32_t y = w & 0x80808080;
	return (x << 1) ^ (y >> 7) * 0x1b;
}

// See: https://en.wikipedia.org/wiki/Rijndael_S-box#Example_implementation_in_C_language
void init_aes_s_box(uint8_t sbox[256]) {
	uint8_t p = 1;
	uint8_t q = 1;

	do {
		p = p ^ (p << 1) ^ (p & 0x80 ? 0x1B : 0);

		q ^= q << 1;
		q ^= q << 2;
		q ^= q << 4;
		q ^= q & 0x80 ? 0x09 : 0;

		uint8_t xformed = q ^ ROTL8(q, 1) ^ ROTL8(q, 2) ^ ROTL8(q, 3) ^ ROTL8(q, 4);

		sbox[p] = xformed ^ 0x63;
	} while(p != 1);

	sbox[0] = 0x63;
}

void add_round_key(uint8_t *key, uint8_t *block) {
	// Go over each byte in the block
	for (int i = 0; i < 16; i++) {
		block[i] = block[i] ^ key[i];
	}
}

void sub_bytes(uint8_t sbox[256], uint8_t *block) {
	for (int i = 0; i < 16; i++) {
		block[i] = sbox[block[i]];
	}
}

void shift_rows(uint8_t *block) {
	// TODO: Make more efficient
	uint8_t over1 = 0;
	uint8_t over2 = 0;

	// 2nd row, move every item by one
	over1 = block[1];
	block[1] = block[5];
	block[5] = block[9];
	block[9] = block[13];
	block[13] = over1;

	// 3rd row, move every item by two
	over1 = block[2];
	over2 = block[6];
	block[2] = block[10];
	block[6] = block[14];
	block[10] = over1;
	block[14] = over2;

	// 4th row, move every item by three
	over1 = block[15];
	block[15] = block[11];
	block[11] = block[7];
	block[7] = block[3];
	block[3] = over1;
}

void mix_columns(uint32_t *block) {
	for (int i = 0; i < 4; i++) {
		uint32_t x = block[i];
		uint32_t y = mul_by_x(x) ^ ror32(x, 16);
		block[i] = y ^ ror32(x ^ y, 8);
	}
}

void print_roundkeys(uint8_t *roundkeys) {
	printf("\nRoundkeys: \n");
	for (int i = 0; i < 16 * 11; i++) {
		if (i != 0 && i % 16 == 0) printf(" ");
		if (i != 0 && i % 48 == 0) printf("\n");
		// printf("%c", roundkeys[i]);
		printf("%02x", roundkeys[i]);
	}
	printf("\n\n");
}

void print_data(uint8_t *data) {
	printf("Data:     ");
	for (int i = 0; i < 16; i++) {
		printf("%02X", data[i]);
	}
	printf("\n");
}

int main() {
	int rounds = 10;
	int block_size = 16;

	// char *data = "hello world, this is my secret";
	// char *key = "k38duvhd901nd830";
	char *data = "Two One Nine Two";
	char *key = "Thats my Kung Fu";

	printf("Data: %s\n", data);
	printf("Key: %s\n", key);

	uint8_t sbox[256] = {};
	init_aes_s_box(sbox);

	// Print the table
	/* for (int i = 0; i < 256; i++) {
		if (i != 0 && i % (0x0f + 1) == 0) printf("\n");
		printf("0x%02x ", sbox[i]);
	}
	printf("\n"); */

	// Split the data into chunks of 16 bytes / 128 bits
	size_t data_size = strlen(data);
	size_t overflow = block_size - data_size % block_size;
	overflow = overflow == 16 ? 0 : overflow;
	size_t output_size = data_size + overflow;
	size_t alloc = block_size * (data_size / block_size + (overflow != 0 ? 1 : 0));

	/* printf("\n");
	printf("Overflow: %zu\n", overflow);
	printf("Size: %zu\n", data_size);
	printf("Malloc: %zu\n", alloc); */

	uint8_t *output = (uint8_t *) malloc(alloc);
	memcpy(output, data, data_size);
	memset((char*) output + data_size, overflow, overflow); // TODO: Is the overflow correct?

	// Expand the key and create the round keys
	size_t roundkey_size = block_size * (rounds + 1);
	uint8_t *roundkeys = (uint8_t *) malloc(roundkey_size);
	// memset(roundkeys, 0x3, roundkey_size);
	memcpy(roundkeys, key, block_size);

	// Create 10 additional roundkeys
	for (int r = 1, rc = 1; r <= 10; r++, rc = mul_by_x(rc)) {
		int offset = r * 4;

		// Has to be unsigned, otherwise ROR32 might bess it up
		// due to pulling in 0xf on bit shift on signed
		uint32_t *rkp = ((uint32_t *) roundkeys) + offset - 4; // Previous key
		uint32_t *rkc = ((uint32_t *) roundkeys) + offset; // Current key

		rkc[0] = rkp[3]; // Take 3, because we take the word before the new current

		// Same as the code below, but easier to understand
		/*
		uint8_t *bytes = (uint8_t *) rkc;
		bytes[0] = sbox[bytes[0]];
		bytes[1] = sbox[bytes[1]];
		bytes[2] = sbox[bytes[2]];
		bytes[3] = sbox[bytes[3]];
		*/

		rkc[0] = (sbox[(rkc[0] & 0xff)]) ^
				 (sbox[(rkc[0] >>  8) & 0xff] <<  8) ^
				 (sbox[(rkc[0] >> 16) & 0xff] << 16) ^
				 (sbox[(rkc[0] >> 24) & 0xff] << 24);

		rkc[0] = ror32(rkc[0], 8);

		// Wi-N ^ S(Wi-1 << 8) ^ Ci/N-1
		rkc[0] = rkp[0] ^ rkc[0] ^ rc;
		rkc[1] = rkp[1] ^ rkc[0];
		rkc[2] = rkp[2] ^ rkc[1];
		rkc[3] = rkp[3] ^ rkc[2];
	}

	print_roundkeys(roundkeys);

	// Encrypt the data
	// Iterate over each data block
	for (int i = 0; i < output_size / 16; i++) {
		printf("Encrypt block %i\n", i);

		uint8_t *block = ((uint8_t *) output) + i * block_size;

		// Initial AddRoundKey step
		add_round_key((uint8_t *) roundkeys, block);

		for (int r = 0; r <= 10; r++) {
			// SubBytes
			sub_bytes(sbox, block);

			// ShiftRows
			shift_rows(block);

			// MixColumns, skip if last round
			if (r != 10) {
				mix_columns((uint32_t *) block);
			}

			// AddRoundKey - r * 16 to get to the next key
			add_round_key(((uint8_t *) roundkeys) + r * 16, block);

			printf("Round %02i - ", r);
			print_data(output);
		}
	}

	// Print the final output in hex format
	printf("\n");
	print_data(output);

	// Created this key from a Node.js script with createCipheriv + aes-128-ecb + null as iv
	// char *expected = "8C8CBCA4CF0F694BC13C925FC8F5820D92C51FDD3027EAB45887AEE2BECF8905";
	// From https://www.simplilearn.com/tutorials/cryptography-tutorial/aes-encryption#how_does_aes_work
	char *expected = "29C3505F571420F6402299B31A02D73AB3E46F11BA8D2B97C18769449A89E868";
	printf("Expected: %s\n", expected);

	free(roundkeys);
	free(output);

	return 0;
}

