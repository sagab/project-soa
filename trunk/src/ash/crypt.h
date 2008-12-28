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
 
 int AES_crypt (void *dest, void *src, int size, void *key, int Nk);
 
 #endif /* crypt.h */
