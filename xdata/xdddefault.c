/*****************************************************************************
 *
 * XDDDEFAULT.C - Default scheduled downtime data routines for Nagios
 *
 *
 * License:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *****************************************************************************/


/*********** COMMON HEADER FILES ***********/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/locations.h"
#include "../include/downtime.h"
#include "../include/macros.h"
#include "../include/objects.h"
#include "../include/nagios.h"
#include "xdddefault.h"


/******************************************************************/
/*********** DOWNTIME INITIALIZATION/CLEANUP FUNCTIONS ************/
/******************************************************************/


/* initialize downtime data */
int xdddefault_initialize_downtime_data(void) {
	scheduled_downtime *temp_downtime = NULL;

	/* clean up the old downtime data */
	xdddefault_validate_downtime_data();

	/* find the new starting index for downtime id if its missing*/
	if(next_downtime_id == 0L) {
		for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
			if(temp_downtime->downtime_id >= next_downtime_id)
				next_downtime_id = temp_downtime->downtime_id + 1;
			}
		}

	/* initialize next downtime id if necessary */
	if(next_downtime_id == 0L)
		next_downtime_id = 1;

	return OK;
	}



/* removes invalid and old downtime entries from the downtime file */
int xdddefault_validate_downtime_data(void) {
	scheduled_downtime *temp_downtime;
	scheduled_downtime *next_downtime;
	int save = TRUE;

	/* remove stale downtimes */
	for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = next_downtime) {

		next_downtime = temp_downtime->next;
		save = TRUE;

		/* delete downtimes with invalid host names */
		if(find_host(temp_downtime->host_name) == NULL) {
			log_debug_info(DEBUGL_DOWNTIME, 1,
					"Deleting downtime with invalid host name: %s\n",
					temp_downtime->host_name);
			save = FALSE;
			}

		/* delete downtimes with invalid service descriptions */
		if(temp_downtime->type == SERVICE_DOWNTIME && find_service(temp_downtime->host_name, temp_downtime->service_description) == NULL) {
			log_debug_info(DEBUGL_DOWNTIME, 1,
					"Deleting downtime with invalid service description: %s\n",
					temp_downtime->service_description);
			save = FALSE;
			}

		/* delete fixed downtimes that have expired */
		if((TRUE == temp_downtime->fixed) &&
				(temp_downtime->end_time < time(NULL))) {
			log_debug_info(DEBUGL_DOWNTIME, 1,
					"Deleting fixed downtime that expired at: %lu\n",
					temp_downtime->end_time);
			save = FALSE;
			}

		/* delete flexible downtimes that never started and have expired */
		if((FALSE == temp_downtime->fixed) &&
				(0 == temp_downtime->flex_downtime_start) &&
				(temp_downtime->end_time < time(NULL))) {
			log_debug_info(DEBUGL_DOWNTIME, 1,
					"Deleting flexible downtime that expired at: %lu\n",
					temp_downtime->end_time);
			save = FALSE;
			}

		/* delete flexible downtimes that started but whose duration
			has completed */
		if((FALSE == temp_downtime->fixed) &&
				(0 != temp_downtime->flex_downtime_start) &&
				((temp_downtime->flex_downtime_start + temp_downtime->duration)
				< time(NULL))) {
			log_debug_info(DEBUGL_DOWNTIME, 1,
					"Deleting flexible downtime whose duration ended at: %lu\n",
					temp_downtime->flex_downtime_start + temp_downtime->duration);
			save = FALSE;
			}

		/* delete the downtime */
		if(save == FALSE) {
			delete_downtime(temp_downtime->type, temp_downtime->downtime_id);
			}
		}

	/* remove triggered downtimes without valid parents */
	for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = next_downtime) {

		next_downtime = temp_downtime->next;
		save = TRUE;

		if(temp_downtime->triggered_by == 0)
			continue;

		if(find_host_downtime(temp_downtime->triggered_by) == NULL && find_service_downtime(temp_downtime->triggered_by) == NULL)
			save = FALSE;

		/* delete the downtime */
		if(save == FALSE) {
			delete_downtime(temp_downtime->type, temp_downtime->downtime_id);
			}
		}

	return OK;
	}
