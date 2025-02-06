
#include <stdio.h>
#include <math.h>
#include "predictor.h"


// FOR THE CUSTOM PREDICTOR
#define NO_TX_TABLES 7

#define C_T0_LBITS 15
#define C_TX_LBITS 8
#define C_T0_ENTRIES (1 << C_T0_LBITS)
#define C_TX_ENTRIES (1 << C_TX_LBITS)

uint8_t c_t0_pred[C_T0_ENTRIES];

uint8_t c_tx_tag[NO_TX_TABLES][C_TX_ENTRIES];
uint8_t c_tx_u[NO_TX_TABLES][C_TX_ENTRIES];
uint8_t c_tx_pred[NO_TX_TABLES][C_TX_ENTRIES];

uint64_t c_ghr;

void init_custom()
{
    c_ghr = 0;
    for (uint32_t i = 0; i < C_T0_ENTRIES; i++) {
        c_t0_pred[i] = WT;
    }
}

uint32_t c_index_hash(uint32_t pc, uint32_t t_idx)
{
    if (t_idx == 0) {
        return pc & (C_T0_ENTRIES - 1);
    }
    uint32_t val = 0;
    for (uint8_t xor_stg = 0; xor_stg < t_idx; xor_stg++) {
        uint8_t s = (uint32_t)(c_ghr >> (xor_stg*8)) & ((1 << 8) - 1);
        val = val ^ s;
    }

    val = val % 251;
    val = val ^ ((pc >> 8) & ((1 << 8) - 1));
    return val;
}

uint32_t c_tag_hash(uint32_t pc, uint32_t t_idx)
{
    uint32_t val = (pc >> 8) & ((1 << 8) - 1);
    for (uint8_t xor_stg = 0; xor_stg < t_idx; xor_stg++) {
        uint8_t s = (uint32_t)(c_ghr >> (xor_stg*8)) & ((1 << 8) - 1);
        val = val ^ s;
    }
    return val;
}

uint8_t custom_predict(uint32_t pc)
{
    uint32_t tx = 0;
    uint8_t tx_entry = 0;

    for (tx = NO_TX_TABLES - 1; tx > 0; tx--)
    {
        tx_entry = c_index_hash(pc, tx);
        if (c_tx_tag[tx][tx_entry] == c_tag_hash(pc, tx))
        {
            // match found in this table
            return (c_tx_pred[tx][tx_entry] >= WT3) ? TAKEN : NOTTAKEN;
        }
    }

    // no match was found
    tx_entry = c_index_hash(pc, 0);
    return (c_t0_pred[tx_entry] >= WT) ? TAKEN : NOTTAKEN;
}

void train_custom(uint32_t pc, uint8_t outcome)
{
    uint32_t tx = 0;
    uint8_t tx_entry = 0;

    for (tx = NO_TX_TABLES - 1; tx > 0; tx--)
    {
        tx_entry = c_index_hash(pc, tx);
        if (c_tx_tag[tx][tx_entry] == c_tag_hash(pc, tx))
        {
            break;
        }
    }

    bool update_proj;
    if (tx == 0) {
        // no match was found
        tx_entry = c_index_hash(pc, 0);
        
        if (outcome == TAKEN)
        {
            INC_CNTR(c_t0_pred[tx_entry]);
        }
        else
        {
            DEC_CNTR(c_t0_pred[tx_entry]);
        }
    } else {

    }

    c_ghr = (c_ghr << 1) | outcome;
}
