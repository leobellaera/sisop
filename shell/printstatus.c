#include "printstatus.h"

// prints information of process' status
void print_status_info(struct cmd* cmd) {

	if (strlen(cmd->scmd) == 0
		|| cmd->type == PIPE)
		return;
	
	if (WIFEXITED(status)) {

		fprintf(stdout, "%s	Program: [%s] exited, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, WEXITSTATUS(status), COLOR_RESET);
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {

		fprintf(stdout, "%s	Program: [%s] killed, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, -WTERMSIG(status), COLOR_RESET);
		status = -WTERMSIG(status);

	} else if (WTERMSIG(status)) {

		fprintf(stdout, "%s	Program: [%s] stopped, status: %d %s\n",
			COLOR_BLUE, cmd->scmd, -WSTOPSIG(status), COLOR_RESET);
		status = -WSTOPSIG(status);
	}
}

// prints info when a background process is spawned
void print_back_info(struct cmd* back) {

	fprintf(stdout, "%s  [PID=%d] %s\n",
		COLOR_BLUE, back->pid, COLOR_RESET);
}

