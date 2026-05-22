#ifndef FATIGUE_RUNNER_H
#define FATIGUE_RUNNER_H

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include "rknn_api.h"
#include "retinaface_facenet.h"

struct FatigueMetrics {
    float ear = 0.0f;
    float mar = 0.0f;
    float perclos = 0.0f;
    int consecutive_eye_closed = 0;
    int consecutive_mouth_open = 0;
};

struct FatigueFaceResult {
    cv::Rect face_rect;
    bool is_tired = false;
    FatigueMetrics metrics;
    std::vector<cv::Point2f> landmarks;
};

struct FatigueResult {
    std::vector<FatigueFaceResult> faces;
};

class FatigueDetectorRunner
{
public:
    FatigueDetectorRunner();
    ~FatigueDetectorRunner();

    bool init(const std::string &paddle_model_path, const std::string &pfld_rknn_path, int pfld_size);
    void deinit();
    bool isReady() const;
    void setThresholds(float ear_threshold, float mar_threshold, int eye_closed_frames, int mouth_open_frames);
    void setPerclosWindow(int window);

    int run(const cv::Mat &frame, FatigueResult *result, cv::Mat *annotated = nullptr);

private:
    struct RknnModel;
    std::vector<int> m_mouthPoints;
    rknn_app_context_t m_retinafaceCtx{};
    std::unique_ptr<RknnModel> m_pfldModel;
    int m_pfldSize = 0;
    bool m_ready = false;

    float m_earThreshold = 0.22f;
    float m_marThreshold = 0.60f;
    int m_eyeClosedFrames = 15;
    int m_mouthOpenFrames = 20;

    int m_consecutiveEyeClosed = 0;
    int m_consecutiveMouthOpen = 0;
    std::deque<bool> m_eyeStateBuffer;
    int m_perclosWindow = 200;
    float m_perclos = 0.0f;
};

#endif
