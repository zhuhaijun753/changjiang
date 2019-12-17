#include "encryption_init.h"
#include "atca_command.h"

ATCAIfaceCfg *gCfg = &cfg_ateccx08a_i2c_default;



// Undefine the Unity FAIL macro so it doesn't conflict with the ASF definition
#undef FAIL

#if !defined(_WIN32) && !defined(__linux__) && !defined(__XC32__)
#ifdef ATMEL_START
#include "atmel_start.h"
#else
#include <asf.h>
#endif
#endif

#include <string.h>
#ifndef _WIN32
//#include "cbuf.h"
#endif
#include "cryptoauthlib.h"
#include "atca_cfgs.h"

static ATCA_STATUS set_test_config(ATCADeviceType deviceType);
static int certdata_unit_tests(void);
static int certio_unit_tests(void);
static ATCA_STATUS is_device_locked(uint8_t zone, bool *isLocked);
static ATCA_STATUS lock_status(void);
static ATCA_STATUS lock_config_zone(void);
static ATCA_STATUS lock_data_zone(void);
static ATCA_STATUS get_info(uint8_t *revision);
static ATCA_STATUS get_serial_no(uint8_t *sernum);
static ATCA_STATUS do_randoms(void);
void read_config(void);
void write_config(void);
void lock_config(void);
static void lock_data(void);
static void info(void);
static void sernum(void);
static void discover(void);
static void select_device(ATCADeviceType device_type);
static int run_test(void* fptest);
static void select_204(void);
static void select_108(void);
static void select_508(void);
static void select_608(void);
static void write_sKey(uint8_t *key,uint16_t len);
static void write_rKey(uint8_t *key,uint16_t len);
static void encryptstring(uint8_t *plaintext, uint16_t plaintextLen, uint16_t keyslot, uint8_t *encrypted_data_out);
static void decryptstring(uint8_t *ciphertext,uint16_t keyslot, uint8_t *decrypted_data_out);
static void run_basic_tests(void);
static void run_unit_tests(void);
static void run_otpzero_tests(void);
//static void run_helper_tests(void);
static void help(void);
static int parse_cmd(const char *command);
//static void run_all_tests(void);
//static ATCA_STATUS set_chip_mode(uint8_t i2c_user_extra_add, uint8_t ttl_enable, uint8_t watchdog, uint8_t clock_divider);
static void set_clock_divider_m0(void);
static void set_clock_divider_m1(void);
static void set_clock_divider_m2(void);

void write_sKey_testing(void);
void write_rKey_testing(void);
static void encryptstring_testing(void);
static void decryptstring_testing(void);

uint8_t w25_test_ecc608_configdata[ATCA_CONFIG_SIZE] = {
	0x01, 0x23, 0x4E, 0x87, 0x00, 0x00, 0x60, 0x01, 0x8F, 0xBF, 0x0C, 0xF4, 0xEE, 0xC1, 0x39, 0x00,
	0xC0, 0x00, 0x00, 0x00, 0x84, 0x04, 0x84, 0x04, 0x84, 0x04, 0x84, 0x04, 0x8F, 0x8F, 0x8F, 0x8F,
	0x9F, 0x8F, 0xAF, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xAF, 0x8F, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3A, 0x00, 0x3A, 0x00, 0x3A, 0x00, 0x3A, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00,
	0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x1C, 0x00
};

// Input plaintext for testing.
// This input test data is commonly used by NIST for some AES test vectors
static const uint8_t g_plaintext[64] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
	0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
	0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};


static const char* argv[] = { "manual", "-v" };
static t_menu_info mas_menu_info[] =
{
    { "help",        "Display Menu",                                   help                                 },
    { "discover",    "Discover Buses and Devices",                     discover                             },
    { "608",         "Set Target Device to ATECC608A",                 select_608                           },
    { "info",        "Get the Chip Revision",                          info                                 },
    { "sernum",      "Get the Chip Serial Number",                     sernum                               },
    { "rand",        "Generate Some Random Numbers",                   (fp_menu_handler)do_randoms          },
    { "readcfg",     "Read the Config Zone",                           read_config                          },
    { "writecfg",    "Write the Config Zone",                          write_config                         },
		 
    { "clkdivm0",    "Set ATECC608A to ClockDivider M0(0x00)",         set_clock_divider_m0                 },
    { "clkdivm1",    "Set ATECC608A to ClockDivider M1(0x05)",         set_clock_divider_m1                 },
    { "clkdivm2",    "Set ATECC608A to ClockDivider M2(0x0D)",         set_clock_divider_m2                 },		
		 
    { "lockstat",    "Zone Lock Status",                               (fp_menu_handler)lock_status         },
    { "lockcfg",     "Lock the Config Zone",                           lock_config                          },
    { "lockdata",    "Lock Data and OTP Zones",                        lock_data                            },
		
    { "writeslot0",  "Write Skey to Slot0",                            write_sKey_testing                   },
    { "writeslot1",  "Write Rkey to Slot1",                            write_rKey_testing                   },	
	{ "aesencrypt",  "encrypt string with key",                        encryptstring_testing                },
	{ "aesdecrypt",  "decrypt string with key",                        decryptstring_testing                },			

    { NULL,       NULL,                                             NULL                                 },
};

