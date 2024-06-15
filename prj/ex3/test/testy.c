#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

	if (argc != 3) {
		fprintf(stderr, "USAGE: %s <expected output> <file_to_execute>\n", argv[0]);
		return(1);
	}

	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("pipe() failed.");
		return(2);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork() failed.");
		return(3);
	}

	char *expected_output = argv[1];
	char *file_to_exe = argv[2];

	int status;

	if (0 == pid) {
		//
		// Child process
		//    Here we exec the program to run
		//    and return the result of it to the pipe.

		// close unused read end
		close(pipe_fd[0]);

		// redirect stdout to pipe
		dup2(pipe_fd[1], STDOUT_FILENO);

		// redirect stderr to pipe
		dup2(pipe_fd[1], STDERR_FILENO);

		// close original write end
		close(pipe_fd[1]);



		execv(file_to_exe, NULL);

		// if the execv fails this code will run, 
		// otherwise it never runs because its
		// replaced by the exec program.
		perror("execv() failed.");

		exit(4);

	} else {

		// parent process
		//    Here we read the results of the exec'd program
		//    and print/check the results.

		// close unused write end
		close(pipe_fd[1]);

		char buffer[1024];
		ssize_t count;

		while ((count = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
			// null terminate the buffer
			buffer[count] = 0;
			// and remove trailing newline 
			buffer[count-1] = 0;
			// print the captured output in the buffer
			//printf("%s", buffer);

			if (0 == strcmp(buffer, expected_output) ) {
				printf("program under test ran ok.\n");
				status = 0;

			} else {
				printf("program under test did not produce execpted output.\n");
				printf("Expected -->%s<--\n", expected_output);
				printf("Got      -->%s<--\n", buffer);
				status = 5;
			}
		}

		// close read end
		close(pipe_fd[0]);

		int status;
		// wait for child to finish
		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			//printf("Child exited with status %d\n", WEXITSTATUS(status));
			;
		} else {
			printf("Child did not exit ok\n");
		}
	}

	return(status);
}