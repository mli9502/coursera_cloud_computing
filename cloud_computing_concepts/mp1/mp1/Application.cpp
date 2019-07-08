/**********************************
 * FILE NAME: Application.cpp
 *
 * DESCRIPTION: Application layer class function definitions
 **********************************/

#include "Application.h"

int nodeCount = 0;

/**
 * Constructor of the Application class
 */
Application::Application(char *infile) {
	int i;
	par = new Params();
	srand (time(NULL));
	par->setparams(infile);
	log = new Log(par);
	en = new EmulNet(par);
	mp1 = (MP1Node **) malloc(par->EN_GPSZ * sizeof(MP1Node *));

	cout << "@mli: Application constructor after constructing mp1." << endl;

	/*
	 * Init all nodes
	 */
	for( i = 0; i < par->EN_GPSZ; i++ ) {
		Member *memberNode = new Member;
		memberNode->inited = false;
		Address *addressOfMemberNode = new Address();
		Address joinaddr;
		joinaddr = getjoinaddr();
		addressOfMemberNode = (Address *) en->ENinit(addressOfMemberNode, par->PORTNUM);
		mp1[i] = new MP1Node(memberNode, par, en, log, addressOfMemberNode);
		log->LOG(&(mp1[i]->getMemberNode()->addr), "APP");
		delete addressOfMemberNode;
	}
}

/**
 * Destructor
 */
Application::~Application() {
	delete log;
	delete en;
	for ( int i = 0; i < par->EN_GPSZ; i++ ) {
		delete mp1[i];
	}
	free(mp1);
	delete par;
}

/**
 * FUNCTION NAME: run
 *
 * DESCRIPTION: Main driver function of the Application layer
 */
int Application::run()
{
	int i;
	int timeWhenAllNodesHaveJoined = 0;
	// boolean indicating if all nodes have joined
	bool allNodesJoined = false;
	srand(time(NULL));

	// As time runs along
	for( par->globaltime = 0; par->globaltime < TOTAL_RUNNING_TIME; ++par->globaltime ) {
		// Run the membership protocol
		cout << "@mli: ---------------------- time: " << par->globaltime << " ----------------------" << endl;
		mp1Run();
		// Fail some nodes
		fail();
	}

	// Clean up
	en->ENcleanup();

	for(i=0;i<=par->EN_GPSZ-1;i++) {
		 mp1[i]->finishUpThisNode();
	}

	return SUCCESS;
}

/**
 * FUNCTION NAME: mp1Run
 *
 * DESCRIPTION:	This function performs all the membership protocol functionalities
 * 
 * For all the nodes, we first call recvLoop() at each node to receive messages associated with this node.
 * Then, for all the nodes, we run their membership protocal by calling nodeLoop().
 */
// @mli: 
// in mp1Run: 
//	- When receive, the nodes are processed from start to end. 

//  - When 
void Application::mp1Run() {
	int i;

	// For all the nodes in the system
	// @mli: 
	// Nodes in this loop are processed from START to END.
	// The recvLoop() method loops through the msgs from END to START, and insert them into queue.
	// As a result, the last msg being sent in the previous iteration will be processed first.
	for( i = 0; i <= par->EN_GPSZ-1; i++) {

		/*
		 * Receive messages from the network and queue them in the membership protocol queue
		 * In here, all the nodes are being runned.
		 */
		if( par->getcurrtime() > (int)(par->STEP_RATE*i) 
			&& !(mp1[i]->getMemberNode()->bFailed) ) {
			// Receive messages from the network and queue them
			mp1[i]->recvLoop();
		}

	}

	// For all the nodes in the system
	// @mli: EN_GPSZ == 10, STEP_RATE == .25
	// @mli: Note that in here, the nodes starts with a reverse order.
	// 		 Since we start looping from par->EN_GPSZ - 1.
	// @mli: 
	// This loop loops from the END to START.
	// In this loop, a node will either be started or nodeLoop will be called to process the received msgs. 
	for( i = par->EN_GPSZ - 1; i >= 0; i-- ) {
		/*
		 * Introduce nodes into the distributed system
		 */
		if( par->getcurrtime() == (int)(par->STEP_RATE*i) ) {
			// introduce the ith node into the system at time STEP_RATE*i
			mp1[i]->nodeStart(JOINADDR, par->PORTNUM);
			// @mli: log timestamp.
			cout << "@mli: ts==>[" << par->getcurrtime() << "]: " << endl;
			cout<<i<<"-th introduced node is assigned with the address: "<<mp1[i]->getMemberNode()->addr.getAddress() << endl;
			nodeCount += i;
		}

		/*
		 * Handle all the messages in your queue and send heartbeats
		 */
		else if( par->getcurrtime() > (int)(par->STEP_RATE*i) && !(mp1[i]->getMemberNode()->bFailed) ) {
			// handle messages and send heartbeats
			mp1[i]->nodeLoop();
			#ifdef DEBUGLOG
			if( (i == 0) && (par->globaltime % 500 == 0) ) {
				log->LOG(&mp1[i]->getMemberNode()->addr, "@@time=%d", par->getcurrtime());
			}
			#endif
		}

	}
}

/**
 * FUNCTION NAME: fail
 *
 * DESCRIPTION: This function controls the failure of nodes
 *
 * Note: this is used only by MP1
 */
void Application::fail() {
	int i, removed;

	// fail half the members at time t=400
	if( par->DROP_MSG && par->getcurrtime() == 50 ) {
		par->dropmsg = 1;
	}

	if( par->SINGLE_FAILURE && par->getcurrtime() == 100 ) {
		removed = (rand() % par->EN_GPSZ);
		#ifdef DEBUGLOG
		log->LOG(&mp1[removed]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
		#endif
		mp1[removed]->getMemberNode()->bFailed = true;
	}
	else if( par->getcurrtime() == 100 ) {
		removed = rand() % par->EN_GPSZ/2;
		for ( i = removed; i < removed + par->EN_GPSZ/2; i++ ) {
			#ifdef DEBUGLOG
			log->LOG(&mp1[i]->getMemberNode()->addr, "Node failed at time = %d", par->getcurrtime());
			#endif
			mp1[i]->getMemberNode()->bFailed = true;
		}
	}

	if( par->DROP_MSG && par->getcurrtime() == 300) {
		par->dropmsg=0;
	}

}

/**
 * FUNCTION NAME: getjoinaddr
 *
 * DESCRIPTION: This function returns the address of the coordinator
 */
Address Application::getjoinaddr(void){
	//trace.funcEntry("Application::getjoinaddr");
    Address joinaddr;
    joinaddr.init();
    *(int *)(&(joinaddr.addr))=1;
    *(short *)(&(joinaddr.addr[4]))=0;
    //trace.funcExit("Application::getjoinaddr", SUCCESS);
    return joinaddr;
}
