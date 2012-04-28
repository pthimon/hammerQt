#ifndef UNITCONFIGSCHEMA_H_
#define UNITCONFIGSCHEMA_H_

#include <string>

   /// defines API used to model the XML schema for the config file.
   struct UnitConfigSchema
   {
   public:
	   static const std::string UNIT;
	   static const std::string NAME;
	   static const std::string FLYING;
	   static const std::string ARMOUR;
	   
	   static const std::string MOVEMENT;
	   static const std::string MAX_SPEED;
	   static const std::string AVERAGE_SPEED;
	   static const std::string SLOW_SPEED;
	   static const std::string TURNING_RATE;
	   static const std::string SLOWING_DIST;
	   static const std::string STOPPING_DIST;
	   
	   static const std::string PHYSICS;
	   static const std::string MASS;
	   static const std::string MAX_FORCE;
	   static const std::string SUSPENSION_ERP;
	   static const std::string SUSPENSION_CFM;
	   static const std::string TURN_ERP;
	   static const std::string TURN_CFM;
	   
	   static const std::string WHEELS;
	   static const std::string X;
	   static const std::string Y_FRONT;
	   static const std::string Y_REAR;
	   static const std::string Z;
	   
	   static const std::string FIRING;
	   static const std::string RELOAD;
	   static const std::string TURRET;
	   static const std::string BARREL;
	   
	   static const std::string PROJECTILE;
	   static const std::string RADIUS;
	   static const std::string VELOCITY;
	   static const std::string RANGE;
	   static const std::string YIELD;
	   static const std::string TYPE;
	   
	   static const std::string SHADER;
	   static const std::string DEFINITIONS;
	   
	   static const std::string OBJECT;
	   static const std::string CHARACTER;
	   static const std::string MESH;
	   static const std::string NUMBER;
	   
	   static const std::string SENSORS;	   
	   static const std::string LAYER_ID;
	   static const std::string VISIBILITY;
   };

#endif /*UNITCONFIGSCHEMA_H_*/
