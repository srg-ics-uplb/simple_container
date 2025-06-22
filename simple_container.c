#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define STACK_SIZE (1024 * 1024)  // 1MB stack for child process
#define MAX_HOSTNAME 256

struct container_config {
    char *hostname;
    char *rootfs_path;
    char *command;
    char **args;
};

// Function to set up the container environment
int setup_container(struct container_config *config) {
    // Create a new mount namespace and make it private
    if (mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) < 0) {
        perror("Failed to make root mount private");
        return -1;
    }
    
    // Change root directory to the specified rootfs
    if (chroot(config->rootfs_path) < 0) {
        perror("Failed to chroot");
        return -1;
    }
    
    // Change working directory to root
    if (chdir("/") < 0) {
        perror("Failed to change directory to /");
        return -1;
    }
    
    // Set hostname if specified
    if (config->hostname && sethostname(config->hostname, strlen(config->hostname)) < 0) {
        perror("Failed to set hostname");
        return -1;
    }
    
    // Mount proc filesystem
    if (mount("proc", "/proc", "proc", 0, NULL) < 0) {
        perror("Failed to mount /proc");
        return -1;
    }
    
    // Mount sysfs filesystem
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) < 0) {
        perror("Failed to mount /sys");
        return -1;
    }
    
    // Mount tmpfs for /tmp
    if (mount("tmpfs", "/tmp", "tmpfs", 0, "size=100m") < 0) {
        perror("Failed to mount /tmp");
        return -1;
    }
    
    return 0;
}

// Child process function
int child_process(void *arg) {
    struct container_config *config = (struct container_config *)arg;
    
    printf("Container started with PID %d\n", getpid());
    
    // Set up the container environment
    if (setup_container(config) < 0) {
        fprintf(stderr, "Failed to set up container\n");
        exit(EXIT_FAILURE);
    }
    
    // Execute the specified command
    if (execvp(config->command, config->args) < 0) {
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <rootfs_path> <command> [args...]\n", program_name);
    printf("Options:\n");
    printf("  -h, --hostname <hostname>  Set container hostname\n");
    printf("  --help                     Show this help message\n");
    printf("\nExample:\n");
    printf("  %s /path/to/rootfs /bin/bash\n", program_name);
    printf("  %s -h mycontainer /path/to/rootfs /bin/sh -c 'echo Hello from container'\n", program_name);
}

int create_rootfs_directory(const char *path) {
    struct stat st;
    
    // Check if directory exists
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0; // Directory already exists
        } else {
            fprintf(stderr, "Error: %s exists but is not a directory\n", path);
            return -1;
        }
    }
    
    // Try to create directory
    if (mkdir(path, 0755) < 0) {
        perror("Failed to create rootfs directory");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    struct container_config config = {0};
    int arg_index = 1;
    
    // Parse command line arguments
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--hostname") == 0) {
            if (arg_index + 1 >= argc) {
                fprintf(stderr, "Error: -h/--hostname requires a hostname argument\n");
                return EXIT_FAILURE;
            }
            config.hostname = argv[++arg_index];
        } else if (strcmp(argv[arg_index], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            break;
        }
        arg_index++;
    }
    
    // Remaining arguments: rootfs_path command [args...]
    if (arg_index >= argc) {
        fprintf(stderr, "Error: Missing rootfs path\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    config.rootfs_path = argv[arg_index++];
    
    if (arg_index >= argc) {
        fprintf(stderr, "Error: Missing command to execute\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    config.command = argv[arg_index];
    config.args = &argv[arg_index];
    
    // Check if running as root
    if (getuid() != 0) {
        fprintf(stderr, "Error: This program must be run as root\n");
        return EXIT_FAILURE;
    }
    
    // Verify rootfs path exists
    struct stat st;
    if (stat(config.rootfs_path, &st) < 0) {
        perror("Failed to access rootfs path");
        return EXIT_FAILURE;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory\n", config.rootfs_path);
        return EXIT_FAILURE;
    }
    
    printf("Starting container...\n");
    printf("Rootfs: %s\n", config.rootfs_path);
    printf("Command: %s\n", config.command);
    if (config.hostname) {
        printf("Hostname: %s\n", config.hostname);
    }
    
    // Allocate stack for child process
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("Failed to allocate stack");
        return EXIT_FAILURE;
    }
    
    // Create child process with new namespaces
    int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | SIGCHLD;
    pid_t child_pid = clone(child_process, stack + STACK_SIZE, flags, &config);
    
    if (child_pid < 0) {
        perror("Failed to clone process");
        free(stack);
        return EXIT_FAILURE;
    }
    
    printf("Container process started with PID %d\n", child_pid);
    
    // Wait for child process to complete
    int status;
    if (waitpid(child_pid, &status, 0) < 0) {
        perror("Failed to wait for child process");
        free(stack);
        return EXIT_FAILURE;
    }
    
    printf("Container process exited with status %d\n", WEXITSTATUS(status));
    
    free(stack);
    return EXIT_SUCCESS;
}
