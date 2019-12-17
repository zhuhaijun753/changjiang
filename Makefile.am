DIR_SRC = ./src
ENCRYPTION_SRC = ./src/cryptoauthlib

AM_CFLAGS = -Wall \
    -Wundef \
    -Wstrict-prototypes \
    -Wno-trigraphs \
    -g -O0 \
    -Dstrlcpy=g_strlcpy \
    -Dstrlcat=g_strlcat \
    $(QMIFRAMEWORK_CFLAGS) \
    $(QMI_CFLAGS) \
    -DFEATURE_DSS_LINUX \
    -I$(WORKSPACE)/tbox/inc \
    -I$(WORKSPACE)/diag/include \
	-I$(WORKSPACE)/common/inc \
	-I$(WORKSPACE)/sdk-includes/qmi \
	-I$(WORKSPACE)/sdk-includes/qmi-framework \
	-I$(WORKSPACE)/sdk-includes/mcm \
	-I$(WORKSPACE)/sdk-includes/dsutils \
	-I$(WORKSPACE)/sdk-includes/dsi \
	-I$(WORKSPACE)/sdk-includes/data \
    -I$(WORKSPACE)/sdk-includes/common \
    -I./src/cryptoauthlib  \
    -I./src/cryptoauthlib/jwt   \
    -I./src/cryptoauthlib/host   \
    -I./src/cryptoauthlib/hal   \
    -I./src/cryptoauthlib/crypto   \
    -I./src/cryptoauthlib/crypto/hashes   \
    -I./src/cryptoauthlib/basic   \
    -I./src/cryptoauthlib/atcacert 



