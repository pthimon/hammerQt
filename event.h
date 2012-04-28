#ifndef EVENT_H_
#define EVENT_H_

class Event {
	
	public:
	
	enum EventType
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
        DETONATION,
        PAUSE,
        GAME_OVER,
        TIME_SYNC,
        TIME_SCALE
    };
};

#endif /*EVENT_H_*/
