/*
 * Watchdog.cpp
 *
 *  Created on: 11 Feb 2010
 *      Author: sbutler
 */

#include "Watchdog.h"

#include <ctime>
#include <boost/bind.hpp>
#include <iostream>
#include <signal.h>


void termination_handler (int /*signum*/) {
	exit(2);
}

Watchdog::Watchdog(int timeout):
	mStopRequested(false),
	mTimeout(timeout)
{
	//initialise start time
	mStartTime = time(NULL);
	mTime = 0;

	struct sigaction new_action;

	/* Set up the structure to specify the new action. */
	new_action.sa_handler = termination_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction (SIGUSR1, &new_action, NULL);
}

Watchdog::~Watchdog() {
	stop();
}

// Create the thread and start work
void Watchdog::go() {
	mThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Watchdog::watch, this)));
}

void Watchdog::stop() {
    mStopRequested = true;
    std::cout << "Watchdog: Stopping" << std::endl;
    mThread->join();
}

void Watchdog::frame(double tick) {
	boost::mutex::scoped_lock l(mMutex);
	mTime = (int)tick;
}

void Watchdog::watch() {
	while (!mStopRequested) {
		int t;
		{
			//get last tick time
			boost::mutex::scoped_lock l(mMutex);
			t = mTime + mStartTime;
		}
		if (t + mTimeout < time(NULL)) {
			//no ticks in the last 10 seconds, so exit
			std::cout << "Watchdog: Simulation timeout" << std::endl;
			system("killall -s SIGUSR1 hammerQt");
			break;
		} else {
			//wait
			sleep(1);
		}
	}
}
