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
#ifndef EVENTLOG
#define EVENTLOG

#include <string>
#include <map>
#include <fstream>

#include <osg/Referenced>
#include <osg/Vec3>
#include <dtCore/refptr.h>
#include <dtUtil/export.h>

#include "units/Unit.h"
#include "event.h"

class EventLogFile
   {
   public:
      ///Change the name of the log file (defaults to "delta3d_log.html")
      static void SetFileName(const std::string& name);

      ///Get the current filename of the log file.
      static const std::string GetFileName();
      
      /// change the title string used in HTML 
      /// defaults to "Delta 3D Engine Log File" or "Delta 3D Engine Log File (Debug Libs)"
      static void SetTitle(const std::string& title);
      
      ///Get the current HTML title string.
      static const std::string& GetTitle();
   };

    /**
     * Helps making logging a little easier.  However, printf style
    *   logging is desired, you cannot use this macro.
     */
 /*    #define LOG_DEBUG(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_DEBUG);

     #define LOG_INFO(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_INFO);

     #define LOG_WARNING(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_WARNING);

     #define LOG_ERROR(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ERROR);

     #define LOG_ALWAYS(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ALWAYS);

     #define LOGN_DEBUG(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_DEBUG);

     #define LOGN_INFO(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_INFO);

     #define LOGN_WARNING(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_WARNING);

     #define LOGN_ERROR(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ERROR);

     #define LOGN_ALWAYS(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ALWAYS);*/
    
   /**
     * Log class which the engine uses for all of its logging
     * needs.  The log file is formatted using html tags,
     * therefore, any browser should display the log without
     *  any problems.
     */
    class EventLog : public osg::Referenced 
    {
    public:

        /**
         * The different types of event messages. (Now in event.h)
         */
        /*enum EventMessageType
        {
            POSITION,
            HEADING,
            POSITION_AND_HEADING,
            SEES,
            HIDE,
            FIRE,
            HURT,
            NEW_GOAL,
            SELECTED,
            DETONATION
        };*/

		static void SetLogSide(UnitSide side);

		void init(int blue, int red, std::string map);

        /**
         * Logs a time-stamped message.
         * @param str, the string to log
         */
        void logEvent(const std::string &str) const;
        
        void log(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data);

        /*
         * Retrieve singleton instance of the log class.
         */
        static EventLog& GetInstance();

        /*
         * Retrieve singleton instance of the log class for a give string name.
         * @param name logger name
         */
        static EventLog& GetInstance(const std::string& name);

        enum OutputStreamOptions
        {
           NO_OUTPUT =   0x00000000, ///<Log messages don't get written to any device
           TO_FILE =     0x00000001,   ///<Log messages get sent to the output file
           TO_CONSOLE =  0x00000002,///<Log messages get sent to the console
           STANDARD = TO_FILE ///<The default setting
        };

        ///Configure where the Log messages get directed
        void SetOutputStreamBit(unsigned int option);

        ///Get the currently defined output stream options
        unsigned int GetOutputStreamBit() const;
        
        ///Returns the name of this logger.
        const std::string& GetName() const { return mName; }
        
        void setLogInterval(double time) { mLogInterval = time; }
        double getLogInterval() { return mLogInterval; }
        bool isTimeForNextLog(); 
        void intervalDone();

    //Constructor and destructor are both protected since this is a singleton.
    protected:
        /**
         * Opens the log file and writes the html header information.
         */
        EventLog(const std::string& name);

        /**
         * Writes any closing html tags and closes the log file.
         */
        ~EventLog();

    private:
        static const std::string mDefaultName;

        //EventMessageType mLevel;
        unsigned int mOutputStreamBit; ///<the current output stream option
        std::string mName;
        double mLogInterval;
        double mLastLogTime;
    };



#endif
