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


//21264 Alpha processor : tournament predictor

// t = table
// r = register 
uint32_t tourney_lhistoryBits = 10; // Number of bits used for Local History
uint32_t tourney_ghistoryBits = 12; // Number of bits used for Global History
uint32_t tourney_choiceBits   = 12; // Number of bits used for Choice Table
uint32_t *tourney_local_ht;
uint32_t tourney_global_hr;

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


//Tourney Predictor : 
// tourament functions
void init_tourney()
{
  int local_bht_entries = 1 << tourney_lhistoryBits;
  tourney_local_pred = (uint8_t *)malloc(local_bht_entries * sizeof(uint8_t));
  tourney_local_ht = (uint32_t *)malloc(local_bht_entries * sizeof(uint32_t));
  int i = 0;
  for (i = 0; i < local_bht_entries; i++)
  {
    tourney_local_pred[i] = WT3;
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

}

uint8_t tourney_predict_global(uint32_t pc) {
    uint32_t choice_t_entries = 1 << tourney_choiceBits; 
    uint32_t ghr_lower_bits = tourney_global_hr & (choice_t_entries - 1);

    switch (tourney_global_pred[ghr_lower_bits])
    {
    case SN:
        return NOTTAKEN;
    case WN:
        return NOTTAKEN;
    case WT:
        return TAKEN;
    case ST:
        return TAKEN;
    default:
        printf("Warning: Undefined state of entry in TOURNEY GLOBAL BHT!\n");
        return NOTTAKEN;
    }
}

uint8_t tourney_predict_local(uint32_t pc) {
    // need to index using pc[9:0]
    int local_bht_entries = 1 << tourney_lhistoryBits;
    uint32_t pc_lower_bits = pc & (local_bht_entries - 1);
    uint32_t index = tourney_local_ht[pc_lower_bits] & (local_bht_entries - 1);
    switch (tourney_local_pred[index])
    {
    case SN0:
        return NOTTAKEN;
    case WN1:
        return NOTTAKEN;
    case WN2:
        return NOTTAKEN;
    case WN3:
        return NOTTAKEN;
    case WT3:
        return TAKEN;
    case WT2:
        return TAKEN;
    case WT1:
        return TAKEN;
    case ST0:
        return TAKEN;
    default:
        printf("Warning: Undefined state of entry in TOURNEY GLOBAL BHT!\n");
        return NOTTAKEN;
    }
}

uint8_t tourney_predict(uint32_t pc)
{
  // First we need to choose if we're using local or global tables so we use the choice predictor
  uint32_t choice_t_entries = 1 << tourney_choiceBits; 
  uint32_t ghr_lower_bits = tourney_global_hr & (choice_t_entries - 1);

  if (tourney_choice_pred[ghr_lower_bits] == WG || tourney_choice_pred[ghr_lower_bits] == SG ) {
        // we should use the global predictor here
        // printf("using global\n");
        return tourney_predict_global(pc);
  } else if (tourney_choice_pred[ghr_lower_bits] == WL || tourney_choice_pred[ghr_lower_bits] == SL ) {
        // we should use the local predictor here
        // printf("using local\n");
        return tourney_predict_local(pc);
  } else {
        printf("Warning: Choice predictor gave an invalid value!\n");
        return NOTTAKEN;
  }
}



void train_tourney(uint32_t pc, uint8_t outcome)
{

    uint32_t choice_t_entries = 1 << tourney_choiceBits; 
    uint32_t ghr_lower_bits = tourney_global_hr & (choice_t_entries - 1);

    uint32_t local_bht_entries = 1 << tourney_lhistoryBits;
    uint32_t pc_lower_bits = pc & (local_bht_entries - 1);
    uint32_t index = tourney_local_ht[pc_lower_bits] & (local_bht_entries - 1);

    bool global_correct;
    bool local_correct;
    
    global_correct = (tourney_predict_global(pc) == outcome);
    local_correct = (tourney_predict_local(pc) == outcome);

    // update the choice predictor to choose the correct one next time
    if (global_correct != local_correct) {
        switch (tourney_choice_pred[ghr_lower_bits])
        {
        case SG:
            tourney_choice_pred[ghr_lower_bits] = (global_correct) ? SG : WG;
            break;
        case WG:
            tourney_choice_pred[ghr_lower_bits] = (global_correct) ? SG : WL;
            break;
        case WL:
            tourney_choice_pred[ghr_lower_bits] = (global_correct) ? WG : SL;
            break;
        case SL:
            tourney_choice_pred[ghr_lower_bits] = (global_correct) ? WL : SL;
            break;
        default:
            printf("Warning: Undefined state of entry in tourney choice!\n");
            break;
        }
    }


    // Update state of entry in bht based on outcome
    switch (tourney_local_pred[index])
    {
        case SN0:
            tourney_local_pred[index] = (outcome == TAKEN) ? WN1 : SN0;
            break;
        case WN1:
            tourney_local_pred[index] = (outcome == TAKEN) ? WN2 : SN0;
            break;
        case WN2:
            tourney_local_pred[index] = (outcome == TAKEN) ? WN3 : WN1;
            break;
        case WN3:
            tourney_local_pred[index] = (outcome == TAKEN) ? WT3 : WN2;
            break;
        case WT3:
            tourney_local_pred[index] = (outcome == TAKEN) ? WT2 : WN3;
            break;
        case WT2:
            tourney_local_pred[index] = (outcome == TAKEN) ? WT1 : WT3;
            break;
        case WT1:
            tourney_local_pred[index] = (outcome == TAKEN) ? ST0 : WT2;
            break;
        case ST0:
            tourney_local_pred[index] = (outcome == TAKEN) ? ST0 : WT1;
            break;
        default:
            printf("Warning: Undefined state of entry in TOURNEY GLOBAL BHT!\n");
            break;
    }

    // Update state of entry in bht based on outcome
    switch (tourney_global_pred[ghr_lower_bits])
    {
        case SN:
            tourney_global_pred[ghr_lower_bits] = (outcome == TAKEN) ? WN : SN;
            break;
        case WN:
            tourney_global_pred[ghr_lower_bits] = (outcome == TAKEN) ? WT : SN;
            break;
        case WT:
            tourney_global_pred[ghr_lower_bits] = (outcome == TAKEN) ? ST : WN;
            break;
        case ST:
            tourney_global_pred[ghr_lower_bits] = (outcome == TAKEN) ? ST : WT;
            break;
        default:
            printf("Warning: Undefined state of entry in GLOBAL TOURNEY BHT!\n");
            break;
    }

    // Update history registers
    tourney_global_hr = ((tourney_global_hr << 1) | outcome) & (choice_t_entries - 1);
    tourney_local_ht[pc_lower_bits] = ((tourney_local_ht[pc_lower_bits] << 1) | outcome) & (local_bht_entries - 1);
    // printf("Checking stuff : %0X %0X %0X\n", tourney_global_hr, pc_lower_bits, tourney_local_ht[pc_lower_bits]);
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
    return tourney_predict(pc);
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
      return;
    default:
      break;
    }
  }
}
