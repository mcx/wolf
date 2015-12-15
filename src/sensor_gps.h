//TODO indentation: add a tab at everything inside the class: protected and public must have 1 tab


#ifndef SENSOR_GPS_H_
#define SENSOR_GPS_H_

//wolf
#include "sensor_base.h"

/*
 * temporary! To store navigation data. For now is fake because I don't have real data from gps
 */
class SatelliteNavData
{
public:
    SatelliteNavData(const std::string &_sat_id, const TimeStamp &_timestamp, const WolfScalar &_pseudorange, const WolfScalar &_param1, const WolfScalar &_param2) :
            sat_id_(_sat_id), timestamp_(_timestamp), pseudorange_(_pseudorange), param1_(_param1), param2_(_param2)
    {
        std::cout << "Received navigation data for satellite " << sat_id_ << std::endl;
    }

    const std::string &getSatId() const {
        return sat_id_;
    }

    const TimeStamp &getTimestamp() const {
        return timestamp_;
    }

    WolfScalar getPseudorange() const {
        return pseudorange_;
    }

    WolfScalar getParam1() const {
        return param1_;
    }

    WolfScalar getParam2() const {
        return param2_;
    }

protected:
    std::string sat_id_;
    TimeStamp timestamp_;
    WolfScalar pseudorange_;
    WolfScalar param1_;
    WolfScalar param2_;

};




class SensorGPS : public SensorBase
{
//    TODO parameters needed:
//        * INTRINSIC PARAM:
//                bias                  [ ok ]
//        * EXTRINSIC PARAM:
//                nav data              [TODO]
//                gps mounting point    [ ok ]

protected:
    std::list<SatelliteNavData> nav_data_;

public:
    //pointer to sensor position, orientation and bias
    SensorGPS(StateBlock *_p_ptr, StateBlock *_o_ptr, StateBlock* _bias_ptr);

    /** \brief Default destructor (not recommended)
     *
     * Default destructor (please use destruct() instead of delete for guaranteeing the wolf tree integrity)
     *
     **/
    virtual ~SensorGPS();

    void addNavData(const std::string &_sat_id, const TimeStamp &_timestamp, const WolfScalar &_pseudorange, const WolfScalar &_param1, const WolfScalar &_param2);
};


#endif //SENSOR_GPS_H_
