
#include <stdio.h>
#include <math.h>
#include "predictor.h"

// FOR THE CUSTOM PREDICTOR
#define NO_TX_TABLES 5

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
    if (t_idx == 1) {
        val = (uint64_t)(c_ghr & ((1 << 4) - 1));
    } else {
        for (uint8_t xor_stg = 0; xor_stg <= (1 << (t_idx - 2)); xor_stg++) {
            uint8_t s = (uint64_t)(c_ghr >> (8*(xor_stg-1))) & ((1 << 8) - 1);
            val = val ^ s;
        }   
    }

    val = val ^ ((pc >> 8) & ((1 << 8) - 1));

    return val;
}

uint32_t c_tag_hash(uint32_t pc, uint32_t t_idx)
{
    uint32_t val = 0;
    if (t_idx == 1) {
        val = (uint64_t)(c_ghr & ((1 << 4) - 1));
    } else {
        for (uint8_t xor_stg = 0; xor_stg <= (1 << (t_idx - 2)); xor_stg++) {
            uint8_t s = (uint64_t)(c_ghr >> (8*(xor_stg-1))) & ((1 << 8) - 1);
            val = val ^ s;
        }   
    }

    val = val % 251;
    val = val ^ (pc >> 8) & ((1 << 8) - 1);
    return val;
}

uint8_t custom_predict(uint32_t pc)
{
    uint32_t tx = 0;
    uint8_t tx_entry = 0;

    printf("using pred\n");

    for (tx = NO_TX_TABLES - 1; tx > 0; tx--)
    {
        tx_entry = c_index_hash(pc, tx);
        if (c_tx_tag[tx][tx_entry] == c_tag_hash(pc, tx))
        {
            // match found in this table
            // printf("Using tx table %0d %0d\n", tx, (c_tx_pred[tx][tx_entry] >= WT3));
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
    bool tx_matches[NO_TX_TABLES] = {false};
    uint8_t tx_matches_cnt = 0;
    uint8_t tx_entry = 0;
    uint8_t tx_pred = 0;

    for (tx = NO_TX_TABLES - 1; tx > 0; tx--)
    {
        tx_entry = c_index_hash(pc, tx);
        if (c_tx_tag[tx][tx_entry] == c_tag_hash(pc, tx))
        {
            tx_matches[tx] = true;
            tx_pred = (tx_pred == 0) ? tx : tx_pred ;
            tx_matches_cnt++;
        }
    }

    uint8_t pred = 0;
    if (tx == 0) {
        // no match was found
        tx_entry = c_index_hash(pc, 0);
        pred = (c_t0_pred[tx_entry] >= WT) ? TAKEN : NOTTAKEN;
    } else {
        tx_entry = c_index_hash(pc, tx_pred);
        pred = (c_tx_pred[tx_pred][tx_entry] >= WT3) ? TAKEN : NOTTAKEN;
    }


    if (pred == outcome) {
        INC_3B_CNTR(c_tx_pred[tx_pred][tx_entry]);
        if (tx_matches_cnt == 1) {
            tx_entry = c_index_hash(pc, tx_pred);
            INC_CNTR(c_tx_u[tx_pred][tx_entry]);
        } else {
            //Subtracting u counters for alt-preds
            for (uint32_t i = 1; i < tx_pred; i++) {
                if (tx_matches[i] == true) {
                    tx_entry = c_index_hash(pc, i);
                    DEC_CNTR(c_tx_u[i][tx_entry]);
                }
            }
        }
    } else {
        DEC_3B_CNTR(c_tx_pred[tx_pred][tx_entry]);
        for (uint32_t i = 1; i < tx_pred; i++) {
            if (tx_matches[i] == true) {
                tx_entry = c_index_hash(pc, i);
                uint8_t i_pred = (c_tx_pred[i][tx_entry] >= WT3) ? TAKEN : NOTTAKEN;
                if (i_pred == outcome) {
                    for (uint32_t j = i; j < tx_pred; j++) {
                        if (tx_matches[j] == true) {
                            tx_entry = c_index_hash(pc, j);
                            DEC_CNTR(c_tx_u[j][tx_entry]);
                        }
                    }
                    break;
                }
            }
        }
        bool entry_promoted = false;

        for (uint32_t i = tx_pred; i < NO_TX_TABLES; i++) {
            tx_entry = c_index_hash(pc, i);
            if (c_tx_u[i][tx_entry] == 0) {
                entry_promoted = true;
                c_tx_tag[i][tx_entry] = c_tag_hash(pc, i);
                c_tx_u[i][tx_entry] = 0;
                c_tx_pred[i][tx_entry] = WT;
                break;
            }
        }

        if (entry_promoted == false) {
            for (uint32_t i = tx_pred; i < NO_TX_TABLES; i++) {
                tx_entry = c_index_hash(pc, i);
                DEC_CNTR(c_tx_u[i][tx_entry]);
            }
        }
    }
    
    if (tx == 0) {
        // no match was found
        tx_entry = c_index_hash(pc, 0);
        
        if (outcome == TAKEN) {
            INC_CNTR(c_t0_pred[tx_entry]);
        } else {
            DEC_CNTR(c_t0_pred[tx_entry]);
        }
    }

    c_ghr = (c_ghr << 1) | outcome;
}