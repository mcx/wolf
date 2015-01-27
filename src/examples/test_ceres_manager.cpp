//std includes
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <typeinfo>
#include <ctime>
#include <queue>          // std::queue

// Eigen includes
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Geometry>

//Ceres includes
#include "ceres/jet.h"
#include "ceres/ceres.h"
#include "glog/logging.h"

//Wolf includes
#include "frame_base.h"
#include "capture_base.h"
#include "wolf.h"

/**
 * This test implements an optimization using CERES of a vehicle trajectory using odometry and GPS simulated data.
 *
 **/

using namespace Eigen;
enum costFunctionType {
	AUTO,
	NUMERIC};
enum correspondenceType {
	GPS_2D,
	ODOM_2D_COMPLEX,
	ODOM_2D_THETA};
enum parametrizationType {
	NONE,
	COMPLEX_ANGLE,
	QUATERNION,
	PO_2D};
enum captureType{
	CAPT_GPS_2D,
	CAPT_ODOM_2D};

class Frame2DPO;
class CaptureXBase;
class StateBase;
class CorrespondenceXBase;
typedef std::shared_ptr<CorrespondenceXBase> CorrespondenceXShPtr;
typedef std::shared_ptr<Frame2DPO> Frame2DPOShPtr;
typedef std::shared_ptr<CaptureXBase> CaptureXShPtr;
typedef std::shared_ptr<StateBase> StateXShPtr;

class StateBase
{
	protected:
		WolfScalar* state_ptr_;

	public:

		StateBase(VectorXs& _st_remote, const unsigned int _idx) :
			state_ptr_(_st_remote.data() + _idx)
		{
		}

		StateBase(WolfScalar* _st_ptr) :
			state_ptr_(_st_ptr)
		{
		}

		StateBase(StateBase& _st) :
			state_ptr_(_st.state_ptr_)
		{
		}


		virtual ~StateBase()
		{
		}

		virtual WolfScalar* getPtr()
		{
			return state_ptr_;
		}

		virtual parametrizationType getParametrizationType() const = 0;

		virtual void print() const = 0;
};

class StatePoint2D: public StateBase
{
	public:
		static const unsigned int BLOCK_SIZE = 2;

		StatePoint2D(VectorXs& _st_remote, const unsigned int _idx) :
			StateBase(_st_remote, _idx)
		{
		}

		StatePoint2D(WolfScalar* _st_ptr) :
			StateBase(_st_ptr)
		{
		}

		virtual ~StatePoint2D()
		{
		}

		virtual parametrizationType getParametrizationType() const
		{
			return NONE;
		}

		virtual void print() const
		{
			std::cout << *this->state_ptr_ << " " << *(this->state_ptr_+1) << std::endl;
		}
};

class StateThetaAngle: public StateBase
{
	public:
		static const unsigned int BLOCK_SIZE = 1;

		StateThetaAngle(VectorXs& _st_remote, const unsigned int _idx) :
			StateBase(_st_remote, _idx)
		{
		}

		StateThetaAngle(WolfScalar* _st_ptr) :
			StateBase(_st_ptr)
		{
		}

		virtual ~StateThetaAngle()
		{
		}

		virtual parametrizationType getParametrizationType() const
		{
			return NONE;
		}

		virtual void print() const
		{
			std::cout << *this->state_ptr_ << std::endl;
		}
};

class StateComplexAngle: public StateBase
{
	public:
		static const unsigned int BLOCK_SIZE = 2;

		StateComplexAngle(VectorXs& _st_remote, const unsigned int _idx) :
			StateBase(_st_remote, _idx)
		{
		}

		StateComplexAngle(WolfScalar* _st_ptr) :
			StateBase(_st_ptr)
		{
		}

		virtual ~StateComplexAngle()
		{
		}

		virtual parametrizationType getParametrizationType() const
		{
			return COMPLEX_ANGLE;
		}

		virtual void print() const
		{
			std::cout << *this->state_ptr_ << " " << *(this->state_ptr_+1) << std::endl;
		}
};

class ComplexAngleParameterization : public ceres::LocalParameterization
{
	public:
		virtual ~ComplexAngleParameterization()
		{
		}

		virtual bool Plus(const double* x_raw, const double* delta_raw, double* x_plus_delta_raw) const
		{
			x_plus_delta_raw[0] = x_raw[0] * cos(delta_raw[0]) - x_raw[1] * sin(delta_raw[0]);
			x_plus_delta_raw[1] = x_raw[1] * cos(delta_raw[0]) + x_raw[0] * sin(delta_raw[0]);

			//normalize
			//double norm = sqrt(x_plus_delta_raw[0] * x_plus_delta_raw[0] + x_plus_delta_raw[1] * x_plus_delta_raw[1]);
			//std::cout << "(before normalization) norm = " << norm << std::endl;
			//x_plus_delta_raw[0] /= norm;
			//x_plus_delta_raw[1] /= norm;

			return true;
		}

