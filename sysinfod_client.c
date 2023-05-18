#include <xpc/xpc.h>
#include <os/log.h>

#define DAEMON_ID "com.logan.sysinfod"

static os_log_t get_log(void) {
    static dispatch_once_t once;
    static os_log_t log;
    
    // Ensures log is a singleton
    dispatch_once(&once, ^{
        log = os_log_create(DAEMON_ID, "client");
    });

    return log;
}

static xpc_object_t create_message() {
    xpc_object_t message = xpc_dictionary_create_empty();
    xpc_dictionary_set_string(message, "ACK", "ACK-MESSAGE");
    return message;
}

static void process_reply(xpc_object_t reply, xpc_connection_t connection, xpc_object_t message) {
    int retry = 5;
    
    while (xpc_get_type(reply) == XPC_TYPE_ERROR) {
        if (retry == 0) {
            os_log(get_log(), "Too many retries performed, exiting...");
            exit(1);
        }
        
        else if (reply == XPC_ERROR_CONNECTION_INTERRUPTED) {
            // Retry sending message
            reply = xpc_connection_send_message_with_reply_sync(connection, message);
        }
        
        else {
            os_log(get_log(), "Something is permanently wrong here, exiting...");
            exit(1);
        }
        
        retry--;
    }

    uint64_t response = xpc_dictionary_get_uint64(reply, "FIN");
    os_log(get_log(), "Uptime is %llu seconds\n", response);
}

static xpc_connection_t create_daemon_connection() {
    char queue_name[sizeof(DAEMON_ID) + 8];
    sprintf(queue_name, "%s%s", DAEMON_ID, ".client");
    
    char mach_name[sizeof(DAEMON_ID) + 6];
    sprintf(mach_name, "%s%s", DAEMON_ID, ".ping");
    
    dispatch_queue_t queue = dispatch_queue_create(queue_name, NULL);
    xpc_connection_t connection = xpc_connection_create_mach_service(mach_name, queue, 0);
    
    xpc_connection_set_event_handler(connection, ^(xpc_object_t object) {});
    
    xpc_connection_activate(connection);

    return connection;
}

int main(int argc, const char * argv[]) {
    os_log(get_log(), "sysinfod client starting up\n");

    dispatch_async(dispatch_get_main_queue(), ^{
        xpc_object_t message = create_message();
        xpc_connection_t connection = create_daemon_connection();
        
        xpc_object_t reply = xpc_connection_send_message_with_reply_sync(connection, message);
        
        os_log(get_log(), "Sending message: %s", xpc_dictionary_get_string(message, "ACK"));
        
        process_reply(reply, connection, message);

        exit(0);
    });
    
    dispatch_main();
}
