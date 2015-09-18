
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>

    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/file.h>
    #include <unistd.h>

    static int g_single_proc_inst_lock_fd = -1;

    static void single_proc_inst_lockfile_cleanup(void)
    {
        if (g_single_proc_inst_lock_fd != -1) {
            close(g_single_proc_inst_lock_fd);
            g_single_proc_inst_lock_fd = -1;
        }
    }

    bool is_single_proc(const char *process_name)
    {
        char lock_file[128];
        snprintf(lock_file, sizeof(lock_file), "/var/tmp/%s.lock", process_name);

        g_single_proc_inst_lock_fd = open(lock_file, O_CREAT|O_RDWR, 0644);
        if (-1 == g_single_proc_inst_lock_fd) {
            fprintf(stderr, "Fail to open lock file(%s). Error: %s\n",
                lock_file, strerror(errno));
            return false;
        }

        if (0 == flock(g_single_proc_inst_lock_fd, LOCK_EX | LOCK_NB)) {
            atexit(single_proc_inst_lockfile_cleanup);
            return true;
        }

        close(g_single_proc_inst_lock_fd);
        g_single_proc_inst_lock_fd = -1;
        return false;
    }