		virtual bool ComputeJacobian(const double* x, double* jacobian) const
		{
			jacobian[0] = -x[1];
			jacobian[1] =  x[0];
			return true;
		}

		virtual int GlobalSize() const
		{
			return 2;
		}

		virtual int LocalSize() const
		{
			return 1;
		}
};

class CorrespondenceXBase
{
	protected:
		WolfScalar *measurement_ptr_;

    public:

		CorrespondenceXBase(WolfScalar * _measurement_ptr) :
        	measurement_ptr_(_measurement_ptr)
        {
        }

        virtual ~CorrespondenceXBase()
        {
        }

        virtual correspondenceType getType() const = 0;
        virtual const std::vector<WolfScalar*> getBlockPtrVector() = 0;
};

template <const unsigned int MEASUREMENT_SIZE,
				unsigned int BLOCK_0_SIZE,
				unsigned int BLOCK_1_SIZE = 0,
				unsigned int BLOCK_2_SIZE = 0,
				unsigned int BLOCK_3_SIZE = 0,
				unsigned int BLOCK_4_SIZE = 0,
				unsigned int BLOCK_5_SIZE = 0,
				unsigned int BLOCK_6_SIZE = 0,
				unsigned int BLOCK_7_SIZE = 0,
				unsigned int BLOCK_8_SIZE = 0,
				unsigned int BLOCK_9_SIZE = 0>
class CorrespondenceSparse: public CorrespondenceXBase
{
    protected:
		std::vector<WolfScalar*> state_block_ptr_vector_;
		std::vector<unsigned int> block_sizes_vector_;

    public:
		static const unsigned int measurementSize = MEASUREMENT_SIZE;
		static const unsigned int block0Size = BLOCK_0_SIZE;
		static const unsigned int block1Size = BLOCK_1_SIZE;
		static const unsigned int block2Size = BLOCK_2_SIZE;
		static const unsigned int block3Size = BLOCK_3_SIZE;
		static const unsigned int block4Size = BLOCK_4_SIZE;
		static const unsigned int block5Size = BLOCK_5_SIZE;
		static const unsigned int block6Size = BLOCK_6_SIZE;
		static const unsigned int block7Size = BLOCK_7_SIZE;
		static const unsigned int block8Size = BLOCK_8_SIZE;
		static const unsigned int block9Size = BLOCK_9_SIZE;

		CorrespondenceSparse(WolfScalar* _measurementPtr, WolfScalar** _blockPtrArray) :
        	CorrespondenceXBase(_measurementPtr),
			block_sizes_vector_({BLOCK_0_SIZE,BLOCK_1_SIZE,BLOCK_2_SIZE,BLOCK_3_SIZE,BLOCK_4_SIZE,BLOCK_5_SIZE,BLOCK_6_SIZE,BLOCK_7_SIZE,BLOCK_8_SIZE,BLOCK_9_SIZE})
        {
			for (uint i = 0; i<block_sizes_vector_.size(); i++)
			{
				if (block_sizes_vector_.at(i) == 0)
				{
					block_sizes_vector_.resize(i);
					break;
				}
				else
					state_block_ptr_vector_.push_back(_blockPtrArray[i]);
			}
        }

		CorrespondenceSparse(WolfScalar* _measurementPtr,
							 WolfScalar* _state0Ptr,
							 WolfScalar* _state1Ptr = nullptr,
							 WolfScalar* _state2Ptr = nullptr,
							 WolfScalar* _state3Ptr = nullptr,
							 WolfScalar* _state4Ptr = nullptr,
							 WolfScalar* _state5Ptr = nullptr,
							 WolfScalar* _state6Ptr = nullptr,
							 WolfScalar* _state7Ptr = nullptr,
							 WolfScalar* _state8Ptr = nullptr,
							 WolfScalar* _state9Ptr = nullptr ) :
			CorrespondenceXBase(_measurementPtr),
			state_block_ptr_vector_({_state0Ptr,_state1Ptr,_state2Ptr,_state3Ptr,_state4Ptr,_state5Ptr,_state6Ptr,_state7Ptr,_state8Ptr,_state9Ptr}),
			block_sizes_vector_({BLOCK_0_SIZE,BLOCK_1_SIZE,BLOCK_2_SIZE,BLOCK_3_SIZE,BLOCK_4_SIZE,BLOCK_5_SIZE,BLOCK_6_SIZE,BLOCK_7_SIZE,BLOCK_8_SIZE,BLOCK_9_SIZE})
		{
			for (uint i = 0; i<block_sizes_vector_.size(); i++)
			{
				if (block_sizes_vector_.at(i) == 0)
				{
					block_sizes_vector_.resize(i);
					state_block_ptr_vector_.resize(i);
					break;
				}
			}

			//TODO: Check if while size OK, pointers NULL
		}

