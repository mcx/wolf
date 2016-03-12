/*
 * \processor_tracker.h
 *
 *  Created on: 27/02/2016
 *      \author: jsola
 */

#ifndef PROCESSOR_TRACKER_H_
#define PROCESSOR_TRACKER_H_

#include "processor_base.h"
#include "capture_base.h"

/** \brief General tracker processor
 *
 * This class implements the incremental tracker. It contains three pointers to three Captures of type CaptureBase, named \b origin, \b last and \b incoming:
 *   - \b origin: this points to a Capture where all Feature tracks start.
 *   - \b last: the last Capture tracked by the tracker. A sufficient subset of the Features in \b origin is still alive in \b last.
 *   - \b incoming: the capture being received. The tracker operates on this Capture,
 *     establishing correspondences between the features here and the features in \b origin. Each successful correspondence
 *     results in an extension of the track of the Feature up to the \b incoming Capture.
 *
 * A tracker can be declared autonomous or non-autonomous. This property must be set at construction time.
 *   - An autonomous Tracker is allowed to create new KeyFrames and new Landmarks
 *     as the result of the data processing. A single call to process() accomplishes
 *     all the work needed.
 *   - A non-autonomous Tracker, on the contrary, limits itself to detect and match Features,
 *     but cannot alter the size of the Wolf Problem by adding new elements (Frames and/or Landmarks).
 *     Calling process() is an error, and an outside manager is required to control the tracker
 *     algorithm (by implementing an algorithm similar to process() outside the Tracker).
 *
 * A tracker can be designed to track either Features or Landmarks. This property must be set at construction time.
 *   - If tracking Features, it establishes constraints Feature-Feature;
 *     it does not use Landmarks, nor it creates Landmarks.
 *   - If tracking Landmarks, it establishes constraints Feature-Landmark;
 *     it uses Landmarks for tracking, in an active-search approach,
 *     and it creates Landmarks with each new Feature detected.
 *
 * The pipeline of actions for an autonomous tracker can be resumed as follows:
 *   - Init the tracker with an \b origin Capture: init();
 *   - On each incoming Capture,
 *     - Track known features in the \b incoming Capture: processKnownFeatures();
 *     - Check if enough Features are still tracked, and vote for a new KeyFrame if this number is too low:
 *     - if voteForKeyFrame()
 *       - Look for new Features and make Landmarks with them:
 *       - detectNewFeatures()
 *       - if we use landmarks, do for each detected Feature:
 *         - create landmarks: createOneLandmark()
 *       - create constraints Feature-Landmark or Feature-Feature: createConstraint()
 *       - Make a KeyFrame with the \b last Capture: makeKeyFrame();
 *       - Reset the tracker with the \b last Capture as the new \b origin: reset();
 *     - else
 *       - Advance the tracker one Capture ahead: advance()
 *
 * This functionality exists by default in the virtual method process(). You can overload it at your convenience.
 *
 * This is an abstract class. The following pure virtual methods have to be implemented in derived classes:
 *   - processKnownFeatures()
 *   - voteForKeyFrame()
 *   - detectNewFeatures()
 *   - createLandmark()
 *   - createConstraint()
 */
class ProcessorTracker : public ProcessorBase
{
    public:

        /** \brief Constructor with options
         * \param _autonomous Set to make the tracker autonomous to create KeyFrames and/or Landmarks.
         * \param _uses_landmarks Set to make the tracker work with Landmarks. Un-set to work only with Features.
         */
        ProcessorTracker(bool _autonomous = true, bool _uses_landmarks = true);
        virtual ~ProcessorTracker();

        /** \brief Full processing of an incoming Capture.
         *
         * Usually you do not need to overload this method in derived classes.
         * Overload it only if you want to alter this algorithm.
         */
        virtual void process(CaptureBase* const _incoming_ptr);

        /** \brief Initialize tracker.
         */
        void init(CaptureBase* _origin_ptr);

        /** \brief Tracker function
         * \return The number of successful tracks.
         *
         * This is the tracker function to be implemented in derived classes.
         * It operates on the \b incoming capture pointed by incoming_ptr_.
         *
         * This should do one of the following, depending on the design of the tracker -- see use_landmarks_:
         *   - Track Features against other Features in the \b origin Capture.
         *     An intermediary step of matching against Features in the \b last Capture makes tracking easier.
         *     Once tracked against last, then the link to \b origin is provided by the Features in \b last.
         *   - Track Features against Landmarks in the Map.
         *
         * The function must generate the necessary Features in the \b incoming Capture,
         * of the correct type, derived from FeatureBase.
         *
         * It must also generate the constraints, of a type derived from ConstraintBase.
         */
        virtual unsigned int processKnownFeatures() = 0;

        /**\brief Process new Features
         *
         */
        virtual unsigned int processNewFeatures();


