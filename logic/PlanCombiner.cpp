#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <dirent.h>

#include <dtCore/globals.h>
#include <dtUtil/log.h>

#include <boost/random.hpp>

#include "PlanCombiner.h"
#include "../HTAppBase.h"
#include "../units/Unit.h"

IMPLEMENT_MANAGEMENT_LAYER(PlanCombiner)

const float width_multiplier = 3;
const float min_width = 0.5;

PlanCombiner::PlanCombiner() :
  m_DemoPhase(0), m_NumPlans(25), m_PathNoiseFactor(300), m_MaxSpeed(1000),
      m_AnimPlanPhase1(0), m_SolutionPhase(0)
  {
  }

PlanCombiner::~PlanCombiner()
  {
    DeregisterInstance(this);
  }

/** Generate path combinations for demo.
 */
void PlanCombiner::CombinePlansDemo()
  {
    Terrain* terrain = theApp->getTerrain();

    std::cout << "mDemoPhase: " << m_DemoPhase << std::endl;

    if (m_DemoPhase == 0)
      {
        GeneratePlans();
        ResetPrimitiveProperties();
        terrain->RedrawPrimitives();

        m_FrameCount = 0;
        m_DemoPhase = 1;
        m_AnimPlanPhase1 = 0;
      }
    else if (m_DemoPhase == 1)
      {
        // show the greedy selection
        GreedySelect();

        // clear animation phase
        terrain->StopTransitions();
        ResetPrimitiveProperties();

        // show the greedy plan
        for (unsigned act_plan_ind = 0; act_plan_ind<solutions[0].size(); act_plan_ind++)
          {
            highlight_plan(solutions[0][act_plan_ind], agg::rgba(1, 1, 0, 1), 5
                *value_per_plan[ solutions[0][act_plan_ind] ]);
          }

        m_FrameCount = 0;
        m_DemoPhase = 2;
      }
    else if (m_DemoPhase == 2)
      {
        /*        unsigned myints[]=
         { 10, 20, 30, 40, 50 };
         set_type test_set(myints, myints+5);
         powerset_type result = powerset(test_set);
         powerset_type::iterator ps_it;
         set_type::iterator s_it;

         for (ps_it = result.begin(); ps_it != result.end(); ps_it++)
         {
         std::cout << "outgoing elements: ";
         for (s_it = ps_it->begin(); s_it != ps_it->end(); s_it++)
         {
         std::cout << *s_it << " ";
         }
         std::cout << std::endl;
         }*/

        LocalImprovement();
        set_type::iterator s_it;

        std::cout << "Total number of solutions: " << solutions.size()
            << std::endl;
        for (unsigned t = 0; t < solutions.size(); t++)
          {
            std::cout << "solution " << t << " : ";
            for (unsigned k = 0; k < solutions[t].size(); k++)
              {
                std::cout << solutions[t][k] << " ";
              }
            std::cout << ", value: " << solution_costs[t] << std::endl;
            if (t != solutions.size()-1)
              {
                std::cout << "removed sets: ";
                for (s_it = removed_sets[t].begin(); s_it != removed_sets[t].end(); s_it++)
                  {
                    std::cout << *s_it << " ";
                  }
                std::cout << std::endl;
                std::cout << "added sets: ";
                for (s_it = added_sets[t].begin(); s_it != added_sets[t].end(); s_it++)
                  {
                    std::cout << *s_it << " ";
                  }
                std::cout << std::endl;
              }
          }

        m_FrameCount = 0;
        m_SolutionPhase = 0;
        m_DemoPhase = 3;
      }
    else if (m_DemoPhase == 3)
      {
        terrain->StopTransitions();
        HidePrimitives();
        terrain->RedrawPrimitives();
        m_FrameCount = 0;
        // show CoAs
        m_SolutionPhase = solutions.size()-1;
        m_DemoPhase = 4;
      }
    else if (m_DemoPhase == 4)
      {
        terrain->StopTransitions();

        // delete everything and start over
        for (unsigned act_plan = 0; act_plan < handles_per_plan.size(); act_plan++)
          {
            for (unsigned act_handle_ind = 0; act_handle_ind
                < handles_per_plan[act_plan].size(); act_handle_ind++)
              {
                terrain->RemovePath(handles_per_plan[act_plan][act_handle_ind]);
              }
          }

        terrain->RedrawPrimitives();

        value_per_plan.clear();
        set_graph.clear();
        solutions.clear();
        solution_costs.clear();
        handles_per_plan.clear();
        handles_per_unit.clear();
        plans_per_unit.clear();
        color_per_handle.clear();
        removed_sets.clear();
        added_sets.clear();

        m_DemoPhase = 0;
      }
  }

