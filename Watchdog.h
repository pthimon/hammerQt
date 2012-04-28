/*
 * Watchdog.h
 *
 *  Created on: 11 Feb 2010
 *      Author: sbutler
 */

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

class Watchdog {
public:
	Watchdog(int timeout);
	virtual ~Watchdog();

	void go();
	void stop();

	void frame(double tick);

	void watch();

private:
	bool mStopRequested;
    boost::shared_ptr<boost::thread> mThread;
    boost::mutex mMutex;
    int mTimeout;
    int mTime;
    int mStartTime;
};

#endif /* WATCHDOG_H_ */
