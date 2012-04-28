/****************************************************************************
**
** Trolltech hereby grants a license to use the Qt/Eclipse Integration
** plug-in (the software contained herein), in binary form, solely for the
** purpose of creating code to be used with Trolltech's Qt software.
**
** Qt Designer is licensed under the terms of the GNU General Public
** License versions 2.0 and 3.0 ("GPL License"). Trolltech offers users the
** right to use certain no GPL licensed software under the terms of its GPL
** Exception version 1.2 (http://trolltech.com/products/qt/gplexception).
**
** THIS SOFTWARE IS PROVIDED BY TROLLTECH AND ITS CONTRIBUTORS (IF ANY) "AS
** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
** PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
** OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** Since we now have the GPL exception I think that the "special exception
** is no longer needed. The license text proposed above (other than the
** special exception portion of it) is the BSD license and we have added
** the BSD license as a permissible license under the exception.
**
****************************************************************************/

#include <dtCore/system.h>
#include <dtCore/globals.h>
#include <dtCore/refptr.h>
#include <dtUtil/log.h>
#include <dtUtil/exception.h>

#include <QtGui>
#include <QApplication>

#include "HTAppBase.h"
#include "HTapp.h"
#include "HTAppQt.h"
#include "gui/hammerqt.h"

// the one and only application class
dtCore::RefPtr<HTAppBase> theApp;


////////////////////////////////////////////////////////////////////////////////
class EmbeddedWindowSystemWrapper: public osg::GraphicsContext::WindowingSystemInterface
{
   public:
      EmbeddedWindowSystemWrapper(osg::GraphicsContext::WindowingSystemInterface& oldInterface):
         mInterface(&oldInterface)
      {
      }

#if OSG_VER >= 28
      virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier, osg::GraphicsContext::ScreenSettings & resolution) { return; }

      virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier, osg::GraphicsContext::ScreenSettingsList & resolutionList) { return; }
#endif
      virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier =
         osg::GraphicsContext::ScreenIdentifier())
      {
         return mInterface->getNumScreens(screenIdentifier);
      }

      virtual void getScreenResolution(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier,
               unsigned int& width, unsigned int& height)
      {
         mInterface->getScreenResolution(screenIdentifier, width, height);
      }

      virtual bool setScreenResolution(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier,
               unsigned int width, unsigned int height)
      {
         return mInterface->setScreenResolution(screenIdentifier, width, height);
      }

      virtual bool setScreenRefreshRate(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier,
               double refreshRate)
      {
         return mInterface->setScreenRefreshRate(screenIdentifier, refreshRate);
      }

      virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits* traits)
      {
         return new osgViewer::GraphicsWindowEmbedded(traits);
      }

   protected:
      virtual ~EmbeddedWindowSystemWrapper() {};
   private:
      dtCore::RefPtr<osg::GraphicsContext::WindowingSystemInterface> mInterface;
};


