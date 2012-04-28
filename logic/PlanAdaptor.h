#ifndef PLANADAPTOR_H_
#define PLANADAPTOR_H_

#include <vector> 
#include <list> 
#include <set> 
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "../common.h" 
#include "Feature.h" 

typedef std::vector< std::pair<Unit*,std::list<PlanFeature> > > adapted_plan;

struct template_plan_data
{
  // the features in the plan
  std::set<Feature*> features;
  // the baricentric coordinate matrix
  boost::numeric::ublas::compressed_matrix<double> bcc;
  // average height of the template features
  float average_height;
  // the experienced value
  float value;
  // level of segmentation
  unsigned level;
};

struct adapted_plan_data
{
  // this is the plan itself
  adapted_plan plan;
  // this is the deformation measure which can be used as a quality of fit (energy) 
  float deformation;
  // this is the original template plan's data
  template_plan_data* template_plan;
};

class DT_CORE_EXPORT PlanAdaptor : public dtCore::Base                                     
{      
        DECLARE_MANAGEMENT_LAYER(PlanAdaptor)
        
public:
    /** Constructor. Accepts a file name storing the database for plan adaptation.
     */
    PlanAdaptor(std::string database_name = "logic", std::string grid_name = "0.03");
    /** Destructor. Clears all features allocated by this class.
     */
    ~PlanAdaptor();    
    
    /** Sets the static target features (like environmental features),
     * which do not change between calls to plan adaptation requests.
     */
    void SetStaticTargetFeatures(std::set<Feature*>& features);
    
    /** Adapts all the plans to the current situation.
     * Dynamic features are features who may change between calls to this functions (like agents), so
     * their comparison cache is not re-used.
     * @return The total number of adapted plans. 
     */
    unsigned AdaptPlans(std::set<Feature*>& dynamic_target_features);
    
    /** this is the core function for the adaptation. You may call it with providing the features, but
     * usually one will use AdaptPlans instead.
     */
    void Adapt(const std::set<Feature*>& template_features, const std::set<Feature*>& target_features);
    
    /** gives plan coordinates for each agent in the given adapted plan.
     * return A vector of pairs where the first member is the agent, the second is a list of PlanFeatures.
     */
    const adapted_plan_data & RetrieveAdaptedPlan(unsigned plan_index);
    
    /** Clears the cached comparison values so frees up memory. Comparisons are re-created at the next call.
     */
    void ClearCaches();
    
    /** Compare two features by utilising the cache.
     */
    float CompareFeatures(Feature* first, Feature* second);
        
protected:
  
  /** This operates on a set of features and sets all of its features' static bit to the desired value.
   */  
  void SetStaticBitTo(std::set<Feature*>& features, bool set_static_to);

  /** This function joins two sets of features and creates a new set.
   */
  std::set<Feature*> JoinFeatureSet(const std::set<Feature*>& fset1, const std::set<Feature*>& fset2);

  /** This function adds the second set to the first. 
   */
  void AddToFeatureSet(std::set<Feature*>& fset1, const std::set<Feature*>& fset2);
  
  /** This function opens a file and handles possible problems.
   */
  
  
  // these do not vary from calls to calls, so we can keep their comparison to the (static) template features 
  std::set<Feature*>* m_StaticTargetFeatures;
  
  // we assume that the same feature instances are used between calls so we can store the cache as a map
  
  // this is the overall cache
  std::map< std::pair<Feature*,Feature*>,float> m_Cache;
  // this is the cache storing comparisons of static features, so it is not expected to change
  // (we initialize all values to it when adaptation is executed) 
  std::map< std::pair<Feature*,Feature*>,float> m_StaticCache;
  
  // these are the plans from the database
  std::vector<template_plan_data> m_Templates;

  // these are the adapted plans
  std::vector<adapted_plan_data> m_AdaptedPlans;
  
  /// distortion energy coefficient
  float m_Lambda; 
  /// exponent of the support size
  float m_Tau;
  /// initial support size of the robust estimator
  float m_SigmaNull;
  /// decay rate of the support
  float m_RhoNull;
  /// target decay rate
  float m_RhoMax;
  /// stopping condition: unconditionally stop if below
  float m_SigmaMin; 
  /// stopping condition: may conditionally stop if below
  float m_SigmaMax;
  /// maximum number of triangles allowed to change orientation
  unsigned m_MaxTriangleChange;
  /// if true, keep the initial assignments between iterations
  bool m_KeepInitialAssignment;
  /// this is the K grid force matrix
  boost::numeric::ublas::compressed_matrix<double> m_GridK;    
  };

/// reads in a compressed_matrix
template<class T> std::ifstream& operator>>( std::ifstream& ifs, boost::numeric::ublas::compressed_matrix<T>& matrix );
  
#endif /*PLANADAPTOR_H_*/
