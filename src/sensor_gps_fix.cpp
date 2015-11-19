#include "sensor_gps_fix.h"

SensorGPSFix::SensorGPSFix(StateBase* _p_ptr, StateBase* _o_ptr, const double& _noise) :
        SensorBase(GPS_FIX, _p_ptr, _o_ptr, Eigen::VectorXs::Constant(1,_noise))
{
    //
}

SensorGPSFix::~SensorGPSFix()
{
    //
}

WolfScalar SensorGPSFix::getNoise() const
{
    return params_(0);
}