#if 0

#if defined(_WIN32) || defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include "cmd-processor.h"
int main(int argc, char* argv[])
{

    char buffer[1024];
    size_t bufsize = sizeof(buffer);

    if (!buffer)
    {
        fprintf(stderr, "Failed to allocated a buffer");
        return 1;
    }

    while (true)
    {
        printf("$ ");
        if (fgets(buffer, bufsize, stdin))
        {
            parse_cmd(buffer);
        }
    }

    return 0;
}
#endif

#endif

#if 0

#ifndef _WIN32
int processCmd(void)
{
    static char cmd[cmdQ_SIZE + 1];
    uint16_t i = 0;

    while (!CBUF_IsEmpty(cmdQ) && i < sizeof(cmd))
    {
        cmd[i++] = CBUF_Pop(cmdQ);
    }
    cmd[i] = '\0';
    //printf("\r\n%s\r\n", command );
    parse_cmd(cmd);
    printf("$ ");

    return ATCA_SUCCESS;
}
#endif


#endif

static int parse_cmd(const char *command)
{
    uint8_t index = 0;

    printf("\r\n");
    if (command[0] == '\0' || command[0] == '\n')
    {
        return ATCA_SUCCESS;
    }
    while (mas_menu_info[index].menu_cmd != NULL)
    {
        if (strstr(command, mas_menu_info[index].menu_cmd))
        {
            if (mas_menu_info[index].fp_handler)
            {
                mas_menu_info[index].fp_handler();
            }
            break;
        }
        index++;
    }

    if (mas_menu_info[index].menu_cmd == NULL)
    {
        printf("syntax error in command: %s", command);
    }
    printf("\r\n");
    return ATCA_SUCCESS;
}

static void help(void)
{
    uint8_t index = 0;

    printf("Usage:\r\n");
    while (mas_menu_info[index].menu_cmd != NULL)
    {
        printf("%s - %s\r\n", mas_menu_info[index].menu_cmd, mas_menu_info[index].menu_cmd_description);
        index++;
    }
}

static void select_204(void)
{
    select_device(ATSHA204A);
}
static void select_108(void)
{
    select_device(ATECC108A);
}
static void select_508(void)
{
    select_device(ATECC508A);
}
static void select_608(void)
{
    select_device(ATECC608A);
}

static void update_chip_mode(uint8_t* chip_mode, uint8_t i2c_user_extra_add, uint8_t ttl_enable, uint8_t watchdog, uint8_t clock_divider)
{
    if (i2c_user_extra_add != 0xFF)
    {
        *chip_mode &= ~ATCA_CHIPMODE_I2C_ADDRESS_FLAG;
        *chip_mode |= i2c_user_extra_add & ATCA_CHIPMODE_I2C_ADDRESS_FLAG;
    }
    if (ttl_enable != 0xFF)
    {
        *chip_mode &= ~ATCA_CHIPMODE_TTL_ENABLE_FLAG;
        *chip_mode |= ttl_enable & ATCA_CHIPMODE_TTL_ENABLE_FLAG;
    }
    if (watchdog != 0xFF)
    {
        *chip_mode &= ~ATCA_CHIPMODE_WATCHDOG_MASK;
        *chip_mode |= watchdog & ATCA_CHIPMODE_WATCHDOG_MASK;
    }
    if (clock_divider != 0xFF)
    {
        *chip_mode &= ~ATCA_CHIPMODE_CLOCK_DIV_MASK;
        *chip_mode |= clock_divider & ATCA_CHIPMODE_CLOCK_DIV_MASK;
    }
}


#if 0
static ATCA_STATUS check_clock_divider(void)
{
    ATCA_STATUS status;
    uint8_t chip_mode = 0;

    if (gCfg->devtype != ATECC608A)
    {
        printf("Current device doesn't support clock divider settings (only ATECC608A)\r\n");
        return ATCA_GEN_FAIL;
    }

    // Update the actual ATECC608A chip mode so it takes effect immediately
    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    do
    {
        // Read current config values
        status = atcab_read_bytes_zone(ATCA_ZONE_CONFIG, 0, ATCA_CHIPMODE_OFFSET, &chip_mode, 1);
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_read_bytes_zone() failed with ret=0x%08X\r\n", status);
            break;
        }

        // Update the ATECC608A test config data so all the unit tests will run with the new chip mode
        update_chip_mode(&test_ecc608_configdata[ATCA_CHIPMODE_OFFSET], 0xFF, 0xFF, chip_mode & ATCA_CHIPMODE_WATCHDOG_MASK, chip_mode & ATCA_CHIPMODE_CLOCK_DIV_MASK);

    }
    while (0);

    atcab_release();
    return status;
}
#endif


