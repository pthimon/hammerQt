#include "HTPhysicsController.h"
#include <dtUtil/log.h>

HTPhysicsController::HTPhysicsController(dtCore::Base* msgSender) :
	dtCore::ODEController(msgSender),
	m_MaxPhysicsStep(0),
	mPhysicsTimeRemaining(0)
{
}

HTPhysicsController::~HTPhysicsController()
{
}

//To compile: need to make mCollidableContents and Step()
// protected in dtCore/odecontroller.h or use delta3d > 2.2.0
void HTPhysicsController::Iterate(double deltaFrameTime)
{
   double stepSize = deltaFrameTime;

   // if step size is set, use it instead of the delta frame time
   if (GetPhysicsStepSize() > 0.0)
   {
	   stepSize = GetPhysicsStepSize();
   }

   TransformableVector::const_iterator it;

#if DELTA3D_VER >= 23
	for (it = GetRegisteredCollidables().begin();
           it != GetRegisteredCollidables().end(); it++)
#else
	for (it = mCollidableContents.begin();
           it != mCollidableContents.end(); it++ )
#endif
	{
	     (*it)->PrePhysicsStepUpdate();
	}

    mPhysicsTimeRemaining += deltaFrameTime;

    if ((m_MaxPhysicsStep != 0.0) && (mPhysicsTimeRemaining > m_MaxPhysicsStep)) {
  	  //limit the number of steps to avoid big jumps
  	  dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_DEBUG,__FUNCTION__, __LINE__,"Time step of %f exceeded maximum physics step",mPhysicsTimeRemaining);
  	  mPhysicsTimeRemaining = m_MaxPhysicsStep;
    }

    //do physics steps, saving the left-over for the next frame
    while (mPhysicsTimeRemaining > stepSize)
    {
  	  Step(stepSize);

  	  mPhysicsTimeRemaining -= stepSize;
    }

	//for (it = GetRegisteredCollidables().begin(); //use this for delta3d > 2.2.0
    //   it != GetRegisteredCollidables().end();
#if DELTA3D_VER >= 23
        for (it = GetRegisteredCollidables().begin();
           it != GetRegisteredCollidables().end(); it++)
#else
        for (it = mCollidableContents.begin();
           it != mCollidableContents.end(); it++ )
#endif
   {
      (*it)->PostPhysicsStepUpdate();
   }
}
