#define UNW_LOCAL_ONLY
#include "tests/test.h"
#include <dirent.h>
#include <dlfcn.h>
#include <libunwind.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSG_START   "S"
#define MSG_FINISH  "F"
#define MSG_ERROR   "E"

// SIGSEGV handler
void signal_handler(int signal, siginfo_t *info, void *unused);
void usage(void);

//Global variables
FILE *out;

//Data for each test
struct TestData {
    struct Test test;
    void *handler;
    struct TestData *next;
};

struct TestData *g_test_list = NULL;
struct TestData *g_current_test = NULL;
sigjmp_buf g_jmp_buffer; 
int g_in_test = 0;
int g_socket = 0;
struct sockaddr_in g_client;

int main(int argc, char *argv[])
{
    char path[PATH_MAX];

    // Input data
    if (argc > 3) {
        usage();
        return 1;
    }

    out = stdout;
    if (argc >= 2) {
        out = fopen(argv[1],"a");
        if (!out) {
            fprintf(out, "[INTERNAL] Cannot open output file: %s\n",
                    strerror(errno));
            return 1;
        }
    } 

    unsigned int port = 0;
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
        
    // Register SIGSEGV handler
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        fprintf(out, "[INTERNAL] Cannot attach signal handler\n");
        return 1;
    }


    // Load tests
    struct dirent *entry;
    DIR* dir = opendir("tests"); 
    if (!dir) {
        fprintf(out, "[INTERNAL] Cannot open test directory\n");
        return 1;
    }

    while((entry = readdir(dir))) {
        // Look only for .tst files
        size_t len = strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + (len - 4), ".tst") == 0) {
            fprintf(out, "[INTERNAL] Loading test: %s... ", entry->d_name);
            sprintf(path, "tests/%s", entry->d_name);
            void *handle = dlopen(path, RTLD_NOW);
            if (handle) {
                void *test_func_ptr = dlsym(handle, "test_func");
                if (test_func_ptr) {
                    void *check_func_ptr = dlsym(handle, "check_func");
                    struct TestData *old = g_current_test;
                    g_current_test = (struct TestData*)
                                      malloc(sizeof(struct TestData));
                    memset(g_current_test, 0, sizeof(struct TestData));
                    g_current_test->handler = handle;
                    g_current_test->test.file = out;
                    g_current_test->test.name = entry->d_name;
                    g_current_test->test.max_retries = 1;
                    g_current_test->test.test_func = 
                        (TestFunctionPtr)test_func_ptr;
                    g_current_test->test.check_func =
                        (CheckFunctionPtr)check_func_ptr;
                    if (!old) g_test_list = g_current_test;
                    else old->next = g_current_test;
                    fprintf(out,"OK\n");
                } else {
                    fprintf(out, "failed: 'test_func' not defined\n");
                    dlclose(handle);
                }
            } else {
                fprintf(out, "failed: %s\n", dlerror());
            }
        }
    }

    // Setup socket
    g_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket == -1) {
        fprintf(out, "[INTERNAL] Cannot create a datagram socket: %s\n",
                strerror(errno));
        return 1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(g_socket, (const struct sockaddr*)(&addr), sizeof(addr)) == -1) {
        fprintf(out, "[INTERNAL] Cannot bound to port: %s\n", strerror(errno));
        return 1;
    }
    socklen_t len = sizeof(addr);
    if (getsockname(g_socket, (struct sockaddr*)(&addr), &len) == -1) {
        fprintf(out, "[INTERNAL] Cannot get socket name: %s\n", 
                strerror(errno));
        return 1;
    }
    fprintf(out, "[INTERNAL] Created socket: %s:%d\n",
                 inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    ssize_t size = 0;
    char buffer[256] = {0};
    len = sizeof(g_client);

    // Wait for a command
    size = recvfrom(g_socket, buffer, 256, 0, 
                    (struct sockaddr*) &g_client, &len);
    buffer[255] = '\0'; 
    fprintf(out, "[INTERNAL] Msg: %dB, Injection: %s, Retries: %d\n", 
            buffer[0], buffer+2, buffer[1]);

    // Send OK response
    size = sendto(g_socket, MSG_START, 1, 0, 
                  (const struct sockaddr*)(&g_client), len);
    if (size != 1) {
        fprintf(out, "[INTERNAL] Cannot send OK msg: %s\n", strerror(errno));
        return 1;
    }

    // Update retries count
    struct TestData *test = g_test_list;
    while(test) {
        test->test.max_retries = buffer[1];
        test = test->next;
    }

    // Inject fault(s) 
    FILE *file = fopen("/proc/kernelinjector","a"); 
    if (!file) {
        fprintf(out, "[INTERNAL] Kinjector module isn't probably loaded: %s\n",
                strerror(errno));
        return 1;
    }
    fprintf(file, "%s", buffer+2);
    fflush(file);

    // Do tests
    test = g_test_list;
    while(test) {
        g_current_test = test;
        if (sigsetjmp(g_jmp_buffer, 1) == 0) {
            g_in_test = 1;
            test_start(&test->test);
        }
        g_in_test = 0;
        test = test->next;
    }

    // Close tests nicely
    test = g_test_list;
    while(test) {
        struct TestData *next = test->next; 
        fprintf(out, "[INTERNAL] Closing test: %s... ", test->test.name);
        if (dlclose(test->handler) == 0) fprintf(out, "OK\n");
        else fprintf(out, "failed: %s\n", dlerror());
        free(test);
        test = next;
    }

    // Send FINISH response
    size = sendto(g_socket, MSG_FINISH, 1, 0, 
                  (const struct sockaddr*)(&g_client), len);
    if (size != 1) {
        fprintf(out, "[INTERNAL] Cannot send OK msg: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

void signal_handler(int signal, siginfo_t *info, void *ucontext)
{
    (void)(ucontext);
    const char *header = "INTERNAL";
    if (g_current_test && g_in_test) header = g_current_test->test.name;

    fprintf(out, "[%s] %s handler invoked:\n", header, strsignal(signal));
    fprintf(out, "[%s] Got signal: %s\n", header, strsignal(info->si_signo));
    fprintf(out, "[%s] Last errno: %s\n", header, strerror(info->si_errno));
    fprintf(out, "[%s] At address: <%016lx>\n", header, 
                                               (unsigned long)info->si_addr);

    unw_context_t context;
    unw_cursor_t cursor;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    char proc_name[256] = {0};
    unw_word_t ip, off;

    while(unw_step(&cursor) > 0) {
        unw_get_proc_name(&cursor, proc_name, 256, &off);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (ip > 0)
            fprintf(out, "[%s]  <0x%016lx> %20s + 0x%04lx\n",
                    header, ip, proc_name, off);
    }

    if (g_current_test && g_in_test) test_error(&g_current_test->test, 1);

    // Send ERROR msg
    sendto(g_socket, MSG_FINISH, 1, 0, (const struct sockaddr*)(&g_client), 
           sizeof(g_client));

    exit(1);
}

void usage(void)
{
    printf("Usage:\n");
    printf("fstest [output_file] [port_nr]\n");
}
