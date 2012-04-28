/*
* Delta3D Open Source Game and Simulation Engine
* Copyright (C) 2005, BMH Associates, Inc.
*
* This library is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Matthew W. Campbell
*/
#include <dtUtil/bits.h>

#include <iomanip>
#include <iostream>
#include <cstdarg>
#include <ctime>
#include <sstream>

#include "HTapp.h"
#include "eventlog.h"
#include "units/Projectile.h"

   const std::string EventLog::mDefaultName("__+default+__");
   static const char *sLogFileName = "eventlog.txt";

   //////////////////////////////////////////////////////////////////////////

   class EventLogManager: public osg::Referenced
   {
   public:
      std::ofstream logFile;
      UnitSide mSide;

      EventLogManager(UnitSide side)
      {
         //std::cout << "Creating logger" << std::endl;
         mSide = side;

         if (!logFile.is_open())
         {
            OpenFile();
         }
         //std::cout.flush();
      }

      ~EventLogManager()
      {
         mInstances.clear();
         //std::cout << "BEING DESTROYED - LogManager" << std::endl;
         //std::cout.flush();
         if (logFile.is_open())
         {
            //std::cout << "Closing log file" << std::endl;
            //std::cout.flush();
            EndFile();
            //Log::logFile.close();
         }
      }

      void EndFile()
      {
         logFile << std::endl;
         logFile.flush();
      }

      void OpenFile()
      {
         //std::cout << "LogManager try to open file to " << sLogFileName << std::endl;
         if (logFile.is_open())
         {
            logFile << "Change to log file: "<< sLogFileName<< std::endl;
            TimeTag();
            EndFile();
            logFile.close();
         }

         //First attempt to create the log file.
         std::ostringstream filename;
         filename << "results/logs/" << theApp->getStartTimeString() << "-eventLog";
         if (mSide == RED) {
         	filename << "Red";
         } else if (mSide == BLUE) {
         	filename << "Blue";
         }
         filename << ".txt";
         logFile.open(filename.str().c_str());
         //logFile.open(sLogFileName);
         if (!logFile.is_open())
         {
            std::cout << "could not open file \""<<filename.str()<<"\"" << std::endl;
            return;
         }
         else
         {
            //std::cout << "Using file \"delta3d_log.html\" for logging" << std::endl;
         }

         //TimeTag();

		 //logFile << std::endl;
         logFile.flush();
         //std::cout.flush();
      }

      void TimeTag() {
         logFile << std::fixed << std::setw(8) << theApp->getSimTime();
         logFile.flush();
      }

      bool AddInstance(const std::string& name, EventLog* log)
      {
         return mInstances.insert(std::make_pair(name, dtCore::RefPtr<EventLog>(log))).second;
      }

      EventLog* GetInstance(const std::string& name)
      {
         std::map<std::string, dtCore::RefPtr<EventLog> >::iterator i = mInstances.find(name);
         if (i == mInstances.end())
         {
            return NULL;
         }
         return i->second.get();
      }

   private:
      std::map<std::string, dtCore::RefPtr<EventLog> > mInstances;
   };

   static dtCore::RefPtr<EventLogManager> manager(NULL);

   /** This will close the existing file (if opened) and create a new file with
    *  the supplied filename.
    * @param name : The name of the new file (will be written using HTML)
    */
   void EventLogFile::SetFileName(const std::string& name)
   {
      //std::cout << "LogFile try to change files to " << name << std::endl;

      sLogFileName = name.c_str();
      if (manager == NULL) {
         manager = new EventLogManager(BLUE);
      } else {
         manager->OpenFile();
      }
   }

   const std::string EventLogFile::GetFileName()
   {
      return std::string(sLogFileName);
   }

   //////////////////////////////////////////////////////////////////////////
   EventLog::EventLog(const std::string& name)
      :mOutputStreamBit(EventLog::STANDARD),
      mName(name),
      mLogInterval(1),
      mLastLogTime(-INFINITY)
   {
   }

   //////////////////////////////////////////////////////////////////////////
   EventLog::~EventLog()
   {
   }

   void EventLog::SetLogSide(UnitSide side) {
		if (manager == NULL)
			manager = new EventLogManager(side);
   }

   void EventLog::init(int blue, int red, std::string map) {
	   	if (mOutputStreamBit == EventLog::NO_OUTPUT)
	         return;

	    if (!manager->logFile.is_open())
	         return;

		//write header
		std::ostringstream oss;
		oss	<< "       0 MAP  " << map << std::endl;
		oss	<< "       0 BLUE " << blue << std::endl;
		oss	<< "       0 RED  " << red << std::endl;
		oss << "       0 FREQ " << mLogInterval << std::endl;

	    if (dtUtil::Bits::Has(mOutputStreamBit, EventLog::TO_FILE))
	    {
			manager->logFile << oss.str();
			manager->logFile.flush();
	    }
	    if (dtUtil::Bits::Has(mOutputStreamBit, EventLog::TO_CONSOLE))
      	{
      		std::cout << oss.str();
      	}
   }


   //////////////////////////////////////////////////////////////////////////
   void EventLog::logEvent(const std::string &msg) const
   {
      if (mOutputStreamBit == EventLog::NO_OUTPUT)
         return;

      if (!manager->logFile.is_open())
         return;

      if (dtUtil::Bits::Has(mOutputStreamBit, EventLog::TO_FILE))
      {
      	 //insert timestamp in logfile
	  	 manager->TimeTag();
	  	 //insert message
         manager->logFile << ' ' << msg << std::endl;

         manager->logFile.flush(); //Make sure everything is written, in case of a crash.
      }

      if (dtUtil::Bits::Has(mOutputStreamBit, EventLog::TO_CONSOLE))
      {
         std::cout << ' ' << msg << std::endl;
      }

   }

   //log an event (unit2 & pos are optional)
   void EventLog::log(Event::EventType event,  std::vector<std::string>& participants, std::vector<double>& data) {
   		std::ostringstream oss;
		switch (event) {
			case Event::POSITION:
				oss << "POS  " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0) << ' ' << std::setw(5)  << std::fixed << data.at(1) << ' ' << std::setw(5) << std::fixed << data.at(2);
				break;
			case Event::HEADING:
				oss << "HEAD " << std::setw(9) << participants.at(0) << ' ' << std::setw(9) << std::fixed << data.at(0);
				break;
			case Event::POSITION_AND_HEADING:
				if (isTimeForNextLog()) {
					oss << "POSH " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0) << ' ' << std::setw(5) << std::fixed << data.at(1) << ' ' << std::setw(5) << std::fixed << data.at(2) << ", " << std::setw(9) << std::fixed << data.at(3);
				} else {
					return;
				}
				break;
			case Event::NEW_GOAL:
				oss << "GOAL " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0) << ' ' << std::setw(5) << std::fixed << data.at(1) << ' ' << std::setw(5) << std::fixed << data.at(2);
				break;
			case Event::SELECTED:
				oss << "SEL  " << std::setw(9) << participants.at(0);
				break;
			case Event::DETONATION:
				oss << "DETN " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0) << ' ' << std::setw(5) << std::fixed << data.at(1) << ' ' << std::setw(5) << std::fixed << data.at(2);
				break;
			case Event::SEES:
				oss << "SEES " << std::setw(9) << participants.at(0) << ' ' << std::setw(9) << participants.at(1);
				break;
			case Event::HIDE:
				oss << "HIDE " << std::setw(9) << participants.at(0) << ' ' << std::setw(9) << participants.at(1);
				break;
			case Event::FIRE:
				oss << "FIRE " << std::setw(9) << participants.at(0) << ' ' << std::setw(9) << participants.at(1);
				break;
			case Event::HURT:
				oss << "HURT " << std::setw(9) << participants.at(0) << ' ' << std::setw(9) << participants.at(1) << ' ' << std::setw(8) << std::fixed << data.at(0);
				break;
			case Event::PAUSE:
				oss << "PAUS " << " Player " << data.at(0) << " sets " << data.at(1);
				break;
            case Event::GAME_OVER:
                if (data.at(0) == 0)
                  oss << "GMOV " << "BLUE WINS";
                else
                  oss << "GMOV " << "RED WINS";
                break;
            case Event::TIME_SYNC:
            	oss << "SYNC " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0) << ' ' << std::setw(5) << std::fixed << data.at(1);
            	break;
            case Event::TIME_SCALE:
            	oss << "SCLE " << std::setw(9) << participants.at(0) << ' ' << std::setw(5) << std::fixed << data.at(0);
            	break;
			default:
				LOG_DEBUG("Unhandled event type");
				return;
		}
		logEvent(oss.str());
   }


   //////////////////////////////////////////////////////////////////////////
   EventLog &EventLog::GetInstance()
   {
      return GetInstance(mDefaultName);
   }

   //////////////////////////////////////////////////////////////////////////
   EventLog &EventLog::GetInstance(const std::string& name)
   {
      if (manager == NULL) {
         //manager = new EventLogManager;
         throw new dtUtil::Exception("You must call SetLogSide before trying to log", "eventlog.cpp", __LINE__);
      }
      EventLog* l = manager->GetInstance(name);
      if (l == NULL)
      {
         l = new EventLog(name);
         manager->AddInstance(name, l);
      }

      return *l;
   }

   /** Tell the Log where to send output messages.  The supplied parameter is a
    *  bitwise combination of OutputStreamOptions.  The default is STANDARD, which
    *  directs messages to the output file.
    *  For example, to tell the Log to output to the file and console:
    *  \code
    *   EventLog::GetInstance().SetOutputStreamBit(EventLog::TO_FILE | EventLog::TO_CONSOLE);
    *  \endcode
    *  \param option A bitwise combination of options.
    */
   void EventLog::SetOutputStreamBit(unsigned int option)
   {
      mOutputStreamBit = option;
   }

   unsigned int EventLog::GetOutputStreamBit() const
   {
      return mOutputStreamBit;
   }

   bool EventLog::isTimeForNextLog() {
   		return (theApp->getSimTime() > (mLastLogTime + mLogInterval));
   }

   void EventLog::intervalDone() {
   		if (isTimeForNextLog())
   			mLastLogTime = theApp->getSimTime();
   }