void PlanCombiner::AnimatePlansDemo()
  {
    Terrain* terrain = theApp->getTerrain();

    if (m_DemoPhase == 0)
      return;
    else if (m_DemoPhase == 1)
      {
        if (m_FrameCount % 45 == 0)
          {
            m_FrameCount = 0;
            highlight_plan(m_AnimPlanPhase1, agg::rgba(1, 1, 1, 1), 5
                *value_per_plan[m_AnimPlanPhase1]);

            unsigned plan_to_fade = ( (m_NumPlans + m_AnimPlanPhase1 - 1)
                % m_NumPlans);

            unhighlight_plan(plan_to_fade);

            m_AnimPlanPhase1++;
            if (m_AnimPlanPhase1 == m_NumPlans)
              m_AnimPlanPhase1 = 0;

          }
        m_FrameCount++;
      }
    else if (m_DemoPhase == 3)
      {
        // show removed vs. added sets
        if (m_FrameCount % 45 == 0)
          {
            m_FrameCount = 0;

            // do not do anything at the last phase as we were running out of removed sets and added sets
            if (m_SolutionPhase < solutions.size() - 1)
              {
                set_type::iterator s_it;

                for (s_it = removed_sets[m_SolutionPhase].begin(); s_it
                    != removed_sets[m_SolutionPhase].end(); s_it++)
                  {
                    highlight_plan(*s_it, agg::rgba(1, 1, 1, 0));
                  }
                for (s_it = added_sets[m_SolutionPhase].begin(); s_it
                    != added_sets[m_SolutionPhase].end(); s_it++)
                  {
                    highlight_plan(*s_it, agg::rgba(0, 1, 1, 1), 5
                        *value_per_plan[*s_it]);
                  }
              }
            m_SolutionPhase++;
            if (m_SolutionPhase == solutions.size())
              {
                m_SolutionPhase = 0;

                ResetPrimitiveProperties();

                // show the greedy plan again
                for (unsigned act_plan_ind = 0; act_plan_ind
                    <solutions[0].size(); act_plan_ind++)
                  {
                    highlight_plan(solutions[0][act_plan_ind], agg::rgba(1, 1,
                        0, 1), min_width + width_multiplier*value_per_plan[ solutions[0][act_plan_ind] ]);
                  }
              }
          }
        m_FrameCount++;
      }
    else if (m_DemoPhase == 4)
      {
        // show CoAs
        if (m_FrameCount % 90 == 0)
          {
            m_FrameCount = 0;

            HidePrimitives();
            terrain->RedrawPrimitives();

            for (unsigned act_plan_ind = 0; act_plan_ind
                <solutions[m_SolutionPhase].size(); act_plan_ind++)
              {
                unhighlight_plan(solutions[m_SolutionPhase][act_plan_ind]);
                // std::cout << solutions[m_SolutionPhase][act_plan_ind] << std::endl;
              }

/*            unsigned num_solutions = solutions.size();
            unsigned solution_to_fade = ( (num_solutions + m_SolutionPhase + 1)
                % num_solutions);

            for (unsigned act_plan_ind = 0; act_plan_ind
                <solutions[solution_to_fade].size(); act_plan_ind++)
              {
                highlight_plan(solutions[solution_to_fade][act_plan_ind], agg::rgba(1, 1,
                    1, 0), min_width + width_multiplier*value_per_plan[ solutions[solution_to_fade][act_plan_ind] ]);
              }*/

            if (m_SolutionPhase == 0)
              m_SolutionPhase = solutions.size()-1;
            else
              m_SolutionPhase--;
          }
        m_FrameCount++;
      }

  }

