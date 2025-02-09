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
#define tourney_lhistoryBits 10    // Number of bits used for Local Pattern History
#define tourney_lbhthistoryBits 16 // Number of bits used for Local BHT
#define tourney_ghistoryBits 16    // Number of bits used for Global History
#define tourney_choiceBits 16      // Number of bits used for Choice Table

uint64_t tourney_global_hr;

uint16_t tourney_local_ht[1 << tourney_lhistoryBits];
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
        tourney_local_ht[i] = 0;
    }

    int local_pbht_entries = 1 << tourney_lbhthistoryBits;
    for (i = 0; i < local_pbht_entries; i++)
    {
        tourney_local_pred[i] = WN;
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
    uint32_t local_pbht_entries = 1 << tourney_lbhthistoryBits;
    uint16_t pht_index = pc & (local_bht_entries - 1);
    uint16_t index = tourney_local_ht[pht_index];

    if (index > 1023) index = 0;

    return (tourney_local_pred[index] >= 2) ? TAKEN : NOTTAKEN;
}

uint8_t tourney_predict(uint32_t pc)
{
    uint8_t local_pred = tourney_predict_local(pc);
    uint8_t global_pred = tourney_predict_global(pc);

    uint32_t choice_entries = 1 << tourney_choiceBits;
    uint32_t index = tourney_global_hr & (choice_entries - 1);

    if (tourney_choice_pred[index] >= WT)
        return global_pred;
    else
        return local_pred;
}

void train_tourney(uint32_t pc, uint8_t outcome)
{
    uint8_t local_pred = tourney_predict_local(pc);
    uint8_t global_pred = tourney_predict_global(pc);

    uint32_t choice_entries = 1 << tourney_choiceBits;
    uint32_t choice_index = tourney_global_hr & (choice_entries - 1);

    if ((local_pred == outcome) && (global_pred != outcome))
    {
        tourney_choice_pred[choice_index] = DEC_CNTR(tourney_choice_pred[choice_index]);
    }
    else if ((global_pred == outcome) && (local_pred != outcome))
    {
        tourney_choice_pred[choice_index] = INC_CNTR(tourney_choice_pred[choice_index]);
    }

    uint32_t global_bht_entries = 1 << tourney_ghistoryBits;
    uint32_t global_index = tourney_global_hr & (global_bht_entries - 1);

    uint32_t local_bht_entries = 1 << tourney_lhistoryBits;
    uint32_t local_pbht_entries = 1 << tourney_lbhthistoryBits;
    uint32_t pht_index = pc & (local_bht_entries - 1);
    uint32_t local_index = tourney_local_ht[pht_index];
    if (local_index > 1023) local_index = 0;

    if (outcome == TAKEN)
    {
        tourney_global_pred[global_index] = INC_CNTR(tourney_global_pred[global_index]);
        tourney_local_pred[local_index] = INC_CNTR(tourney_local_pred[local_index]);
    }
    else
    {
        tourney_global_pred[global_index] = DEC_CNTR(tourney_global_pred[global_index]);
        tourney_local_pred[local_index] = DEC_CNTR(tourney_local_pred[local_index]);
    }

    tourney_global_hr = ((tourney_global_hr << 1) | outcome);
    tourney_local_ht[pht_index] = ((tourney_local_ht[pht_index] << 1) | outcome);
}

// FOR THE CUSTOM PREDICTOR
#define NUM_TABLES 8
#define BIMODAL_SIZE 16384 // 16K entries
#define TAG_BITS 14
#define MAX_HISTORY 1024
#define GHR_LENGTH 512

// Geometric history lengths (4, 6, 10, 16, 25, 40, 64, 102, 164, 262, 420, 672)
// const uint32_t HIST_LENGTHS[NUM_TABLES] = {4,6,10,16,25,40,64,102,164,262,420,672};
const uint32_t HIST_LENGTHS[NUM_TABLES] = {4, 6, 10, 16, 25, 40, 64, 102};

typedef struct
{
    uint16_t tag;
    uint8_t ctr;
    uint8_t useful;
} TageEntry;

// Predictor components
uint8_t bimodal[BIMODAL_SIZE];
TageEntry tage_tables[NUM_TABLES][BIMODAL_SIZE]; // 196KB per table

// History components
uint8_t ghr[MAX_HISTORY];
uint32_t ghr_ptr;
uint64_t path_history;

// Enhanced statistical corrector
uint8_t sc_table[BIMODAL_SIZE]; // 16K entries

// Folding function for large histories
static inline uint64_t fold_hash(uint64_t val, uint32_t bits)
{
    if (bits >= 64)
    {
        return val; // No need to fold if bits == full width
    }

    uint64_t folded = 0;
    uint32_t remaining_bits = 64;

    while (remaining_bits > 0)
    {
        uint32_t current_shift = (remaining_bits > bits) ? bits : remaining_bits;
        folded ^= val & ((1ULL << current_shift) - 1);
        val >>= current_shift;
        remaining_bits -= current_shift;
    }
    return folded;
}

