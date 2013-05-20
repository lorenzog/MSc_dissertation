#ifndef _SCENARIO_C
#define _SCENARIO_C

#include "scenario.h"

/******************* scenario handling **************/

inline void dbg(char* message) {
#ifdef DBG
	printf(message);
#endif
}

struct scenario_t *gen_scenario() {
	float a, b;
	struct scenario_t *s;
	s = malloc(sizeof(struct scenario_t));
	s->obj1 = malloc(sizeof(struct object_t));
	s->obj2 = malloc(sizeof(struct object_t));
	s->sensor = malloc(sizeof(struct sensor_t));

	/* the first object in the first half of the space */
	a = genrand_real3() * 0.4 + 0.05;
	b = genrand_real3() * 0.4 + 0.05;
	
	s->obj1->start_x = a < b ? a : b;
	s->obj1->end_x = a > b ? a : b;

	/* the second object, in the second half of the space */
	a = genrand_real3() * 0.4 + 0.55;
	b = genrand_real3() * 0.4 + 0.55;

	s->obj2->start_x = a < b ? a : b;
	s->obj2->end_x = a > b ? a : b;

	/* generate distances */
	a = genrand_real3() * 0.4 + 0.05;
	b = genrand_real3() * 0.4 + 0.55;

	if ( gen_rand32() % 2 == 0 ) {
		/* first object is closer */
		s->obj1->start_y = a;
		s->obj2->start_y = b;
		/* save centre of nearest object into scenario information
		 * (used to train/test neural network */
		s->nearest_object_centre = (s->obj1->end_x - s->obj1->start_x) /2;
	} else {
		/* second object is closer */
		s->obj1->start_y = b;
		s->obj2->start_y = a;
		s->nearest_object_centre = (s->obj2->end_x - s->obj2->start_x) /2;
	}

	/* make object depth */
	s->obj1->end_y = s->obj1->start_y + OBJECT_DEPTH;
	s->obj2->end_y = s->obj2->start_y + OBJECT_DEPTH;

	/* place sensor at initial position */
	s->sensor->pos = 0.0;
	s->sensor->angle = M_PI_2;	/* facing up */

	return s;
}

void destroy_scenario(struct scenario_t *s) {
	free(s->obj1);
	free(s->obj2);
	free(s->sensor);
	free(s);
}

/* generate initial conditions */
void init_condition(struct condition_t *now) {
	now->sensor_pos = 0.0;
	now->sensor_angle = M_PI_2;
	now->sensor_status = 0.0;
}

/*************** strategy handling ******************/


static inline int get_num_actions(char* strategy) {
	return strlen(strategy)/2;
}

inline int get_input_neurones(char* strategy) {
	/* 3 values per single action + initial */
	return get_num_actions(strategy) *3;
}

/* 
 * returns 0 when there are no more actions
 * or returns the number of already made actions
 */
static int parse_strategy(char* strategy, int *move, enum action_e *action, enum condition_e *condition) {
	/* check if we're at the end of the strategy */
	if ( strategy[*move] == '\0' )
		return 0;

	*action = strategy[(*move)++];
	*condition = strategy[(*move)++];

	return *move;
}

static void get_angular_coeff(struct object_t *obj, float sens_pos, float sens_angle, float *m1, float *m2) {
	/* initialised so they won't divide by zero */
	float near_x = sens_pos+1, near_y = 0.0, far_x = sens_pos+1, far_y = 0.0;
	if ( sens_pos < obj->start_x ) {
		/* sensor is on the left */
		near_x = obj->end_x;
		near_y = obj->start_y;

		far_x = obj->start_x;
		far_y = obj->end_y;
	} else {
		if ( sens_pos >= obj->start_x && sens_pos <= obj->end_x ) {
			/* sensor inbetween */
			near_x = obj->start_x;
			near_y = obj->start_y;

			far_x = obj->end_x;
			far_y = obj->start_y;
		} else {
			if ( sens_pos > obj->end_x ) {
				/* sensor on the right */
				near_x = obj->end_x;
				near_y = obj->end_y;

				far_x = obj->start_x;
				far_y = obj->start_y;
			}
		}
	}
	*m1 = near_y / ( near_x - sens_pos );
	*m2 = far_y / ( far_x - sens_pos );

}

