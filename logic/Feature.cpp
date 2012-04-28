#include "Feature.h"
#include "../units/Unit.h"

/** Height difference scaling.
    Comparison value is calculated as 1-scaling*log10(abs(height_difference)), clamped at 0.
    So at scaling = 0.001 a difference of 1000 meters mean 0 similarity even if the features match
    attribute-wisely. If the difference is 10 meters, then they have a similarity of 0.999.
*/  
const float height_scaling = 0.001f;

// this determine how much plan points attract roads
const float plan_road_attractive_force = 1.0f;
// this determine how much tank's plan points repel cities and forests
const float tank_plan_forest_city_repelling_force = -1.0f;

float AgentFeature::compare(const Feature& ft) const
    {
      const AgentFeature* conv_ft = dynamic_cast<const AgentFeature*>(&ft);
      if (conv_ft)
        {
          // it is an agent
          if (conv_ft->agent_type == agent_type && conv_ft->agent_side == agent_side)
            return 1;           // it has the same type and side
          else return 0; // it is different
        }

      // default: no match 
      return 0;
    }

AgentFeature::AgentFeature(Unit& unit)
  {
    osg::Vec3 pos = unit.getPosition();
    m_xPos = pos.x();
    m_yPos = pos.y();
    if (unit.getType() == "tank")
      agent_type = TANK;
    else if (unit.getType() == "soldier")
      agent_type = SOLDIER;
    
    agent_side = unit.getSide();  
    m_Unit = &unit;
  }


float TerrainFeature::compare(const Feature& ft) const
  {
    const TerrainFeature* pterr_ft = dynamic_cast<const TerrainFeature*>(&ft);
    if (pterr_ft) // it is another terrain feature
      {
        if (pterr_ft->attr == attr) // it has the same attributes
          {
            // compare the heights
            float ft_height = pterr_ft->getHeight();
            float height_diff = ft_height - getHeight();
            
            return HT_MAX(0, 1 - height_scaling*log10(abs(height_diff)));
          }
        else return 0;
      }
    
    const PlanFeature* pplan_ft = dynamic_cast<const PlanFeature*>(&ft);
    if (pplan_ft) // it is a plan feature, use the code there to calculate similarity
      return pplan_ft->compare(*this);
    
    return 0;
  }

float PlanFeature::compare(const Feature& ft) const
  {
    const TerrainFeature* conv_ft = dynamic_cast<const TerrainFeature*>(&ft);
    if (conv_ft) // it is a terrain feature
      {        
        // we like if plans lie on roads
        unsigned long int road_mask = 1l << theApp->getTerrain()->GetLayerIndex("road");
        unsigned long int city_mask = 1l << theApp->getTerrain()->GetLayerIndex("forest");
        unsigned long int forest_mask = 1l << theApp->getTerrain()->GetLayerIndex("city");

        if (conv_ft->attr & road_mask)
          return plan_road_attractive_force;
        // but dislike if we are tanks and plans lie on forests or cities
        if (owner->agent_type == TANK)
          if (conv_ft->attr & city_mask || conv_ft->attr & forest_mask)
            return tank_plan_forest_city_repelling_force;
      }    
    return 0;
  }

std::ifstream& operator>>( std::ifstream& ifs, Feature& ft )
  {
    ifs >> ft.is_required;
    ifs >> ft.is_exclusive;
    ifs >> ft.is_static;
    
    return ifs;
  }

std::ifstream& operator>>( std::ifstream& ifs, PlanarFeature& ft )
  {
    ifs >> *(dynamic_cast<Feature*>(&ft));
    ifs >> ft.m_xPos;
    ifs >> ft.m_yPos;

    return ifs;
  }

std::ifstream& operator>>( std::ifstream& ifs, AgentFeature& ft )
  {
    int tmp;

    ifs >> *(dynamic_cast<PlanarFeature*>(&ft));
    ifs >> tmp;
    ft.agent_type = (UnitType)tmp;
    ifs >> tmp;
    ft.agent_side = (UnitSide)tmp;
    ifs >> ft.id;
    ft.m_Unit = 0;

    return ifs;
  }

