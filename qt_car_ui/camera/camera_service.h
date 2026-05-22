#ifndef CAMERA_SERVICE_H
#define CAMERA_SERVICE_H
#include <QImage>
#include <utility>
#include "sample_comm.h"
#include "luckfox_mpi.h"
class CameraService {
public:
    CameraService();
    bool init(int width, int height, const char* iq_dir = "/etc/iqfiles", rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL);
    bool grabFrameRGB(QImage& out, int timeout_ms = 1000);
    void shutdown();
private:
    volatile bool m_inited;
    volatile int m_width;
    volatile int m_height;
};

inline CameraService::CameraService() : m_inited(false), m_width(0), m_height(0) {}

inline bool CameraService::init(int width, int height, const char* iq_dir, rk_aiq_working_mode_t hdr_mode) {
    if (m_inited) {
        return true;
    }
    m_width = width;
    m_height = height;
    RK_BOOL multi_sensor = RK_FALSE;
    SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
    SAMPLE_COMM_ISP_Run(0);
    if (RK_MPI_SYS_Init() != RK_SUCCESS) {
        SAMPLE_COMM_ISP_Stop(0);
        return false;
    }
    if (vi_dev_init() != 0) {
        RK_MPI_SYS_Exit();
        SAMPLE_COMM_ISP_Stop(0);
        return false;
    }
    if (vi_chn_init(0, m_width, m_height) != 0) {
        RK_MPI_VI_DisableDev(0);
        RK_MPI_SYS_Exit();
        SAMPLE_COMM_ISP_Stop(0);
        return false;
    }
    m_inited = true;
    return true;
}

inline bool CameraService::grabFrameRGB(QImage& out, int timeout_ms) {
    if (!m_inited) {
        return false;
    }
    VIDEO_FRAME_INFO_S stViFrame;
    RK_S32 ret = RK_MPI_VI_GetChnFrame(0, 0, &stViFrame, timeout_ms);
    if (ret != RK_SUCCESS) {
        return false;
    }
    void* data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);
    int stride = stViFrame.stVFrame.u32VirWidth;
    int strideHeight = stViFrame.stVFrame.u32VirHeight;
    int imgWidth = stViFrame.stVFrame.u32Width;
    int imgHeight = stViFrame.stVFrame.u32Height;
    if (stride <= 0 || strideHeight <= 0) {
        stride = m_width;
        strideHeight = m_height;
    }
    if (imgWidth <= 0 || imgHeight <= 0) {
        imgWidth = stride;
        imgHeight = strideHeight;
    }
    const unsigned char* yuv = static_cast<const unsigned char*>(data);
    QImage img(imgWidth, imgHeight, QImage::Format_RGB888);
    const int dstStride = img.bytesPerLine();
    unsigned char* dst = img.bits();
    const unsigned char* yPlane = yuv;
    const unsigned char* uvPlane = yuv + stride * imgHeight;
    for (int y = 0; y < imgHeight; ++y) {
        unsigned char* drow = dst + y * dstStride;
        const unsigned char* yrow = yPlane + y * stride;
        const unsigned char* uvrow = uvPlane + (y / 2) * stride;
        for (int x = 0; x < imgWidth; ++x) {
            int Y = int(yrow[x]);
            int U = int(uvrow[(x & ~1) + 0]);
            int V = int(uvrow[(x & ~1) + 1]);
            int C = Y - 16; if (C < 0) C = 0;
            int D = U - 128;
            int E = V - 128;
            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;
            if (R < 0) R = 0; else if (R > 255) R = 255;
            if (G < 0) G = 0; else if (G > 255) G = 255;
            if (B < 0) B = 0; else if (B > 255) B = 255;
            unsigned char* px = drow + x * 3;
            px[0] = (unsigned char)R;
            px[1] = (unsigned char)G;
            px[2] = (unsigned char)B;
        }
    }
    out = std::move(img);
    RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame);
    return true;
}

inline void CameraService::shutdown() {
    if (!m_inited) {
        return;
    }
    RK_MPI_VI_DisableChn(0, 0);
    RK_MPI_VI_DisableDev(0);
    SAMPLE_COMM_ISP_Stop(0);
    RK_MPI_SYS_Exit();
    m_inited = false;
}
#endif