        virtual ~CorrespondenceSparse()
        {
        }

        virtual correspondenceType getType() const = 0;

		virtual const std::vector<WolfScalar *> getBlockPtrVector()
		{
			return state_block_ptr_vector_;
		}
};

class CorrespondenceGPS2D : public CorrespondenceSparse<2,2>
{
	public:
		static const unsigned int N_BLOCKS = 1;
		const double stdev_ = 1;

		CorrespondenceGPS2D(WolfScalar* _measurementPtr, WolfScalar* _statePtr) :
			CorrespondenceSparse<2,2>(_measurementPtr, _statePtr)
		{
		}

		CorrespondenceGPS2D(WolfScalar* _measurementPtr, const StateXShPtr& _statePtr) :
			CorrespondenceSparse<2,2>(_measurementPtr, _statePtr->getPtr())
		{
		}

		virtual ~CorrespondenceGPS2D()
		{
		}

		template <typename T>
		bool operator()(const T* const _x, T* _residuals) const
		{
			_residuals[0] = (T(*this->measurement_ptr_) - _x[0]) / T(stdev_);
			_residuals[1] = (T(*(this->measurement_ptr_+1)) - _x[1]) / T(stdev_);

			return true;
		}

		virtual correspondenceType getType() const
		{
			return GPS_2D;
		}
};

class Correspondence2DOdometry : public CorrespondenceSparse<2,2,2,2,2>
{
	public:
		static const unsigned int N_BLOCKS = 4;
		const double stdev_ = 0.01; //model parameters

		Correspondence2DOdometry(WolfScalar* _measurementPtr, WolfScalar** _blockPtrs) :
			CorrespondenceSparse<2,2,2,2,2>(_measurementPtr, _blockPtrs)
		{
		}

		Correspondence2DOdometry(WolfScalar* _measurementPtr, WolfScalar* _block0Ptr, WolfScalar* _block1Ptr, WolfScalar* _block2Ptr, WolfScalar* _block3Ptr) :
			CorrespondenceSparse<2,2,2,2,2>(_measurementPtr, _block0Ptr, _block1Ptr, _block2Ptr, _block3Ptr)
		{
		}

		Correspondence2DOdometry(WolfScalar* _measurementPtr, const StateXShPtr& _state0Ptr, const StateXShPtr& _state1Ptr, const StateXShPtr& _state2Ptr, const StateXShPtr& _state3Ptr) :
			CorrespondenceSparse<2,2,2,2,2>(_measurementPtr, _state0Ptr->getPtr(), _state1Ptr->getPtr(),_state2Ptr->getPtr(), _state3Ptr->getPtr())
		{
		}

		virtual ~Correspondence2DOdometry()
		{
		}

        template <typename T>
        bool operator()(const T* const _p1, const T* const _o1, const T* const _p2, const T* const _o2, T* _residuals) const
        {
			// Expected measurement
			T expected_range = (_p1[0]-_p2[0])*(_p1[0]-_p2[0]) + (_p1[1]-_p2[1])*(_p1[1]-_p2[1]); //square of the range
			T expected_rotation = atan2(-_o1[0]*_o2[1] + _o1[1]*_o2[0], _o1[0]*_o2[0] + _o1[1]*_o2[1]);

			// Residuals
			_residuals[0] = (expected_range - T((*this->measurement_ptr_)*(*this->measurement_ptr_))) / T(stdev_);
			_residuals[1] = (expected_rotation - T(*(this->measurement_ptr_+1))) / T(stdev_);

			return true;
        }

        virtual correspondenceType getType() const
        {
        	return ODOM_2D_COMPLEX;
        }
};

class Correspondence2DOdometryTheta : public CorrespondenceSparse<2,2,1,2,1>
{
	public:
		static const unsigned int N_BLOCKS = 4;
		const double stdev_ = 0.01; //model parameters

