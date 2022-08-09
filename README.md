# System Message Task

The system message task allows multiple modules to receive messages without creating a task for each module.

The system message task allows modules to receive callbacks when a framework message occurs.  The attribute changed callback allows the attributes to be used as an interface between modules. Analogous to the system work queue, modules should not block for long periods in the callback functions.

## Example

Add a linked list node to the system message task.  The node (agent) contains a list of message codes, a callback, and an optional context.

```
static const FwkMsgCode_t MSG_CODES[] =
    { FMC_FACTORY_RESET, FMC_APP_SPECIFIC, FMC_MODULE_SPECIFIC };

struct smt_agent agent;

agent.msg_codes = MSG_CODES;
agent.msg_code_count = ARRAY_SIZE(MSG_CODES);
agent.callback = smt_attr_changed_callback;
agent.context = &my_object; /* or NULL */
smt_register_message_agent(&agent);
```
