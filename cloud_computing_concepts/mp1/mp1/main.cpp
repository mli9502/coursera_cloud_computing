#include "Application.h"

/**********************************
 * FUNCTION NAME: main
 *
 * DESCRIPTION: main function. Start from here
 **********************************/

// FIXME: Need to remove this definiation.
// This is only used for testing.
// #define TEST

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
#ifdef TEST
	// vector<Address> addrVec;
	// string port = "8080";
	// int numNodes = 10;
	// for(int i = 0; i < numNodes; i ++) {
	// 	addrVec.push_back(Address(to_string(i) + ":" + port));
	// }
	// for(auto& addr : addrVec) {
	// 	cout << addr.getAddress() << endl;
	// }
	// MembershipList ml;
	// for(auto& addr : addrVec) {
	// 	ml.insertEntry(MembershipListEntry(addr));
	// }
	// ml.printVec();

	// int numTargetsToPing = ml.getSize() + 1;
	// cout << "----- Select " << numTargetsToPing << " targets for ping -----" << endl;
	// for(int i = 0; i < numTargetsToPing; i ++) {
	// 	auto target = MembershipListEntry(Address());
	// 	ml.getPingTarget(target, Address());
	// 	cout << "select: " << target << " for ping..." << endl;
	// }
	// cout << "----- Membership List after selecting " << numTargetsToPing << endl;
	// ml.printVec();
	// cout << "----- Remove 1:8080 from list -----" << endl;
	// ml.removeEntry("1:8080");
	// ml.printVec();
	// cout << "----- Get top 5 with 2 as max piggyback cnt -----" << endl;
	// auto topK = ml.getTopK(5, 2);
	// for(auto& entry : topK) {
	// 	cout << entry << endl;
	// }
	// cout << "----- Membership list after getTopK -----" << endl;
	// ml.printVec();
	
	// cout << "----- Get top 3 with 2 as max incarnation cnt -----" << endl;
	// topK = ml.getTopK(3, 2);
	// for(const auto& entry : topK) {
	// 	cout << entry << endl;
	// }
	// cout << "----- Membership list after getTopK -----" << endl;
	// ml.printVec();
	// cout << "----- Test getEntryMsg and decodeEntrymsg -----" << endl;
	// cout << MembershipListEntry::decodeEntryMsg(topK[0].getEntryMsg()) << endl;

	// cout << "----- Test genTopKMsg and decodeTopKMsg -----" << endl;
	// auto msgPair = ml.genTopKMsg(3, 2);
	// cout << "msg size: " << msgPair.first << endl;
	// auto decodedEntries = MembershipList::decodeTopKMsg(msgPair.second);
	// for(auto& decodedEntry : decodedEntries) {
	// 	cout << decodedEntry << endl;
	// }
#endif

#ifndef TEST
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
#endif
}