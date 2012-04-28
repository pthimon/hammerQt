#ifndef UNITCONFIGDATA_H_
#define UNITCONFIGDATA_H_

#include <string>                           // for data members
#include <vector>
#include <map>

	struct MeshData {
      	std::string type, name, mesh;
      };

/// defines API used to obtain values of the config file.
   struct UnitConfigData
   {
      UnitConfigData();
      ~UnitConfigData();
      
      std::string unit_name;
      bool flying;
      double armour;
      
      struct {
      	float max_speed, slow_speed, average_speed, turning_rate, slowing_dist, stopping_dist;
      } movement;
      
      struct {
      	float mass, max_force, suspension_erp, suspension_cfm, turn_erp, turn_cfm;
      } physics; 
      
      struct {
      	float mass, x, y_front, y_rear, z;
      } wheels;
      
      struct {
      	float reload, turret, barrel;
      } firing;
      
      struct {
      	float mass, radius, velocity, yield, range;
      	std::string type;
      } projectile;
      
      struct {
        std::map<std::string, float> range, visibility;
      } sensors;
      
      std::vector<MeshData> meshes;
   };

#endif /*UNITCONFIGDATA_H_*/
