#include "capture_imu.h"

namespace wolf {


CaptureIMU::CaptureIMU(const TimeStamp& _init_ts, const TimeStamp& _final_ts, SensorBase* _sensor_ptr,
                             const Eigen::Vector6s& _data) :
        CaptureMotion("IMU", _init_ts, _final_ts, _sensor_ptr, _data)
{
    //
}

CaptureIMU::CaptureIMU(const TimeStamp& _init_ts, const TimeStamp& _final_ts, SensorBase* _sensor_ptr,
                             const Eigen::Vector6s& _data, const Eigen::Matrix<Scalar,6,3>& _data_covariance) :
        CaptureMotion("IMU", _init_ts, _final_ts, _sensor_ptr, _data, _data_covariance)
{
    //
}

CaptureIMU::~CaptureIMU()
{
    //std::cout << "Destroying CaptureIMU capture...\n";
}

inline void CaptureIMU::process()
{

}

Eigen::VectorXs CaptureIMU::computeFramePose(const TimeStamp& _now) const
{


}

void CaptureIMU::addConstraints()
{

}

void CaptureIMU::integrateCapture(CaptureMotion* _new_capture)
{

}

CaptureIMU* CaptureIMU::interpolateCapture(const TimeStamp& _ts)
{

}

} //namespace wolf