std::ifstream& operator>>( std::ifstream& ifs, TerrainFeature& ft )
  {
    ifs >> *(dynamic_cast<PlanarFeature*>(&ft));
    ifs >> ft.attr;
    ifs >> ft.m_Height;

    return ifs;
  }

std::ifstream& operator>>( std::ifstream& ifs, PlanFeature& ft )
  {
    ifs >> *(dynamic_cast<PlanarFeature*>(&ft));
    ifs >> ft.owner_id;

    return ifs;
  }

std::ofstream& operator<<( std::ofstream& ofs, const Feature& ft )
  {
    ofs << ft.is_required;
    ofs << ft.is_exclusive;
    ofs << ft.is_static;
    
    return ofs;
  }

std::ofstream& operator<<( std::ofstream& ofs, const PlanarFeature& ft )
  {
    ofs << *(dynamic_cast<const Feature*>(&ft));
    ofs << ft.m_xPos;
    ofs << ft.m_yPos;

    return ofs;
  }

std::ofstream& operator<<( std::ofstream& ofs, const AgentFeature& ft )
  {
    ofs << *(dynamic_cast<const PlanarFeature*>(&ft));
    ofs << ft.agent_type;
    ofs << ft.agent_side;
    ofs << ft.id;

    return ofs;
  }

std::ofstream& operator<<( std::ofstream& ofs, const TerrainFeature& ft )
  {
    ofs << *(dynamic_cast<const PlanarFeature*>(&ft));
    ofs << ft.attr;
    ofs << ft.m_Height;

    return ofs;
  }

std::ofstream& operator<<( std::ofstream& ofs, const PlanFeature& ft )
  {
    ofs << *(dynamic_cast<const PlanarFeature*>(&ft));
    ofs << ft.owner_id;

    return ofs;
  }

// load in a set of features

std::ifstream& operator>>( std::ifstream& ifs, std::set<Feature*>& features )
  {
    unsigned int num_features;
    feature_type ft;
    Feature* act_feature;
    AgentFeature* new_agent;
    PlanFeature* plan_feature;
    
    std::map<std::string,AgentFeature*> agents;
    
    ifs >> num_features;
        
    for (unsigned i = 0; i< num_features; i++)
      {
        int tmp;
        ifs >> tmp;
        ft = (feature_type)tmp;
        
        switch(ft)
          {
            case AGENT:
              new_agent = new AgentFeature();
              act_feature = new_agent;
              ifs >> *act_feature;
              agents[new_agent->id] = new_agent;
              break;
            case PLAN:
              act_feature = new PlanFeature();
              ifs >> *act_feature;
              break;
            case TERRAIN:
              act_feature = new TerrainFeature();
              ifs >> *act_feature;
              break;
            default:
              LOG_ERROR("Unknown feature type.");              
              break;
          }
        
        features.insert(act_feature);
                
        if (!ifs.good())
        {
            LOG_ERROR("Error while reading feature set.");
        }       
      }
    
    // setup owner agents
    std::set<Feature*>::iterator it;

    for (it = features.begin(); it != features.end(); it++)
      {
        if ( (plan_feature = dynamic_cast<PlanFeature*>(*it)) )
          {
            plan_feature->owner = agents[plan_feature->owner_id];
          }
      }
    
    return ifs;
  }

// writes out a set of features

std::ofstream& operator<<( std::ofstream& ofs, const std::set<Feature*>& features )
  {
    AgentFeature* agent_feature;
    PlanFeature* plan_feature;
    TerrainFeature* terrain_feature;
    std::set<Feature*>::const_iterator it;
    
    unsigned int size = features.size();    
    ofs << size;
        
    for (it = features.begin(); it != features.end(); it++)
      {
        // dump feature type        
        if ( (agent_feature = dynamic_cast<AgentFeature*>(*it)) )
          {
            ofs << AGENT;
            ofs << &agent_feature;
          }
        else if ( (plan_feature = dynamic_cast<PlanFeature*>(*it)) )
          {
            ofs << PLAN;
            ofs << &plan_feature;
          }
        else if ( (terrain_feature = dynamic_cast<TerrainFeature*>(*it)) )
          {
            ofs << TERRAIN;
            ofs << &terrain_feature;
          }
                        
        if (!ofs.good())
        {
            LOG_ERROR("Error while writing feature set.");
        }       
      }    
    
    return ofs;
  }