void init_custom()
{
    // Initialize bimodal table
    for (int i = 0; i < BIMODAL_SIZE; i++)
    {
        bimodal[i] = 4; // Weakly taken (4/7)
    }

    // Initialize TAGE tables
    for (int t = 0; t < NUM_TABLES; t++)
    {
        for (int i = 0; i < BIMODAL_SIZE; i++)
        {
            tage_tables[t][i].tag = 0;
            tage_tables[t][i].ctr = 4; // Weakly taken
            tage_tables[t][i].useful = 0;
        }
    }

    // Initialize history
    ghr_ptr = 0;
    path_history = 0;
    for (int i = 0; i < MAX_HISTORY; i++)
        ghr[i] = 0;

    // Initialize statistical corrector
    for (int i = 0; i < BIMODAL_SIZE; i++)
        sc_table[i] = 4;
}

uint8_t custom_predict(uint32_t pc)
{
    // Base prediction from bimodal
    uint32_t bimodal_idx = pc % BIMODAL_SIZE;
    uint8_t pred = bimodal[bimodal_idx] >= 4;
    int provider = -1;

    // TAGE prediction
    for (int t = NUM_TABLES - 1; t >= 0; t--)
    {
        uint64_t hist = fold_hash(path_history, HIST_LENGTHS[t]);
        uint32_t idx = (pc ^ hist ^ fold_hash(ghr[ghr_ptr], HIST_LENGTHS[t])) % BIMODAL_SIZE;
        uint16_t tag = (pc >> 2) ^ (hist >> 16) ^ (ghr[ghr_ptr] << 4);

        if (tage_tables[t][idx].tag == (tag & ((1 << TAG_BITS) - 1)))
        {
            provider = t;
            pred = tage_tables[t][idx].ctr >= 4;
            break;
        }
    }

    // Statistical corrector
    uint32_t sc_idx = (pc ^ fold_hash(path_history, 16)) % BIMODAL_SIZE;
    uint8_t sc_pred = sc_table[sc_idx] >= 4;

    // Combined prediction
    return (pred != sc_pred) ? sc_pred : pred;
}

void train_custom(uint32_t pc, uint8_t outcome)
{
    // Update bimodal
    uint32_t bimodal_idx = pc % BIMODAL_SIZE;
    if (outcome)
    {
        bimodal[bimodal_idx] += (bimodal[bimodal_idx] < 7) ? 1 : 0;
    }
    else
    {
        bimodal[bimodal_idx] -= (bimodal[bimodal_idx] > 0) ? 1 : 0;
    }

    // TAGE update
    int provider = -1;
    int alloc_table = -1;

    // Find provider and potential allocation candidates
    for (int t = NUM_TABLES - 1; t >= 0; t--)
    {
        uint64_t hist = fold_hash(path_history, HIST_LENGTHS[t]);
        uint32_t idx = (pc ^ hist ^ fold_hash(ghr[ghr_ptr], HIST_LENGTHS[t])) % BIMODAL_SIZE;
        uint16_t tag = (pc >> 2) ^ (hist >> 16) ^ (ghr[ghr_ptr] << 4);

        if (tage_tables[t][idx].tag == (tag & ((1 << TAG_BITS) - 1)))
        {
            if (provider == -1)
            {
                provider = t;
                // Update counter
                if (outcome)
                {
                    tage_tables[t][idx].ctr += (tage_tables[t][idx].ctr < 7) ? 1 : 0;
                }
                else
                {
                    tage_tables[t][idx].ctr -= (tage_tables[t][idx].ctr > 0) ? 1 : 0;
                }
            }
            // Update usefulness
            if (tage_tables[t][idx].useful > 0)
            {
                tage_tables[t][idx].useful--;
            }
        }
        else if (alloc_table == -1 && tage_tables[t][idx].useful == 0)
        {
            alloc_table = t;
        }
    }

    // Allocation logic
    if ((provider == -1) || (tage_tables[provider][0].ctr >= 4) != outcome)
    {
        if (alloc_table == -1)
        {
            // Find table with least useful entries
            uint8_t min_useful = 7;
            for (int t = 0; t < NUM_TABLES; t++)
            {
                uint32_t idx = (pc + t) % BIMODAL_SIZE;
                if (tage_tables[t][idx].useful < min_useful)
                {
                    min_useful = tage_tables[t][idx].useful;
                    alloc_table = t;
                }
            }
        }

        if (alloc_table != -1)
        {
            uint64_t hist = fold_hash(path_history, HIST_LENGTHS[alloc_table]);
            uint32_t idx = (pc ^ hist ^ fold_hash(ghr[ghr_ptr], HIST_LENGTHS[alloc_table])) % BIMODAL_SIZE;
            tage_tables[alloc_table][idx].tag = (pc >> 2) ^ (hist >> 16) ^ (ghr[ghr_ptr] << 4);
            tage_tables[alloc_table][idx].ctr = outcome ? 6 : 2;
            tage_tables[alloc_table][idx].useful = 3;
        }
    }

    // Update statistical corrector
    uint32_t sc_idx = (pc ^ fold_hash(path_history, 16)) % BIMODAL_SIZE;
    if (outcome)
    {
        sc_table[sc_idx] += (sc_table[sc_idx] < 7) ? 1 : 0;
    }
    else
    {
        sc_table[sc_idx] -= (sc_table[sc_idx] > 0) ? 1 : 0;
    }

    // Update history
    ghr[ghr_ptr] = (ghr[ghr_ptr] << 1) | outcome;
    ghr_ptr = (ghr_ptr + 1) % GHR_LENGTH;
    path_history = (path_history << 1) | (pc & 1);
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
