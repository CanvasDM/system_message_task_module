# System Message Task

The system message task allows multiple modules to receive messages without creating a task for each module.

The system message task allows modules to receive callbacks when a framework message occurs.  The attribute changed callback allows the attributes to be used as an interface between modules. Analogous to the system work queue, modules should not block for long periods in the callback functions.
