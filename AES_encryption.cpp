#include "AES_encryption.h"


#define BLOCKSIZEINBYTES 16

AES_encryption::AES_encryption(QString encryptionKey, int keyLength)
{
	//the key should be one of 3 sizes 128, 192, or 256
	if( encryptionKey.length() > (128/8) ) {
		m_keyLength = 128;
	} else if( encryptionKey.length() > (192/8) ) {
		m_keyLength = 192;
	} else {
		m_keyLength = 256;
	}

	//Setup our two byte array sizes
	mp_key = new unsigned char[ m_keyLength/8 ];
	
	//Clear out the key and IV byte arrays in preparation for use
	memset( mp_key, 0x00, m_keyLength/8 );
    
	//Now lets copy each byte of our encryptionKey into the "key" byte array
	QByteArray keyByteArray = encryptionKey.toAscii();
	int i = 0;
	while ( i < keyByteArray.length() ) {
		if ( i == m_keyLength -1 )					//Key is too long! Truncate it
			break;									//End the cpoy
		else
			mp_key[i] = keyByteArray[i];
		
		i++;
	}
	for ( i = keyByteArray.length(); i < m_keyLength/8; i++ ) {
		mp_key[i] = 0;
	}


	mp_CipherEngine = new AES;
	mp_CipherEngine->SetParameters( m_keyLength, BLOCKSIZEINBYTES * 8 );


}

AES_encryption::~AES_encryption(void)
{
	if (mp_key)
		delete[] mp_key;

	if (mp_CipherEngine)
		delete mp_CipherEngine;
}

QString AES_encryption::encrypt(const QString plaintext)
{
	if(!mp_key) {
		return "";
	}

	//since the keyLength is one block then we should be able to calculate the block size of the text
	int textSizeInBlocks = (plaintext.size() + 1) / BLOCKSIZEINBYTES;
	if ( (plaintext.size() + 1) % BLOCKSIZEINBYTES != 0 )
		textSizeInBlocks++;
	int textSizeInBytes = textSizeInBlocks * BLOCKSIZEINBYTES;

	//make arrays for the two strings
	unsigned char * cipherArray = new unsigned char[textSizeInBytes];
	unsigned char * plainTextArray = new unsigned char[textSizeInBytes];
	QByteArray cipherQByteArray;

	//fill the arrays
	for( int i = 0; i < plaintext.size(); i++ ) {
		plainTextArray[i] = plaintext[i].toLatin1();
	}
	plainTextArray[plaintext.size()] = 0;

	for( int i = plaintext.size() + 1; i < textSizeInBytes; i++ ) {
		plainTextArray[i] = (textSizeInBytes - (plaintext.length()+1));
	}

	//everything should be preped now lets do the encription.
	mp_CipherEngine->StartEncryption(mp_key);
	mp_CipherEngine->Encrypt( plainTextArray, cipherArray, textSizeInBlocks );


    for ( int i = 0; i < textSizeInBytes; i++) {
		cipherQByteArray.append(cipherArray[i]);
    }

	//Here is some magic!!! lets convert the cipherThing to HEX!!!!!
	cipherQByteArray = cipherQByteArray.toHex();

	//Now we need to clean up
	delete[] cipherArray;
	delete[] plainTextArray;

	return QString(cipherQByteArray);
}

QString AES_encryption::decrypt(QString ciphertext)
{
	if(!mp_key) {
		return "";
	}
	bool removePadding = true;

	QByteArray cipherQByteArray = QByteArray::fromHex(ciphertext.toLocal8Bit());



	//Assuming that since this has been encripted by our library that it will be an even block size i.e. NOT 3.5
	int textSizeInBytes = cipherQByteArray.size();
	int textSizeInBlocks = textSizeInBytes / BLOCKSIZEINBYTES;

	unsigned char * cipherArray = new unsigned char[textSizeInBytes];
	unsigned char * plainTextArray = new unsigned char[textSizeInBytes];

	for(int i = 0; i < textSizeInBytes; i++) {
		cipherArray[i] = cipherQByteArray[i];
	}

	mp_CipherEngine->StartDecryption(mp_key);
	mp_CipherEngine->Decrypt(cipherArray, plainTextArray, textSizeInBlocks);

	int charsToRemove = plainTextArray[textSizeInBytes - 1];

	if( charsToRemove < 16 && charsToRemove < ciphertext.size()) {
		for ( int i = 1; i < charsToRemove; i++ ) {
			if( plainTextArray[textSizeInBytes - i] != plainTextArray[textSizeInBytes - i - 1] ) {
				removePadding = false;
				break;
			}
		}
	}

	if( removePadding ) 
                textSizeInBytes -= charsToRemove;	

	QString returnValue = QString::fromLocal8Bit( (const char*)plainTextArray, textSizeInBytes);

	//clean up of memory
	delete[] cipherArray;
	delete[] plainTextArray;

	return returnValue;
}
