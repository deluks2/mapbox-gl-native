#include "util.hpp"

#include <mbgl/platform/log.hpp>

#include <csignal>

#include <unistd.h>

namespace mbgl {
namespace test {

pid_t startServer(const char *executable) {
    const std::string parent_pid = std::to_string(getpid());
    int pipefd[2];

    if (pipe(pipefd)) {
        throw std::runtime_error("Cannot create server pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Cannot create server process");
    } else if (pid == 0) {
        close(STDIN_FILENO);

        if (dup(pipefd[1])) {
            Log::Error(Event::Setup, "Failed to start server: %s", strerror(errno));
        }

        close(pipefd[0]);
        close(pipefd[1]);

        char *args[] = { const_cast<char *>(executable),
                         const_cast<char *>(parent_pid.c_str()),
                         nullptr };
        int ret = execv(executable, args);
        // This call should not return. In case execve failed, we exit anyway.
        if (ret < 0) {
            Log::Error(Event::Setup, "Failed to start server: %s", strerror(errno));
        }
        exit(0);
    } else {
        char buffer[8];

        // Wait until the server process sends something on the pipe.
        if (!read(pipefd[0], buffer, sizeof(buffer))) {
            throw std::runtime_error("Error reading a message from the server");
        }

        close(pipefd[0]);
        close(pipefd[1]);
    }
    return pid;
}

void stopServer(pid_t pid) {
    kill(pid, SIGTERM);
}

}
}
