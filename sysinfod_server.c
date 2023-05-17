#include <xpc/xpc.h>
#include <sys/sysctl.h>
#include <os/log.h>
#include <pthread.h>

#define DAEMON_ID "com.logan.sysinfod"

static os_log_t getLog(void) {
    static dispatch_once_t once;
    static os_log_t log;
    
    // Ensures log is a singleton
    dispatch_once(&once, ^{
        log = os_log_create(DAEMON_ID, "info");
    });

    return log;
}

static uint64_t uptime() {
    struct timeval boottime;
    size_t len = sizeof(boottime);
    
    // Part of kern MIB, and boottime specifically is what we want
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0) {
        perror("sysctl() failed");
        return 0;
    }
    
    return (uint64_t)difftime(time(NULL), boottime.tv_sec);
}

static void send_reply(xpc_object_t connection, xpc_object_t object ) {
    const char *input = xpc_dictionary_get_string(object, "ACK");
    os_log(getLog(), "Connection received: %s", input);
    
    xpc_object_t reply = xpc_dictionary_create_reply(object);
    xpc_dictionary_set_uint64(reply, "FIN", uptime());
    xpc_connection_send_message(connection, reply);
    free(reply);
}

static void handle_connection(xpc_object_t connection) {
    os_log(getLog(), "sysinfod server woke for XPC with PID: %d, thread ID: %lu", getpid(), (unsigned long)pthread_self());
    
    xpc_connection_set_event_handler(connection, ^(xpc_object_t object) {
        if (xpc_get_type(object) == XPC_TYPE_ERROR) {
            os_log(getLog(), "Goodbye");
        }
        
        else if (xpc_get_type(object) == XPC_TYPE_DICTIONARY) {
            send_reply(connection, object);
        }
        
        else {
            char *description = xpc_copy_description(object);
            os_log(getLog(), "Connection received with erroneous object: %s", description);
            free(description);
        }
    });
    
    xpc_connection_activate(connection);
}

int main(int argc, const char * argv[]) {
    os_log(getLog(), "sysinfod server starting up with PID: %d, thread ID: %lu", getpid(), (unsigned long)pthread_self());
    
    char queue_name[sizeof(DAEMON_ID) + 8 + 1];
    sprintf(queue_name, "%s%s", DAEMON_ID, ".server");
    
    char mach_name[sizeof(DAEMON_ID) + 5 + 1];
    sprintf(mach_name, "%s%s", DAEMON_ID, ".ping");
    
    dispatch_queue_t queue = dispatch_queue_create(queue_name, NULL);
    xpc_connection_t listener = xpc_connection_create_mach_service(mach_name, queue, XPC_CONNECTION_MACH_SERVICE_LISTENER);
    
    xpc_connection_set_event_handler(listener, ^(xpc_object_t object) {
        if (xpc_get_type(object) == XPC_TYPE_CONNECTION) {
            handle_connection(object);
        }
        
        else {
            char *description = xpc_copy_description(object);
            os_log(getLog(), "Connection errored with object: %s", description);
            free(description);
        }
    });
    
    xpc_connection_activate(listener);

    dispatch_main();
}