void PlanCombiner::GeneratePlans()
  {
    Terrain* terrain = theApp->getTerrain();
    std::set<Unit*> units = theApp->GetAllUnits();

    srand(1026);
    boost::mt19937 rng(1026);
    boost::normal_distribution<> normal_map(0, 1); // 0 mean, 1 sigma dist
    boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >
        boost_normal(rng, normal_map);
    boost::uniform_real<> uni_map(0, 1); // 0-1 range
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> > boost_uni(
        rng, uni_map);

    // list of handles for each plan
    handles_per_plan.resize(m_NumPlans);
    // list of handles for each unit
    handles_per_unit.resize(units.size());
    // list of plans for each unit
    plans_per_unit.resize(units.size());
    // value of plans
    value_per_plan.resize(m_NumPlans);
    color_per_handle.clear();

    std::vector<unsigned> selected_units;

    for (unsigned int act_plan = 0; act_plan < m_NumPlans; act_plan++)
      {
        // select units to participate randomly
        int num_chosen_units = random(2, HT_MIN(6,units.size()-1));
        int already_chosen = 0;
        selected_units.resize(num_chosen_units);

        while (already_chosen < num_chosen_units)
          {
            int chosen_unit = random(0, units.size());
            if (selected_units.end() == find(selected_units.begin(),
                selected_units.end(), chosen_unit))
              {
                selected_units[already_chosen] = chosen_unit;
                already_chosen++;
              }
          }

        // we have now at 2-6 (or max) unit numbers in the selected_units array for this plan
        // generate trajectories for each
        for (unsigned int act_unit = 0; act_unit<selected_units.size(); act_unit++)
          {
            unsigned unit_number = selected_units[act_unit];

            // set unit_it to the unit number-th unit
            std::set<Unit*>::iterator unit_it = units.begin(), target_it, closest_it;
            for (unsigned int j = 0; j<unit_number; j++)
              unit_it++;

            plans_per_unit[unit_number].push_back(act_plan);

            // fill path primitive defaults
            PathPrimitive new_primitive;
            new_primitive.smooth = 1;
            new_primitive.filled = false;
            new_primitive.start_marker = false;
            new_primitive.end_marker = true;
            new_primitive.closed = false;
            new_primitive.dash = NORMAL;
            new_primitive.width = 1;

            if ((*unit_it)->getSide() == RED)
              new_primitive.color = agg::rgba(1, 0, 0, 1);
            if ((*unit_it)->getSide() == BLUE)
              new_primitive.color = agg::rgba(0, 0, 1, 1);

            osg::Vec3 act_pos3D = (*unit_it)->getPosition();
            osg::Vec2 act_pos(act_pos3D.x(), act_pos3D.y());

            // point it towards the closest enemy
            osg::Vec3 closest_diff(0,0,0);
            for (target_it = units.begin(); target_it != units.end(); target_it++)
              {
                osg::Vec3 target_pos3D = (*target_it)->getPosition();
                if ((*unit_it)->getSide() != (*target_it)->getSide() )
                  {
                    osg::Vec3 diff = target_pos3D-act_pos3D;
                    if (diff.length() < closest_diff.length() || closest_diff.length() == 0)
                      {
                        closest_diff = diff;
                      }
                  }
              }

            closest_diff.normalize();

            double init_speed = m_MaxSpeed*boost_uni();
            osg::Vec2 act_speed(init_speed*closest_diff.x(), init_speed*closest_diff.y());
            osg::Vec2 act_acc(m_PathNoiseFactor*(2*boost_uni()-1), m_PathNoiseFactor*(2*boost_uni()-1));

            unsigned int num_waypoints = random(2, 8);

            for (unsigned int act_waypoint = 0; act_waypoint<num_waypoints; act_waypoint++)
              {
                new_primitive.vertices.push_back(act_pos);
                act_pos += act_speed;
                act_speed += act_acc;
                // clamp
                if (act_speed.length() > m_MaxSpeed)
                  {
                    act_speed.normalize();
                    act_speed *= m_MaxSpeed;
                  }

                act_acc.x() = m_PathNoiseFactor*boost_normal();
                act_acc.y() = m_PathNoiseFactor*boost_normal();
              }

            unsigned int handle = terrain->AddPath(new_primitive, false);
            handles_per_unit[unit_number].push_back(handle);
            handles_per_plan[act_plan].push_back(handle);
            if ((*unit_it)->getSide() == RED)
              color_per_handle[handle] = agg::rgba(1, 0, 0, 1);
            if ((*unit_it)->getSide() == BLUE)
              color_per_handle[handle] = agg::rgba(0, 0, 1, 1);
          }

        // assign a value to the plan
        value_per_plan[act_plan] = boost_uni();
      }

    // fill the set graph
    // search for intersecting plans
    // this is practically the edges of the full graph of the 'plans_per_unit[i]' set

    set_graph.resize(m_NumPlans);
    unsigned int act_unit, from_plan_ind, to_plan_ind;
    for (act_unit = 0; act_unit<plans_per_unit.size(); act_unit++)
      {
        for (from_plan_ind = 0; from_plan_ind < plans_per_unit[act_unit].size(); from_plan_ind++)
          {
            // add one entry for each other plan, leave out itself
            for (to_plan_ind = 0; to_plan_ind < plans_per_unit[act_unit].size(); to_plan_ind++)
              {
                if (to_plan_ind != from_plan_ind)
                  set_graph[ plans_per_unit[act_unit][from_plan_ind] ].push_back(plans_per_unit[act_unit][to_plan_ind]);
              }

          }
      }

    // sort entries
    for (from_plan_ind = 0; from_plan_ind < set_graph.size(); from_plan_ind++)
      {
        std::sort(set_graph[from_plan_ind].begin(),
            set_graph[from_plan_ind].end());
      }
  }

