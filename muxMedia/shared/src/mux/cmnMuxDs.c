#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // open function
#include <unistd.h> // close function
#include <sys/ioctl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "libCmn.h"

#define	_DEBUG_MUX_DS				0

#define	MUX_DS_DEVICE		"/dev/muxds"

unsigned char mux_bind_data[32] = { 'M', 'U', 'X', 'L', 'A', 'B', ' ', 'H', 'D', 'M', 'I', 'O', 'V', 'E', 'R', 'I', 'P', ' ', 'H', '2', '6', '4', '/', '5',
		' ', '5', '0', '0', '7', '6', '2', '\0' };

//unsigned char mux_bind_data[32] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
unsigned char mux_data[8] = { "MBJAPJRF" };

#define DS_MAGIC 'D'
#define READ_ROM  				_IOR(DS_MAGIC, 1, unsigned int *)  
#define MATCH_ROM  				_IOR(DS_MAGIC, 2, unsigned int *)  
#define SEARCH_ROM	  			_IOR(DS_MAGIC, 3, unsigned int *)  
#define SKIP_ROM	 	 		_IOR(DS_MAGIC, 4, unsigned int *)  
#define RESUME		  			_IOR(DS_MAGIC, 5, unsigned int *)  
#define ODRIVE_SKIP ROM		  	_IOR(DS_MAGIC, 6, unsigned int *)  
#define ODRIVE_MATCH_ROM	  	_IOR(DS_MAGIC, 7, unsigned int *) 
#define LOAD_SECRET_TEST	  	_IOR(DS_MAGIC, 8, unsigned int *)  
#define LOAD_TEST_READ_MAC		_IOR(DS_MAGIC, 9, unsigned int *) 
#define READ_CHIP_MEM			_IOWR(DS_MAGIC, 10, unsigned int *)  
#define LOAD_CHALLENGE_READ_MAC	_IOWR(DS_MAGIC, 11, unsigned int *)  
#define SET_SECRET_PRODUCTION	_IOW(DS_MAGIC, 12, unsigned int *)  
#define INIT_EEPROM_PRODUCTION	_IOWR(DS_MAGIC, 13, unsigned int *) 

struct ds_msg {
	unsigned char rom_id[8];	//7 byte id(start from 0x2F) + 1 byte CRC
	unsigned char secret[8];	//secret write to chip
	unsigned char challenge[8];	//random data write to chip
	unsigned char mac[20];		//mac data read from chip
	unsigned char page_data[32];		//page data read from chip
	unsigned char bind_data[8];	//binding data for writing EEPROM
	unsigned char bind_addr;	//binding data write start address, or read address of read memory operation
	unsigned char page_num;		//page number
	unsigned char flag;			//default 0; 1--operation success, 0--operation fail
};

