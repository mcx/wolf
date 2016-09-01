#ifndef CONSTRAINT_IMU_THETA_H_
#define CONSTRAINT_IMU_THETA_H_

//Wolf includes
#include "constraint_sparse.h"
#include "feature_imu.h"
#include "frame_imu.h"

namespace wolf {

class ConstraintIMU : public ConstraintSparse<9, 3, 4, 3, 3, 3, 3, 4, 3>
{
    public:
        static const unsigned int N_BLOCKS = 8;

        ConstraintIMU(FeatureIMU* _ftr_ptr, FrameIMU* _frame_ptr, bool _apply_loss_function = false,
                      ConstraintStatus _status = CTR_ACTIVE);

        /** \brief Default destructor (not recommended)
         *
         * Default destructor (please use destruct() instead of delete for guaranteeing the wolf tree integrity)
         *
         **/
        virtual ~ConstraintIMU();

        template<typename T>
        bool operator ()(const T* const _p1, const T* const _o1, const T* const _v1, const T* const _b_a1, const T* _b_g1,
                         const T* const _p2, const T* const _o2, const T* const _v2,
                         T* _residuals) const;

        /** \brief Returns the jacobians computation method
         **/
        virtual JacobianMethod getJacobianMethod() const;

    public:
        static wolf::ConstraintBase* create(FeatureIMU* _feature_ptr, NodeBase* _correspondant_ptr);

    private:
        // Helper functions, TODO: implement them all from Sola-16. Use stored biases, jacobians, and all state blocks, as internal variables. Change the API at your will, this is just a suggestion.
        template<typename T>
        Eigen::Matrix<T, 10, 1> predictDelta();

        template<typename T>
        Eigen::Matrix<T, 10, 1> correctDelta(const Eigen::Matrix<T, 10, 1>& _delta);

        template<typename T>
        Eigen::Matrix<T, 10, 1> deltaMinusDelta(const Eigen::Matrix<T, 10, 1>& _delta1, const Eigen::Matrix<T, 10, 1>& _delta2);

        template<typename T>
        Eigen::Matrix<T, 9, 1> minimalDeltaError(const Eigen::Matrix<T, 10, 1>& _delta_error);


};

inline ConstraintIMU::ConstraintIMU(FeatureIMU* _ftr_ptr, FrameIMU* _frame_ptr, bool _apply_loss_function,
                                    ConstraintStatus _status) :
        ConstraintSparse<9, 3, 4, 3, 3, 3, 3, 4, 3>(CTR_IMU, _frame_ptr, _apply_loss_function, _status,
                                                    _frame_ptr->getPPtr(), _frame_ptr->getOPtr(), _frame_ptr->getVPtr(),
                                                    _frame_ptr->getBAPtr(), _frame_ptr->getBGPtr(),
                                                    _ftr_ptr->getFramePtr()->getPPtr(),
                                                    _ftr_ptr->getFramePtr()->getOPtr(),
                                                    _ftr_ptr->getFramePtr()->getVPtr())
{
    setType("IMU");
}

inline ConstraintIMU::~ConstraintIMU()
{
    //
}

template<typename T>
inline bool ConstraintIMU::operator ()(const T* const _p1, const T* const _o1, const T* const _v1, const T* const _ba, const T* _bg,
                                       const T* const _p2, const T* const _o2, const T* const _v2,
                                       T* _residuals) const
{
    // MAPS
    Eigen::Map<Eigen::Matrix<T,9,1> > residuals_map(_residuals);

    Eigen::Map<const Eigen::Matrix<T,3,1> > p1_map(_p1);
    Eigen::Map<const Eigen::Quaternion<T> > q1_map(_o1);
    Eigen::Map<const Eigen::Matrix<T,3,1> > v1_map(_v1);
    Eigen::Map<const Eigen::Matrix<T,3,1> > ba_map(_ba);
    Eigen::Map<const Eigen::Matrix<T,3,1> > bg_map(_bg);

    Eigen::Map<const Eigen::Matrix<T,3,1> > p2_map(_p2);
    Eigen::Map<const Eigen::Quaternion<T> > q2_map(_o2);
    Eigen::Map<const Eigen::Matrix<T,3,1> > v2_map(_v2);

    Eigen::Matrix<T, 9, 1> expected_measurement;

    // Expected measurement
    

    // Residual
    residuals_map = expected_measurement - getMeasurement().cast<T>();

    // TODO: Something like this:
    // residual = minimal ( Delta_corrected (-) Delta_predicted )
    // residuals_map = minimalDeltaError( deltaMinusDelta( correctDelta( getMeasurement() ), predictDelta() ) );

    return true;
}

inline JacobianMethod ConstraintIMU::getJacobianMethod() const
{
    return JAC_AUTO;
}

inline wolf::ConstraintBase* ConstraintIMU::create(FeatureIMU* _feature_ptr, NodeBase* _correspondant_ptr)
{
    return new ConstraintIMU(_feature_ptr, (FrameIMU*)(_correspondant_ptr));
}

} // namespace wolf

#endif
