//
// Created by shida on 06/12/16.
//

#ifndef __MODELDRAWER_H
#define __MODELDRAWER_H

#include <pangolin/pangolin.h>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <list>
#include <deque>
#include <vector>
#include "Modeler/Matrix.h"
#include "Modeler/Modeler.h"
#include "Modeler/TextureFrame.h"
#include "NetworkIntegration/ChunkCache.h"

namespace ORB_SLAM2
{

    class KeyFrame;
    class Modeler;

    class ModelDrawer
    {
    public:
        ModelDrawer();

        void DrawModel(bool bRGB);
        void SendModel(bool mbRGB, ChunkCache& cache, const std::string& ownAddress, const std::string& unityAddress);
        void DrawModelPoints();
        void DrawTriangles(pangolin::OpenGlMatrix &Twc);
        void DrawFrame(bool bRGB);
        cv::Mat DrawLines();

        // Returns true if a new model was just committed to mModel
        bool UpdateModel();

        // Called by the Modeler thread to stage a new model and then mark it done
        void SetUpdatedModel(const std::vector<dlovi::Matrix> & modelPoints,
                             const std::list<dlovi::Matrix> & modelTris);
        void MarkUpdateDone();

        // Polled by the Modeler thread
        bool UpdateRequested();
        bool UpdateDone();

        // NOTE: these return references; callers must hold mMutexModel (shared) while accessing
        std::vector<dlovi::Matrix> & GetPoints();
        std::list<dlovi::Matrix> & GetTris();

        void SetModeler(Modeler* pModeler);
        Modeler* mpModeler;

    private:
        // Protects mbModelUpdateRequested / mbModelUpdateDone and mModel/mUpdatedModel
        mutable std::shared_timed_mutex mMutexModel;

        bool mbModelUpdateRequested;
        bool mbModelUpdateDone;

        std::pair<std::vector<dlovi::Matrix>, std::list<dlovi::Matrix>> mModel;
        std::pair<std::vector<dlovi::Matrix>, std::list<dlovi::Matrix>> mUpdatedModel;
    };

} //namespace ORB_SLAM2

#endif //__MODELDRAWER_H
