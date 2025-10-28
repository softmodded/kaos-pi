#ifndef _RIJNDAEL_H_
#define _RIJNDAEL_H_

/*****************************************************************************
 * rijndael.h - AES encryption/decryption header
 *****************************************************************************/

#define KEYBITS 128

int rijndaelSetupEncrypt(unsigned long *rk, const unsigned char *key,
  int keybits);
int rijndaelSetupDecrypt(unsigned long *rk, const unsigned char *key,
  int keybits);
void rijndaelEncrypt(const unsigned long *rk, int nrounds,
  const unsigned char plaintext[16], unsigned char ciphertext[16]);
void rijndaelDecrypt(const unsigned long *rk, int nrounds,
  const unsigned char ciphertext[16], unsigned char plaintext[16]);

#define MAXKC   (256/32)
#define MAXKB   (256/8)
#define MAXNR   14

#endif /* _RIJNDAEL_H_ */