		Correspondence2DOdometryTheta(WolfScalar* _measurementPtr, WolfScalar** _blockPtrs) :
			CorrespondenceSparse<2,2,1,2,1>(_measurementPtr, _blockPtrs)
		{
		}

		Correspondence2DOdometryTheta(WolfScalar* _measurementPtr, WolfScalar* _block0Ptr, WolfScalar* _block1Ptr, WolfScalar* _block2Ptr, WolfScalar* _block3Ptr) :
			CorrespondenceSparse<2,2,1,2,1>(_measurementPtr, _block0Ptr, _block1Ptr, _block2Ptr, _block3Ptr)
		{
		}

		Correspondence2DOdometryTheta(WolfScalar* _measurementPtr, const StateXShPtr& _state0Ptr, const StateXShPtr& _state1Ptr, const StateXShPtr& _state2Ptr, const StateXShPtr& _state3Ptr) :
			CorrespondenceSparse<2,2,1,2,1>(_measurementPtr, _state0Ptr->getPtr(), _state1Ptr->getPtr(),_state2Ptr->getPtr(), _state3Ptr->getPtr())
		{
		}

		virtual ~Correspondence2DOdometryTheta()
		{
		}

        template <typename T>
        bool operator()(const T* const _p1, const T* const _o1, const T* const _p2, const T* const _o2, T* _residuals) const
        {
			// Expected measurement
			T expected_range = (_p2[0]-_p1[0])*(_p2[0]-_p1[0]) + (_p2[1]-_p1[1])*(_p2[1]-_p1[1]); //square of the range
			T expected_rotation = _o2[0]-_o1[0];

			// Residuals
			_residuals[0] = (expected_range - T((*this->measurement_ptr_)*(*this->measurement_ptr_))) / T(stdev_);
			_residuals[1] = (expected_rotation - T(*(this->measurement_ptr_+1))) / T(stdev_);

			return true;
        }

        virtual correspondenceType getType() const
        {
        	return ODOM_2D_THETA;
        }
};

class Frame2DPO : public FrameBase
{
	protected:
		StateXShPtr p_ptr_;
		StateXShPtr o_ptr_;

	public:
		Frame2DPO(const StateXShPtr& _p_ptr, const StateXShPtr& _o_ptr, const WolfScalar & _ts, const FrameType & _tp = REGULAR_FRAME):
			FrameBase(_tp, _ts),
			p_ptr_(_p_ptr),
			o_ptr_(_o_ptr)
		{
		}

		virtual ~Frame2DPO()
		{
		}

		StateXShPtr getPPtr()
		{
			return p_ptr_;
		}

		StateXShPtr getOPtr()
		{
			return o_ptr_;
		}
};

class CaptureXBase
{
	public:
		VectorXs capture;
		WolfScalar time_stamp;

		CaptureXBase(const VectorXs& _capture, const WolfScalar& _time_stamp) :
			capture(_capture),
			time_stamp(_time_stamp)
		{
		}

		virtual ~CaptureXBase()
		{
		}

		virtual captureType getCaptureType() const = 0;

		WolfScalar* getPtr()
		{
			return capture.data();
		}
};

class CaptureOdom2D : public CaptureXBase
{
	public:
	CaptureOdom2D(const VectorXs& _capture, const WolfScalar& _time_stamp) :
			CaptureXBase(_capture, _time_stamp)
		{
		}

		virtual ~CaptureOdom2D()
		{
		}

		virtual captureType getCaptureType() const
		{
			return CAPT_ODOM_2D;
		}
};

class CaptureGPS2D : public CaptureXBase
{
	public:
	CaptureGPS2D(const VectorXs& _capture, const WolfScalar& _time_stamp) :
			CaptureXBase(_capture, _time_stamp)
		{
		}

		virtual ~CaptureGPS2D()
		{
		}

		virtual captureType getCaptureType() const
		{
			return CAPT_GPS_2D;
		}
};

class WolfManager
{
    protected:
		VectorXs state_;
		unsigned int first_empty_state_;
		bool use_complex_angles_;
//		Frame2DPO current_frame_;
		std::vector<Frame2DPOShPtr> frames_;
        std::vector<CorrespondenceXShPtr> correspondences_;
        std::vector<VectorXs> odom_captures_;
        std::vector<VectorXs> gps_captures_;
        std::queue<CaptureXShPtr> new_captures_;
        std::vector<CaptureXShPtr> captures_;

