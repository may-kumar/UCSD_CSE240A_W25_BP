//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//                                                        //
// This code is implemented by Mayank Kumar               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Mayank Kumar";
const char *studentID = "A69030454";
const char *email = "mak025@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// 21264 Alpha processor : tournament predictor

// t = table
// r = register
uint32_t tourney_lhistoryBits = 10; // Number of bits used for Local History
uint32_t tourney_ghistoryBits = 16; // Number of bits used for Global History
uint32_t tourney_choiceBits = 16;   // Number of bits used for Choice Table
uint32_t *tourney_local_ht;
uint64_t tourney_global_hr;

uint8_t *tourney_local_pred;
uint8_t *tourney_global_pred;
uint8_t *tourney_choice_pred;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
    int bht_entries = 1 << ghistoryBits;
    bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
    int i = 0;
    for (i = 0; i < bht_entries; i++)
    {
        bht_gshare[i] = WN;
    }
    ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
    // get lower ghistoryBits of pc
    uint32_t bht_entries = 1 << ghistoryBits;
    uint32_t pc_lower_bits = pc & (bht_entries - 1);
    uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
    uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
    switch (bht_gshare[index])
    {
    case WN:
        return NOTTAKEN;
    case SN:
        return NOTTAKEN;
    case WT:
        return TAKEN;
    case ST:
        return TAKEN;
    default:
        printf("Warning: Undefined state of entry in GSHARE BHT!\n");
        return NOTTAKEN;
    }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
    // get lower ghistoryBits of pc
    uint32_t bht_entries = 1 << ghistoryBits;
    uint32_t pc_lower_bits = pc & (bht_entries - 1);
    uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
    uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

    // Update state of entry in bht based on outcome
    switch (bht_gshare[index])
    {
    case WN:
        bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
        break;
    case SN:
        bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
        break;
    case WT:
        bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
        break;
    case ST:
        bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
        break;
    default:
        printf("Warning: Undefined state of entry in GSHARE BHT!\n");
        break;
    }

    // Update history register
    ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
    free(bht_gshare);
}

// Tourney Predictor :
//  tourament functions
void init_tourney()
{
    int local_bht_entries = 1 << tourney_lhistoryBits;
    tourney_local_pred = (uint8_t *)malloc(local_bht_entries * sizeof(uint8_t));
    tourney_local_ht = (uint32_t *)malloc(local_bht_entries * sizeof(uint32_t));
    int i = 0;
    for (i = 0; i < local_bht_entries; i++)
    {
        tourney_local_pred[i] = WT;
        tourney_local_ht[i] = 0;
    }

    int global_bht_entries = 1 << tourney_ghistoryBits;
    tourney_global_pred = (uint8_t *)malloc(global_bht_entries * sizeof(uint8_t));
    for (i = 0; i < global_bht_entries; i++)
    {
        tourney_global_pred[i] = WT;
    }

    int choice_t_entries = 1 << tourney_choiceBits;
    tourney_choice_pred = (uint8_t *)malloc(choice_t_entries * sizeof(uint8_t));
    for (i = 0; i < choice_t_entries; i++)
    {
        tourney_choice_pred[i] = SG;
    }
    tourney_global_hr = 0;
}

uint8_t tourney_predict_global(uint32_t pc)
{
    uint32_t global_bht_entries = 1 << tourney_ghistoryBits;
    uint32_t index = tourney_global_hr & (global_bht_entries - 1);

    return (tourney_global_pred[index] >= 2) ? TAKEN : NOTTAKEN;
}

uint8_t tourney_predict_local(uint32_t pc)
{
    uint32_t local_bht_entries = 1 << tourney_lhistoryBits;
    uint32_t pht_index = pc & (local_bht_entries - 1);
    uint32_t index = tourney_local_ht[pht_index];

    return (tourney_local_pred[index] >= 2) ? TAKEN : NOTTAKEN;
}