AM_CXXFLAGS = \
        $(AM_CFLAGS) \
        -fpermissive

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_SINGLE_LINK \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_LINUX \
        -DFEATURE_NATIVELINUX \
        -DFEATURE_DSM_DUP_ITEMS \
        -DFEATURE_LE_DIAG \
        -DFEATURE_MULTIPROCESSOR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_SMD \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_DS_LINUX_ANDROID \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DFEATURE_NO_REX \
        -DFEATURE_XMLLIB \
        $(DIAG_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        $(XMLLIB_CFLAGS) \
        $(CFLAGS) \
        -I$(WORKSPACE)/tbox/inc \
        -Dstrlcpy=g_strlcpy \
        -Dstrlcat=g_strlcat 

SRCS = \
      $(DIR_SRC)/main.cpp    \
	  $(DIR_SRC)/Message.cpp  \
      $(DIR_SRC)/mcuUart.cpp   \
      $(DIR_SRC)/NetworkConnection.cpp   \
      $(DIR_SRC)/OTAUpgrade.cpp   \
      $(DIR_SRC)/SelfCheck.cpp  \
      $(DIR_SRC)/WiFiControl.cpp  \
      $(DIR_SRC)/DnsResolv.cpp    \     
      $(DIR_SRC)/BluetoothControl.cpp  \
      $(DIR_SRC)/SQLiteDatabase.cpp  \
      $(DIR_SRC)/TBoxDataPool.cpp  \
      $(DIR_SRC)/common.cpp  \
      $(DIR_SRC)/Crc32Count.cpp   \
      $(DIR_SRC)/FTPClient.cpp  \
      $(DIR_SRC)/GBT32960.cpp  \
      $(DIR_SRC)/LedControl.cpp  \
      $(DIR_SRC)/AdcVoltageCheck.cpp  \
      $(DIR_SRC)/LTEModuleAtCtrl.cpp  \
      $(DIR_SRC)/FTPSClient.cpp  \
      $(DIR_SRC)/VoiceCall.cpp  \
      $(DIR_SRC)/DataCall.cpp  \
      $(DIR_SRC)/GpioWake.cpp  \
      $(DIR_SRC)/WDSControl.cpp  \
      $(DIR_SRC)/NASControl.cpp  \
      $(DIR_SRC)/HmacSHA-1.cpp  \
      $(DIR_SRC)/AES_acp.cpp  \
      $(DIR_SRC)/HMAC_MD5.cpp  \
      $(DIR_SRC)/FAWACP.cpp  \
      $(DIR_SRC)/uart_debug.cpp \
      $(DIR_SRC)/OTAWiFi.cpp  \
      $(DIR_SRC)/IVI_Communication.cpp  \
	  $(DIR_SRC)/FactoryPattern_Communication.cpp	\
      $(DIR_SRC)/SystemTime.cpp                  \
      $(DIR_SRC)/libnts_crypt_parse.cpp                  \
    $(ENCRYPTION_SRC)/encryption_init.c  \
    $(ENCRYPTION_SRC)/atca_cfgs.c  \
    $(ENCRYPTION_SRC)/atca_command.c  \
    $(ENCRYPTION_SRC)/atca_device.c  \
    $(ENCRYPTION_SRC)/atca_execution.c  \
    $(ENCRYPTION_SRC)/atca_iface.c  \
    $(ENCRYPTION_SRC)/atcacert/atcacert_client.c \
    $(ENCRYPTION_SRC)/atcacert/atcacert_date.c  \
    $(ENCRYPTION_SRC)/atcacert/atcacert_def.c   \
    $(ENCRYPTION_SRC)/atcacert/atcacert_der.c   \
    $(ENCRYPTION_SRC)/atcacert/atcacert_host_hw.c   \
    $(ENCRYPTION_SRC)/atcacert/atcacert_host_sw.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic.c \
    $(ENCRYPTION_SRC)/basic/atca_basic_aes.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_checkmac.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_counter.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_derivekey.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_ecdh.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_gendig.c \
    $(ENCRYPTION_SRC)/basic/atca_basic_genkey.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_hmac.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_info.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_kdf.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_lock.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_mac.c \
    $(ENCRYPTION_SRC)/basic/atca_basic_nonce.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_privwrite.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_random.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_read.c   \
    $(ENCRYPTION_SRC)/basic/atca_basic_secureboot.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_selftest.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_sha.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_sign.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_updateextra.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_verify.c  \
    $(ENCRYPTION_SRC)/basic/atca_basic_write.c  \
    $(ENCRYPTION_SRC)/basic/atca_helpers.c  \
    $(ENCRYPTION_SRC)/crypto/atca_crypto_sw_ecdsa.c  \
    $(ENCRYPTION_SRC)/crypto/atca_crypto_sw_rand.c  \
    $(ENCRYPTION_SRC)/crypto/atca_crypto_sw_sha1.c  \
    $(ENCRYPTION_SRC)/crypto/atca_crypto_sw_sha2.c  \
    $(ENCRYPTION_SRC)/crypto/hashes/sha1_routines.c  \
    $(ENCRYPTION_SRC)/crypto/hashes/sha2_routines.c  \
    $(ENCRYPTION_SRC)/hal/atca_hal.c  \
    $(ENCRYPTION_SRC)/hal/hal_linux_i2c_userspace.c  \
    $(ENCRYPTION_SRC)/hal/hal_linux_timer.c  \
    $(ENCRYPTION_SRC)/host/atca_host.c  \
    $(ENCRYPTION_SRC)/jwt/atca_jwt.c 

	  
bin_PROGRAMS = tbox monitorTbox
tbox_CC = @CC@
pkgconfigdir = $(libdir)/pkgconfig
tbox_SOURCES = $(SRCS)

tbox_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) -fPIC -rdynamic -Wall -DATCA_HAL_I2C -DATCAPRINTF -Os @GLIB_CFLAGS@
tbox_CPPFLAGS = $(AM_CPPFLAGS) -DUSE_GLIB  @GLIB_CFLAGS@
tbox_LDFLAGS = -lpthread -lrt -lssl  -ldl -lcrypto $(QMIFRAMEWORK_LIBS) $(QMI_LIBS) $(DSUTILS_LIBS)
tbox_LDFLAGS += -l$(WORKSPACE)/oe-core/meta-qcom/recipes/mcm-core/prebuilt/usr/lib/libmcm.so.0
tbox_LDFLAGS += -l$(WORKSPACE)/oe-core/build/tmp-glibc/work/armv7a-vfp-neon-oe-linux-gnueabi/sqlite3/3_3.8.7.4-r0/image/usr/lib/libsqlite3.so
tbox_LDFLAGS += -l$(WORKSPACE)/oe-core/meta-qcom/recipes/data/prebuilt/data/usr/lib/libdsi_netctrl.so
tbox_LDFLAGS += -l$(WORKSPACE)/tbox/libnts_crypt.so

monitorTbox_CC = @CC@
monitorTbox_SOURCES = $(DIR_SRC)/monitorTbox.c

monitorTbox_CFLAGS = $(AM_CFLAGS) -fPIC 
monitorTbox_CPPFLAGS = $(AM_CPPFLAGS)