    public: 
        WolfManager(const unsigned int& _state_length=1000, const bool _complex_angle=false) :
        	state_(_state_length),
			first_empty_state_(0),
        	use_complex_angles_(_complex_angle),
        	frames_(0),
			correspondences_(0)
		{
        	VectorXs init_frame(use_complex_angles_ ? 4 : 3);
        	if (use_complex_angles_)
        		init_frame << 0, 0, 1, 0;
        	else
        		init_frame << 0, 0, 0;
        	createFrame(init_frame, 0);
		}

        virtual ~WolfManager()
        {
        }

        unsigned int getCorrespondencesSize()
        {
        	return correspondences_.size();
        }

        void createFrame(const VectorXs& _frame_state, const WolfScalar& _time_stamp)
        {
        	// Store in state_
        	state_.segment(first_empty_state_, use_complex_angles_ ? 4 : 3) << _frame_state;

        	// Create frame
        	if (use_complex_angles_)
				frames_.push_back(Frame2DPOShPtr(new Frame2DPO(StateXShPtr(new StatePoint2D(state_.data()+first_empty_state_)),
															   StateXShPtr(new StateComplexAngle(state_.data()+first_empty_state_+2)),
															   _time_stamp)));

        	else
				frames_.push_back(Frame2DPOShPtr(new Frame2DPO(StateXShPtr(new StatePoint2D(state_.data()+first_empty_state_)),
															   StateXShPtr(new StateThetaAngle(state_.data()+first_empty_state_+2)),
															   _time_stamp)));

        	// Update first free state location index
        	first_empty_state_ += use_complex_angles_ ? 4 : 3;
        }

        void addOdomCapture(const VectorXs& _odom_capture, const WolfScalar& _time_stamp)
        {
        	new_captures_.push(CaptureXShPtr(new CaptureOdom2D(_odom_capture, _time_stamp)));
        }

        void addGPSCapture(const VectorXs& _gps_capture, const WolfScalar& _time_stamp)
        {
        	new_captures_.push(CaptureXShPtr(new CaptureGPS2D(_gps_capture, _time_stamp)));
        }

        void computeOdomCapture(const CaptureXShPtr& _odom_capture)
		{
        	Frame2DPOShPtr prev_frame_ptr = frames_.back();

        	// STORE CAPTURE
        	captures_.push_back(_odom_capture);
        	VectorXs capture_data = _odom_capture->capture;

        	// PRIOR
        	VectorXs pose_predicted(use_complex_angles_ ? 4 : 3);
        	Map<VectorXs> previous_pose(prev_frame_ptr->getPPtr()->getPtr(), use_complex_angles_ ? 4 : 3);
        	if (use_complex_angles_)
			{
				double new_pose_predicted_2 = previous_pose(2) * cos(capture_data(1)) - previous_pose(3) * sin(capture_data(1));
				double new_pose_predicted_3 = previous_pose(2) * sin(capture_data(1)) + previous_pose(3) * cos(capture_data(1));
				pose_predicted(0) = previous_pose(0) + capture_data(0) * new_pose_predicted_2;
				pose_predicted(1) = previous_pose(1) + capture_data(0) * new_pose_predicted_3;
				pose_predicted(2) = new_pose_predicted_2;
				pose_predicted(3) = new_pose_predicted_3;
			}
			else
			{
				pose_predicted(0) = previous_pose(0) + capture_data(0) * cos(previous_pose(2) + (capture_data(1)));
				pose_predicted(1) = previous_pose(1) + capture_data(0) * sin(previous_pose(2) + (capture_data(1)));
				pose_predicted(2) = previous_pose(2) + (capture_data(1));
			}

        	// NEW FRAME
        	createFrame(pose_predicted, _odom_capture->time_stamp);

			// CORRESPONDENCE ODOMETRY
			if (use_complex_angles_)
				correspondences_.push_back(CorrespondenceXShPtr(new Correspondence2DOdometry(_odom_capture->getPtr(),
																						   prev_frame_ptr->getPPtr()->getPtr(),
																						   prev_frame_ptr->getOPtr()->getPtr(),
																						   frames_.back()->getPPtr()->getPtr(),
																						   frames_.back()->getOPtr()->getPtr())));

			else
				correspondences_.push_back(CorrespondenceXShPtr(new Correspondence2DOdometryTheta(_odom_capture->getPtr(),
																						   	    prev_frame_ptr->getPPtr()->getPtr(),
																								prev_frame_ptr->getOPtr()->getPtr(),
																								frames_.back()->getPPtr()->getPtr(),
																								frames_.back()->getOPtr()->getPtr())));
		}