void PlanCombiner::GreedySelect()
  {
    // key is the plan index, value is the plan value
    std::map<unsigned,double> available_plans;
    std::map<unsigned,double>::iterator map_it, best_it;

    // initially add all plans
    for (unsigned i = 0; i<m_NumPlans; i++)
      {
        available_plans.insert(std::pair<unsigned, double>(i,value_per_plan[i]));
      }

    solutions.resize(1);
    solutions[0].clear();
    solution_costs.resize(1);
    solution_costs[0] = 0;

    while (!available_plans.empty() )
      {
        // add the plan with the largest value
        double best_value = -1;

        for (map_it = available_plans.begin(); map_it != available_plans.end(); map_it++)
          {
            if ( best_value < (*map_it).second )
              {
                best_value = (*map_it).second;
                best_it = map_it;
              }
          }

        unsigned plan_to_add = (*best_it).first;

        // store its index in the greedy solution
        solutions[0].push_back( plan_to_add );
        solution_costs[0] += (*best_it).second;
        // remove its neighbours
        for (unsigned t = 0; t<set_graph[plan_to_add].size(); t++)
        available_plans.erase(set_graph[plan_to_add][t]);
        // remove it from the list of available plans
        available_plans.erase(best_it);
      }

    std::sort(solutions[0].begin(), solutions[0].end());

    for (unsigned t = 0; t<solutions[0].size(); t++)
    std::cout << "greedy " << t << " : " << solutions[0][t] << std::endl;

    std::cout << "greedy total cost: " << solution_costs[0] << std::endl;
  }