//static void run_basic_tests(void)
//{
    //if (gCfg->devtype == ATECC608A)
    //{
        //check_clock_divider();
    //}
    //run_test(RunAllBasicTests);
//}

//static void run_unit_tests(void)
//{
    //if (gCfg->devtype == ATECC608A)
    //{
        //check_clock_divider();
    //}
    //run_test(RunAllFeatureTests);
//}
//static void run_otpzero_tests(void)
//{
    //run_test(RunBasicOtpZero);
//}
/*static void run_helper_tests(void)
{
    run_test(RunAllHelperTests);
}*/

void read_config(void)
{
    ATCA_STATUS status;
    uint8_t config[ATCA_ECC_CONFIG_SIZE];
    size_t config_size = 0;
    size_t i = 0;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed: %02x\r\n", status);
        return;
    }

    do
    {
        status = atcab_get_zone_size(ATCA_ZONE_CONFIG, 0, &config_size);
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_get_zone_size() failed: %02x\r\n", status);
            break;
        }

        status = atcab_read_config_zone(config);
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_read_config_zone() failed: %02x\r\n", status);
            break;
        }

        for (i = 0; i < config_size; i++)
        {
            if (i % 16 == 0)
            {
                printf("\r\n");
            }
            else if (i % 8 == 0)
            {
                printf("  ");
            }
            else
            {
                printf(" ");
            }
            printf("%02X", (int)config[i]);
        }
        printf("\r\n");
    }
    while (0);

    atcab_release();
}

void write_config(void)
{
	ATCA_STATUS status;
	uint8_t config[ATCA_ECC_CONFIG_SIZE];
	size_t config_size = 0;
	size_t i = 0;

	status = atcab_init(gCfg);
	if (status != ATCA_SUCCESS)
	{
		printf("atcab_init() failed: %02x\r\n", status);
		return;
	}

	do
	{
		status = atcab_get_zone_size(ATCA_ZONE_CONFIG, 0, &config_size);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_get_zone_size() failed: %02x\r\n", status);
			break;
		}

		status = atcab_write_config_zone(w25_test_ecc608_configdata);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_write_config_zone() failed: %02x\r\n", status);
			break;
		}

		printf("\r\n");
	}
	while (0);

	atcab_release();
}
//查询锁状态(配置区和数据区)
static ATCA_STATUS lock_status(void)
{
    ATCA_STATUS status;
    bool is_locked = false;

    if ((status = is_device_locked(LOCK_ZONE_CONFIG, &is_locked)) != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
        return status;
    }
    printf("Config Zone: %s\r\n", is_locked ? "LOCKED" : "unlocked");

    if ((status = is_device_locked(LOCK_ZONE_DATA, &is_locked)) != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
        return status;
    }
    printf("Data Zone  : %s\r\n", is_locked ? "LOCKED" : "unlocked");

    return status;
}

//锁配置区
void lock_config(void)
{
	lock_config_zone();
    lock_status();
}

static void lock_data(void)
{
    lock_data_zone();
    lock_status();
}
void write_sKey_testing(void)
{
	uint8_t data[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
	
	write_sKey(&data, sizeof(data));
}
static void write_sKey(uint8_t *key,uint16_t len)
{
	bool is_locked = 0;
	int slot;
	size_t slot_size = 0;


	// test writing slot 0

	ATCA_STATUS status = ATCA_SUCCESS;
	
	status = atcab_init(gCfg);
	
	if (status != ATCA_SUCCESS)
	{
		printf("atcab_init() failed: %02x\r\n", status);
		return;
	}

    do 
    {
		status = atcab_get_zone_size(ATCA_ZONE_DATA, 0, &slot_size);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_get_zone_size() failed: %02x\r\n", status);
			break;
		}


		status = atcab_write_bytes_zone( ATCA_ZONE_DATA, 0, 0, key, len);
		
		if(status != ATCA_SUCCESS)
		{
			printf("error: %02x\r\n", status);
			break;
		}
		
		printf("atcab_write_slot0 finished");		
		printf("\r\n");
    } while (0);

	atcab_release();


}

