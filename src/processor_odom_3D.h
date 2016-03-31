/**
 * \file processor_odom_3D.h
 *
 *  Created on: Mar 18, 2016
 *      \author: jsola
 */

#ifndef SRC_PROCESSOR_ODOM_3D_H_
#define SRC_PROCESSOR_ODOM_3D_H_

#include "processor_motion2.h"

// Declare the Motion Delta type as a composite struct
typedef struct{
        Eigen::Vector3s dp;
        Eigen::Quaternions dq;
}Odo3dDeltaType;



class ProcessorOdom3d : public ProcessorMotion2<Odo3dDeltaType>
{
    public:
        ProcessorOdom3d(WolfScalar _delta_t) : ProcessorMotion2(PRC_ODOM_3D, _delta_t, 7, 6) {}
        virtual ~ProcessorOdom3d(){}
        virtual void data2delta(const Eigen::VectorXs& _data, Odo3dDeltaType& _delta);

    private:
        void xPlusDelta(const Eigen::VectorXs& _x, const Odo3dDeltaType& _delta, Eigen::VectorXs& _x_plus_delta);
        void deltaPlusDelta(const Odo3dDeltaType& _delta1, const Odo3dDeltaType& _delta2, Odo3dDeltaType& _delta1_plus_delta2);
        virtual void deltaMinusDelta(const Odo3dDeltaType& _delta1, const Odo3dDeltaType& _delta2,
                                     Odo3dDeltaType& _delta2_minus_delta1);
        Odo3dDeltaType deltaZero()
        {
            return Odo3dDeltaType{Eigen::Vector3s::Zero(),Eigen::Quaternions::Identity()};
        }

    private:
        Eigen::Quaternions quat1_;
};


inline void ProcessorOdom3d::data2delta(const Eigen::VectorXs& _data, Odo3dDeltaType& _delta)
{
    // Trivial implementation
    _delta.dp = _data.head(3);
    Eigen::v2q(_data.tail(3), _delta.dq);
}



// Compose a frame with a delta frame and give a frame (sizes 7, 6, 7 respectively)
inline void ProcessorOdom3d::xPlusDelta(const Eigen::VectorXs& _x, const Odo3dDeltaType& _delta, Eigen::VectorXs& _x_plus_delta)
{
    assert(_x.size() == 7 && "Wrong _x vector size");
    assert(_x_plus_delta.size() == 7 && "Wrong _x_plus_delta vector size");

    quat1_ = Eigen::Map<const Eigen::Quaternions>(&_x(3));

    _x_plus_delta.head(3) = _x.head(3) + quat1_*_delta.dp;
    _x_plus_delta.tail(4) = (quat1_*_delta.dq).coeffs();
}

// Compose two delta-frames and give a delta-frame (all of size 6)
inline void ProcessorOdom3d::deltaPlusDelta(const Odo3dDeltaType& _delta1, const Odo3dDeltaType& _delta2, Odo3dDeltaType& _delta1_plus_delta2)
{
    _delta1_plus_delta2.dp = _delta1.dp + _delta1.dq*_delta2.dp;
    _delta1_plus_delta2.dq = _delta1.dq * _delta2.dq;
}

inline void ProcessorOdom3d::deltaMinusDelta(const Odo3dDeltaType& _delta1, const Odo3dDeltaType& _delta2,
                                             Odo3dDeltaType& _delta2_minus_delta1)
{
    _delta2_minus_delta1.dp = _delta1.dq.conjugate() * (_delta2.dp - _delta1.dp);
    _delta2_minus_delta1.dq = _delta1.dq.conjugate() * _delta2.dq;
}

#endif /* SRC_PROCESSOR_ODOM_3D_H_ */