        /** \brief Detect new Features
         * \param _capture_ptr Capture for feature detection. Defaults to incoming_ptr_.
         * \param _new_features_list The list of detected Features. Defaults to member new_features_list_.
         * \return The number of detected Features.
         *
         * This function detects Features that do not correspond to known Features/Landmarks in the system.
         *
         * The function sets the member new_features_list_, the list of newly detected features,
         * to be used for landmark initialization.
         */
        virtual unsigned int detectNewFeatures() = 0;

        /** \brief Vote for KeyFrame generation
         *
         * If a KeyFrame criterion is validated, this function returns true,
         * meaning that it wants to create a KeyFrame at the \b last Capture.
         *
         * WARNING! This function only votes! It does not create KeyFrames!
         */
        virtual bool voteForKeyFrame() = 0;

        /** \brief Reset the tracker to a new \b origin and \b last Captures
         */
        void reset(CaptureBase* _origin_ptr, CaptureBase* _last_ptr);

        /** \brief Reset the tracker using the \b last Capture as the new \b origin.
         */
        void reset();

        /** \brief Advance the incoming Capture to become the last.
         *
         * Call this when the tracking and keyframe policy work is done and
         * we need to get ready to accept a new incoming Capture.
         */
        void advance();

        // getters and setters
        bool isAutonomous() const;
        bool usesLandmarks() const;
        CaptureBase* getOriginPtr() const;
        CaptureBase* getLastPtr() const;
        CaptureBase* getIncomingPtr() const;
        void setOriginPtr(CaptureBase* const _origin_ptr);
        void setLastPtr(CaptureBase* const _last_ptr);
        void setIncomingPtr(CaptureBase* const _incoming_ptr);
        FeatureBaseList& getNewFeaturesList();

    protected:

        /**\brief Make a KeyFrame using the provided Capture.
         */
        virtual void makeKeyFrame(CaptureBase* _capture_ptr);

        /** \brief Create one landmark
         *
         * Implement in derived classes to build the type of landmark you need for this tracker.
         */
        virtual LandmarkBase* createLandmark(FeatureBase* _feature_ptr) = 0;

        /** \brief Create a new constraint
         *
         * Implement in derived classes to build the type of constraint appropriate for the pair feature-landmark used by this tracker.
         */
        virtual ConstraintBase* createConstraint(FeatureBase* _feature_ptr, LandmarkBase* _lmk_ptr) = 0;

    private:
        bool autonomous_;    ///< Sets whether the tracker is autonomous to create new KeyFrames and/or Landmarks.
        bool use_landmarks_; ///< Set if the tracker uses and creates landmarks. Clear if only uses features.
        CaptureBase* origin_ptr_;    ///< Pointer to the origin of the tracker.
        CaptureBase* last_ptr_;      ///< Pointer to the last tracked capture.
        CaptureBase* incoming_ptr_;  ///< Pointer to the incoming capture being processed.
        FeatureBaseList new_features_list_; ///< List of new features for landmark initialization and tracker reset.

};

// IMPLEMENTATION //

inline void ProcessorTracker::init(CaptureBase* _origin_ptr)
{
    origin_ptr_ = _origin_ptr;
    last_ptr_ = _origin_ptr;
}

inline void ProcessorTracker::reset(CaptureBase* _origin_ptr, CaptureBase* _last_ptr)
{
    origin_ptr_ = _origin_ptr;
    last_ptr_ = _last_ptr;
    incoming_ptr_ = nullptr;   // This line is not really needed, but it makes things clearer.
}

inline void ProcessorTracker::reset()
{
    reset(incoming_ptr_, incoming_ptr_);
}

inline void ProcessorTracker::advance()
{
    last_ptr_->getFramePtr()->addCapture(incoming_ptr_); // Add incoming Capture to the tracker's Frame
    last_ptr_->destruct();     // Destruct now the obsolete last before reassigning a new pointer
    last_ptr_ = incoming_ptr_; // Incoming Capture takes the place of last Capture
    incoming_ptr_ = nullptr;   // This line is not really needed, but it makes things clearer.
}

inline bool ProcessorTracker::isAutonomous() const
{
    return autonomous_;
}

inline bool ProcessorTracker::usesLandmarks() const
{
    return use_landmarks_;
}

inline CaptureBase* ProcessorTracker::getOriginPtr() const
{
    return origin_ptr_;
}

inline CaptureBase* ProcessorTracker::getLastPtr() const
{
    return last_ptr_;
}

inline CaptureBase* ProcessorTracker::getIncomingPtr() const
{
    return incoming_ptr_;
}

inline void ProcessorTracker::setOriginPtr(CaptureBase* const _origin_ptr)
{
    origin_ptr_ = _origin_ptr;
}

inline void ProcessorTracker::setLastPtr(CaptureBase* const _last_ptr)
{
    last_ptr_ = _last_ptr;
}

inline void ProcessorTracker::setIncomingPtr(CaptureBase* const _incoming_ptr)
{
    incoming_ptr_ = _incoming_ptr;
}

inline FeatureBaseList& ProcessorTracker::getNewFeaturesList()
{
    return new_features_list_;
}

#endif /* PROCESSOR_TRACKER_H_ */
