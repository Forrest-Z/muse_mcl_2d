#include <muse_mcl_2d/scheduling/scheduler_2d.hpp>

#include <cslibs_plugins/plugin.hpp>

#include <unordered_map>

namespace muse_mcl_2d {
class Rate : public muse_mcl_2d::Scheduler2D
{
public:
    using Ptr                 = std::shared_ptr<Rate>;
    using rate_t              = cslibs_time::Rate;
    using update_t            = muse_smc::Update<StateSpaceDescription2D, cslibs_plugins_data::Data>;
    using time_priority_map_t = std::unordered_map<id_t, double>;
    using resampling_t        = muse_smc::Resampling<StateSpaceDescription2D, cslibs_plugins_data::Data>;
    using sample_set_t        = muse_smc::SampleSet<StateSpaceDescription2D>;
    using time_t              = cslibs_time::Time;
    using duration_t          = cslibs_time::Duration;
    using update_model_map_t  = std::map<std::string, UpdateModel2D::Ptr>;


    inline void setup(const update_model_map_t &,
                      ros::NodeHandle &nh) override
    {
        auto param_name = [this](const std::string &name){return name_ + "/" + name;};
        double preferred_rate = nh.param<double>(param_name("preferred_rate"), 5.0);
        resampling_period_ = duration_t(preferred_rate > 0.0 ? 1.0 / preferred_rate : 0.0);
    }

    virtual bool apply(typename update_t::Ptr     &u,
                       typename sample_set_t::Ptr &s) override
    {
        auto now = []()
        {
            return time_t(ros::Time::now().toNSec());
        };

        const time_t time_now = now();
        if(last_update_time_.isZero()) {
           last_update_time_ = time_now;
        }

        const time_t next_update_time = next_update_time_ + (last_update_time_ - time_now);

        const time_t stamp = u->getStamp();
        if(stamp >= next_update_time) {
            const time_t start = now();
            u->apply(s->getWeightIterator());
            const duration_t dur = (now() - start);
            next_update_time_ = stamp + dur;
            last_update_time_ = time_now;

            return true;
        }
        return false;
    }


    virtual bool apply(typename resampling_t::Ptr &r,
                       typename sample_set_t::Ptr &s) override
    {
        const cslibs_time::Time &stamp = s->getStamp();

        auto now = []()
        {
            return time_t(ros::Time::now().toNSec());
        };

       const time_t time_now = now();

        if(resampling_time_.isZero())
            resampling_time_ = time_now;

        auto do_apply = [&stamp, &r, &s, &time_now, this] () {
            r->apply(*s);

            resampling_time_ = time_now + resampling_period_;
            return true;
        };
        auto do_not_apply = [] () {
            return false;
        };
        return resampling_time_ < stamp ? do_apply() : do_not_apply();
    }

protected:
    time_t              next_update_time_;
    time_t              last_update_time_;
    time_t              resampling_time_;
    duration_t          resampling_period_;
};
}

CLASS_LOADER_REGISTER_CLASS(muse_mcl_2d::Rate, muse_mcl_2d::Scheduler2D)