void PlanCombiner::LocalImprovement()
  {
    // improve over greedy solution
    std::vector<set_type> loc_improvements;
    std::vector<set_type> loc_added_sets;
    std::vector<set_type> loc_removed_sets;
    std::vector<double> payoffs;

    // stop when no improvements can be found
    while (1)
      {
        unsigned curr_solution_index = solutions.size()-1;

        std::cout << "curr_solution: " << curr_solution_index << std::endl;

        // list all local improvements by replacing the elements with
        // independent sets of its neighbours
        loc_improvements.clear();
        loc_added_sets.clear();
        loc_removed_sets.clear();
        payoffs.clear();

        // iterate over all vertices (plans) in the actual independent set
        for (unsigned act_plan_ind = 0; act_plan_ind
            < solutions[curr_solution_index].size(); act_plan_ind++)
          {
            unsigned act_plan = solutions[curr_solution_index][act_plan_ind];

            std::cout << "act_plan: " << act_plan << std::endl;

            // the neighbour set of this plan is listed in set_graph[act_plan]
            // search for independent sets in this set

            // first list all subsets of this set
            set_type act_nbrs(set_graph[act_plan].begin(),
                set_graph[act_plan].end() );

            std::cout << "neighbours length: " << act_nbrs.size() << std::endl;

            powerset_type subsets = powerset(act_nbrs);
            powerset_type::iterator ps_it;
            set_type::iterator s_it, s_it2;

            std::cout << "returned subsets: " << subsets.size() << std::endl;

            // check for independence in all subsets
            for (ps_it = subsets.begin(); ps_it != subsets.end(); ps_it++)
              {
                bool is_independent = true;
                for (s_it = ps_it->begin(); s_it != ps_it->end(); s_it++)
                  {
                    // check whether this element is occurring in any nbs of the other elements
                    for (s_it2 = ps_it->begin(); s_it2 != ps_it->end(); s_it2++)
                      {
                        if (std::find(set_graph[*s_it2].begin(),
                            set_graph[*s_it2].end(), *s_it)
                            != set_graph[*s_it2].end() )
                          {
                            // found
                            is_independent = false;
                            break;
                          }
                      }
                    if (!is_independent)
                      break;
                  }

                // std::cout << "found independent: " << is_independent << std::endl;

                if (is_independent)
                  {
                    // initialize with the original
                    set_type modified_set(
                        solutions[curr_solution_index].begin(),
                        solutions[curr_solution_index].end());

                    set_type added_set, removed_set;

                    // calculate this independent set's total value (w(Q))
                    double added_value = 0;
                    double removed_value = 0;

                    for (s_it = ps_it->begin(); s_it != ps_it->end(); s_it++)
                      {
                        added_value += value_per_plan[*s_it];

                        added_set.insert(*s_it);

                        // remove conflicting neighbours from the original set
                        // count the removed vertices' total sum

                        for (unsigned t = 0; t < set_graph[*s_it].size(); t++)
                          {
                            if (modified_set.erase(set_graph[*s_it][t]) == 1)
                              {
                                removed_value
                                    += value_per_plan[ set_graph[*s_it][t] ];

                                removed_set.insert(set_graph[*s_it][t]);
                              }
                          }
                      }
                    // add the new sets
                    for (s_it = ps_it->begin(); s_it != ps_it->end(); s_it++)
                      {
                        modified_set.insert(*s_it);
                      }

                    // calculate payoff factor
                    double payoff = added_value/removed_value;

                    std::cout << "added: " << added_value <<", removed: "
                        << removed_value << std::endl;

                    if (payoff > 1)
                      {
                        std::cout << "improvement found with payoff: "
                            << payoff << std::endl;
                        loc_improvements.push_back(modified_set);
                        payoffs.push_back(payoff);
                        loc_added_sets.push_back(added_set);
                        loc_removed_sets.push_back(removed_set);
                      }

                  }

              }
          }

        // if all payoff factors are <= 1, end
        if (loc_improvements.empty())
          {
            std::cout << "total improvements found: " << solutions.size()-1
                << std::endl;
            break;
          }

        std::vector<double>::iterator max_payoff_it = std::max_element(
            payoffs.begin(), payoffs.end());
        int max_payoff_ind = std::distance(payoffs.begin(), max_payoff_it);

        std::cout << "improvement selected: " << max_payoff_ind << std::endl;

        // store this improvement as the next step
        std::vector<unsigned> vec_form(loc_improvements[max_payoff_ind].begin(),
            loc_improvements[max_payoff_ind].end());

        solutions.push_back(vec_form);
        added_sets.push_back(loc_added_sets[max_payoff_ind]);
        removed_sets.push_back(loc_removed_sets[max_payoff_ind]);

        // calculate total cost
        double total_cost = 0;
        for (unsigned t = 0; t< vec_form.size(); t++)
          {
            total_cost += value_per_plan[ vec_form[t] ];
          }
        solution_costs.push_back(total_cost);
      }
  }