//constants used in SHA computation
static const long KTN[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

// calculation used for the SHA MAC
static long NLF(long B, long C, long D, int n)
{
	if (n < 20)
		return ((B & C) | ((~B) & D));
	else if (n < 40)
		return (B ^ C ^ D);
	else if (n < 60)
		return ((B & C) | (B & D) | (C & D));
	else
		return (B ^ C ^ D);
}

//----------------------------------------------------------------------
// Computes SHA-1 given the 64 byte MT digest buffer.  
//
// Note: This algorithm is the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
// 'MT'       - buffer containing the message digest
// 'MAC'      - result MAC in byte order used by 1-Wire device
//
static void _computeSHA(unsigned char* MT, unsigned char* MAC)
{
	unsigned long MTword[80];
	long hash[5];
	int i, j, offset;
	long ShftTmp;
	long Temp;

	for (i = 0; i < 16; i++)
			{
		MTword[i] = (((long) MT[i * 4] & 0x00FF) << 24)
				| (((long) MT[i * 4 + 1] & 0x00FF) << 16)
				| (((long) MT[i * 4 + 2] & 0x00FF) << 8)
				| ((long) MT[i * 4 + 3] & 0x00FF);
	}

	for (; i < 80; i++)
			{
		ShftTmp = MTword[i - 3] ^ MTword[i - 8] ^ MTword[i - 14] ^ MTword[i - 16];
		MTword[i] = ((ShftTmp << 1) & 0xFFFFFFFE) |
				((ShftTmp >> 31) & 0x00000001);
	}

	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;
	hash[4] = 0xC3D2E1F0;

	for (i = 0; i < 80; i++)
			{
		ShftTmp = ((hash[0] << 5) & 0xFFFFFFE0) | ((hash[0] >> 27) & 0x0000001F);
		Temp = NLF(hash[1], hash[2], hash[3], i) + hash[4]
				+ KTN[i / 20] + MTword[i] + ShftTmp;
		hash[4] = hash[3];
		hash[3] = hash[2];
		hash[2] = ((hash[1] << 30) & 0xC0000000) | ((hash[1] >> 2) & 0x3FFFFFFF);
		hash[1] = hash[0];
		hash[0] = Temp;
	}

	//iButtons use LSB first, so we have to turn
	//the result around a little bit.  Instead of
	//result A-B-C-D-E, our result is E-D-C-B-A,
	//where each letter represents four bytes of
	//the result.
	for (j = 4; j >= 0; j--)
	{
		Temp = hash[j];
		offset = (4 - j) * 4;
		for (i = 0; i < 4; i++)
		{
			MAC[i + offset] = (unsigned char) Temp;
			Temp >>= 8;
		}
	}
}

//----------------------------------------------------------------------
// Computes SHA-1 given the 64 byte MT digest buffer after first iserting
// the secret at the correct location in the array defined by all Maxim
// devices. Since part of the input is secret it is called a Message
// Autnentication Code (MAC).
//
// Note: This algorithm is the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
// 'MT'       - buffer containing the message digest
// 'MAC'      - result MAC in byte order used by 1-Wire device
//
static void _computeMAC(unsigned char* MT, unsigned char* MAC, unsigned char* secret)
{
	memcpy(&MT[0], &secret[0], 4);
	memcpy(&MT[48], &secret[4], 4);
	_computeSHA(MT, MAC);
}

static int _calc_crc(int data_byte, int crc)
{
	int bit_mask = 0, carry_check = 0, temp_data = 0;
	temp_data = data_byte;

	for (bit_mask = 0; bit_mask <= 7; bit_mask++)
	{
		data_byte = data_byte ^ crc;
		crc = crc / 2;
		temp_data = temp_data / 2;
		carry_check = data_byte & 0x01;
		if (carry_check)
		{
			crc = crc ^ 0x8C;
		}
		data_byte = temp_data;
	}
	return (crc);
}

//struct ds_msg chip_ds_msg;

static int readID(unsigned char * chip_id)
{
	struct ds_msg chip_ds_msg;
	int fd;
	int i;
	int testcrc, data_byte;
	unsigned char dev_info[8];

	if (chip_id == NULL)
		return -2;

	//init chip_ds_msg
	for (i = 0; i < 8; i++) 
	{
		chip_ds_msg.rom_id[i] = 0;
		chip_ds_msg.secret[i] = 0x5a;
		chip_ds_msg.challenge[i] = 0xa9;
	}
	
	chip_ds_msg.flag = 0;
	for (i = 0; i < 20; i++)
		chip_ds_msg.mac[i] = 0x0;
	for (i = 0; i < 32; i++)
		chip_ds_msg.page_data[i] = 0x0;
	chip_ds_msg.flag = 0;

	testcrc = 0x00;
	fd = open(MUX_DS_DEVICE, O_RDWR);
	if (fd < 0)
	{
		MUX_ERROR("can not open device file");
		return -1;
	}

	memset(dev_info, 0, sizeof(dev_info));

	if (ioctl(fd, READ_ROM, &chip_ds_msg) == -1)
	{
		close(fd);
		MUX_ERROR("DS IOCTL READ_ROM read ds error");
		return -3;
	}
	close(fd);

	for (i = 0; i < 8; i++)
	{
		dev_info[i] = chip_ds_msg.rom_id[i];
		//printf(" 0x%02x ", dev_info[i]);
	}

	testcrc = 0x00;
	for (i = 0; i < 7; i++)
	{
		data_byte = dev_info[i];
		testcrc = _calc_crc(data_byte, testcrc);
	}

	if (testcrc == dev_info[7])
	{
		//printf("read ROM ID OK!\n");
		memcpy(chip_id, dev_info, 8);
		return 0;
	}
	else
	{
		//printf("read ROM ID FAIL! Exit!\n");
		return -4;
	}
}

static int calSrtKey(unsigned char * mux_id, unsigned char * chip_id, unsigned char * srtKey)
{
	int i;
	if (mux_id == NULL || chip_id == NULL)
		return -1;

	for (i = 0; i < 8; i++)
	{
		srtKey[i] = mux_id[i] + chip_id[7 - i];
	}

	return 0;
}

static int writeSrtKey(unsigned char * srtKey)
{
	struct ds_msg chip_ds_msg;
	int fd, ret;

	memcpy(chip_ds_msg.secret, srtKey, 8);

	fd = open(MUX_DS_DEVICE, O_RDWR);
	if (fd < 0)
	{
		MUX_ERROR("can not open secure device!");
		return -1;
	}

	ret = ioctl(fd, SET_SECRET_PRODUCTION, &chip_ds_msg);
	if (ret == -1)
	{
		MUX_ERROR("DS IOCTL SET_SECRET_PRODUCTION FAIL!");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static int verifyMAC(unsigned char *mux_data)
{
	struct ds_msg chip_ds_msg;
	int i, fd, ret;
	unsigned char challenge[8];
	unsigned char secret[8];
	unsigned char page_data[32];
	unsigned char page_num;
	unsigned char SHA_MT[64];
	unsigned char calc_mac[20];
	int anon = 0;

	ret = readID(chip_ds_msg.rom_id);
	if (ret)
	{
		return -1;
	}

	fd = open(MUX_DS_DEVICE, O_RDWR);
	if (fd < 0)
	{
		MUX_ERROR("can not open /dev/muxds!");
		return -1;
	}

	/*for (i = 0; i < 8; i++) {
	 chip_ds_msg.challenge[i] = i;
	 }*/

	srand((unsigned) time(NULL));	//init random seed
	//printf("Set challenge to chip, read mac and verify.\n");
	for (i = 0; i < 8; i++)
	{
		chip_ds_msg.challenge[i] = rand() % 256;
	}

	//set page_num
	chip_ds_msg.page_num = 0;
	for (i = 0; i < 8; i++)
	{
		chip_ds_msg.secret[i] = 0x0;
	}

	//Read MAC from chip, this step does not need to send secret to chip. ONLY 8 byte challenge data is used.
	if (ioctl(fd, LOAD_CHALLENGE_READ_MAC, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL LOAD_CHALLENGE_READ_MAC FAIL!");
		return -2;
	}

	// calculate the MAC
	// print the info needed in MAC algorithm
	//printf("Calculate MAC from CPU software\n");

	ret = calSrtKey(mux_data, chip_ds_msg.rom_id, chip_ds_msg.secret);
	if (ret)
	{
		return -1;
	}

#if _DEBUG_MUX_DS
	printf("page_num = 0x%x\n", chip_ds_msg.page_num);
	for (i = 0; i < 8; i++)
		printf("chipID[%d] = 0x%x\n", i, chip_ds_msg.rom_id[i]);
	for (i = 0; i < 8; i++)
		printf("chip_ds_msg.secret[%d] = 0x%x\n", i, chip_ds_msg.secret[i]);
	for (i = 0; i < 8; i++)
		printf("chip_ds_msg.challenge[%d] = 0x%x\n", i, chip_ds_msg.challenge[i]);
	for (i = 0; i < 32; i++)
		printf("chip_ds_msg.page_data[%d] = 0x%x\n", i, chip_ds_msg.page_data[i]);
#endif

	memcpy(secret, &chip_ds_msg.secret[0], 8);
	memcpy(challenge, &chip_ds_msg.challenge[0], 8);
	memcpy(page_data, &chip_ds_msg.page_data[0], 32);
	page_num = chip_ds_msg.page_num;

	memcpy(&SHA_MT[4], page_data, 32);
	SHA_MT[36] = challenge[2];
	SHA_MT[37] = challenge[3];
	SHA_MT[38] = 0xFF;
	SHA_MT[39] = 0xFF;
	SHA_MT[40] = 0x40 | page_num;
	if (anon)
		memset(&SHA_MT[41], 0xFF, 7);
	else
		memcpy(&SHA_MT[41], chip_ds_msg.rom_id, 7);
	// Secret set in VerifyMAC: SHA_MT[48] - SHA_MT[51]
	memcpy(&SHA_MT[52], &challenge[4], 3);
	SHA_MT[55] = 0x80;
	memset(&SHA_MT[56], 0x00, 6);
	SHA_MT[62] = 0x01;
	SHA_MT[63] = 0xB8;
	// compare calculated MAC to the MAC read from device

	close(fd);

	// calculate the MAC
	_computeMAC(SHA_MT, calc_mac, secret);
	// Compare calculated mac with one read from device
#if _DEBUG_MUX_DS
	printf("\n");
#endif
	for (i = 0; i < 20; i++) {
#if _DEBUG_MUX_DS
		printf("chip_mac[%d] = 0x%x; calc_mac[%d] = 0x%x \n", i, chip_ds_msg.mac[i], i, calc_mac[i]);
#endif
		if (chip_ds_msg.mac[i] != calc_mac[i])
		{
#if _DEBUG_MUX_DS
			printf("chip_ds_msg.mac[%d] = 0x%x; calc_mac[%d] = 0x%x \n", i, chip_ds_msg.mac[i], i, calc_mac[i]);
			printf("============MAC verify error!=========\n");
#endif
			return -3;
		}
	}

	return 0;

}

static int writeData(unsigned char * secret) {
	int fd, i;
	struct ds_msg chip_ds_msg;

	//unsigned char mux_bind_data[32] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	//memcpy(chip_ds_msg.secret, secret, 8);
	int ret = readID(chip_ds_msg.rom_id);
	if (ret)
	{
		return -1;
	}

	fd = open(MUX_DS_DEVICE, O_RDWR);
	if (fd < 0)
	{
		MUX_ERROR("can not open device file");
		return -1;
	}
	srand((unsigned) time(NULL));	//init random seed
#if _DEBUG_MUX_DS
	printf("Set challenge to chip, read mac and verify.");
#endif
	for (i = 0; i < 8; i++)
	{
		chip_ds_msg.challenge[i] = rand() % 256;
	}

	chip_ds_msg.page_num = 0;
	for (i = 0; i < 8; i++) {
		chip_ds_msg.secret[i] = 0x0;
	}

	//printf("\nInit 8 bytes in PAGE 0, start from 0x0\n");
	//refresh chip_ds_msg
	if (ioctl(fd, LOAD_CHALLENGE_READ_MAC, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL LOAD_CHALLENGE_READ_MAC FAIL!");
		close(fd);
		return -3;
	}

	memcpy(chip_ds_msg.secret, secret, 8);

	chip_ds_msg.bind_addr = 0x0;
	memcpy(chip_ds_msg.bind_data, mux_bind_data, 8);
	if (ioctl(fd, INIT_EEPROM_PRODUCTION, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL INIT_EEPROM_PRODUCTION FAIL!");
		close(fd);
		return -2;
	}

	//printf("Init 8 bytes in PAGE 0, start from 0x08\n");
	//refresh chip_ds_msg
	if (ioctl(fd, LOAD_CHALLENGE_READ_MAC, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL LOAD_CHALLENGE_READ_MAC FAIL!");
		close(fd);
		return -3;
	}
	
	chip_ds_msg.bind_addr = 0x08;
	memcpy(chip_ds_msg.bind_data, (mux_bind_data + 0x8), 8);
	if (ioctl(fd, INIT_EEPROM_PRODUCTION, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL INIT_EEPROM_PRODUCTION FAIL!");
		close(fd);
		return -2;
	}

	//printf("Init 8 bytes in PAGE 0, start from 0x10\n");
	//refresh chip_ds_msg
	if (ioctl(fd, LOAD_CHALLENGE_READ_MAC, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL LOAD_CHALLENGE_READ_MAC FAIL!");
		close(fd);
		return -3;
	}
	
	chip_ds_msg.bind_addr = 0x10;
	memcpy(chip_ds_msg.bind_data, (mux_bind_data + 0x10), 8);
	if (ioctl(fd, INIT_EEPROM_PRODUCTION, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL INIT_EEPROM_PRODUCTION FAIL!");
		close(fd);
		return -2;
	}

	//printf("Init 8 bytes in PAGE 0, start from 0x18\n");
	//refresh chip_ds_msg
	if (ioctl(fd, LOAD_CHALLENGE_READ_MAC, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL LOAD_CHALLENGE_READ_MAC FAIL!");
		close(fd);
		return -3;
	}
	chip_ds_msg.bind_addr = 0x18;
	memcpy(chip_ds_msg.bind_data, (mux_bind_data + 0x18), 8);
	if (ioctl(fd, INIT_EEPROM_PRODUCTION, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL INIT_EEPROM_PRODUCTION FAIL!");
		close(fd);
		return -2;
	}

	chip_ds_msg.bind_addr = 0x0;
	//printf("Read 32 byte from chip memory to verfify. Start address: 0x%x\n", chip_ds_msg.bind_addr);
	if (ioctl(fd, READ_CHIP_MEM, &chip_ds_msg) == -1)
	{
		MUX_ERROR("DS IOCTL READ_CHIP_MEM FAIL!");
		close(fd);
		return -3;
	}

	close(fd);

	for (i = 0; i < 32; i++)
	{
		if (mux_bind_data[i] != chip_ds_msg.page_data[i])
			return -4;
	}
	return 0;
}


int cmnMuxDsValidate(void)
{
	int ret;
#if 0
	struct ds_msg chip_ds_msg;
	int i;

	//MAC related
	//unsigned char chip_id[8];

	//init chip_ds_msg
	for (i = 0; i < 8; i++) {
		chip_ds_msg.rom_id[i] = 0;
		chip_ds_msg.secret[i] = 0x5a;
		chip_ds_msg.challenge[i] = 0xa9;
	}
	chip_ds_msg.flag = 0;
	for (i = 0; i < 20; i++)
		chip_ds_msg.mac[i] = 0x0;
	for (i = 0; i < 32; i++)
		chip_ds_msg.page_data[i] = 0x0;
	chip_ds_msg.flag = 0;

#if _DEBUG_MUX_DS
	printf("\n-----------------step 1: read chip id----------------\n");
#endif
	ret = readID(chip_ds_msg.rom_id);
	if (ret)
	{
		return -1;
	}
	else
	{
		MUX_DEBUG("Read rom ID OK!\n");
	}

#if _DEBUG_MUX_DS
	printf("\n-----------------step 2: mix with mux data to generate secret key----------------\n");
#endif
	ret = calSrtKey(mux_data, chip_ds_msg.rom_id, chip_ds_msg.secret);
	if (ret)
	{
		return -1;
	}
	else
	{
		/*printf("secret key generated : ");
		 for (i = 0; i < 8; i++) {
		 printf("0x%x ", chip_ds_msg.secret[i]);
		 }
		 printf("\n");*/
	}

#if _DEBUG_MUX_DS
	printf("\n-----------------step 3: write secret key to chip----------------\n");
#endif
	ret = writeSrtKey(chip_ds_msg.secret);
	if (ret)
	{
		printf("Failed to write secret key!\n");
		return -1;
	}
	else
	{
//		printf("secret key written!\n");
	}

#if _DEBUG_MUX_DS
	printf("\n-----------------step 4: write muxlab data to page 0----------------\n");
#endif
	ret = writeData(chip_ds_msg.secret);
	if (ret)
	{
		MUX_ERROR("Failed to write data!\n");
		return -1;
	}
	else
	{
#if _DEBUG_MUX_DS
		printf("Data written!\n");
#endif
	}

#if _DEBUG_MUX_DS
	printf("\n-----------------step 5: Do the MAC verfity----------------\n");
	printf("Set challenge to chip, read mac from chip.\n");
#endif
#endif

	ret = verifyMAC(mux_data);
	if (ret)
	{
		MUX_ERROR("MAC verification failed!\n");
		return -1;
	}
	else
	{
		MUX_INFO("MAC verification OK!\n");
	}

	return 0;
}