int main(int argc, char *argv[])
{
   // set random seed
   srand((unsigned)time(0));

   dtCore::SetDataFilePathList( dtCore::GetDeltaDataPathList() + ";" +
                                dtCore::GetDeltaDataPathList() + "/gui_5;" +
                                "./data;../data");

   dtUtil::Log::GetInstance().SetLogLevel(dtUtil::Log::LOG_DEBUG);

   // use an ArgumentParser object to manage the program arguments.
   osg::ArgumentParser arguments(&argc,argv);

   // set up the usage document, in case we need to print out how to use this program.
   arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
   arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is a simulation for the HAMMERTIMME project.");
   arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] value ...");
   arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display command line options");
   arguments.getApplicationUsage()->addCommandLineOption("--id <num>","An identification number for this instance (as referred to by the scenario XML file) [default: 0]");
   arguments.getApplicationUsage()->addCommandLineOption("--connect <host>","Set the hostname of the server to connect to [client side only]");
   arguments.getApplicationUsage()->addCommandLineOption("--port <portnum>","Set the port to use [default: 4444]");
   arguments.getApplicationUsage()->addCommandLineOption("--scenario <scenariofile>","Set the file name which stores scenario XML data [server side only, default: scenario.xml]");
   arguments.getApplicationUsage()->addCommandLineOption("--wait_controller","If specified, do not start simulation until a controller is connected [server side only, optional]");
   arguments.getApplicationUsage()->addCommandLineOption("--headless","If specified, do not show GUI, but respond using the network interface [optional]");
   arguments.getApplicationUsage()->addCommandLineOption("--gui","If specified, show the Qt-based GUI [optional]");
   arguments.getApplicationUsage()->addCommandLineOption("--replay <logfile>","If specified, read in the log file and playback the GOAL commands [optional]");
   arguments.getApplicationUsage()->addCommandLineOption("--launch","If specified, launch the hypotheses automatically [optional]");
   arguments.getApplicationUsage()->addCommandLineOption("--config <configfile>","If specified, set the filename which stores hypothesis config XML data [server side only, optional]");


   // Handle our command line arguments
   // if user request help write it out to cout.
   if (arguments.read("-h") || arguments.read("--help"))
   {
      arguments.getApplicationUsage()->write(std::cout);
      return 1;
   }

   std::string hostName;
   int portNum;
   std::string scenarioFile;
   bool wait_client;
   bool runHeadless;
   int id;
   bool showGui;
   std::string replayFile;
   bool launchHypotheses;
   std::string configFile;

   if (!arguments.read("--connect", hostName))
   {
   	hostName = "";
   }
   if (!arguments.read("--port", portNum))
   {
   	portNum = 4444;
   }
   if (!arguments.read("--scenario", scenarioFile))
   {
   	scenarioFile = "scenario.xml";
   }
   if (!arguments.read("--id", id))
   {
	   if (!hostName.empty()) {
		   arguments.reportError("Identification number expected as '--connect <host>' has been specified (use '--id <num>')");
	   }
	   id = 0;
   }

   wait_client = (arguments.read("--wait_controller") || arguments.read("--wait-controller"));

   runHeadless = arguments.read("--headless");

   showGui = arguments.read("--gui");

   if (!arguments.read("--replay", replayFile)) {
	   replayFile = "";
   }

   launchHypotheses = arguments.read("--launch");

	if (!arguments.read("--config", configFile))
	{
		configFile = "";
	}

   // any option left unread are converted into errors to write out later.
   arguments.reportRemainingOptionsAsUnrecognized();

   // report any errors if they have occured when parsing the program aguments.
   if (arguments.errors())
   {
      arguments.writeErrorMessages(std::cout);
      return 1;
   }

	try
	{
		if (!showGui) {
			//OpenGL based gui

			if (runHeadless) {
				dtCore::System::GetInstance().SetShutdownOnWindowClose(false);
				theApp = new HTAppBase("config.xml", hostName, portNum, wait_client, scenarioFile, id, replayFile, configFile);
			} else {
				theApp = new HTApplication("config.xml", hostName, portNum, wait_client, scenarioFile, id, replayFile, configFile);
			}
			theApp->Init();
			if (launchHypotheses) {
				theApp->triggerLaunchHypotheses();
			}
			theApp->Run();

		} else {
			//Qt based gui

			QApplication a(argc, argv);
			hammerQt w;
			w.show();

			//Set osg to use an embedded window system before a DeltaWin is created
			osg::GraphicsContext::WindowingSystemInterface* winSys = osg::GraphicsContext::getWindowingSystemInterface();
			if (winSys != NULL) {
				osg::GraphicsContext::setWindowingSystemInterface(new EmbeddedWindowSystemWrapper(*winSys));
			}

			//Initialise application (requires the qtwindow to contain an opengl widget)
			HTAppQt *qtApp;
			theApp = qtApp = new HTAppQt("config.xml", hostName, portNum, wait_client, scenarioFile, id, replayFile, configFile);
			qtApp->Init(&w);
			if (launchHypotheses) {
				qtApp->triggerLaunchHypotheses();
			}

			a.exec();
		}
	}
	catch (dtUtil::Exception &ex)
	{
		std::cout << ex.What() << std::endl;
	}

	return EXIT_SUCCESS;
}