        void computeGPSCapture(const CaptureXShPtr& _gps_capture)
		{
			// STORE CAPTURE
        	captures_.push_back(_gps_capture);

			// CORRESPONDENCE GPS
			correspondences_.push_back(CorrespondenceXShPtr(new CorrespondenceGPS2D(_gps_capture->getPtr(), frames_.back()->getPPtr()->getPtr())));
		}

        void update(std::queue<StateXShPtr>& new_state_units, std::queue<CorrespondenceXShPtr>& new_correspondences)
        {
        	while (!new_captures_.empty())
        	{
        		switch (new_captures_.front()->getCaptureType())
        		{
        			case CAPT_GPS_2D:
        				computeGPSCapture(new_captures_.front());
        				new_correspondences.push(correspondences_.back());
        				break;
        			case CAPT_ODOM_2D:
        				computeOdomCapture(new_captures_.front());
        				new_correspondences.push(correspondences_.back());
        				new_state_units.push(frames_.back()->getPPtr());
        				new_state_units.push(frames_.back()->getOPtr());
        				break;
        			default:
        				std::cout << "unknown capture...\n";
        		}
        		new_captures_.pop();
        	}

        }

        VectorXs getState()
        {
        	return state_;
        }

        CorrespondenceXShPtr getCorrespondencePrt(unsigned int i)
        {
        	return correspondences_.at(i);
        }

        std::queue<StateXShPtr> getStateUnitsPtrs(unsigned int i)
		{
			return std::queue<StateXShPtr>({frames_.at(i)->getPPtr(),frames_.at(i)->getOPtr()});
		}
};

class CeresManager
{
	protected:

		std::vector<std::pair<ceres::ResidualBlockId, CorrespondenceXShPtr>> correspondence_list_;
		ceres::Problem ceres_problem_;

	public:
		CeresManager()
		{
		}

		~CeresManager()
		{
		}

		ceres::Solver::Summary solve(const ceres::Solver::Options& _ceres_options)
		{
			// create summary
			ceres::Solver::Summary ceres_summary_;

			// run Ceres Solver
			ceres::Solve(_ceres_options, &ceres_problem_, &ceres_summary_);

			//display results
			return ceres_summary_;
		}

		void addCorrespondences(std::queue<CorrespondenceXShPtr>& _new_correspondences)
		{
			while (!_new_correspondences.empty())
			{
				addCorrespondence(_new_correspondences.front());
				_new_correspondences.pop();
			}
		}

		void addCorrespondence(const CorrespondenceXShPtr& _corr_ptr)
		{
			ceres::ResidualBlockId blockIdx = ceres_problem_.AddResidualBlock(createCostFunction(_corr_ptr), NULL, _corr_ptr->getBlockPtrVector());
			correspondence_list_.push_back(std::pair<ceres::ResidualBlockId, CorrespondenceXShPtr>(blockIdx,_corr_ptr));
		}

		void addStateUnits(std::queue<StateXShPtr>& _new_state_units)
		{
			while (!_new_state_units.empty())
			{
				addStateUnit(_new_state_units.front());
				_new_state_units.pop();
			}
		}

		void addStateUnit(const StateXShPtr& _st_ptr)
		{
			//std::cout << "Adding a State Unit to wolf_problem... " << std::endl;
			//_st_ptr->print();

			switch (_st_ptr->getParametrizationType())
			{
				case COMPLEX_ANGLE:
				{
					//std::cout << "Adding Complex angle Local Parametrization to the List... " << std::endl;
					//ceres_problem_.SetParameterization(_st_ptr->getPtr(), new ComplexAngleParameterization);
					ceres_problem_.AddParameterBlock(_st_ptr->getPtr(), ((StateComplexAngle*)_st_ptr.get())->BLOCK_SIZE, new ComplexAngleParameterization);
					break;
				}
//				case QUATERNION:
//				{
//					std::cout << "Adding Quaternion Local Parametrization to the List... " << std::endl;
//					ceres_problem_.SetParameterization(_st_ptr->getPtr(), new EigenQuaternionParameterization);
//					break;
//				}
				case NONE:
				{
					//std::cout << "No Local Parametrization to be added" << std::endl;
					break;
				}
				default:
					std::cout << "Unknown  Local Parametrization type!" << std::endl;
			}
		}

