#pragma once

#include <stdint.h>

typedef uint8_t u8;

// http://wiiubrew.org/wiki/Hardware/eFuse

struct otp_bin {
    u8 boot1_hash[0x14];
    u8 common_key[0x10];
    u8 device_id[0x04];             // wii NG-id
    u8 device_private_key[0x1E];    // wii ecc_private_key  , we have taken 2 bytes from hmac_key     
    u8 truncated_nand_hmac_key[0x12];         // first two bytes moved to dev_priv_key
    u8 nand_key[0x10];
    u8 backup_key[0x10];            
    u8 unknown[0x08]; 
    
    u8 interlude[0x280];   // banks 1-5

    u8 common_ms_ca[0x08];
    u8 common_cert_manufacturing_date[0x04];    // wii ng_key_id
    u8 common_cert_signature[0x3c];             // wii ng_sig
    
    u8 rest[0xb8];
    
};

typedef struct otp_bin otp_bin;