void PlanCombiner::ResetPrimitiveProperties()
  {
    Terrain* terrain = theApp->getTerrain();

    for (unsigned act_plan = 0; act_plan<m_NumPlans; act_plan++)
      {
        for (unsigned int act_handle_index = 0; act_handle_index
            < handles_per_plan[act_plan].size(); act_handle_index++)
          {
            PathPrimitive & target_path =
                terrain->GetPath(handles_per_plan[act_plan][act_handle_index]);
            target_path.color
                = color_per_handle[handles_per_plan[act_plan][act_handle_index]];
            target_path.width = min_width + width_multiplier*value_per_plan[ act_plan ];
          }
      }
  }

void PlanCombiner::HidePrimitives()
  {
    Terrain* terrain = theApp->getTerrain();

    for (unsigned act_plan = 0; act_plan<m_NumPlans; act_plan++)
      {
        for (unsigned int act_handle_index = 0; act_handle_index
            < handles_per_plan[act_plan].size(); act_handle_index++)
          {
            PathPrimitive & target_path =
                terrain->GetPath(handles_per_plan[act_plan][act_handle_index]);
            target_path.color
                = color_per_handle[handles_per_plan[act_plan][act_handle_index]];
            target_path.color.a = 0;
            target_path.width = min_width + width_multiplier*value_per_plan[ act_plan ];
          }
      }
  }

powerset_type PlanCombiner::powerset(set_type const& set)
  {
    typedef set_type::const_iterator set_iter;
    typedef std::vector<set_iter> vec;
    typedef vec::iterator vec_iter;

    struct local
      {
      static int dereference(set_iter v)
        {
          return *v;
        }
      };

    powerset_type result;

    vec elements;
    do
      {
        set_type tmp;
        std::transform(elements.begin(), elements.end(), std::inserter(tmp,
            tmp.end()), local::dereference);
        result.insert(tmp);
        if (!elements.empty() && ++elements.back() == set.end())
          {
            elements.pop_back();
          }
        else
          {
            set_iter iter;
            if (elements.empty())
              {
                iter = set.begin();
              }
            else
              {
                iter = elements.back();
                ++iter;
              }
            for (; iter != set.end(); ++iter)
              {
                elements.push_back(iter);
              }
          }
      } while (!elements.empty());

    return result;
  }

void PlanCombiner::highlight_plan(unsigned plan, agg::rgba color, float width)
  {
    Terrain* terrain = theApp->getTerrain();
    for (unsigned int act_handle_index = 0; act_handle_index
        < handles_per_plan[plan].size(); act_handle_index++)
      {
        PathPrimitive target_path =
            terrain->GetPath(handles_per_plan[plan][act_handle_index]);
        target_path.color = color;
        if (width != 0)
          target_path.width = width;
        terrain->TransitionPrimitives(handles_per_plan[plan][act_handle_index],
            target_path);
      }
  }

void PlanCombiner::unhighlight_plan(unsigned plan)
  {
    Terrain* terrain = theApp->getTerrain();
    for (unsigned int act_handle_index = 0; act_handle_index
        < handles_per_plan[plan].size(); act_handle_index++)
      {
        PathPrimitive target_path =
            terrain->GetPath(handles_per_plan[plan][act_handle_index]);
        target_path.color
            = color_per_handle[handles_per_plan[plan][act_handle_index]];
        target_path.width = min_width + width_multiplier*value_per_plan[ plan ];
        terrain->TransitionPrimitives(handles_per_plan[plan][act_handle_index],
            target_path);
      }
  }
