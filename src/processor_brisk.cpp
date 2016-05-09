// Wolf includes
#include "processor_brisk.h"

// OpenCV includes

// other includes
#include <bitset>
#include <algorithm>

namespace wolf
{


//ProcessorBrisk::ProcessorBrisk(cv::FeatureDetector* _det_ptr, cv::DescriptorExtractor* _desc_ext_ptr,
//               cv::DescriptorMatcher* _match_ptr, ProcessorImageParameters _params) :
//        ProcessorTrackerFeature(PRC_TRACKER_BRISK),
//        detector_ptr_(_det_ptr), descriptor_ptr_(_desc_ext_ptr), matcher_ptr_(_match_ptr),
//        act_search_grid_(_params.image.width, _params.image.height,
//                _params.active_search.grid_width, _params.active_search.grid_height,
//                _params.descriptor.nominal_pattern_radius*_params.descriptor.pattern_scale,
//                _params.active_search.separation),
//        params_(_params)
//{
//    ProcessorTrackerFeature::setMaxNewFeatures(_params.algorithm.max_new_features);
//    params_.detector.pattern_radius = (float)(_params.detector.nominal_pattern_radius)*pow(2,_params.detector.octaves);
//    params_.descriptor.pattern_radius = (float)(_params.descriptor.nominal_pattern_radius)*_params.descriptor.pattern_scale;
//    params_.descriptor.size_bits = descriptor_ptr_->descriptorSize() * 8;

//    std::cout << "detector   pattern radius: " << params_.detector.pattern_radius << std::endl;
//    std::cout << "descriptor pattern radius: " << params_.descriptor.pattern_radius << std::endl;
//    std::cout << "descriptor size          : " << params_.descriptor.size_bits << std::endl;
//}

ProcessorBrisk::ProcessorBrisk(ProcessorImageParameters _params, DetectorDescriptorParamsBase* _dd_base_ptr) :
    ProcessorTrackerFeature(PRC_TRACKER_BRISK),
    detector_descriptor_ptr_(nullptr), matcher_ptr_(nullptr), params_(_params),
    act_search_grid_()
{
    switch (_dd_base_ptr->type){
        case DD_BRISK:
            {
            DetectorDescriptorParamsBrisk* dd_brisk = (DetectorDescriptorParamsBrisk*) _dd_base_ptr;
            //used to test
            _dd_base_ptr->nominal_pattern_radius = 18;
            //
            detector_descriptor_ptr_ = new cv::BRISK(dd_brisk->threshold,dd_brisk->octaves,dd_brisk->pattern_scale);
            detector_descriptor_params_.pattern_radius_ = std::max((unsigned int)((_dd_base_ptr->nominal_pattern_radius)*pow(2,dd_brisk->octaves)),
                                                                   (unsigned int)((_dd_base_ptr->nominal_pattern_radius)*dd_brisk->pattern_scale));
            act_search_grid_.setParameters(_params.image.width, _params.image.height,
                    _params.active_search.grid_width, _params.active_search.grid_height,
                    _dd_base_ptr->nominal_pattern_radius * dd_brisk->pattern_scale,
                    _params.active_search.separation);
            break;
            }
        case DD_ORB:
            {
            DetectorDescriptorParamsOrb* dd_orb = (DetectorDescriptorParamsOrb*) _dd_base_ptr;
            //used to test
            _dd_base_ptr->nominal_pattern_radius = 18;
            //
            detector_descriptor_ptr_ = new cv::ORB(dd_orb->nfeatures,dd_orb->scaleFactor,dd_orb->nlevels,dd_orb->edgeThreshold,
                                                   dd_orb->firstLevel,dd_orb->WTA_K,dd_orb->scoreType,dd_orb->patchSize);
            detector_descriptor_params_.pattern_radius_ =
                    (unsigned int)((_dd_base_ptr->nominal_pattern_radius)*pow(dd_orb->scaleFactor,dd_orb->nlevels-1));
            act_search_grid_.setParameters(_params.image.width, _params.image.height,
                    _params.active_search.grid_width, _params.active_search.grid_height,
                    std::max(dd_orb->edgeThreshold,dd_orb->patchSize), //check this out /2?
                    _params.active_search.separation);
            break;
            }
        default:
            throw std::runtime_error("Unknown detector-descriptor");
    }
    matcher_ptr_ = new cv::BFMatcher(_params.matcher.similarity_norm);
    detector_descriptor_params_.size_bits_ = detector_descriptor_ptr_->descriptorSize() * 8;
}

//Destructor
ProcessorBrisk::~ProcessorBrisk()
{

}

void ProcessorBrisk::preProcess()
{
    image_incoming_ = ((CaptureImage*)incoming_ptr_)->getImage();

    if (last_ptr_ == nullptr)
        image_last_ = ((CaptureImage*)incoming_ptr_)->getImage();
    else
        image_last_ = ((CaptureImage*)last_ptr_)->getImage();

    act_search_grid_.renew();

    if(last_ptr_ != nullptr)
        resetVisualizationFlag(*(last_ptr_->getFeatureListPtr()));

    // Clear of the lists used to debug
    tracker_roi_.clear();
    tracker_roi_inflated_.clear();
    detector_roi_.clear();
    tracker_target_.clear();
    tracker_candidates_.clear();

//    if(image_last_.data == image_incoming_.data)
//        std::cout << "--------------------------------------------------------------------------SON IGUALES (pre)" << std::endl;
}

void ProcessorBrisk::postProcess()
{
    drawFeatures(last_ptr_);
//    if(image_last_.data == image_incoming_.data)
//        std::cout << "--------------------------------------------------------------------------SON IGUALES (post)" << std::endl;
//    drawRoi(image_last_,detector_roi_,cv::Scalar(88.0, 70.0, 255.0));//detector roi(now only shown when it collides with the the image)
//    drawRoi(image_last_,tracker_roi_, cv::Scalar(88.0, 70.0, 255.0)); //tracker roi
//    drawRoi(image_last_,tracker_roi_inflated_,cv::Scalar(225.0, 0.0, 255.0));//inflated roi(now only shown when it collides with the the image)
//    drawTrackingFeatures(image_last_,tracker_target_,tracker_candidates_);
    cv::waitKey(30);
}

bool ProcessorBrisk::correctFeatureDrift(const FeatureBase* _last_feature, FeatureBase* _incoming_feature)
{
    return true;
}

unsigned int ProcessorBrisk::detect(cv::Mat _image, cv::Rect& _roi, std::vector<cv::KeyPoint>& _new_keypoints,
                                    cv::Mat& new_descriptors)
{
    cv::Mat _image_roi;

    adaptRoi(_image_roi, _image, _roi);

    std::cout << "detect roi: " << _roi << std::endl;

//    detector_.detect(_image_roi, _new_keypoints);
//    descriptor_.compute(_image_roi, _new_keypoints, new_descriptors);
    detector_descriptor_ptr_->detect(_image_roi, _new_keypoints);
    for (unsigned int i = 0; i < _new_keypoints.size(); i++)
    {
        //std::cout << "keypoints: " << _new_keypoints[i].pt << std::endl;
    }
    detector_descriptor_ptr_->compute(_image_roi, _new_keypoints, new_descriptors);

    //std::cout << "detect roi2: " << _roi << std::endl;

    for (unsigned int i = 0; i < _new_keypoints.size(); i++)
    {
        //std::cout << "keypoints 2: " << _new_keypoints[i].pt << std::endl;
        _new_keypoints[i].pt.x = _new_keypoints[i].pt.x + _roi.x;
        _new_keypoints[i].pt.y = _new_keypoints[i].pt.y + _roi.y;
    }
    return _new_keypoints.size();
}

unsigned int ProcessorBrisk::detectNewFeatures(const unsigned int& _max_new_features)
{
    std::cout << "\n---------------- detectNewFeatures -------------" << std::endl;
    cv::Rect roi;
    std::vector<cv::KeyPoint> new_keypoints;
    cv::Mat new_descriptors;
    unsigned int n_new_features = 0;

    for (unsigned int n_iterations = 0; n_iterations < 50 * 1.25; n_iterations++)//_max_new_features * 1.25; n_iterations++)
    {
        if (act_search_grid_.pickRoi(roi))
        {
        	detector_roi_.push_back(roi);
//            std::cout << "roi: " << roi << std::endl;
            if (detect(image_last_, roi, new_keypoints, new_descriptors))
            {
                //Escoger uno de los features encontrados -> el 0 o primero.
//                std::cout << new_keypoints.size() << " new features detected in active search roi. Picking only the first one." << std::endl;

                FeaturePointImage* point_ptr = new FeaturePointImage(new_keypoints[0], new_descriptors.row(0), false);
                point_ptr->setTrackId(point_ptr->id());
                addNewFeatureLast(point_ptr);
                act_search_grid_.hitCell(new_keypoints[0]);

                std::cout << "Added point " << point_ptr->trackId() << " at: " << new_keypoints[0].pt << std::endl;

                n_new_features++;
                if (n_new_features >= _max_new_features)
                    break;
            }
            else
            {
                act_search_grid_.blockCell(roi);
            }
        }
        else
        {
            break;
        }
    }

    std::cout << "Number of new features detected: " << n_new_features << std::endl;

    return n_new_features;
}

void ProcessorBrisk::resetVisualizationFlag(FeatureBaseList& _feature_list_last)
{
    for (auto feature_base_last_ptr : _feature_list_last)
    {
        FeaturePointImage* feature_last_ptr = (FeaturePointImage*)feature_base_last_ptr;
        feature_last_ptr->setIsKnown(true);
    }
}

unsigned int ProcessorBrisk::trackFeatures(const FeatureBaseList& _feature_list_in, FeatureBaseList& _feature_list_out,
                                           FeatureMatchMap& _feature_matches)
{
    std::cout << "\n---------------- trackFeatures ----------------" << std::endl;

    unsigned int roi_width = params_.matcher.roi_width;
    unsigned int roi_heigth = params_.matcher.roi_height;
    unsigned int roi_x;
    unsigned int roi_y;
    std::vector<cv::KeyPoint> candidate_keypoints;
    cv::Mat candidate_descriptors;
    std::vector<cv::DMatch> cv_matches;

    std::cout << "Number of features to track: " << _feature_list_in.size() << std::endl;

    for (auto feature_base_ptr : _feature_list_in)
    {
        FeaturePointImage* feature_ptr = (FeaturePointImage*)(((feature_base_ptr)));
        act_search_grid_.hitCell(feature_ptr->getKeypoint());

        std::cout << "Search feature: " << feature_ptr->trackId() << " at: " << feature_ptr->getKeypoint().pt;

        roi_x = (feature_ptr->getKeypoint().pt.x) - (roi_heigth / 2);
        roi_y = (feature_ptr->getKeypoint().pt.y) - (roi_width / 2);
        cv::Rect roi(roi_x, roi_y, roi_width, roi_heigth);

//        std::cout << "roi: " << roi << std::endl;

        //lists used to debug
        tracker_target_.push_back(feature_ptr->getKeypoint().pt);
        tracker_roi_.push_back(roi);

        if (detect(image_incoming_, roi, candidate_keypoints, candidate_descriptors))
        {
            //POSIBLE PROBLEMA: Brisk deja una distancia a la hora de detectar. Si es muy pequeño el roi puede que no detecte nada

        	std::cout << " --> " << candidate_keypoints.size() << " candidates";

//            matcher_.match(feature_ptr->getDescriptor(), candidate_descriptors, cv_matches);
            matcher_ptr_->match(feature_ptr->getDescriptor(), candidate_descriptors, cv_matches);

            std::cout << "\n\tBest is: [" << cv_matches[0].trainIdx << "]:" << cv_matches[0].distance;

            std::cout << " | at: " << candidate_keypoints[cv_matches[0].trainIdx].pt;

            Scalar normalized_score = 1 - (Scalar)(cv_matches[0].distance)/detector_descriptor_params_.size_bits_;

            std::cout << " | score: " << normalized_score;

            if (normalized_score > params_.matcher.min_normalized_score)
            {
                std::cout << "\t <--TRACKED" << std::endl;
                FeaturePointImage* incoming_point_ptr = new FeaturePointImage(
                        candidate_keypoints[cv_matches[0].trainIdx], (candidate_descriptors.row(cv_matches[0].trainIdx)),
                        feature_ptr->isKnown());
                _feature_list_out.push_back(incoming_point_ptr);

                incoming_point_ptr->setTrackId(feature_ptr->trackId());

                _feature_matches[incoming_point_ptr] = FeatureMatch(feature_base_ptr,
                                                            normalized_score); //FIXME: 512 is the maximum HAMMING distance

            }
            else
            {
                std::cout << "\t <--NOT TRACKED" << std::endl;
            }
            for (unsigned int i = 0; i < candidate_keypoints.size(); i++)
            {
                tracker_candidates_.push_back(candidate_keypoints[i].pt);

            }
        }
        else
            std::cout << "\t <--NOT FOUND" << std::endl;
    }
    std::cout << "Number of Features tracked: " << _feature_list_out.size() << std::endl;
    return _feature_list_out.size();
}


void ProcessorBrisk::trimRoi(cv::Rect& _roi)
{
    if(_roi.x < 0)
    {
        int diff_x = -_roi.x;
        _roi.x = 0;
        _roi.width = _roi.width - diff_x;
    }
    if(_roi.y < 0)
    {
        int diff_y = -_roi.y;
        _roi.y = 0;
        _roi.height = _roi.height - diff_y;
    }
    if((unsigned int)(_roi.x + _roi.width) > params_.image.width)
    {
        int diff_width = params_.image.width - (_roi.x + _roi.width);
        _roi.width = _roi.width+diff_width;
    }
    if((unsigned int)(_roi.y + _roi.height) > params_.image.height)
    {
        int diff_height = params_.image.height - (_roi.y + _roi.height);
        _roi.height = _roi.height+diff_height;
    }
}

void ProcessorBrisk::inflateRoi(cv::Rect& _roi)
{
    // now both the detector and descriptor patter_radius is the same, but when not, shouldn't the method have that as input parameter?
    int inflation_rate = detector_descriptor_params_.pattern_radius_;

    _roi.x = _roi.x - inflation_rate;
    _roi.y = _roi.y - inflation_rate;
    _roi.width = _roi.width + 2*inflation_rate;
    _roi.height = _roi.height + 2*inflation_rate;
}

void ProcessorBrisk::adaptRoi(cv::Mat& _image_roi, cv::Mat _image, cv::Rect& _roi)
{

    inflateRoi(_roi);
    trimRoi(_roi);

    tracker_roi_inflated_.push_back(_roi);

    _image_roi = _image(_roi);
}

// draw functions ===================================================================

void ProcessorBrisk::drawTrackingFeatures(cv::Mat _image, std::list<cv::Point> _target_list, std::list<cv::Point> _candidates_list)
{
    // These "tracking features" are the feature to be used in tracking as well as its candidates

    for(auto target_point : _target_list)
    {
        //target
        cv::circle(_image, target_point, 2, cv::Scalar(0.0, 255.0, 255.0), -1, 8, 0);
    }
    for(auto candidate_point : _candidates_list)
    {
        //candidate - cyan
        cv::circle(_image, candidate_point, 2, cv::Scalar(255.0, 255.0, 0.0), -1, 8, 0);
    }

    cv::imshow("Feature tracker", _image);

}

void ProcessorBrisk::drawRoi(cv::Mat _image, std::list<cv::Rect> _roi_list, cv::Scalar _color)
{
    for (auto roi : _roi_list)
    {
        cv::rectangle(_image, roi, _color, 1, 8, 0);
    }
    cv::imshow("Feature tracker", _image);
}

void ProcessorBrisk::drawFeatures(CaptureBase* const _last_ptr)
{
    for (auto feature_ptr : *(last_ptr_->getFeatureListPtr()))
    {
        FeaturePointImage* point_ptr = (FeaturePointImage*)feature_ptr;
        if (point_ptr->isKnown())
        {
            cv::circle(image_last_, point_ptr->getKeypoint().pt, 7, cv::Scalar(51.0, 255.0, 51.0), 1, 3, 0);
        }
        else
        {
            cv::circle(image_last_, point_ptr->getKeypoint().pt, 4, cv::Scalar(51.0, 51.0, 255.0), -1, 3, 0);
        }
        cv::putText(image_last_, std::to_string(feature_ptr->trackId()), point_ptr->getKeypoint().pt, cv:: FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255.0, 255.0, 0.0));
    }
    cv::imshow("Feature tracker", image_last_);
}

} // namespace wolf