		ceres::CostFunction* createCostFunction(const CorrespondenceXShPtr& _corrPtr)
		{
			switch (_corrPtr->getType())
			{
				case GPS_2D:
				{
					CorrespondenceGPS2D* specific_ptr = (CorrespondenceGPS2D*)(_corrPtr.get());
					return new ceres::AutoDiffCostFunction<CorrespondenceGPS2D,
															specific_ptr->measurementSize,
															specific_ptr->block0Size,
															specific_ptr->block1Size,
															specific_ptr->block2Size,
															specific_ptr->block3Size,
															specific_ptr->block4Size,
															specific_ptr->block5Size,
															specific_ptr->block6Size,
															specific_ptr->block7Size,
															specific_ptr->block8Size,
															specific_ptr->block9Size>(specific_ptr);
					break;
				}
				case ODOM_2D_COMPLEX:
				{
					Correspondence2DOdometry* specific_ptr = (Correspondence2DOdometry*)(_corrPtr.get());
					return new ceres::AutoDiffCostFunction<Correspondence2DOdometry,
															specific_ptr->measurementSize,
															specific_ptr->block0Size,
															specific_ptr->block1Size,
															specific_ptr->block2Size,
															specific_ptr->block3Size,
															specific_ptr->block4Size,
															specific_ptr->block5Size,
															specific_ptr->block6Size,
															specific_ptr->block7Size,
															specific_ptr->block8Size,
															specific_ptr->block9Size>(specific_ptr);
					break;
				}
				case ODOM_2D_THETA:
				{
					Correspondence2DOdometryTheta* specific_ptr = (Correspondence2DOdometryTheta*)(_corrPtr.get());
					return new ceres::AutoDiffCostFunction<Correspondence2DOdometryTheta,
															specific_ptr->measurementSize,
															specific_ptr->block0Size,
															specific_ptr->block1Size,
															specific_ptr->block2Size,
															specific_ptr->block3Size,
															specific_ptr->block4Size,
															specific_ptr->block5Size,
															specific_ptr->block6Size,
															specific_ptr->block7Size,
															specific_ptr->block8Size,
															specific_ptr->block9Size>(specific_ptr);
					break;
				}
				default:
					std::cout << "Unknown correspondence type! Please add it in the CeresWrapper::createCostFunction()" << std::endl;

					return NULL;
			}
		}
};