void write_rKey_testing(void)
{
	uint8_t data[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
	write_rKey(&data,sizeof(data));
}
static void write_rKey(uint8_t *key,uint16_t len)
{
	bool is_locked = 0;
	int slot;
	size_t slot_size = 0;

	// test writing slot 1

	ATCA_STATUS status = ATCA_SUCCESS;

	status = atcab_init(gCfg);
	
	if (status != ATCA_SUCCESS)
	{
		printf("atcab_init() failed: %02x\r\n", status);
		return;
	}

	do
	{
		status = atcab_get_zone_size(ATCA_ZONE_DATA, 0, &slot_size);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_get_zone_size() failed: %02x\r\n", status);
			break;
		}

		status = atcab_write_bytes_zone( ATCA_ZONE_DATA, 1, 0, key, len);
		
		if(status != ATCA_SUCCESS)
		{
			printf("error: %02x\r\n", status);
			break;
		}
		printf("atcab_write_slot1 finished");
		printf("\r\n");
	} while (0);

	int stas = atcab_release();

}

static void encryptstring_testing(void)
{
	uint8_t plaintext[16]= {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};;
	uint16_t keyslot;
	
	keyslot = 0;
	encryptstring(&plaintext, sizeof(plaintext),keyslot, NULL);
}

static void encryptstring(uint8_t *plaintext,uint16_t plaintextLen, uint16_t keyslot, uint8_t *encrypted_data_out)
{
	uint8_t encrypted_out[16] = {0};
	uint8_t key_block,i,j = 0;

	printf("plaintextLen == %d\n", plaintextLen);
	for(i = 0; i < plaintextLen; i += 16)
	{
		memset(encrypted_out, 0, sizeof(encrypted_out));
		ATCA_STATUS status;
		//size_t data_block;
		//uint8_t encrypted_data_out[16];
		uint16_t key_slot = keyslot;
		//char displaystr[32];
		//int displaylen = sizeof(displaystr);
		//bool persistent_latch_state;
		//bool is_locked = 0;
		
		status = atcab_init(gCfg);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_init() failed: %02x\r\n", status);
			return;
		}
	  
		// Test encryption with the AES keys
		atcab_aes_encrypt(key_slot, 0, &plaintext[j * AES_DATA_SIZE], encrypted_out);
		//for (j = 0; j < sizeof(AES_DATA_SIZE); j++)
		//{
		//	printf("%02X", plaintext[i*AES_DATA_SIZE+j]);
		//}
		memcpy(&encrypted_data_out[j*AES_DATA_SIZE], encrypted_out, AES_DATA_SIZE);
		printf("encrypt finished\n");
		atcab_release();
		j++;
	}
}
	


static void decryptstring_testing(void)
{
	uint8_t ciphertext[16]= {0x47, 0xC5, 0x8D, 0x5E, 0x21, 0xCA, 0xAF, 0x84, 0x0D, 0x01, 0x5B, 0x7D, 0x9B, 0x91, 0x09, 0x81};
	uint16_t keyslot;
	
	keyslot = 0;
	decryptstring(&ciphertext,keyslot, NULL);
}
static void decryptstring(uint8_t *ciphertext,uint16_t keyslot, uint8_t *decrypted_data_out)
{
	ATCA_STATUS status;
	//uint8_t key_block,i;
	size_t data_block;
	//uint8_t decrypted_data_out[16];
	uint16_t key_slot = keyslot;
	//char displaystr[30];
	//int displaylen = sizeof(displaystr);
	//bool persistent_latch_state;
	//bool is_locked = 0;
	
	status = atcab_init(gCfg);
	if (status != ATCA_SUCCESS)
	{
		printf("atcab_init() failed: %02x\r\n", status);
		return;
	}
	
	// Test decryption with the AES keys
	atcab_aes_decrypt(key_slot, 0, &ciphertext[data_block * AES_DATA_SIZE], decrypted_data_out);
//	for (i = 0; i < sizeof(decrypted_data_out); i++)
	//{
	//	printf("%02X", (int)decrypted_data_out[i]);
	//}
	//printf("\r\n");
	printf("decrypt finished\n");
	//printf("\r\n");	
}
static ATCA_STATUS do_randoms(void)
{
    ATCA_STATUS status;
    uint8_t randout[RANDOM_RSP_SIZE];
    char displayStr[RANDOM_RSP_SIZE * 3];
    int displen = sizeof(displayStr);
    int i;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    for (i = 0; i < 5; i++)
    {
        if ((status = atcab_random(randout)) != ATCA_SUCCESS)
        {
            break;
        }

        atcab_bin2hex(randout, 32, displayStr, &displen);
        printf("%s\r\n", displayStr);
    }

    if (status != ATCA_SUCCESS)
    {
        printf("error: %02x\r\n", status);
    }

    atcab_release();

    return status;
}
static void discover(void)
{
    ATCAIfaceCfg ifaceCfgs[10];
    int i;
    const char *devname[] = { "ATSHA204A", "ATECC108A", "ATECC508A", "ATECC608A" };  // indexed by ATCADeviceType

    for (i = 0; i < (int)(sizeof(ifaceCfgs) / sizeof(ATCAIfaceCfg)); i++)
    {
        ifaceCfgs[i].devtype = ATCA_DEV_UNKNOWN;
        ifaceCfgs[i].iface_type = ATCA_UNKNOWN_IFACE;
    }

    printf("Searching...\r\n");
    atcab_cfg_discover(ifaceCfgs, sizeof(ifaceCfgs) / sizeof(ATCAIfaceCfg));
    for (i = 0; i < (int)(sizeof(ifaceCfgs) / sizeof(ATCAIfaceCfg)); i++)
    {
        if (ifaceCfgs[i].devtype != ATCA_DEV_UNKNOWN)
        {
            printf("Found %s ", devname[ifaceCfgs[i].devtype]);
            if (ifaceCfgs[i].iface_type == ATCA_I2C_IFACE)
            {
                printf("@ bus %d addr %02x", ifaceCfgs[i].atcai2c.bus, ifaceCfgs[i].atcai2c.slave_address);
            }
            if (ifaceCfgs[i].iface_type == ATCA_SWI_IFACE)
            {
                printf("@ bus %d", ifaceCfgs[i].atcaswi.bus);
            }
            printf("\r\n");
        }
    }
}
static void info(void)
{
    ATCA_STATUS status;
    uint8_t revision[4];
    char displaystr[15];
    int displaylen = sizeof(displaystr);

    status = get_info(revision);
    if (status == ATCA_SUCCESS)
    {
        // dump revision
        atcab_bin2hex(revision, 4, displaystr, &displaylen);
        printf("revision:\r\n%s\r\n", displaystr);
    }
}