uint8_t tourney_predict(uint32_t pc)
{
    uint8_t local_pred = tourney_predict_local(pc);
    uint8_t global_pred = tourney_predict_global(pc);

    uint32_t choice_entries = 1 << tourney_choiceBits;
    uint32_t index = tourney_global_hr & (choice_entries - 1);

    if (tourney_choice_pred[index] >= 2)
        return local_pred;
    else
        return global_pred;
}

void train_tourney(uint32_t pc, uint8_t outcome)
{
    uint8_t local_pred = tourney_predict_local(pc);
    uint8_t global_pred = tourney_predict_global(pc);

    uint32_t choice_entries = 1 << tourney_choiceBits;
    uint32_t choice_index = tourney_global_hr & (choice_entries - 1);

    if ((local_pred == outcome) && (global_pred != outcome))
    {
        INC_CNTR(tourney_choice_pred[choice_index]);
    }
    else if ((global_pred == outcome) && (local_pred != outcome))
    {
        DEC_CNTR(tourney_choice_pred[choice_index]);
    }

    uint32_t global_bht_entries = 1 << tourney_ghistoryBits;
    uint32_t global_index = tourney_global_hr & (global_bht_entries - 1);

    uint32_t local_bht_entries = 1 << tourney_lhistoryBits;
    uint32_t pht_index = pc & (local_bht_entries - 1);
    uint32_t local_index = tourney_local_ht[pht_index];

    if (outcome == TAKEN)
    {
        INC_CNTR(tourney_global_pred[global_index]);
        INC_CNTR(tourney_local_pred[local_index]);
    }
    else
    {
        DEC_CNTR(tourney_global_pred[global_index]);
        DEC_CNTR(tourney_local_pred[local_index]);
    }

    tourney_global_hr = ((tourney_global_hr << 1) | outcome) & (global_bht_entries - 1);
    tourney_local_ht[pht_index] = ((tourney_local_ht[pht_index] << 1) | outcome) & (local_bht_entries - 1);
}

// FOR THE CUSTOM PREDICTOR
uint32_t custom_lhistoryBits = 10; // Number of bits used for Local History
uint32_t custom_ghistoryBits = 16; // Number of bits used for Global History
uint32_t custom_choiceBits = 16;   // Number of bits used for Choice Table
uint32_t *custom_local_ht;
uint64_t custom_global_hr;

uint8_t *custom_local_pred;
uint8_t *custom_global_pred;
uint8_t *custom_choice_pred;

uint8_t c_t0_pred[1 << 15];

uint8_t c_tx_tag[7][1024];
uint8_t c_tx_u[7][1024];
uint8_t c_tx_pred[7][1024];

uint32_t c_t0_entries = 1 << 15;
uint32_t c_t1_entries = 1 << 10;
uint32_t c_t2_entries = 1 << 10;
uint32_t c_t3_entries = 1 << 10;
uint32_t c_t4_entries = 1 << 10;
uint32_t c_t5_entries = 1 << 10;
uint32_t c_t6_entries = 1 << 10;
uint32_t c_t7_entries = 1 << 10;

int8_t W[1024][64];

#define LOOP_ENTRIES 256
uint32_t loop_cntr[LOOP_ENTRIES];
uint32_t used_cntr[LOOP_ENTRIES];
uint8_t age_cntr[LOOP_ENTRIES];
uint8_t confidence[LOOP_ENTRIES];
uint8_t hash_tag[LOOP_ENTRIES];
bool init_entry[LOOP_ENTRIES];

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

    for (i = 0 ; i < LOOP_ENTRIES; i++) {
        loop_cntr[i] = 0;
        used_cntr[i] = 0;
        age_cntr[i] = 0;
        confidence[i] = 0;
        hash_tag[i] = 0;
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

    return custom_local_pred[index] >= 2;
}

