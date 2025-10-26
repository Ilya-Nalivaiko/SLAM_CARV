//
// Created by shida on 06/12/16.
//

#include "Modeler/ModelDrawer.h"
#include <algorithm>
#include <utility>

namespace ORB_SLAM2
{
    ModelDrawer::ModelDrawer()
        : mbModelUpdateRequested(false), mbModelUpdateDone(true)
    {
    }

    void ModelDrawer::DrawModel(bool bRGB)
    {
        // select 1 KF (last)
        int numKFs = 3;
        std::vector<std::pair<cv::Mat,TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);

        if (imAndTexFrame.size() >= static_cast<size_t>(numKFs)) {
            static unsigned int frameTex[1] = {0};
            if (!frameTex[0])
                glGenTextures(numKFs, frameTex);

            cv::Size imSize = imAndTexFrame[0].first.size();

            for (int i = 0; i < numKFs; i++) {
                glBindTexture(GL_TEXTURE_2D, frameTex[i]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                const void* data = imAndTexFrame[i].first.data;
                if (bRGB) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imSize.width, imSize.height, 0,
                                 GL_BGR, GL_UNSIGNED_BYTE, data);
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imSize.width, imSize.height, 0,
                                 GL_RGB, GL_UNSIGNED_BYTE, data);
                }
            }

            glEnable(GL_TEXTURE_2D);

            glBegin(GL_TRIANGLES);
            glColor3f(1.0,1.0,1.0);

            // READ-LOCK while accessing model geometry
            {
                std::shared_lock<std::shared_timed_mutex> lk(mMutexModel);
                auto& points = mModel.first;
                auto& tris   = mModel.second;

                for (auto it = tris.begin(); it != tris.end(); ++it) {
                    dlovi::Matrix point0 = points[(*it)(0)];
                    dlovi::Matrix point1 = points[(*it)(1)];
                    dlovi::Matrix point2 = points[(*it)(2)];

                    dlovi::Matrix edge10 = point1 - point0;
                    dlovi::Matrix edge20 = point2 - point0;

                    dlovi::Matrix normal = edge20.cross(edge10);
                    normal = normal / normal.norm();

                    glNormal3d(normal(0), normal(1), normal(2));

                    std::vector<double> dotProducts;
                    std::vector<int> indexTex;
                    dotProducts.reserve(numKFs);
                    indexTex.reserve(numKFs);
                    for (int i = 0; i < numKFs; i++){
                        cv::Mat texOrient = imAndTexFrame[i].second.GetOrientation();
                        dlovi::Matrix orientation(3,1);
                        orientation(0) = texOrient.at<float>(0);
                        orientation(1) = texOrient.at<float>(1);
                        orientation(2) = texOrient.at<float>(2);
                        dotProducts.push_back(normal.dot(orientation));
                        indexTex.push_back(i);
                    }

                    std::sort(indexTex.begin(), indexTex.end(),
                              [&](int i1, int i2) { return dotProducts[i1] > dotProducts[i2]; });

                    for (int i = 0; i < numKFs; i++){
                        int indexCurr = indexTex[i];

                        TextureFrame tex = imAndTexFrame[indexCurr].second;
                        std::vector<float> uv0 = tex.GetTexCoordinate(point0(0),point0(1),point0(2),imSize);
                        std::vector<float> uv1 = tex.GetTexCoordinate(point1(0),point1(1),point1(2),imSize);
                        std::vector<float> uv2 = tex.GetTexCoordinate(point2(0),point2(1),point2(2),imSize);

                        if (uv0.size() == 2 && uv1.size() == 2 && uv2.size() == 2) {
                            glTexCoord2f(uv0[0], uv0[1]);
                            glVertex3d(point0(0), point0(1), point0(2));

                            glTexCoord2f(uv1[0], uv1[1]);
                            glVertex3d(point1(0), point1(1), point1(2));

                            glTexCoord2f(uv2[0], uv2[1]);
                            glVertex3d(point2(0), point2(1), point2(2));
                            break;
                        }
                    }
                }
            }

            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
    }

    void ModelDrawer::DrawModelPoints()
    {
        glPointSize(3);
        glBegin(GL_POINTS);
        glColor3f(0.5, 0.5, 0.5);

        {
            std::shared_lock<std::shared_timed_mutex> lk(mMutexModel);
            const auto& pts = mModel.first;
            for (const auto& p : pts) {
                glVertex3d(static_cast<GLdouble>(p(0)),
                        static_cast<GLdouble>(p(1)),
                        static_cast<GLdouble>(p(2)));
            }
        }

        glEnd();
    }

    void ModelDrawer::DrawTriangles(pangolin::OpenGlMatrix &Twc)
    {
        glPushMatrix();
    #ifdef HAVE_GLES
        glMultMatrixf(Twc.m);
    #else
        glMultMatrixd(Twc.m);
    #endif
        GLfloat light_position[] = { 0.0, 0.0, 1.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glPopMatrix();

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glShadeModel(GL_FLAT);

        GLfloat material_diffuse[] = {0.2, 0.5, 0.8, 1};
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0,1.0,1.0);

        // READ-LOCK while accessing geometry
        {
            std::shared_lock<std::shared_timed_mutex> lk(mMutexModel);
            auto& points = mModel.first;
            auto& tris   = mModel.second;

            for (auto it = tris.begin(); it != tris.end(); ++it) {
                dlovi::Matrix point0 = points[(*it)(0)];
                dlovi::Matrix point1 = points[(*it)(1)];
                dlovi::Matrix point2 = points[(*it)(2)];

                dlovi::Matrix edge10 = point1 - point0;
                dlovi::Matrix edge20 = point2 - point0;

                dlovi::Matrix normal = edge20.cross(edge10);
                normal = normal / normal.norm();

                glNormal3d(normal(0), normal(1), normal(2));
                glVertex3d(point0(0), point0(1), point0(2));
                glVertex3d(point1(0), point1(1), point1(2));
                glVertex3d(point2(0), point2(1), point2(2));
            }
        }

        glEnd();
        glDisable(GL_LIGHTING);
    }

    void ModelDrawer::DrawFrame(bool bRGB)
    {
        // select the last frame
        int numKFs = 1;
        std::vector<std::pair<cv::Mat,TextureFrame>> imAndTexFrame = mpModeler->GetTextures(numKFs);
        if (imAndTexFrame.size() < static_cast<size_t>(numKFs))
            return;

        glColor3f(1.0,1.0,1.0);

        if (imAndTexFrame[0].first.empty()){
            std::cerr << "ERROR: empty frame image" << std::endl;
            return;
        }
        cv::Size imSize = imAndTexFrame[0].first.size();

        if (bRGB) {
            pangolin::GlTexture imageTexture(imSize.width, imSize.height, GL_RGB, false, 0, GL_BGR, GL_UNSIGNED_BYTE);
            imageTexture.Upload(imAndTexFrame[0].first.data, GL_BGR, GL_UNSIGNED_BYTE);
            imageTexture.RenderToViewportFlipY();
        } else {
            pangolin::GlTexture imageTexture(imSize.width, imSize.height, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
            imageTexture.Upload(imAndTexFrame[0].first.data, GL_RGB, GL_UNSIGNED_BYTE);
            imageTexture.RenderToViewportFlipY();
        }
    }

    cv::Mat ModelDrawer::DrawLines()
    {
        return mpModeler->GetImageWithLines();
    }

    // Returns true if a staged model was just committed.
    bool ModelDrawer::UpdateModel()
    {
        std::unique_lock<std::shared_timed_mutex> lk(mMutexModel);

        if (mbModelUpdateRequested && !mbModelUpdateDone) {
            // still being generated by modeler thread
            return false;
        }

        if (mbModelUpdateRequested && mbModelUpdateDone) {
            // Swap in completed model
            mModel = std::move(mUpdatedModel);
            mbModelUpdateRequested = false;
            return true;
        }

        // No in-flight request: request one now
        mbModelUpdateDone = false;
        mbModelUpdateRequested = true; // modeler thread polls UpdateRequested()
        return false;
    }

    void ModelDrawer::SetUpdatedModel(const std::vector<dlovi::Matrix> & modelPoints,
                                      const std::list<dlovi::Matrix> & modelTris)
    {
        std::unique_lock<std::shared_timed_mutex> lk(mMutexModel);
        mUpdatedModel.first  = modelPoints;
        mUpdatedModel.second = modelTris;
    }

    void ModelDrawer::MarkUpdateDone()
    {
        std::unique_lock<std::shared_timed_mutex> lk(mMutexModel);
        mbModelUpdateDone = true;
    }

    bool ModelDrawer::UpdateRequested()
    {
        std::shared_lock<std::shared_timed_mutex> lk(mMutexModel);
        return mbModelUpdateRequested;
    }

    bool ModelDrawer::UpdateDone()
    {
        std::shared_lock<std::shared_timed_mutex> lk(mMutexModel);
        return mbModelUpdateDone;
    }

    std::vector<dlovi::Matrix> & ModelDrawer::GetPoints()
    {
        return mModel.first;
    }

    std::list<dlovi::Matrix> & ModelDrawer::GetTris()
    {
        return mModel.second;
    }

    void ModelDrawer::SetModeler(Modeler* pModeler)
    {
        mpModeler = pModeler;
    }

} // namespace ORB_SLAM2