/* verifies the condition */
static int verify_condition(struct scenario_t *scenario, enum condition_e condition, struct condition_t *now) {

	dbg("verifying condition: ");
	/* angular coefficient of the occupancy of the object */
	float m1_obj1, m2_obj1, m1_obj2, m2_obj2;
	int found_obj1, found_obj2;

	/* sensor angle */
	float tan_left, tan_right;
	float angle_right, angle_left;
	angle_left = now->sensor_angle + SENSOR_CONE;
	angle_right = now->sensor_angle - SENSOR_CONE;
	tan_left = tanf(angle_left);
	tan_right = tanf(angle_right);

	/* get the angular coefficients of the object's corners */
	get_angular_coeff(scenario->obj1, now->sensor_pos, now->sensor_angle, 
			&m1_obj1, &m2_obj1);

	/* is the first object in the line of sight? */
	if ( (tan_left > m1_obj1 && tan_left < m2_obj1)
			|| (tan_right > m1_obj1 && tan_right < m2_obj1) ) {
		found_obj1 = 1;
	} else {
		found_obj1 = 0;
	} 

	/* if we're in front of the object, it's the opposite */
	if ( now->sensor_pos > scenario->obj1->start_x && 
			now->sensor_pos < scenario->obj1->end_x ) {
		dbg("in front of obj1; ");
		found_obj1 = ( found_obj1 == 1 ) ? 0 : 1;
	}

	/* we were looking for the object and we found it */
	if ( condition == OBJECT && found_obj1 == 1 ) {
		dbg("condition verified, object found\n");
		/* update sensor status */
		now->sensor_status = 1;
		return 1;
	}

	/* we were not looking for the object but we found it
	 * -- this test is done here to avoid 'transparency', i.e.
	 *  seeing through objects */
	if ( condition == NON_OBJECT && found_obj1 == 1 ) {
		dbg("condition not verified, object found\n");
		now->sensor_status = 1;
		return 0;
	}

	/* otherwise let's check for the second object */

	get_angular_coeff(scenario->obj2, now->sensor_pos, now->sensor_angle, 
			&m1_obj2, &m2_obj2);
	if ( (tan_left > m1_obj2 && tan_left < m2_obj2)
			|| (tan_right > m1_obj2 && tan_right < m2_obj2) ) {
		found_obj2 = 1;
	} else {
		found_obj2 = 0;
	}

	/* if we're in front of the object, it's the opposite */
	if ( now->sensor_pos > scenario->obj2->start_x && 
			now->sensor_pos < scenario->obj2->end_x ) {
		dbg("in front of obj2; ");
		found_obj2 = ( found_obj2 == 1 ) ? 0 : 1;
	} 

	/* we were looking for the object and we found it */
	if ( condition == OBJECT && found_obj2 == 1 ) {
		dbg("condition verified, object found\n");
		now->sensor_status = 1;
		return 1;
	}

	/* or we were not looking for any object and we did not find any */
	if ( condition == NON_OBJECT && found_obj1 == 0 && found_obj2 == 0 ) {
		dbg("condition verified, object not found\n");
		now->sensor_status = 0; 
		return 1;
	}

	/* finally, if we didn't find any object whatsoever */
	if ( found_obj1 == 0 && found_obj2 == 0 )
		now->sensor_status = 0;

	/* otherwise the condition has not been fulfilled */
	dbg("Condition not verified\n");
	return 0;

}


/** 
 * execute atomic action
 */
static int do_move(struct scenario_t *scenario, enum condition_e condition, struct condition_t *now, int direction) {
	do {
		if ( now->sensor_pos > 1.0 ) {
			now->sensor_pos = 1.0;
			/* end of rail */
			dbg("End of rail\n");
			return -1;
		}
		if ( now->sensor_pos < 0 ) {
			now->sensor_pos = 0.0;
			dbg("End of rail\n");
			return -1;
		}
		/* step movement */
		now->sensor_pos += direction * SENSOR_LATERALSTEP;
	} while ( verify_condition(scenario, condition, now) == 0 );

	return 0;
}

/** 
 * execute atomic action
 */
