#include <iostream>
#include <fstream>
#include <cstdio>
#include <dirent.h>

#include <dtCore/globals.h>
#include <dtUtil/log.h>

#include <boost/random.hpp>

#include "PlanAdaptor.h"

using namespace boost::numeric::ublas;

IMPLEMENT_MANAGEMENT_LAYER(PlanAdaptor)

PlanAdaptor::PlanAdaptor(std::string database_name, std::string grid_name)
  {
    // give the parameters their default value

    // distortion energy coefficient
    m_Lambda = 10;
    // exponent of the support size
    m_Tau = 2;
    // initial support size of the robust estimator
    m_SigmaNull = 0.2;
    // decay rate of the support
    m_RhoNull = 0.9;
    // target decay rate
    m_RhoMax = 0.95;
    // stopping condition: unconditionally stop if below
    m_SigmaMin = 0.05;
    // stopping condition: may conditionally stop if below
    m_SigmaMax = 0.007;
    // maximum number of triangles allowed to change orientation
    m_MaxTriangleChange = 100;
    // if true, keep the initial assignments between iterations
    m_KeepInitialAssignment = false;

    std::ostringstream msg;

    // load the grid matrix

    std::string grid_matrix_file = database_name + "/grid_matrix" + grid_name
        + ".txt";
    std::string grid_matrix_file_path =
        dtCore::FindFileInPathList(grid_matrix_file);

    if (grid_matrix_file_path.empty())
      {
        LOG_WARNING("Could not find grid matrix file to load. Missing file name is """ + grid_matrix_file + """." );
      }

    // open the file for reading
    std::ifstream grid_matrix_stream(grid_matrix_file_path.c_str(),
        std::ios::in);

    if (!grid_matrix_stream.good())
      {
        msg.clear();
        msg << "Error while loading grid file: " << grid_matrix_file
            << std::endl;
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg.str(),
            dtUtil::Log::LOG_ERROR);
      }

    grid_matrix_stream >> m_GridK;

    grid_matrix_stream.close();

    // grid matrix loaded

    // load all template plans

    std::string database_path = dtCore::FindFileInPathList(database_name);

    if (database_path.empty())
      {
        LOG_WARNING("Could not find plan database to load.  Missing directory name is """ + database_path + """." );
      }

    // walk through the directory entries
    DIR* database_dir;
    struct dirent *dir;
    database_dir = opendir(database_path.c_str());
    if (database_dir)
      {
        while ((dir = readdir(database_dir)) != NULL)
          {
            std::string entry_name(dir->d_name);

            // filter the entries to get the plan name
            if (entry_name.substr(0, 4) == "plan") // it starts with the text "plan"
              {
                // it is a plan description file, read it into a template_plan_data structure

                // open the file for reading
                std::ifstream template_plan_stream(entry_name.c_str(),
                    std::ios::in);

                if (!template_plan_stream.good())
                  {
                    msg.clear();
                    msg << "Error while loading template plan file:"
                        << entry_name << std::endl;
                    dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg.str(),
                        dtUtil::Log::LOG_ERROR);
                  }

                template_plan_data temp_data;

                template_plan_stream >> temp_data.level;
                template_plan_stream >> temp_data.value;
                template_plan_stream >> temp_data.average_height;
                template_plan_stream >> temp_data.bcc;
                template_plan_stream >> temp_data.features;

                template_plan_stream.close();
              }
          }
        closedir(database_dir);
      }
    else
      {
        LOG_ERROR("Cannot open database directory.");
      }
  }

PlanAdaptor::~PlanAdaptor()
  {
    // delete all features created for the templates (other features are the responsibility of others)
    std::set<Feature*>::iterator it;
    for (unsigned i = 0; i<m_Templates.size(); i++)
      {
        for (it = m_Templates[i].features.begin(); it
            != m_Templates[i].features.end(); it++)
          {
            delete *it;
          }
      }

    DeregisterInstance(this);
  }

unsigned PlanAdaptor::AdaptPlans(std::set<Feature*>& dynamic_features)
  {
    // reset the comparison cache to the static values
    m_Cache = m_StaticCache;

    // mark these new features as non-static
    SetStaticBitTo(dynamic_features, false);

    // join all features into one single set
    std::set<Feature*> target_features = JoinFeatureSet(*m_StaticTargetFeatures,
        dynamic_features);

    bool success;
    std::set< std::pair<Feature*,Feature*> > assignment;

    for (unsigned t = 0; t<m_Templates.size(); t++)
      {
        // create initial assignments
        // ...

        // success = Adapt(m_Templates[t].features, target_features, assignment, transformed_mesh_points, transformed_template);

        if (success)
          {
            // put the result into the m_AdaptedPlan array
            // ...
          }
      }

    return 0;
  }

void PlanAdaptor::Adapt(const std::set<Feature*>& template_features,
    const std::set<Feature*>& target_features)
  {
    // implementation of the MATLAB routine follows

    // function [X, transformed_mesh_points, transformed_template] =
    // nonrigid_match2(template, target, trimesh_grid, matching_prob, F11, FR, X_null, sigma_null, rho_null, rho_max, ...
    //                        tau, lambda, sigma_min, sigma_max, keep_X_null, verbose_level)

    // transform the points into comparable height range before comparisons
  }

const adapted_plan_data & PlanAdaptor::RetrieveAdaptedPlan(unsigned plan_index)
  {
    return m_AdaptedPlans[plan_index];
  }

void PlanAdaptor::ClearCaches()
  {
    m_Cache.clear();
    m_StaticCache.clear();
  }

float PlanAdaptor::CompareFeatures(Feature* f1, Feature* f2)
  {
    std::pair<Feature*,Feature*> key(f1, f2);

    // first check whether they are already in the cache
    std::map< std::pair<Feature*,Feature*>,float>::iterator it;

    if (f1->is_static && f2->is_static)
      {
        it = m_StaticCache.find(key);
        if (it == m_StaticCache.end()) // not found, put it into both caches
          {
            float val = f1->compare(*f2);
            m_StaticCache[key] = val;
            m_Cache[key] = val;
            return val;
          }
        else
          {
            return it->second;
          }
      }

    // one of the features is dynamic, store it in the regular cache only
    it = m_Cache.find(key);
    if (it == m_Cache.end()) // not found, place it
      {
        float val = f1->compare(*f2);
        m_Cache[key] = val;
        return val;
      }
    else
      {
        return it->second;
      }
  }

void PlanAdaptor::SetStaticBitTo(std::set<Feature*>& features, bool set_static_to)
  {
    std::set<Feature*>::iterator it;

    for (it = features.begin(); it != features.end(); it++)
      {
        (*it)->is_static = set_static_to;
      }
  }

void PlanAdaptor::SetStaticTargetFeatures(std::set<Feature*>& features)
  {
    SetStaticBitTo(features, true);
    m_StaticTargetFeatures = &features;
  }

std::set<Feature*> PlanAdaptor::JoinFeatureSet(const std::set<Feature*>& fset1,
    const std::set<Feature*>& fset2)
  {
    std::set<Feature*> new_set(fset1);
    new_set.insert(fset2.begin(), fset2.end());
    return new_set;
  }

void PlanAdaptor::AddToFeatureSet(std::set<Feature*>& fset1,
    const std::set<Feature*>& fset2)
  {
    fset1.insert(fset2.begin(), fset2.end());
  }

// helper function to read in a sparse matrix
template<class T> std::ifstream& operator>>(std::ifstream& ifs,
    compressed_matrix<T>& matrix)
  {
    unsigned int xsize, ysize, nonzeros;
    unsigned x_pos, y_pos;
    T value;

    ifs >> xsize;
    ifs >> ysize;
    ifs >> nonzeros;
    matrix.resize(xsize, ysize, false);

    for (unsigned i = 0; i< nonzeros; i++)
      {
        ifs >> x_pos;
        ifs >> y_pos;
        ifs >> value;
        matrix(x_pos-1, y_pos-1) = value;

        if (!ifs.good())
          {
            LOG_ERROR("Error while reading compressed matrix.");
          }
      }

    return ifs;
  }
