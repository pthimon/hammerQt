#ifndef FEATURE_H_
#define FEATURE_H_

#include <set>
#include <vector>
#include <fstream>

#include "../common.h"

enum feature_type { AGENT, TERRAIN, PLAN };

class PlanFeature;
class Unit;

/** This abstract class stores everything what the adaptor needs to know about a feature.
 */
class Feature
  {
    friend class PlanAdaptor;
    friend std::ifstream& operator>>( std::ifstream& ifs, Feature& ft );
    friend std::ofstream& operator<<( std::ofstream& ofs, const Feature& ft );
    
  public:
    /// Constructor.
    Feature() :is_required(false), is_exclusive(false), is_static(true) {}
    /// Returns the x and the y position of this feature.
    virtual void getFeaturePos(float& x, float& y) const = 0;
    /** Compares two features. 
     * @return The similarity of this feature to the other feature represented by a single number.
     * Should be between -1 and 1 (~probability).
    * The sign of the similarity codes the force type. If negative, it means a repelling feature,
    * when positive, they are attractive.
    */
    virtual float compare(const Feature& ft) const = 0;

    // public members - could be get()/set() but they never intended to be very complicated.
    
    /// stores whether the feature is required to match (F_R)
    bool is_required;
    /// stores whether the feature is mutually exclusive (F_11)
    bool is_exclusive;
    
  protected:
    /// stores whether the feature will possibly be deleted often during execution (for caching)
    bool is_static;

  };
  
/// plain 2D feature class having a coordinate stored
class PlanarFeature : public Feature
  {
    friend std::ifstream& operator>>( std::ifstream& ifs, PlanarFeature& ft );
    friend std::ofstream& operator<<( std::ofstream& ofs, const PlanarFeature& ft );

  public:
    PlanarFeature() {}
    virtual void getFeaturePos(float& x, float& y) const { x = m_xPos; y = m_yPos; } 
    virtual void setFeaturePos(const float& x, const float& y) { m_xPos = x; m_yPos = y; } 
    
  protected:
    float m_xPos, m_yPos;
  };
 
/// plain agent feature class storing the agent type. This is used when an earlier scenario is utilized.
class AgentFeature : public PlanarFeature
  {
    friend std::ifstream& operator>>( std::ifstream& ifs, AgentFeature& ft );
    friend std::ofstream& operator<<( std::ofstream& ofs, const AgentFeature& ft );

public:
    // creates an agent feature from a unit
    AgentFeature() : m_Unit(0) {}
    explicit AgentFeature(Unit& unit);
    explicit AgentFeature(UnitType type) : agent_type(type) {}    
    virtual float compare(const Feature& ft) const;
    
    // public members
    UnitType agent_type;
    UnitSide agent_side;
    Unit* m_Unit;
    // this stores its plan features (if any)
    std::vector<PlanFeature*> plan;
    std::string id;
  };

/// plain terrain feature class storing the environmental feature type
class TerrainFeature: public PlanarFeature
  {
    friend std::ifstream& operator>>( std::ifstream& ifs, TerrainFeature& ft );
    friend std::ofstream& operator<<( std::ofstream& ofs, const TerrainFeature& ft );

  public:
    TerrainFeature() {}
    explicit TerrainFeature(unsigned long int layer_bits) : attr(layer_bits) {}
    virtual float compare(const Feature& ft) const;

    // public members
    unsigned long int attr;
    virtual float getHeight() const { return m_Height; }
    virtual void setHeight(float height) { m_Height = height; }

  protected:
    float m_Height;
  };

  /// plain plan feature class storing the plan feature type
  class PlanFeature: public PlanarFeature
  {
    friend std::ifstream& operator>>( std::ifstream& ifs, PlanFeature& ft );
    friend std::ofstream& operator<<( std::ofstream& ofs, const PlanFeature& ft );

  public:
    PlanFeature() {}
    PlanFeature(UnitType type, UnitSide side) {}    
    virtual float compare(const Feature& ft) const;

    // public members
    std::string owner_id;
    AgentFeature* owner;
  };
       
/// reads in a set of features (it allocates the features on-the-fly)
std::ifstream& operator>>( std::ifstream& ifs, std::set<Feature*>& features );
/// writes out a set of features
std::ofstream& operator<<( std::ofstream& ifs, const std::set<Feature*>& features );

#endif /*FEATURE_H_*/
