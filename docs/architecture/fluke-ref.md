Thread
```
fluke_thread_create
fluke_thread_create_hash
fluke_thread_destroy [cannot destroy itself]
fluke_thread_disable_exceptions [can only disable for itself]
fluke_thread_enable_exceptions [can only enable for itself]
fluke_thread_get_client [preliminary iface][itself]
fluke_thread_get_handlers [itself]
fluke_thread_get_saved_state [itself]
fluke_thread_get_server [preliminary iface][itself]
fluke_thread_get_state [cannot call on itself]
fluke_thread_interrupt [cannot call on itself]
fluke_thread_move [cannot call on itself]
fluke_thread_reference
fluke_thread_return_from_exception [itself]
fluke_thread_self [itself]
fluke_thread_set_client [preliminary iface][itself]
fluke_thread_set_handlers [itself]
fluke_thread_set_saved_state [itself]
fluke_thread_set_server [preliminary iface][itself]
fluke_thread_set_state [cannot call on itself]
fluke_thread_schedule  [cannot call on itself] // donate cpu to another thread
```

Task
```
fluke_task_create
fluke_task_create_hash
fluke_task_destroy
fluke_task_get_state
fluke_task_move
fluke_task_reference
fluke_task_set_state
```

Region
```
fluke_region_create
fluke_region_create_hash
fluke_region_destroy
fluke_region_get_state
fluke_region_move
fluke_region_protect
fluke_region_reference
fluke_region_search [can search in other tasks]
fluke_region_set_state
```

Mapping
```
fluke_mapping_create
fluke_mapping_create_hash
fluke_mapping_destroy
fluke_mapping_get_state
fluke_mapping_move
fluke_mapping_protect
fluke_mapping_reference
fluke_mapping_set_state
```

Port // Fluke IPC endpoints
```
fluke_port_create
fluke_port_create_hash
fluke_port_destroy
fluke_port_get_state
fluke_port_move
fluke_port_reference
fluke_port_set_state
```

Port set
```
fluke_pset_create
fluke_pset_create_hash
fluke_pset_destroy
fluke_pset_get_state
fluke_pset_move
fluke_pset_reference
fluke_pset_set_state
```

IPC // C interface, not directly mapped to kernel API
```
fluke_ipc_call // syncronous idempotent call
fluke_ipc_client_connect_send // create a reliable connection to a server
fluke_ipc_client_connect_send_over_receive // perform a reliable IPC to a server (client_{connect_send+over_receive})
fluke_ipc_reply // reply to an idempotent call
fluke_ipc_reply_wait_receive // reply to an idempotent call and wait for a new request (ipc_{reply+wait_receive})
fluke_ipc_send // send a one-way message to a port
fluke_ipc_server_ack_send_wait_receive // reply to a reliable RPC and wait for another (ipc_{server_ack_send+wait_receive})
fluke_ipc_server_send_wait_receive // send data to a reliable RPC connection, disconnect and wait for a new invocation (ipc_{server_send+wait_receive})
fluke_ipc_setup_wait_receive // set up a server thread and wait for incoming IPC invocations
fluke_ipc_client_ack_send // become the sender on a reliable IPC connection
fluke_ipc_server_ack_send
fluke_ipc_client_ack_send_over_receive // reverse a reliable IPC connection, send a message and reverse again
fluke_ipc_server_ack_send_over_receive
fluke_ipc_client_alert // send an interrupt on a reliable IPC connection
fluke_ipc_server_alert
fluke_ipc_client_disconnect // destroy a reliable IPC connection
fluke_ipc_server_disconnect
fluke_ipc_client_over_receive // reverse the transfer direction of a reliable IPC connection
fluke_ipc_server_over_receive
fluke_ipc_client_receive // receive data through reliable IPC
fluke_ipc_server_receive
fluke_ipc_client_send // send data across a reliable IPC connection
fluke_ipc_server_send
fluke_ipc_client_send_over_receive // send a message on a reliable IPC connection and reverse the connection
fluke_ipc_server_send_over_receive
fluke_ipc_wait_receive // wait on a port set for incoming IPC invocations
```

Mutex
```
fluke_mutex_create
fluke_mutex_create_hash
fluke_mutex_destroy
fluke_mutex_get_state
fluke_mutex_lock
fluke_mutex_move
fluke_mutex_reference
fluke_mutex_set_state
fluke_mutex_trylock
fluke_mutex_unlock
```

Condition
```
fluke_cond_broadcast
fluke_cond_create
fluke_cond_create_hash
fluke_cond_destroy
fluke_cond_get_state
fluke_cond_move
fluke_cond_reference
fluke_cond_set_state
fluke_cond_signal
fluke_cond_wait
```

Reference
```
fluke_ref_check
fluke_ref_compare
fluke_ref_copy
fluke_ref_create
fluke_ref_destroy
fluke_ref_hash
fluke_ref_move
fluke_ref_type
```





