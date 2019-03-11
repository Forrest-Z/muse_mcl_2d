#include <muse_mcl_2d_ndt/providers/ndt_gridmap_2d_provider.h>

#include <cslibs_ndt_2d/serialization/dynamic_maps/gridmap.hpp>
#include <cslibs_ndt_2d/conversion/probability_gridmap.hpp>
#include <cslibs_gridmaps/static_maps/conversion/convert_probability_gridmap.hpp>
#include <cslibs_gridmaps/static_maps/algorithms/normalize.hpp>
#include <fstream>
#include <yaml-cpp/yaml.h>

#include <class_loader/class_loader_register_macro.h>
CLASS_LOADER_REGISTER_CLASS(muse_mcl_2d_ndt::NDTGridmap2dProvider, muse_mcl_2d::MapProvider2D)

namespace muse_mcl_2d_ndt {
NDTGridmap2dProvider::state_space_t::ConstPtr NDTGridmap2dProvider::getStateSpace() const
{
    std::unique_lock<std::mutex> l(map_mutex_);
    return map_;
}

void NDTGridmap2dProvider::waitForStateSpace() const
{
  std::unique_lock<std::mutex> l(map_mutex_);
  if (!map_)
      map_notify_.wait(l);
}

void NDTGridmap2dProvider::setup(ros::NodeHandle &nh)
{
    auto param_name = [this](const std::string &name){return name_ + "/" + name;};

    path_     = nh.param<std::string>(param_name("path"), "");
    frame_id_ = nh.param<std::string>(param_name("frame_id"), "/world");

    auto load = [this]() {
        ROS_INFO_STREAM("Loading file '" << path_ << "'...");
        cslibs_ndt_2d::dynamic_maps::Gridmap<double>::Ptr map;
        if (cslibs_ndt_2d::dynamic_maps::loadBinary<double>(path_, map)) {
            std::unique_lock<std::mutex> l(map_mutex_);
            map_.reset(new Gridmap2d(map, frame_id_));
            l.unlock();
            ROS_INFO_STREAM("Successfully loaded file '" << path_ << "'!");
        } else
            throw std::runtime_error("Could not load file '" + path_ + "'!");

        map_notify_.notify_all();
    };
    worker_ = std::thread(load);
}
}
