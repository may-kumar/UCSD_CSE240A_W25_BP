
#include <stdio.h>
#include <math.h>
#include "predictor.h"

uint32_t custom_lhistoryBits = 10; // Number of bits used for Local History
uint32_t custom_ghistoryBits = 16; // Number of bits used for Global History
uint32_t custom_choiceBits = 16;   // Number of bits used for Choice Table
uint32_t *custom_local_ht;
uint64_t custom_global_hr;

uint8_t *custom_local_pred;
uint8_t *custom_global_pred;
uint8_t *custom_choice_pred;

int8_t W[1024][64];

void init_custom()
{
    int local_bht_entries = 1 << custom_lhistoryBits;
    custom_local_pred = (uint8_t *)malloc(local_bht_entries * sizeof(uint8_t));
    custom_local_ht = (uint32_t *)malloc(local_bht_entries * sizeof(uint32_t));
    int i = 0;
    for (i = 0; i < local_bht_entries; i++)
    {
        custom_local_pred[i] = WT;
        custom_local_ht[i] = 0;
    }

    custom_global_hr = 0;
    
    for(int i = 0; i < 1024; i++) {
        for(int j = 0; j < 32; j++) {
            W[i][j] = 0;
        }
    }

    int choice_t_entries = 1 << custom_choiceBits;
    custom_choice_pred = (uint8_t *)malloc(choice_t_entries * sizeof(uint8_t));
    for (i = 0; i < choice_t_entries; i++)
    {
        custom_choice_pred[i] = SG;
    }
}

uint8_t custom_predict_global(uint32_t pc)
{
    uint64_t index = pc & ( (1 << 10 ) - 1);
    int64_t y = 0;

    for (int i = 0; i < 64; i ++) {
        y = y + W[index][63 - i] * ((custom_global_hr >> i) & 1);
    }

    return (y > 0) ? TAKEN : NOTTAKEN ;
}

uint8_t custom_predict_local(uint32_t pc)
{
    uint32_t local_bht_entries = 1 << custom_lhistoryBits;
    uint32_t pht_index = pc & (local_bht_entries - 1);
    uint32_t index = custom_local_ht[pht_index];

    return (custom_local_pred[index] >= 2) ? TAKEN : NOTTAKEN;
}

uint8_t custom_predict(uint32_t pc)
{
    uint8_t local_pred = custom_predict_local(pc);
    uint8_t global_pred = custom_predict_global(pc);

    uint32_t choice_entries = 1 << custom_choiceBits;
    uint32_t index = custom_global_hr & (choice_entries - 1);

    if (custom_choice_pred[index] >= 2)
        return local_pred;
    else
        return global_pred;
}

void train_custom(uint32_t pc, uint8_t outcome)
{
    uint8_t local_pred = custom_predict_local(pc);
    uint8_t global_pred = custom_predict_global(pc);

    uint32_t choice_entries = 1 << custom_choiceBits;
    uint32_t choice_index = custom_global_hr & (choice_entries - 1);

    if ((local_pred == outcome) && (global_pred != outcome))
    {
        INC_CNTR(custom_choice_pred[choice_index]);
    }
    else if ((global_pred == outcome) && (local_pred != outcome))
    {
        DEC_CNTR(custom_choice_pred[choice_index]);
    }

    uint32_t global_bht_entries = 1 << custom_ghistoryBits;
    uint32_t global_index = custom_global_hr & (global_bht_entries - 1);

    uint32_t local_bht_entries = 1 << custom_lhistoryBits;
    uint32_t pht_index = pc & (local_bht_entries - 1);
    uint32_t local_index = custom_local_ht[pht_index];

    uint64_t index = pc & ( (1 << 10 ) - 1 );
    int64_t y = 0;

    for (int i = 0; i < 64; i ++) {
        y = y + W[index][63 - i] * ((custom_global_hr >> i) & 1);
    }

    int corr = (outcome == TAKEN) ? 1 : -1;
    if( ((y < 0) != (corr < 0)) || (y < 32 && y > -32) )
    {
        for(int i = 0; i < 64; i++)
        {
            W[index][63 - i] += ((custom_global_hr >> i) & 1) * corr;
        }           
    }

    if (outcome == TAKEN)
    {
        INC_CNTR(custom_local_pred[local_index]);
    }
    else
    {
        DEC_CNTR(custom_local_pred[local_index]);
    }

    custom_global_hr = ((custom_global_hr << 1) | outcome);
    custom_local_ht[pht_index] = ((custom_local_ht[pht_index] << 1) | outcome) & (local_bht_entries - 1);
}