/**
 * @file system_message_task.h
 * @brief The system message task allows modules to receive callbacks when
 * a framework message occurs.  The attribute changed callback allows the
 * attributes to be used as an interface between modules.
 *
 * Copyright (c) 2022 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

#ifndef __SYSTEM_MESSAGE_TASK_H__
#define __SYSTEM_MESSAGE_TASK_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr.h>
#include <sys/slist.h>

#include "Framework.h"
#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
struct smt_agent {
	sys_snode_t node;
	const FwkMsgCode_t * msg_codes;
	size_t msg_code_count;
	void *context;
	void (*callback)(const FwkMsg_t *msg, void *context);
};

struct smt_attr_changed_agent {
	sys_snode_t node;
	void *context;
	void (*callback)(const attr_id_t *id_list, size_t list_count, void *context);
};

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/

/**
 * @brief Register a callback for a framework message that will occur
 * in system message task context.
 *
 * @param agent linked list element
 * @return int 0 on success, negative error code otherwise
 */
int smt_register_message_agent(struct smt_agent *agent);

/**
 * @brief Register a callback for the ATTR_CHANGED message
 * in system message task context.
 *
 * @param agent linked list element for an attribute changed user
 * @return int 0 on success, negative error code otherwise
 */
int smt_register_attr_changed_agent(struct smt_attr_changed_agent *agent);


#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_MESSAGE_TASK_H__ */
