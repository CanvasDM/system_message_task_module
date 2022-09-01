/**
 * @file system_message_task.c
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(smt, CONFIG_SMT_LOG_LEVEL);
#define THIS_FILE "smt"

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <init.h>

#include "fwk_includes.h"
#include "system_message_task.h"

/**************************************************************************************************/
/* Local Const Definitions                                                                        */
/**************************************************************************************************/
static const FwkMsgCode_t MSG_CODES[] = { FMC_ATTR_CHANGED };

/**************************************************************************************************/
/* Global Data Definitions                                                                        */
/**************************************************************************************************/
/* Use the attribute mutex to prevent priority inheritance issues that
 * occur when using two mutexes.  A user of the attribute changed callback may
 * request the mutex before the thread changing the value has released it.
 */
extern struct k_mutex attr_mutex;

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct {
	FwkMsgTask_t msg_task;
	bool initialized;
	sys_slist_t msg_handler_list;
	sys_slist_t attr_changed_list;
	struct smt_agent agent0;
} smt;

static K_THREAD_STACK_DEFINE(smt_stack, CONFIG_SMT_STACK_SIZE);

K_MSGQ_DEFINE(smt_queue, FWK_QUEUE_ENTRY_SIZE, CONFIG_SMT_QUEUE_SIZE, FWK_QUEUE_ALIGNMENT);

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static void smt_thread(void *arg1, void *arg2, void *arg3);
static void smt_attr_changed_callback(const FwkMsg_t *msg, void *context);

static DispatchResult_t smt_msg_handler(FwkMsgReceiver_t *msg_rxer, FwkMsg_t *msg);

/**************************************************************************************************/
/* Framework Message Dispatcher                                                                   */
/**************************************************************************************************/
static FwkMsgHandler_t *smt_msg_dispatcher(FwkMsgCode_t msg_code)
{
	sys_snode_t *node;
	struct smt_agent *agent;
	FwkMsgHandler_t *handler = NULL;
	int i;

	if (!smt.initialized) {
		return handler;
	}

	if (msg_code == FMC_INVALID) {
		return Framework_UnknownMsgHandler;
	}

	k_mutex_lock(&attr_mutex, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE (&smt.msg_handler_list, node) {
		agent = CONTAINER_OF(node, struct smt_agent, node);
		/* If the message code is found, then set handler to non-null.
		 * From this perspective, there is one handler for all messages.
		 * (Multiple modules may have registered for the same message.)
		 */
		for (i = 0; i < agent->msg_code_count; i++) {
			if (msg_code == agent->msg_codes[i]) {
				handler = smt_msg_handler;
				break;
			}
		}
		if (handler != NULL) {
			break;
		}
	}
	k_mutex_unlock(&attr_mutex);

	return handler;
}

/**************************************************************************************************/
/* SYS INIT                                                                                       */
/**************************************************************************************************/
static int smt_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int status;

	sys_slist_init(&smt.msg_handler_list);
	sys_slist_init(&smt.attr_changed_list);

	smt.msg_task.rxer.id = FWK_ID_SYSTEM_MESSAGE_TASK;
	smt.msg_task.rxer.rxBlockTicks = K_FOREVER;
	smt.msg_task.rxer.pMsgDispatcher = smt_msg_dispatcher;
	smt.msg_task.timerDurationTicks = K_SECONDS(0);
	smt.msg_task.timerPeriodTicks = K_SECONDS(0);
	smt.msg_task.rxer.pQueue = &smt_queue;

	Framework_RegisterTask(&smt.msg_task);

	smt.msg_task.pTid = k_thread_create(&smt.msg_task.threadData, smt_stack,
					    K_THREAD_STACK_SIZEOF(smt_stack), smt_thread, &smt,
					    NULL, NULL, CONFIG_SMT_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(smt.msg_task.pTid, THIS_FILE);

	smt.initialized = true;

	/* Add message agent for the attribute changed message.
	 * This is more efficient than having each module register for the attr changed msg.
	 */
	smt.agent0.msg_codes = MSG_CODES;
	smt.agent0.msg_code_count = ARRAY_SIZE(MSG_CODES);
	smt.agent0.context = NULL;
	smt.agent0.callback = smt_attr_changed_callback;
	status = smt_register_message_agent(&smt.agent0);
	LOG_DBG("Registered attr changed agent %d", status);

	return 0;
}

SYS_INIT(smt_init, POST_KERNEL, CONFIG_SMT_SYS_INIT_PRIORITY);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int smt_register_message_agent(struct smt_agent *agent)
{
	if (!smt.initialized) {
		return -EPERM;
	}

	if (agent == NULL || agent->callback == NULL || agent->msg_codes == NULL ||
	    agent->msg_code_count == 0) {
		LOG_ERR("Attempt to register invalid system message agent");
		return -EINVAL;
	}

	k_mutex_lock(&attr_mutex, K_FOREVER);
	sys_slist_append(&smt.msg_handler_list, &agent->node);
	k_mutex_unlock(&attr_mutex);

	return 0;
}

int smt_register_attr_changed_agent(struct smt_attr_changed_agent *agent)
{
	if (!smt.initialized) {
		return -EPERM;
	}

	if (agent == NULL || agent->callback == NULL) {
		LOG_ERR("Attempt to register invalid attribute changed agent");
		return -EINVAL;
	}

	k_mutex_lock(&attr_mutex, K_FOREVER);
	sys_slist_append(&smt.attr_changed_list, &agent->node);
	k_mutex_unlock(&attr_mutex);

	return 0;
}

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static void smt_thread(void *arg1, void *arg2, void *arg3)
{
	while (true) {
		Framework_MsgReceiver(&smt.msg_task.rxer);
	}
}

static DispatchResult_t smt_msg_handler(FwkMsgReceiver_t *msg_rxer, FwkMsg_t *msg)
{
	ARG_UNUSED(msg_rxer);
	sys_snode_t *node;
	struct smt_agent *agent;
	int i;

	k_mutex_lock(&attr_mutex, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE (&smt.msg_handler_list, node) {
		agent = CONTAINER_OF(node, struct smt_agent, node);
		for (i = 0; i < agent->msg_code_count; i++) {
			if (agent->msg_codes[i] == msg->header.msgCode) {
				if (agent->callback != NULL) {
					agent->callback(msg, agent->context);
					break;
				}
			}
		}
		/* Don't break from slist loop; there can be multiple handlers for each message type. */
	}
	k_mutex_unlock(&attr_mutex);

	return DISPATCH_OK;
}

static void smt_attr_changed_callback(const FwkMsg_t *msg, void *context)
{
	ARG_UNUSED(context);
	attr_changed_msg_t *acm = (attr_changed_msg_t *)msg;
	sys_snode_t *node;
	struct smt_attr_changed_agent *agent;

	k_mutex_lock(&attr_mutex, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE (&smt.attr_changed_list, node) {
		agent = CONTAINER_OF(node, struct smt_attr_changed_agent, node);
		if (agent->callback != NULL) {
			agent->callback(acm->list, acm->count, agent->context);
		}
	}
	k_mutex_unlock(&attr_mutex);
}
