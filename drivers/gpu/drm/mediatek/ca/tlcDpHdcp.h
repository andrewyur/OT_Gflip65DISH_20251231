
#ifndef __TLCDPHDCP_H__
#define __TLCDPHDCP_H__

#include <linux/printk.h>
#include "tci.h"
#include "tlDPHdcpCMD.h"

#define TLCINFO(string, args...) pr_info("[TLC_HDCP]info: "string, ##args)
#define TLCERR(string, args...) pr_info("[TLC_HDCP]line:%d,err:%s:"string,\
	__LINE__, __func__, ##args)

#define RET_ERROR_KEY_INVALID   10
#define RET_SUCCESS 0

#define HDCP_NONE                0x0 // No HDCP supported, no secure data path
#define HDCP_V1                  0x1 // HDCP version 1.0
#define HDCP_V2                  0x2 // HDCP version 2.0 Type 1
#define HDCP_V2_1                0x3 // HDCP version 2.1 Type 1
#define HDCP_V2_2                0x4 // HDCP version 2.2 Type 1
#define HDCP_V2_3                0x5 // HDCP version 2.3 Type 1
// Local display only(content required version use only)
#define HDCP_LOCAL_DISPLAY_ONLY  0xf
#define HDCP_NO_DIGITAL_OUTPUT   0xff // No digital output
#define HDCP_DEFAULT             HDCP_NO_DIGITAL_OUTPUT // Default value

#define HDCP_VERSION_1X 1
#define HDCP_VERSION_2X 2

#ifdef __cplusplus
extern "C"
{
#endif

int tee_addDevice(uint32_t version);

void tee_removeDevice(void);

int tee_clearParing(void);

int tee_calculateLm(uint8_t *Bksv);

int tee_getAksv(uint8_t *Aksv);

int tee_compareR0(uint8_t *r0, uint32_t len);

int tee_hdcp1x_ComputeCompareV(uint8_t *cryptoParam, uint32_t paramLen,
	uint8_t *rxV);

int tee_hdcp1x_setTxAn(uint8_t *an_code);

int tee_hdcp1x_softRst(void);
int tee_hdcp2_softRst(void);

int tee_hdcp_enableEncrypt(bool bEnable, uint8_t version);

int tee_akeCertificate(uint8_t *certificate, bool *bStored,
	uint8_t *out_m, uint8_t *out_ekm);

int tee_encRsaesOaep(uint8_t *ekm);

int tee_akeHPrime(uint8_t *rtx, uint8_t *rrx, uint8_t *rxCaps, uint8_t *txCaps,
	uint8_t *rxH, uint32_t rxH_len);

int tee_akeParing(uint8_t *rx_ekm);

int tee_lcLPrime(uint8_t *rn, uint8_t *rxL, uint32_t len);

int tee_skeEncKs(uint8_t *riv, uint8_t *eks);

int tee_hdcp2_ComputeCompareV(uint8_t *cryptoParam, uint32_t paramLen,
	uint8_t *rxV, uint8_t *txV);

int tee_hdcp2_ComputeCompareM(uint8_t *cryptoParam, uint32_t paramLen,
	uint8_t *rxM);

int tee_loadHdcpKeyById(uint32_t version);

/*for test and need remove in furture*/
int tee_hdcp1x_setKey(uint8_t *key);
int tee_hdcp2_setKey(uint8_t *kpubdcp, uint8_t *lc128);

#ifdef __cplusplus
}
#endif

#endif //__TLCDPHDCP_H__
