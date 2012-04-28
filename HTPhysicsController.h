#ifndef HTPHYSICSCONTROLLER_H_
#define HTPHYSICSCONTROLLER_H_

#include <dtCore/odecontroller.h>
#include <dtCore/transformable.h>

class HTPhysicsController : public dtCore::ODEController
{
public:
	HTPhysicsController(dtCore::Base* msgSender = NULL);
	virtual ~HTPhysicsController();
	
	virtual void Iterate(double deltaFrameTime);
	
	 /**
     * Get the maximum step interval of the physics.  The physics will
     * be updated for at most so much time in each frame. If frame rates are too low
     * to update properly, then physics steps will be discarded. Added to avoid
     * extreme forces when framerate is low. Set it to 0 for do not care
     * about this value at all (default Delta3D behavior).
     *
     * @return the step size in seconds
     * @see SetPhysicsStepSize()
     */
    double GetMaxPhysicsStep() const { return m_MaxPhysicsStep; }

    /// @see GetMaxPhysicsStep()
    void SetMaxPhysicsStep( double stepSize = 0.0 ) { m_MaxPhysicsStep = stepSize; };
	
private:
	  //added to prevent explosions in ODE
	  double m_MaxPhysicsStep;   
	  double mPhysicsTimeRemaining;
};

#endif /*HTPHYSICSCONTROLLER_H_*/
