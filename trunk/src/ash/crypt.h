/*
 * Kernelspace implementation for Rijndael's cryptography algorithm
 * AES-128, AES-192, AES-256 standards
 *
 * Created by:
 * 			   Gabriel Sandu  <gabrim.san@gmail.com>
 *
 * For licensing information, see the file 'LICENSE'
 */
 
 #ifndef __CRYPT_H__
 #define __CRYPT_H__
 
 
 /**
 * Routines for crypting and decrypting an array of size bytes, using
 * a key of Nk words length. Each word has 4 bytes.
 * Nk can be = 4 (AES-128 bits, 4*4 = 16 bytes key)
 * Nk = 6 (AES-192 bits, 6*4 = 24 bytes key)
 * Nk = 8 (AES-256 bits, 8*4 = 32 bytes key)
 *
 * size MUST BE a multiple of 16 bytes, or it will get truncated and
 * last part will not be crypted or copied to dest.
 *
 * dest = src will work
 * src and dest should be already alocated and of the same size. No checks
 * are made. Same goes for key.
 */
 int AES_crypt (void *dest, void *src, int size, void *key, int Nk);
 int AES_decrypt (void *dest, void *src, int size, void *key, int Nk);
 
 #endif /* crypt.h */
