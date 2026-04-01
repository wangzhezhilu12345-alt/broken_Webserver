#include "../server/server.h"
#include <fcntl.h>
#include <memory>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
    bool ShouldRunAsDaemon(int argc, char *argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg(argv[i]);
            if (arg == "-d" || arg == "--daemon")
            {
                return true;
            }
        }
        return false;
    }

    bool Daemonize()
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            return false;
        }
        if (pid > 0)
        {
            _exit(0);
        }

        if (setsid() < 0)
        {
            return false;
        }

        pid = fork();
        if (pid < 0)
        {
            return false;
        }
        if (pid > 0)
        {
            _exit(0);
        }

        umask(0);

        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0)
        {
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO)
            {
                close(null_fd);
            }
        }

        return true;
    }
}

int main(int argc, char *argv[])
{
    if (ShouldRunAsDaemon(argc, argv) && !Daemonize())
    {
        return 1;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    std::shared_ptr<serverapp::Server> svr(new serverapp::Server(8080));
    svr->Init();
    svr->Run();
    return 0;
}
