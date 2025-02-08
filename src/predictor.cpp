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
#define tourney_lhistoryBits 16 // Number of bits used for Local History
#define tourney_ghistoryBits 16 // Number of bits used for Global History
#define tourney_choiceBits 16   // Number of bits used for Choice Table

uint64_t tourney_global_hr;

uint32_t tourney_local_ht[1 << tourney_lhistoryBits];
uint8_t tourney_local_pred[1 << tourney_lhistoryBits];
uint8_t tourney_global_pred[1 << tourney_ghistoryBits];
uint8_t tourney_choice_pred[1 << tourney_choiceBits];

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
    // tourney_local_pred = (uint8_t *)malloc(local_bht_entries * sizeof(uint8_t));
    // tourney_local_ht = (uint32_t *)malloc(local_bht_entries * sizeof(uint32_t));
    int i = 0;
    for (i = 0; i < local_bht_entries; i++)
    {
        tourney_local_pred[i] = WN;
        tourney_local_ht[i] = 0;
    }

    int global_bht_entries = 1 << tourney_ghistoryBits;
    // tourney_global_pred = (uint8_t *)malloc(global_bht_entries * sizeof(uint8_t));
    for (i = 0; i < global_bht_entries; i++)
    {
        tourney_global_pred[i] = WN;
    }

    int choice_t_entries = 1 << tourney_choiceBits;
    // tourney_choice_pred = (uint8_t *)malloc(choice_t_entries * sizeof(uint8_t));
    for (i = 0; i < choice_t_entries; i++)
    {
        tourney_choice_pred[i] = WT;
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
    uint32_t index = tourney_local_ht[pht_index] & (local_bht_entries - 1);

    return (tourney_local_pred[index] >= 2) ? TAKEN : NOTTAKEN;
}

uint8_t tourney_predict(uint32_t pc)
{
    uint8_t local_pred = tourney_predict_local(pc);
    uint8_t global_pred = tourney_predict_global(pc);

    uint32_t choice_entries = 1 << tourney_choiceBits;
    uint32_t index = tourney_global_hr & (choice_entries - 1);

    if (tourney_choice_pred[index] >= WT)
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
    uint32_t local_index = tourney_local_ht[pht_index] & (local_bht_entries - 1);

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

    tourney_global_hr = ((tourney_global_hr << 1) | outcome);
    tourney_local_ht[pht_index] = ((tourney_local_ht[pht_index] << 1) | outcome);
}


// FOR THE CUSTOM PREDICTOR
#define NO_TX_TABLES 7

#define C_T0_LBITS 15
#define C_TX_LBITS 8
#define C_T0_ENTRIES (1 << C_T0_LBITS)
#define C_TX_ENTRIES (1 << C_TX_LBITS)

uint8_t c_t0_pred[C_T0_ENTRIES];

uint32_t cntr = 0;

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
    bool tx_matches[NO_TX_TABLES] = {false, false, false, false, false};
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
                    for (uint32_t j = i; j < NO_TX_TABLES; j++) {
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

        for (uint32_t i = tx_pred+1; (!entry_promoted) && (i < NO_TX_TABLES); i++) {
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

    cntr++;
    if (cntr == 10000000/16) {
        cntr = 0;
        for (uint32_t i = 1; i < NO_TX_TABLES; i++) {
            for (uint32_t j = 0; j < C_TX_ENTRIES; j++) {
                c_tx_u[i][j] = 0;
            }
        }
    }
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
