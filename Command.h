#ifndef COMMAND_H_
#define COMMAND_H_

#include <string>
#include <osg/Vec3>

namespace Command {
	enum CommandType {
		NONE = 0,					// 00000000000000
		GOTO = 1,					// 00000000000001
		HEADTO = 2,					// 00000000000010
		PATHTO = 3,					// 00000000000011
		STEALTHTO = 4,				// 00000000000100
		RENDEZ_VOUS = 63,			// 00000000111111	63 possible rendez-vous
		FORMATION_CIRCLE = 64, 		// 00000001000000
		FORMATION_WEDGE = 128,		// 00000010000000
		FORMATION_LINE = 192,		// 00000011000000
		FORMATION_COLUMN = 256,		// 00000100000000
		FORMATIONS = 4032, 			// 00111111000000	63 possible formations
		MANOEUVRE_ATTACK = 4096,	// 01000000000000
		MANOEUVRE_DEFEND = 8192,	// 10000000000000
		MANOEUVRE_RETREAT = 12288,	// 11000000000000
		MANOEUVRE_PINCER = 16384,	//100000000000000
		MANOEUVRE_AVOID = 20480,	//101000000000000
		MANOEUVRE_CONVOY = 24576,	//110000000000000
		MANOEUVRES = 258048 	// 111111000000000000	63 possible manoeuvres
	};
};

#endif /*COMMAND_H_*/