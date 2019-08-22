#include "Application.h"

/**********************************
 * FUNCTION NAME: main
 *
 * DESCRIPTION: main function. Start from here
 **********************************/

void handler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

int main(int argc, char *argv[]) {
	//signal(SIGSEGV, handler);
	if ( argc != ARGS_COUNT ) {
		cout<<"Configuration (i.e., *.conf) file File Required"<<endl;
		return FAILURE;
	}

	// Create a new application object
	Application *app = new Application(argv[1]);
	// Call the run function
	app->run();
	// When done delete the application object
	delete(app);

	return SUCCESS;
}

