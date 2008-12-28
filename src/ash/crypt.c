/*
 * Kernelspace implementation for Rijndael's cryptography algorithm
 * AES-128, AES-192, AES-256 standards
 *
 * Created by:
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/**
 * Implementation of Euclid's Extended Algorithm to compute the inverse
 * in the GF(2^8) finite field, using x^8 + x^4 + x^3 + x + 1 to define the field
 * @a input polynomial
 * @return the inverse of the polynomial
 */
uint8_t inverse (uint8_t a) {
	uint8_t rem[2], aux[3], tweak;
	
	// 0 doesn't have an inverse, but we return 0 nonetheless
	// 1 is its own inverse
	if (a<=1)
		return a;
		
	// the iterative algorithm needs 2 recursive values
	// rem[0] should be in fact, x^8 + the below value, but
	// we can't use the bit for x^8 in a uint8_t. So as not to use
	// a uint16_t and waste 8 shifts every cycle of division, the first cycle oD will have to be 8.
	rem[0] = 0x1b;
	rem[1] = a;
	aux[0] = 0;
	aux[1] = 1;
	
	tweak = 1;	// needed to account for the missing x^8 bit in rem[0]
	
	while (rem[1] > 1) {
		uint8_t D, d, q, r;
		
		// divide rem[0] by rem[1]
		D = rem[0];
		d = rem[1];
		
		// divide D by d and obtain q (quotient) and r (remainder)
		q = 0;
		aux[2] = 0;	// aux[2] = q * aux[1] + aux[0], where * and + are in modulo 2, so
				// aux[2] shall be computed along with q

		// assume D >= d and d > 0
		while (D >= d || tweak) {
			uint8_t oD, od, shift;
			
			// get the bit for the poly order of D
			oD = 7;
			while ((D & (1 << oD)) == 0)
				oD--;
				
			// tweak so we take into account x^8 from first cycle of Ext Euclid.
			if (tweak == 1) {
				oD = 8;
				tweak = 0;
			}
			
			// get the bit for the poly order of d
			od = 7;
			while ((d & (1 << od)) == 0)
				od--;
			
			// the difference between them is a bit from q
			shift = oD - od;
			
			D = D ^ (d << shift);
			q += 1 << shift;
			aux[2] = aux[2] ^ (aux[1] << shift);	// need to use mod2 +, which is ^.
		}
		
		// if D < d, then r = D anyway, and q = 0 from initialisation
		// if d = 0, we shouldn't be dividing by that in the first place :D.
		r = D;
		
		// cycle the reminders for next step
		rem[0] = rem[1];
		rem[1] = r;
		
		aux[2] = aux[2] ^ aux[0];
		
		// cycle reminders for next step
		aux[0] = aux[1];
		aux[1] = aux[2];
	}

	return aux[1];
}


/**
 * Computes the lookup table for the SubBytes transform
 * @table is a preallocated array with 256 elements
 *
 */
void compute_lookup (uint8_t *table) {
	uint8_t c;
	int i, j, k;
	
	c = 0x63;	// constant
	
	for (i=0; i < 256; i++) {
		uint8_t inv;
		
		inv = inverse(i);
		table[i] = 0;
		
		
		for (j=0; j<8; j++) {
			uint8_t sum, b;
			
			// sum will hold the xored bits from constant and inv
			sum = (c & (1<<j)) >> j;
			
			// xor-ing the j bit itself
			sum^= (inv & (1<<j))>>j;
			
			// the other summed bits are j+4, j+5, j+6, j+7 mod 8.
			b = j+4;
			
			for (k = 1; k<=4; k++) {
				if (b>7) b-=8;		// wrap around 8 bits
				
				// get the b bit from inv to 0 position, and xor it in sum
				sum ^= (inv & (1<<b))>>b;
				b++;
			}
			
			// add the j-th bit to the table[i] value
			table[i] += sum << j;
		}
	}
}


// need a structure to pack parameters that are sent to crypting function
// which does the actual work, without having to alloc memory all the time
struct AES_bundle {
	int Nk;
	int Nr;
	uint8_t *key;
	uint8_t *SB_table;
};



/**
 * SubBytes transform
 * - substitutes every byte in state by the number in the lookup table
 */
void SubBytes (uint8_t in[4][4], uint8_t out[4][4], struct AES_bundle *ab) {
	int i,j;
	
	// do the sub
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			out[i][j] = ab->SB_table[in[i][j]];
}



/**
 * ShiftRows transform
 * -shifts left all rows with the corresponding row index
 * row 0 not shifted, row 1 shifted by 1, etc
 *
 */
void ShiftRows (uint8_t in[4][4], uint8_t out[4][4], struct AES_bundle *ab) {
	// copy first row
	memcpy(out, in, 4);
	
	// 2nd row
	memcpy(out[1], in[1]+1, 3);
	out[1][3] = in[1][0];
	
	// 3rd row
	memcpy(out[2], in[2]+2,2);
	memcpy(out[2]+2, in[2], 2);
	
	// 4th row
	memcpy(out[3]+1, in[3], 3);
	out[3][0] = in[3][3];
}


