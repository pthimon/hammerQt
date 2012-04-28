#ifndef PLANCOMBINER_H_
#define PLANCOMBINER_H_

#include <vector> 
#include <list> 
#include <set> 

#include "../common.h" 
#include "../terrain/terrain.h" 

typedef std::set<unsigned> set_type;
typedef std::set<set_type> powerset_type;
 
class DT_CORE_EXPORT PlanCombiner : public dtCore::Base                                     
{      
        DECLARE_MANAGEMENT_LAYER(PlanCombiner)
        
public:
    PlanCombiner();
    ~PlanCombiner();    
    
    /** Generate path combinations for demo.
     */
    void CombinePlansDemo();
    /** Animate path combinations for demo.
     */
    void AnimatePlansDemo();
    /** Generate random plans.
     */
    void GeneratePlans();
    /** Greedy selection of plans.
     */
    void GreedySelect();
    /** Local improvements.
     */
    void LocalImprovement();
    
protected:
  powerset_type powerset(set_type const& set);
  void highlight_plan( unsigned plan, agg::rgba color, float width = 0);
  void unhighlight_plan(unsigned plan);
  /** reset animation phase to default.
   */
  void ResetPrimitiveProperties();
  /** Hide all primitives.
   */
  void HidePrimitives();
  
  unsigned int m_DemoPhase;
  unsigned int m_NumPlans;
  double m_PathNoiseFactor;
  double m_MaxSpeed;  
  unsigned m_AnimPlanPhase1;
  unsigned m_SolutionPhase;
  unsigned m_FrameCount;
  
  // value of plans
  std::vector< double> value_per_plan;
  // set graph storing for each plan its neighbours
  // (plans which it shares at least one agent with)
  std::vector< std::vector<unsigned> > set_graph;

  // stores the found solutions, first is always the greedy one
  std::vector< std::vector<unsigned> > solutions;
  // stores the total cost of solutions
  std::vector< double > solution_costs;

  std::vector<set_type> removed_sets;
  std::vector<set_type> added_sets;
  
  // list of handles for each plan
  std::vector< std::vector<unsigned> > handles_per_plan;
  // list of handles for each unit
  std::vector< std::vector<unsigned> > handles_per_unit;
  // list of plans for each unit
  std::vector< std::vector<unsigned> > plans_per_unit;
  
  std::map<unsigned, agg::rgba> color_per_handle;
  };

#endif /*PLANCOMBINER_H_*/