int main(int argc, char** argv) 
{
	clock_t t1, t2;
	t1=clock();

    std::cout << " ========= 2D Robot with odometry and GPS ===========" << std::endl << std::endl;
    
    // USER INPUT ============================================================================================
	if (argc!=4)
	{
		std::cout << "Please call me with: [./test_ceres_manager NI PRINT ORIENTATION_MODE], where:" << std::endl;
		std::cout << "     - NI is the number of iterations" << std::endl;
		std::cout << "     - PRINT = 1 for print results" << std::endl;
		std::cout << "     - ORIENTATION_MODE: 0 for theta, 1 for complex angle" << std::endl;
		std::cout << "EXIT due to bad user input" << std::endl << std::endl;
		return -1;
	}

	unsigned int n_execution = (unsigned int) atoi(argv[1]); //number of iterations of the whole execution
	bool print = (bool) atoi(argv[2]);
	bool complex_angle = (bool) atoi(argv[3]);

	// INITIALIZATION ============================================================================================
	//init random generators
	std::default_random_engine generator(1);
	std::normal_distribution<WolfScalar> distribution_odom(0.0,0.01); //odometry noise
	std::normal_distribution<WolfScalar> distribution_gps(0.0,1); //GPS noise

	//init google log
	google::InitGoogleLogging(argv[0]);

	// Ceres initialization
	ceres::Solver::Options ceres_options;
	ceres_options.minimizer_type = ceres::LINE_SEARCH;//ceres::TRUST_REGION;
	ceres_options.max_line_search_step_contraction = 1e-3;
	//    ceres_options.minimizer_progress_to_stdout = false;
	//    ceres_options.line_search_direction_type = ceres::LBFGS;
	//    ceres_options.max_num_iterations = 2;
	CeresManager ceres_manager;
	std::ofstream log_file;  //output file

	// Wolf manager initialization
	WolfManager* wolf_manager = new WolfManager(n_execution, complex_angle);

	//variables
	int dim = (complex_angle ? 4 : 3);
	Eigen::VectorXs odom_inc_true(n_execution*2);//invented motion
	Eigen::VectorXs pose_true(3); //current true pose
	Eigen::VectorXs ground_truth(n_execution*3); //all true poses
	Eigen::VectorXs odom_readings(n_execution*2); // all odometry readings
	Eigen::VectorXs gps_fix_readings(n_execution*3); //all GPS fix readings
	std::queue<StateXShPtr> new_state_units; // new state units in wolf that must be added to ceres
	std::queue<CorrespondenceXShPtr> new_correspondences; // new correspondences in wolf that must be added to ceres

	// Initial pose
	pose_true << 0,0,0;
	ground_truth.head(dim) = pose_true;

	// SENSOR DATA ============================================================================================
	// Ground truth
	for (unsigned int ii = 0; ii<n_execution; ii++)
	{
		// inventing odometry ground truth
		if ( ii < (unsigned int)floor(n_execution/2) )
			odom_inc_true.segment(ii*2,2) << fabs(cos(ii/10.)) , fabs(sin(ii/2000.)); //invented motion increments.
		else
			odom_inc_true.segment(ii*2,2) << fabs(cos(ii/10.)) , -fabs(sin((ii-floor(n_execution/2))/2000.)); //invented motion increments.

		// Computing ground truth trajectory
		pose_true(0) = pose_true(0) + odom_inc_true(ii*2) * cos(pose_true(2) + odom_inc_true(ii*2+1));
		pose_true(1) = pose_true(1) + odom_inc_true(ii*2) * sin(pose_true(2) + odom_inc_true(ii*2+1));
		pose_true(2) = pose_true(2) + odom_inc_true(ii*2+1);
		ground_truth.segment(ii*3,3) << pose_true;

		// corrupting sensor readings (odometry and GPS)
		odom_readings.segment(ii*2,2) << odom_inc_true(ii*2) + distribution_odom(generator),
										 odom_inc_true(ii*2+1) + distribution_odom(generator); //true range and theta with noise
		gps_fix_readings.segment(ii*3,3) << pose_true(0) + distribution_gps(generator),
											pose_true(1) + distribution_gps(generator),
											0. + distribution_gps(generator);
	}

	// START TRAJECTORY ============================================================================================
    new_state_units = wolf_manager->getStateUnitsPtrs(0); // First pose to be added in ceres
    for (uint step=1; step < n_execution; step++)
	{
    	// adding sensor captures
		wolf_manager->addOdomCapture(odom_readings.segment(step*2,2),step);
		wolf_manager->addGPSCapture(gps_fix_readings.segment(step*3,3),step);

		// updating problem
		wolf_manager->update(new_state_units, new_correspondences);

		// adding new state units and correspondences to ceres
		ceres_manager.addStateUnits(new_state_units);
		ceres_manager.addCorrespondences(new_correspondences);

		// print data
		if (print)
		{
			std::cout << " ========= STEP " << step << "===========" << std::endl << std::endl;
			std::cout << "odom : " << odom_inc_true.segment(step*2,2).transpose() << std::endl << std::endl;
			std::cout << "gps measurement : " << gps_fix_readings.segment(step*3,3).transpose() << std::endl << std::endl;
			std::cout << "ground_truth : " << std::endl << ground_truth.head(step*dim).transpose() << std::endl;
		}
	}
    
    // SOLVE OPTIMIZATION ============================================================================================
	ceres::Solver::Summary summary = ceres_manager.solve(ceres_options);
	t2=clock();
	double seconds = ((double)t2-t1)/CLOCKS_PER_SEC;

	// DISPLAY RESULTS ============================================================================================
	std::cout << summary.FullReport() << std::endl;
	std::cout << "optimization seconds: " << summary.total_time_in_seconds << std::endl;
	std::cout << "total seconds: " << seconds << std::endl;

	// change from complex angle to theta
	VectorXs state = wolf_manager->getState();
	VectorXs state_theta(n_execution * 3);
	if (complex_angle)
		for (uint ii = 0; ii<n_execution; ii++)
			state_theta.segment(ii*3,3) << state(ii*4), state(ii*4+1), atan2(state(ii*4+2), state(ii*4+3));
	else
		state_theta = state;

	// Print log file
	std::string homepath = getenv("HOME");
	log_file.open(homepath + "/Desktop/log_file_2.txt", std::ofstream::out); //open log file
	if (log_file.is_open())
	{
		log_file << seconds << std::endl;
		for (unsigned int ii = 0; ii<n_execution; ii++)
			log_file << state_theta.segment(ii*3,3).transpose()
					 << " " << ground_truth.segment(ii*3,3).transpose()
					 << " " << (state_theta.segment(ii*3,3)-ground_truth.segment(ii*3,3)).transpose()
					 << " " << gps_fix_readings.segment(ii*3,3).transpose();
		log_file.close(); //close log file
		std::cout << std::endl << " Result file ~/Desktop/log_data.txt" << std::endl;
	}
	else
		std::cout << std::endl << " Failed to write the file ~/Desktop/log_data.txt" << std::endl;

    std::cout << " ========= END ===========" << std::endl << std::endl;
       
    //exit
    return 0;
}