/**
 * Multiplies a byte-poly by x
 * will be used to compute coeficients in 4-word GF(2^8) poly multiply
 */
uint8_t xtime (uint8_t p) {
	uint8_t rez = p<<1;
	
	// check if i need to apply modulo m(x)
	if (p & 128) {
		rez ^= 0x1b;	
	}
	
	return rez;
}


/**
 * MixColumns transform
 * - multiplies each column mod x^4+1 with a fixed polynomial over GF(2^8) (yes, it's annoying)
 * - the poly is {03}x^3 + {01}x^2 + {01}x + {02}
 */
void MixColumns (uint8_t in[4][4], uint8_t out[4][4], struct AES_bundle *ab) {
	int c;
	
	// take each column c and do SOME MAGIC! :D
	for (c = 0; c < 4; c++) {
		out[0][c] = xtime(in[0][c]) ^ xtime(in[1][c]) ^ in[1][c] ^ in[2][c] ^ in[3][c];
		out[1][c] = in[0][c] ^ xtime(in[1][c]) ^ xtime(in[2][c]) ^ in[2][c] ^ in[3][c];
		out[2][c] = in[0][c] ^ in[1][c] ^ xtime(in[2][c]) ^ xtime(in[3][c]) ^ in[3][c];
		out[3][c] = xtime(in[0][c]) ^ in[0][c] ^ in[1][c] ^ in[2][c] ^ xtime(in[3][c]);
	}
}


/**
 */
void AddRoundKey (uint8_t in[4][4], uint8_t out[4][4], struct AES_bundle *ab) {
}


void KeyExpansion (struct AES_bundle *ab) {

}

/**
 * Crypts a matrix of 4x4 bytes using the AES_bundle parameters
 */
int cypher (uint8_t in[4][4], uint8_t out[4][4], struct AES_bundle *ab) {
	int r;
	uint8_t state1[4][4], state2[4][4];
		
	// matrix copy
	memcpy(state1, in, 16);
	
	AddRoundKey(state1, state2, ab);

	// matrix copy
	memcpy(state1, state2, 16);
	
	// do the crypting rounds
	// I will use state1 and state2 to minimize the needed number of matrix copy ops

	for (r = 1; r < ab->Nr; r++) {
		SubBytes(state1, state2, ab);
		ShiftRows(state2, state1, ab);
		MixColumns(state1, state2, ab);
		AddRoundKey(state2, state1, ab);
	}
	
	SubBytes(state1, state2, ab);
	ShiftRows(state2, state1, ab);
	AddRoundKey(state1, state2, ab);

	// matrix copy
	memcpy(out, state2, 16);

	return 0;
}



/**
 * Crypts an array of bytes using a key of given length
 * dest can be = src, but they must always match in size
 * @dest array of bytes after crypting, assumes it's allocated
 * @src array of bytes to crypt
 * @size MUST BE a multiple of 16 bytes
 * @key array of 4*Nk bytes containing the crypting key
 * @Nk the AES key length, in 4byte words (4,6 or 8)
 */
int AES_crypt (void *dest, void *src, int size, void *key, int Nk) {
	struct AES_bundle ab;
	int i,j,k;
	uint8_t in[4][4], out[4][4];
	
	ab.Nk = Nk;
	ab.key = (uint8_t*) key;
	ab.Nr = 10;		// number of rounds for the algorithm
	
	switch(Nk) {
		case 4: {ab.Nr = 10; break;}
		case 6: {ab.Nr = 12; break;}
		case 8: {ab.Nr = 14; break;}
		default: return -1;
	};
	

	in[0][0]=0xd4;
	in[0][1]=0xe0;
	in[0][2]=0xb8;
	in[0][3]=0x1e;
	in[1][0]=0xbf;
	in[1][1]=0xb4;
	in[1][2]=0x41;
	in[1][3]=0x27;
	in[2][0]=0x5d;
	in[2][1]=0x52;
	in[2][2]=0x11;
	in[2][3]=0x98;
	in[3][0]=0x30;
	in[3][1]=0xae;
	in[3][2]=0xf1;
	in[3][3]=0xe5;
	MixColumns(in,out,&ab);
	
	for (i=0;i<4;i++) {
		for (j=0;j<4;j++)
			printk("%2x ", out[i][j]);
		printk("\n");
	}
	return 0;
	
	// memory alloc for the whole crypting operation
	ab.SB_table = (uint8_t*) kmalloc (256, GFP_ATOMIC);
	if (!ab.SB_table)
		return -ENOMEM;
	
	if (!dest || !src)
		return -ENOMEM;
		
	// build the table
	compute_lookup(ab.SB_table);
	
	// crypt each block of 4x4 bytes
	for (k = 0; k<size; k+=16) {
		
		// copy to input matrix
		for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			in[i][j] = ((uint8_t*) src) [k + i + 4*j];
		
		// crypt
		cypher(in, out, &ab);
		
		// copy to dest
		for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			((uint8_t*) dest) [k + i + 4*j] = out[i][j];
	}	
		
	kfree(ab.SB_table);
	
	return 0;
}