static void sernum(void)
{
    ATCA_STATUS status;
    uint8_t serialnum[ATCA_SERIAL_NUM_SIZE];
    char displaystr[30];
    int displaylen = sizeof(displaystr);

    status = get_serial_no(serialnum);
    if (status == ATCA_SUCCESS)
    {
        // dump serial num
        atcab_bin2hex(serialnum, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen);
        printf("serial number:\r\n%s\r\n", displaystr);
    }
}

void RunAllCertDataTests(void);
static int certdata_unit_tests(void)
{
    UnityMain(sizeof(argv) / sizeof(char*), argv, RunAllCertDataTests);
    return ATCA_SUCCESS;
}

void RunAllCertIOTests(void);
static int certio_unit_tests(void)
{
    UnityMain(sizeof(argv) / sizeof(char*), argv, RunAllCertIOTests);
    return ATCA_SUCCESS;
}

static ATCA_STATUS is_device_locked(uint8_t zone, bool *isLocked)
{
    ATCA_STATUS status;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        return status;
    }

    status = atcab_is_locked(zone, isLocked);
    atcab_release();

    return status;
}

static ATCA_STATUS lock_config_zone(void)
{
    ATCA_STATUS status;

   /* uint8_t ch = 0;

    printf("Locking with test configuration, which is suitable only for unit tests... \r\nConfirm by typing Y\r\n");
    scanf("%c", &ch);

    if (!((ch == 'Y') || (ch == 'y')))
    {
        printf("Skipping Config Lock on request.\r\n");
        return ATCA_GEN_FAIL;
    }*/

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    status = atcab_lock_config_zone();
    atcab_release();
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_lock_config_zone() failed with ret=0x%08X\r\n", status);
    }

    return status;
}

static ATCA_STATUS lock_data_zone(void)
{
    ATCA_STATUS status;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    status = atcab_lock_data_zone();
    atcab_release();
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_lock_data_zone() failed with ret=0x%08X\r\n", status);
    }

    return status;
}

static ATCA_STATUS get_info(uint8_t *revision)
{
    ATCA_STATUS status;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    status = atcab_info(revision);
    atcab_release();
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_info() failed with ret=0x%08X\r\n", status);
    }

    return status;
}

static ATCA_STATUS get_serial_no(uint8_t *sernum)
{
    ATCA_STATUS status;

    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    status = atcab_read_serial_number(sernum);
    atcab_release();
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_read_serial_number() failed with ret=0x%08X\r\n", status);
    }

    return status;
}

static void select_device(ATCADeviceType device_type)
{
    ATCA_STATUS status;

    status = set_test_config(device_type);

    if (status == ATCA_SUCCESS)
    {
        printf("Device Selected.\r\n");
    }
    else
    {
        printf("IFace Cfg are NOT available\r\n");
    }
}

static int run_test(void* fptest)
{
    if (gCfg->devtype < ATCA_DEV_UNKNOWN)
    {
        return UnityMain(sizeof(argv) / sizeof(char*), argv, fptest);
    }
    else
    {
        printf("Device is NOT Selected... Select device before running tests!!!");
        return -1;
    }
}