static int do_rotate(struct scenario_t *scenario, enum condition_e condition, struct condition_t *now, int direction) {
	do { 
		if ( now->sensor_angle > M_PI ) {
			now->sensor_angle = M_PI - SENSOR_ANGULARSTEP;
			dbg("End of platform\n");
			return -1;
		}
		if ( now->sensor_angle < 0 ) {
			dbg("End of platform\n");
			now->sensor_angle = 0.0 + SENSOR_ANGULARSTEP;
			return -1;
		}

		now->sensor_angle += direction * SENSOR_ANGULARSTEP;
	} while ( verify_condition(scenario, condition, now) == 0 );

	return 0;
}

static int do_skip(struct scenario_t *scenario, struct condition_t *now, int direction) {
	if ( now->sensor_pos > 1.0 ) {
		now->sensor_pos = 1.0;
		dbg("End of rail\n");
		return -1;
	}
	if ( now->sensor_pos < 0 ) {
		now->sensor_pos = 0.0;
		dbg("End of rail\n");
		return -1;
	}

	now->sensor_pos += direction * SENSOR_SKIPSTEP;

	/* check if the object is in front of us */
	enum condition_e condition = OBJECT;
	if ( verify_condition(scenario, condition, now) == 1 )
		now->sensor_status = 1;

	return 0;
}
	

/* executes the requested action
 * saves the current condition in 'now'
 *
 * returns -1 if other error (end of rail, etc);
 */
static int execute_action(struct scenario_t *scenario, enum action_e action, enum condition_e condition, struct condition_t *now) {
	int ret = 0;
	dbg("\n>>new action: ");

	/* init the current environment
	 * so that we will update only rotation or position */
	now->sensor_pos = scenario->sensor->pos;
	now->sensor_angle = scenario->sensor->angle;
	/* sensor status is evaluated at every action */

	switch (action) {
		case MOVE_LEFT:
			dbg("moving left\n");
			ret = do_move(scenario, condition, now, -1.0);
			break;
		case MOVE_RIGHT:
			dbg("moving right\n");
			ret = do_move(scenario, condition, now, 1.0);
			break;
		case ROTATE_LEFT:
			dbg("rotating left\n");
			ret = do_rotate(scenario, condition, now, 1.0);
			break;
		case ROTATE_RIGHT:
			dbg("rotating right\n");
			ret = do_rotate(scenario, condition, now, -1.0);
			break;
		case SKIP_LEFT:
			dbg("skip left\n");
			ret = do_skip(scenario, now, -1.0);
			break;
		case SKIP_RIGHT:
			dbg("skip right\n");
			ret = do_skip(scenario, now, 1.0);
			break;
		/* no default */
	}

	return ret;

}


/* core method */
static int run_strategy(char* strategy, struct scenario_t *scenario, struct condition_t *now, int num_actions) {

	enum action_e action;
	enum condition_e condition;
	int move, count;
	
	move = 0; /* beginning of strategy */
	count = 0;

	for ( count = 0 ; count < num_actions ; count++ ) {
		/* keep reading until end of strategy is reached */
		if ( parse_strategy(strategy, &move, &action, &condition) == 0 ) {
			fprintf(stderr, "unexpected end of strategy reached");
			return -1;
		}
		/* returns -1 if end-of-rail, etc;
		 * not checked here, useful in future?
		 */
		execute_action(scenario, action, condition, &now[count]);

		/* update status */
		scenario->sensor->angle = now->sensor_angle;
		scenario->sensor->pos = now->sensor_pos;
		/* sensor status is evaluated for every action */

		count++;
	}

	/* returns the number of actions */
	return count;
}


/* publicly available method */
int run_strategy_mem(char* strategy, struct scenario_t *scenario, fann_type *dest, int dest_len) {
	int num_actions = get_num_actions(strategy);
	int i, j; 
	struct condition_t now[num_actions];

	/* run strategy */
	if ( run_strategy(strategy, scenario, now, num_actions) < 0 )
		/* problem here */
		return -1;

	/* copy into already prepared data structure */
	for ( i = 0, j = 0 ; i < num_actions && j < dest_len ; i++ ) {
		dest[j++] = now[i].sensor_pos;
		dest[j++] = now[i].sensor_angle;
		dest[j++] = now[i].sensor_status;
	}

	return 0;
	
}

#endif