uint8_t custom_predict(uint32_t pc)
{
    uint8_t local_pred = custom_predict_local(pc);
    uint8_t global_pred = custom_predict_global(pc);

    uint32_t choice_entries = 1 << custom_choiceBits;
    uint32_t index = custom_global_hr & (choice_entries - 1);

    
    uint8_t loop_tag = (uint32_t)(pc ^ (pc >> 8) ^ (pc >> 16) ^ (pc >> 24)) & ( (1 << 8) - 1 );
    uint32_t i = 0;
    for (i = 0; i < LOOP_ENTRIES; i++) {
        if (hash_tag[i] == loop_tag) {
            break;
        }
    }

    if (i != LOOP_ENTRIES) {
        if (confidence[i] == ST) {
            if (loop_cntr[i] == used_cntr[i]) {
                // printf("USING LOOP");
                // printf("%0X\n", pc);
                return !(init_entry[i]);
            } else {
                // printf("USING LOOP");
                return init_entry[i];
            }
        }
    }

    if (custom_choice_pred[index] >= 2)
        return local_pred;
    else
        return global_pred;
}

void train_custom(uint32_t pc, uint8_t outcome)
{
    
    uint8_t loop_tag = (uint32_t)(pc ^ (pc >> 8) ^ (pc >> 16) ^ (pc >> 24)) & ( (1 << 8) - 1 );
    uint32_t i = 0;
    for (i = 0; i < LOOP_ENTRIES; i++) {
        if (hash_tag[i] == loop_tag) {
            // printf("FOUND A LOOP");
            break;
        }
    }

    if (i != LOOP_ENTRIES) {
        if (confidence[i] == SN) { //This is a new entry, we're still figuring things out
            if (outcome == init_entry[i]) {
                loop_cntr[i]++;
            } else {
                INC_CNTR(confidence[i]);
            }
        } else { //Ok we've seen this before and got a switch
            if (outcome != init_entry[i]) {
                if  (loop_cntr[i] == used_cntr[i]) {
                    if (confidence[i] == ST) {
                        age_cntr[i] = (age_cntr[i] != 255) ? age_cntr[i]++ : 255;
                    } else {
                        INC_CNTR(confidence[i]);
                    }
                    used_cntr[i] = 0;
                } else {
                    //This is not a loop
                    age_cntr[i] = 0;
                    hash_tag[i] = 0;
                    confidence[i] = 0;
                }
            } else {
                used_cntr[i]++;
            }
        }
    } else {
        uint8_t min_age = 255;
        uint8_t min_idx = 0;
        for (i = 0; i < LOOP_ENTRIES; i++) {
            if (age_cntr[i] == 0) {
                min_idx = i;
                break;
            }
            age_cntr[i]--;
            if (age_cntr[i] < min_age) {
                min_age = age_cntr[i];
                min_idx = i;
            }
        }
        // printf("ADDING A LOOP");
        loop_cntr[min_idx] = 0;
        used_cntr[min_idx] = 0;
        age_cntr[min_idx] = 255;
        confidence[min_idx] = 0;
        init_entry[min_idx] = outcome;
        hash_tag[min_idx] = loop_tag;
    }

    
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


void init_predictor()
{
    switch (bpType)
    {
    case STATIC:
        break;
    case GSHARE:
        init_gshare();
        break;
    case TOURNAMENT:
        init_tourney();
        break;
    case CUSTOM:
        init_custom();
        break;
    default:
        break;
    }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

    // Make a prediction based on the bpType
    switch (bpType)
    {
    case STATIC:
        return TAKEN;
    case GSHARE:
        return gshare_predict(pc);
    case TOURNAMENT:
        return tourney_predict(pc);
    case CUSTOM:
        return custom_predict(pc);
    default:
        break;
    }

    // If there is not a compatable bpType then return NOTTAKEN
    return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
    if (condition)
    {
        switch (bpType)
        {
        case STATIC:
            return;
        case GSHARE:
            return train_gshare(pc, outcome);
        case TOURNAMENT:
            return train_tourney(pc, outcome);
        case CUSTOM:
            return train_custom(pc, outcome);
        default:
            break;
        }
    }
}