#if 0
static void run_all_tests(void)
{
    ATCA_STATUS status;
    bool is_config_locked = false;
    bool is_data_locked = false;
    int fails = 0;

    if (gCfg->devtype == ATECC608A)
    {
        check_clock_divider();
    }
	//配置区锁状态
    status = is_device_locked(LOCK_ZONE_CONFIG, &is_config_locked);
    if (status != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
        return;
    }
	//数据区锁状态
    status = is_device_locked(LOCK_ZONE_DATA, &is_data_locked);
    if (status != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
        return;
    }

    status = lock_status();
    if (status != ATCA_SUCCESS)
    {
        printf("lock_status() failed with ret=0x%08X\r\n", status);
        return;
    }

    if (!is_config_locked)
    {
        fails += run_test(RunAllFeatureTests);
        if (fails > 0)
        {
            printf("unit tests with config zone unlocked failed.\r\n");
            return;
        }

        fails += run_test(RunAllBasicTests);
        if (fails > 0)
        {
            printf("basic tests with config zone unlocked failed.\r\n");
            return;
        }

        status = lock_config_zone();
        if (status != ATCA_SUCCESS)
        {
            printf("lock_config_zone() failed with ret=0x%08X\r\n", status);
            return;
        }
        status = lock_status();
        if (status != ATCA_SUCCESS)
        {
            printf("lock_status() failed with ret=0x%08X\r\n", status);
            return;
        }
    }

    if (!is_data_locked)
    {
        fails += run_test(RunAllFeatureTests);
        if (fails > 0)
        {
            printf("unit tests with data zone unlocked failed.\r\n");
            return;
        }

        fails += run_test(RunAllBasicTests);
        if (fails > 0)
        {
            printf("basic tests with data zone unlocked failed.\r\n");
            return;
        }

        status = lock_data_zone();
        if (status != ATCA_SUCCESS)
        {
            printf("lock_data_zone() failed with ret=0x%08X\r\n", status);
            return;
        }
        status = lock_status();
        if (status != ATCA_SUCCESS)
        {
            printf("lock_status() failed with ret=0x%08X\r\n", status);
            return;
        }
    }

    fails += run_test(RunAllFeatureTests);
    if (fails > 0)
    {
        printf("unit tests with data zone locked failed.\r\n");
        return;
    }

    fails += run_test(RunAllBasicTests);
    if (fails > 0)
    {
        printf("basic tests with data zone locked failed.\r\n");
        return;
    }

    fails = run_test(RunAllHelperTests);
    if (fails > 0)
    {
        printf("util tests failed.\r\n");
        return;
    }

#ifndef DO_NOT_TEST_SW_CRYPTO
    fails += atca_crypto_sw_tests();
    if (fails > 0)
    {
        printf("crypto tests failed.\r\n");
        return;
    }
#endif

#ifndef DO_NOT_TEST_CERT
    if (atIsECCFamily(gCfg->devtype))
    {
        fails += run_test(RunAllCertIOTests);
        if (fails > 0)
        {
            printf("cio tests failed.\r\n");
            return;
        }
    }
    else
    {
        printf("cio tests don't apply to non-ECC devices.\r\n");
    }

    fails += run_test(RunAllCertDataTests);
    if (fails > 0)
    {
        printf("cd tests failed.\r\n");
        return;
    }
#endif

    printf("All unit tests passed.\r\n");
}

#endif

static ATCA_STATUS set_test_config(ATCADeviceType deviceType)
{
    gCfg->devtype = ATCA_DEV_UNKNOWN;
    gCfg->iface_type = ATCA_UNKNOWN_IFACE;

    switch (deviceType)
    {
    case ATSHA204A:
#if defined(ATCA_HAL_I2C)
        *gCfg = cfg_atsha204a_i2c_default;
#elif defined(ATCA_HAL_SWI)
        *gCfg = cfg_atsha204a_swi_default;
#elif defined(ATCA_HAL_KIT_HID)
        *gCfg = cfg_atsha204a_kithid_default;
#elif defined(ATCA_HAL_KIT_CDC)
        *gCfg = cfg_atsha204a_kitcdc_default;
#elif defined(ATCA_HAL_CUSTOM)
        *gCfg = g_cfg_atsha204a_custom;
#else
#error "HAL interface is not selected";
#endif
        break;

    case ATECC108A:
#if defined(ATCA_HAL_I2C)
        *gCfg = cfg_ateccx08a_i2c_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_SWI)
        *gCfg = cfg_ateccx08a_swi_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_HID)
        *gCfg = cfg_ateccx08a_kithid_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_CDC)
        *gCfg = cfg_ateccx08a_kitcdc_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_CUSTOM)
        *gCfg = g_cfg_atecc108a_custom;
#else
#error "HAL interface is not selected";
#endif
        break;

    case ATECC508A:
#if defined(ATCA_HAL_I2C)
        *gCfg = cfg_ateccx08a_i2c_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_SWI)
        *gCfg = cfg_ateccx08a_swi_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_HID)
        *gCfg = cfg_ateccx08a_kithid_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_CDC)
        *gCfg = cfg_ateccx08a_kitcdc_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_CUSTOM)
        *gCfg = g_cfg_atecc508a_custom;
#else
#error "HAL interface is not selected";
#endif
        break;

    case ATECC608A:
#if defined(ATCA_HAL_I2C)
        *gCfg = cfg_ateccx08a_i2c_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_SWI)
        *gCfg = cfg_ateccx08a_swi_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_HID)
        *gCfg = cfg_ateccx08a_kithid_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_KIT_CDC)
        *gCfg = cfg_ateccx08a_kitcdc_default;
        gCfg->devtype = deviceType;
#elif defined(ATCA_HAL_CUSTOM)
        *gCfg = g_cfg_atecc608a_custom;
#else
#error "HAL interface is not selected";
#endif
        break;

    default:
        //device type wasn't found, return with error
        return ATCA_GEN_FAIL;
    }

    #ifdef ATCA_RASPBERRY_PI_3
    gCfg->atcai2c.bus = 1;
    #endif

    return ATCA_SUCCESS;
}

/*
static ATCA_STATUS set_chip_mode(uint8_t i2c_user_extra_add, uint8_t ttl_enable, uint8_t watchdog, uint8_t clock_divider)
{
    ATCA_STATUS status;
    uint8_t config_word[ATCA_WORD_SIZE];
    bool is_config_locked = false;

    if (gCfg->devtype != ATECC608A)
    {
        printf("Current device doesn't support clock divider settings (only ATECC608A)\r\n");
        return ATCA_GEN_FAIL;
    }

    status = is_device_locked(LOCK_ZONE_CONFIG, &is_config_locked);
    if (status != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
        return status;
    }

    if (is_config_locked)
    {
        printf("Current device is config locked. Can't change clock divider. ");
    }

    // Update the actual ATECC608A chip mode so it takes effect immediately
    status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return status;
    }

    do
    {
        // Read current config values
        status = atcab_read_bytes_zone(ATCA_ZONE_CONFIG, 0, 16, config_word, 4);
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_read_bytes_zone() failed with ret=0x%08X\r\n", status);
            break;
        }

        if (is_config_locked)
        {
            printf("Currently set to 0x%02X.\r\n", (int)(config_word[3] >> 3));
            status = ATCA_GEN_FAIL;
            break;
        }

        // Update ChipMode
        update_chip_mode(&config_word[3], i2c_user_extra_add, ttl_enable, watchdog, clock_divider);

        // Write config values back to chip
        status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 16, config_word, 4);
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_write_bytes_zone() failed with ret=0x%08X\r\n", status);
            break;
        }

        // Put to sleep so new values take effect
        status = atcab_wakeup();
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_wakeup() failed with ret=0x%08X\r\n", status);
            break;
        }
        status = atcab_sleep();
        if (status != ATCA_SUCCESS)
        {
            printf("atcab_sleep() failed with ret=0x%08X\r\n", status);
            break;
        }

        // Update the ATECC608A test config data so all the unit tests will run with the new chip mode
        update_chip_mode(&test_ecc608_configdata[ATCA_CHIPMODE_OFFSET], i2c_user_extra_add, ttl_enable, watchdog, clock_divider);

    }
    while (0);

    atcab_release();
    return status;
} */

/*
static void set_clock_divider_m0(void)
{
    ATCA_STATUS status = set_chip_mode(0xFF, 0xFF, ATCA_CHIPMODE_WATCHDOG_SHORT, ATCA_CHIPMODE_CLOCK_DIV_M0);

    if (status == ATCA_SUCCESS)
    {
        printf("Set device to clock divider M0 (0x%02X) and watchdog to 1.3s nominal.\r\n", ATCA_CHIPMODE_CLOCK_DIV_M0 >> 3);
    }
}

static void set_clock_divider_m1(void)
{
    ATCA_STATUS status = set_chip_mode(0xFF, 0xFF, ATCA_CHIPMODE_WATCHDOG_SHORT, ATCA_CHIPMODE_CLOCK_DIV_M1);

    if (status == ATCA_SUCCESS)
    {
        printf("Set device to clock divider M1 (0x%02X) and watchdog to 1.3s nominal.\r\n", ATCA_CHIPMODE_CLOCK_DIV_M1 >> 3);
    }
}

static void set_clock_divider_m2(void)
{
    // Additionally set watchdog to long settings (~13s) as some commands
    // can't complete in time on the faster watchdog setting.
    ATCA_STATUS status = set_chip_mode(0xFF, 0xFF, ATCA_CHIPMODE_WATCHDOG_LONG, ATCA_CHIPMODE_CLOCK_DIV_M2);

    if (status == ATCA_SUCCESS)
    {
        printf("Set device to clock divider M2 (0x%02X) and watchdog to 13s nominal.\r\n", ATCA_CHIPMODE_CLOCK_DIV_M2 >> 3);
    }
}
*/


