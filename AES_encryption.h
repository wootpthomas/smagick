#pragma once



/*****************
 Qt includes
*****************/
#include <QString>

/*****************
 * Memory Leak Detection
*****************/
#if defined(WIN32) && defined(_DEBUG) && ( ! defined(CC_MEM_OPTIMIZE))
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define new DEBUG_NEW
#endif


/*****************
  Includes
*****************/
#include "AES.h"

/** The purpose of this class is to wrap-up the Crypto++ AES
 * library and give us an easy way to encrypt/decrypt stuff */
class AES_encryption
{
public:
	AES_encryption(QString strEncryptionKey, int keyLength = 32);
	//I had the keyLength at CryptoPP::AES::DEFAULT_KEYLENGTH which is 16
	//bumped it up so that we can handle 32 char passwords

	~AES_encryption(void);

	QString encrypt(QString dataToBeEncrypted);

	QString decrypt(QString dataToBeDecrypted);

private:
	//Specifies the lenght of our key
	int m_keyLength;

	/* These are the key and IV used by the AES encryption class
	 * to encrypt/decrypt data */
	unsigned char *mp_key;
	int blockSize;

	std::string m_ciphertext;

	AES *mp_CipherEngine;
};
