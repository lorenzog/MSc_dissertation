#ifndef _EVOLUTION_H
#define _EVOLUTION_H

#include <string.h>
#include <search.h>
#include <assert.h>

#include "scenario.h"

/* for manual interrupts */
short keep_going;

/* GA params */
static const float PROB_MUT = 0.5;
static const float PROB_X = 0.05;

/* FANN parameters: create */
static const unsigned int NUM_LAYERS = 3; 
static const unsigned int NUM_OUTPUT = 1; 

/* FANN parameters: train */
static const unsigned int EPOCHS_BETWEEN_REPORTS = 0; 

/* strategy params */
static const int MIN_COMMANDS = 4;	/* minimum number of commands */

//static const int TRAINING_SESSIONS = 10;
//static const int TESTING_SESSIONS = 100;

void evolve (char* datafile, int generations, unsigned int epochs, float error, int max_len, int starting_len, int training_sessions, int testing_sessions);

#endif