//加密IC初始化
int encryption_init(int nFlagCfg_Encryption)
{
	uint8_t revision[4] = {0}, i;
    ATCA_STATUS status;
	uint8_t config[ATCA_ECC_CONFIG_SIZE];
	size_t config_size = 0;
	bool same;
	bool is_config_locked = false;

	status = atcab_init(gCfg);
    if (status != ATCA_SUCCESS)
    {
        printf("atcab_init() failed with ret=0x%08X\r\n", status);
        return -1;
    }
	
//获取版本信息
	status = atcab_info(revision);
	if (status != ATCA_SUCCESS)
	{
		printf("atcab_info() failed with ret=0x%08X\r\n", status);
		return -1;
	}
	printf("=========get ATECC608A revision: ");
	for (i = 0; i < 4; i++)
		printf("%.2X ", revision[i]);
	printf("\n");
#if 1
	//配置区锁状态
    status = is_device_locked(LOCK_ZONE_CONFIG, &is_config_locked);
    if (status != ATCA_SUCCESS)
    {
        printf("is_device_locked() failed with ret=0x%08X\r\n", status);
		nFlagCfg_Encryption = -1;
		atcab_release();
        return -1;
    }
    printf("Config Zone: %s\r\n", is_config_locked ? "LOCKED" : "unlocked");
	//配置区已锁
	if(is_config_locked)
	{
		nFlagCfg_Encryption = 1;//已锁
		printf("ATECC608A config is locked!\n");
		status = atcab_init(gCfg);		
		//读取配置区数据大小
		status = atcab_get_zone_size(ATCA_ZONE_CONFIG, 0, &config_size);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_get_zone_size() failed: %02x\r\n", status);
			atcab_release();
			return -1;
		}
		printf("Get ATECC608A atcab_get_zone_size!\n");		
		//对比数据
		status = atcab_cmp_config_zone(w25_test_ecc608_configdata, &same);
		if( (status != ATCA_SUCCESS) || (same != true) )
		{
			printf("atcab_cmp_config_zone() failed: %02x\r\n", status);
			atcab_release();
			return -1;
		}
		printf("the ATECC608A is configed!\n");
		nFlagCfg_Encryption = 2;//已配置成功
		
	}
	//配置区未锁，需重新配置
	else	
	{
		printf("the ATECC608A has not been configed!\n");
		//write_config();
		status = atcab_init(gCfg);
		
		status = atcab_get_zone_size(ATCA_ZONE_CONFIG, 0, &config_size);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_get_zone_size() failed: %02x\r\n", status);			
			return -1;
		}
		status = atcab_write_config_zone(w25_test_ecc608_configdata);
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_write_config_zone() failed: %02x\r\n", status);
			return -1;
		}
		nFlagCfg_Encryption = 0;//已配置未锁
		
		//lock_config();
		status = atcab_lock_config_zone();
		if (status != ATCA_SUCCESS)
		{
			printf("atcab_lock_config_zone() failed with ret=0x%08X\r\n", status);
			return -1;
		}
		nFlagCfg_Encryption = 2;//配置完成
	}
	lock_status();//查询锁状态(调试)
#endif
    atcab_release();
	return 0;
}

//写入Rootkey,可修改但不能读取
int writeTsp_rootkey(uint8_t * data1, uint16_t len)
	    {
	write_rKey(data1, len);
	lock_data();
	return 0;
}

//写入skey,可修改但不能读取
int writeTsp_skey(void)
{
	uint8_t data[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

	write_sKey(&data, sizeof(data));
	lock_data();
	return 0;
}
//加密接口0:Skey加密 1:Rootkey加密
int encryptTbox_Data(uint8_t *plaintext, uint16_t plaintextLen, uint16_t keyslot, uint8_t *encrypted_data_out)
{
	//uint8_t plaintext_[16]= {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};

	encryptstring(plaintext, plaintextLen, keyslot, encrypted_data_out);
	return 0;
}
//解密接口0:Skey解密 1:Rootkey解密
int decryptTbox_Data(uint8_t *plaintext, uint16_t keyslot, uint8_t *decrypted_data_out)
{
	//uint8_t ciphertext[16]= {0x47, 0xC5, 0x8D, 0x5E, 0x21, 0xCA, 0xAF, 0x84, 0x0D, 0x01, 0x5B, 0x7D, 0x9B, 0x91, 0x09, 0x81};

	decryptstring(plaintext,keyslot, decrypted_data_out);
	return 0;
}

