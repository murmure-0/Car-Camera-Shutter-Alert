#include "fatigue_runner.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>

namespace {
const std::vector<int> kLeftEyePoints = {35, 41, 40, 42, 39, 37, 33, 36};
const std::vector<int> kRightEyePoints = {89, 95, 94, 96, 93, 91, 87, 90};
const std::vector<int> kMouthOutline = {52, 64, 63, 71, 67, 68, 61, 58, 59, 53, 56, 55};
const std::vector<int> kMouthInner = {65, 66, 62, 70, 69, 57, 60, 54};

float eye_aspect_ratio(const std::vector<cv::Point2f> &eye_points)
{
    if (eye_points.size() < 8) {
        return 0.0f;
    }
    double A = cv::norm(eye_points[1] - eye_points[7]);
    double B = cv::norm(eye_points[2] - eye_points[6]);
    double C = cv::norm(eye_points[3] - eye_points[5]);
    double D = cv::norm(eye_points[0] - eye_points[4]);
    if (D < 1e-5) {
        return 0.0f;
    }
    return static_cast<float>((A + B + C) / (3.0 * D));
}

float mouth_aspect_ratio(const std::vector<cv::Point2f> &mouth_points)
{
    if (mouth_points.size() < 10) {
        return 0.0f;
    }
    const int LEFT_CORNER = 0;
    const int UPPER_CENTER = 3;
    const int RIGHT_CORNER = 6;
    const int LOWER_CENTER = 9;
    double A = cv::norm(mouth_points[UPPER_CENTER] - mouth_points[LOWER_CENTER]);
    double B = cv::norm(mouth_points[UPPER_CENTER] - mouth_points[LEFT_CORNER]);
    double C = cv::norm(mouth_points[UPPER_CENTER] - mouth_points[RIGHT_CORNER]);
    double D = cv::norm(mouth_points[LEFT_CORNER] - mouth_points[RIGHT_CORNER]);
    if (D < 1e-5) {
        return 0.0f;
    }
    return static_cast<float>((A + B + C) / (3.0 * D));
}

bool is_valid_landmark_face(const std::vector<cv::Point2f> &landmarks, const cv::Rect &face_rect)
{
    if (landmarks.size() < 97 || face_rect.width <= 0 || face_rect.height <= 0) {
        return false;
    }

    auto mean_point = [&](const std::vector<int> &idxs) {
        cv::Point2f p(0.0f, 0.0f);
        int n = 0;
        for (int idx : idxs) {
            if (idx >= 0 && idx < static_cast<int>(landmarks.size())) {
                p += landmarks[idx];
                n++;
            }
        }
        if (n > 0) {
            p.x /= static_cast<float>(n);
            p.y /= static_cast<float>(n);
        }
        return p;
    };

    const cv::Point2f left_eye = mean_point(kLeftEyePoints);
    const cv::Point2f right_eye = mean_point(kRightEyePoints);
    const cv::Point2f mouth = mean_point(kMouthOutline);
    const float eye_dist = cv::norm(left_eye - right_eye);
    const float eye_mid_y = 0.5f * (left_eye.y + right_eye.y);

    if (eye_dist < face_rect.width * 0.12f || eye_dist > face_rect.width * 0.90f) {
        return false;
    }
    if (mouth.y <= eye_mid_y + face_rect.height * 0.06f) {
        return false;
    }
    if (mouth.y >= face_rect.y + face_rect.height * 0.95f) {
        return false;
    }

    float min_x = std::numeric_limits<float>::infinity();
    float min_y = std::numeric_limits<float>::infinity();
    float max_x = -std::numeric_limits<float>::infinity();
    float max_y = -std::numeric_limits<float>::infinity();
    for (const auto &pt : landmarks) {
        min_x = std::min(min_x, pt.x);
        min_y = std::min(min_y, pt.y);
        max_x = std::max(max_x, pt.x);
        max_y = std::max(max_y, pt.y);
    }
    const float lm_w = max_x - min_x;
    const float lm_h = max_y - min_y;
    if (lm_w < face_rect.width * 0.20f || lm_h < face_rect.height * 0.20f) {
        return false;
    }

    return true;
}

static float dequant(int32_t v, int32_t zp, float scale)
{
    if (scale == 0.0f) {
        return static_cast<float>(v - zp);
    }
    return (static_cast<float>(v) - static_cast<float>(zp)) * scale;
}

static std::vector<float> read_tensor(const rknn_tensor_attr &attr, rknn_tensor_mem *mem)
{
    std::vector<float> out;
    if (!mem || !mem->virt_addr) {
        return out;
    }
    out.resize(attr.n_elems);
    const uint32_t w_stride = attr.w_stride > 0 ? attr.w_stride : 0;
    if (attr.n_dims == 5) {
        const int d1 = static_cast<int>(attr.dims[1]);
        const int d4 = static_cast<int>(attr.dims[4]);
        if (d1 <= 0 || d4 <= 0) {
            return out;
        }

        int hw = static_cast<int>(attr.n_elems) / (d1 * d4);
        int h = static_cast<int>(attr.dims[2]);
        int w = static_cast<int>(attr.dims[3]);
        if (h <= 0 || w <= 0 || (h * w) != hw) {
            int s = static_cast<int>(std::round(std::sqrt(static_cast<float>(hw))));
            if (s > 0 && s * s == hw) {
                h = s;
                w = s;
            } else {
                h = 1;
                w = std::max(1, hw);
            }
        }

        const uint32_t stride_w = w_stride > 0 ? w_stride : (static_cast<uint32_t>(attr.dims[3]) > static_cast<uint32_t>(w) ? static_cast<uint32_t>(attr.dims[3]) : static_cast<uint32_t>(w));
        const uint32_t row_stride = stride_w * static_cast<uint32_t>(d4);
        const uint32_t group_stride = static_cast<uint32_t>(h) * row_stride;
        const uint32_t dst_row_stride = static_cast<uint32_t>(w) * static_cast<uint32_t>(d4);
        const uint32_t dst_group_stride = static_cast<uint32_t>(h) * dst_row_stride;

        for (int g = 0; g < d1; ++g) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    for (int ci = 0; ci < d4; ++ci) {
                        const uint32_t src_idx = static_cast<uint32_t>(g) * group_stride + static_cast<uint32_t>(y) * row_stride +
                                                 static_cast<uint32_t>(x) * static_cast<uint32_t>(d4) + static_cast<uint32_t>(ci);
                        const uint32_t dst_idx = static_cast<uint32_t>(g) * dst_group_stride + static_cast<uint32_t>(y) * dst_row_stride +
                                                 static_cast<uint32_t>(x) * static_cast<uint32_t>(d4) + static_cast<uint32_t>(ci);
                        if (dst_idx >= out.size()) {
                            continue;
                        }
                        if (attr.type == RKNN_TENSOR_INT8) {
                            out[dst_idx] = dequant(static_cast<const int8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_UINT8) {
                            out[dst_idx] = dequant(static_cast<const uint8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_FLOAT32) {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        } else {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        }
                    }
                }
            }
        }
        return out;
    }
    if (attr.n_dims == 4) {
        int h = 0;
        int w = 0;
        int c = 0;
        const bool force_nchw = (attr.dims[1] == 32 || attr.dims[1] == 3 || attr.dims[1] == 1);
        const bool force_nhwc = (attr.dims[3] == 32 || attr.dims[3] == 3 || attr.dims[3] == 1);
        const bool is_nchw = force_nchw && !force_nhwc;
        if (is_nchw) {
            c = attr.dims[1];
            h = attr.dims[2];
            w = attr.dims[3];
            const uint32_t row_stride = w_stride > 0 ? w_stride : static_cast<uint32_t>(w);
            for (int ci = 0; ci < c; ++ci) {
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        const uint32_t src_idx = (ci * h + y) * row_stride + x;
                        const uint32_t dst_idx = (ci * h + y) * w + x;
                        if (attr.type == RKNN_TENSOR_INT8) {
                            out[dst_idx] = dequant(static_cast<const int8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_UINT8) {
                            out[dst_idx] = dequant(static_cast<const uint8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_FLOAT32) {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        } else {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        }
                    }
                }
            }
        } else {
            h = attr.dims[1];
            w = attr.dims[2];
            c = attr.dims[3];
            const uint32_t row_stride = (w_stride > 0 ? w_stride : static_cast<uint32_t>(w)) * static_cast<uint32_t>(c);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    for (int ci = 0; ci < c; ++ci) {
                        const uint32_t src_idx = y * row_stride + x * c + ci;
                        const uint32_t dst_idx = (y * w + x) * c + ci;
                        if (attr.type == RKNN_TENSOR_INT8) {
                            out[dst_idx] = dequant(static_cast<const int8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_UINT8) {
                            out[dst_idx] = dequant(static_cast<const uint8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                        } else if (attr.type == RKNN_TENSOR_FLOAT32) {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        } else {
                            out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                        }
                    }
                }
            }
        }
        return out;
    }
    if (attr.n_dims == 3) {
        const uint32_t h = attr.dims[1];
        const uint32_t w = attr.dims[2];
        const uint32_t row_stride = w_stride > 0 ? w_stride : w;
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const uint32_t src_idx = y * row_stride + x;
                const uint32_t dst_idx = y * w + x;
                if (attr.type == RKNN_TENSOR_INT8) {
                    out[dst_idx] = dequant(static_cast<const int8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                } else if (attr.type == RKNN_TENSOR_UINT8) {
                    out[dst_idx] = dequant(static_cast<const uint8_t *>(mem->virt_addr)[src_idx], attr.zp, attr.scale);
                } else if (attr.type == RKNN_TENSOR_FLOAT32) {
                    out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                } else {
                    out[dst_idx] = static_cast<const float *>(mem->virt_addr)[src_idx];
                }
            }
        }
        return out;
    }
    if (attr.type == RKNN_TENSOR_INT8) {
        const int8_t *src = static_cast<const int8_t *>(mem->virt_addr);
        for (uint32_t i = 0; i < attr.n_elems; ++i) {
            out[i] = dequant(src[i], attr.zp, attr.scale);
        }
    } else if (attr.type == RKNN_TENSOR_UINT8) {
        const uint8_t *src = static_cast<const uint8_t *>(mem->virt_addr);
        for (uint32_t i = 0; i < attr.n_elems; ++i) {
            out[i] = dequant(src[i], attr.zp, attr.scale);
        }
    } else if (attr.type == RKNN_TENSOR_FLOAT32) {
        const float *src = static_cast<const float *>(mem->virt_addr);
        std::memcpy(out.data(), src, attr.n_elems * sizeof(float));
    } else {
        const float *src = static_cast<const float *>(mem->virt_addr);
        std::memcpy(out.data(), src, attr.n_elems * sizeof(float));
    }
    return out;
}

static void softmax(float *data, int n)
{
    float max_val = data[0];
    for (int i = 1; i < n; ++i) {
        max_val = std::max(max_val, data[i]);
    }
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        data[i] = std::exp(data[i] - max_val);
        sum += data[i];
    }
    for (int i = 0; i < n; ++i) {
        data[i] /= sum;
    }
}

static float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

static float decode_dfl(const std::vector<float> &data,
                        const rknn_tensor_attr &attr,
                        int y,
                        int x,
                        int scale_h,
                        int scale_w,
                        int side)
{
    const int bins = 8;
    float bin_data[bins];
    const int hw = scale_h * scale_w;
    const int pos = y * scale_w + x;
    if (pos < 0 || pos >= hw) {
        return 0.0f;
    }

    if (attr.n_dims == 5 && attr.dims[1] == 2 && attr.dims[4] == 16) {
        const int group = side < 2 ? 0 : 1;
        const int within = side % 2;
        const int base = within * bins;
        for (int i = 0; i < bins; ++i) {
            const size_t idx = (static_cast<size_t>(group) * static_cast<size_t>(hw) + static_cast<size_t>(pos)) * 16u +
                               static_cast<size_t>(base + i);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else if (attr.n_dims == 4 && attr.dims[1] == 32) {
        const int h = static_cast<int>(attr.dims[2]);
        const int w = static_cast<int>(attr.dims[3]);
        if (h != scale_h || w != scale_w) {
            return 0.0f;
        }
        const int c_base = side * bins;
        for (int i = 0; i < bins; ++i) {
            const size_t idx = (static_cast<size_t>(c_base + i) * static_cast<size_t>(h) + static_cast<size_t>(y)) * static_cast<size_t>(w) +
                               static_cast<size_t>(x);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else if (attr.n_dims == 4 && attr.dims[3] == 32) {
        const int w = static_cast<int>(attr.dims[2]);
        const int ch = static_cast<int>(attr.dims[3]);
        if (static_cast<int>(attr.dims[1]) != scale_h || w != scale_w) {
            return 0.0f;
        }
        for (int i = 0; i < bins; ++i) {
            const size_t idx = (static_cast<size_t>(pos) * static_cast<size_t>(ch)) + static_cast<size_t>(side * bins + i);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else if (attr.n_dims == 4 && attr.dims[2] == 32 && attr.dims[3] == 1) {
        if (static_cast<int>(attr.dims[1]) != hw) {
            return 0.0f;
        }
        for (int i = 0; i < bins; ++i) {
            const size_t idx = static_cast<size_t>(pos) * 32u + static_cast<size_t>(side * bins + i);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else if (attr.n_dims == 3 && attr.dims[2] == 32) {
        for (int i = 0; i < bins; ++i) {
            const size_t idx = static_cast<size_t>(pos) * 32u + static_cast<size_t>(side * bins + i);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else if (attr.n_dims == 3 && attr.dims[1] == 32) {
        for (int i = 0; i < bins; ++i) {
            const size_t idx = static_cast<size_t>(side * bins + i) * static_cast<size_t>(hw) + static_cast<size_t>(pos);
            if (idx >= data.size()) {
                return 0.0f;
            }
            bin_data[i] = data[idx];
        }
    } else {
        return 0.0f;
    }

    softmax(bin_data, bins);
    float val = 0.0f;
    for (int i = 0; i < bins; ++i) {
        val += bin_data[i] * static_cast<float>(i);
    }
    return val;
}

static int choose_pattern(const std::vector<float> &data, int width, int height)
{
    const int patterns[3][6] = {
        {0, 1, 2, 3, 4, 5},
        {2, 3, 4, 5, 1, 0},
        {1, 2, 3, 4, 0, 5},
    };
    int best = 0;
    int best_count = -1;
    const int count = static_cast<int>(data.size() / 6);
    for (int p = 0; p < 3; ++p) {
        int valid = 0;
        for (int i = 0; i < count; ++i) {
            const float x1 = data[i * 6 + patterns[p][0]];
            const float y1 = data[i * 6 + patterns[p][1]];
            const float x2 = data[i * 6 + patterns[p][2]];
            const float y2 = data[i * 6 + patterns[p][3]];
            float score = data[i * 6 + patterns[p][4]];
            if (score > 1.0f && score <= 100.0f) {
                score /= 100.0f;
            }
            const float max_coord = std::max(std::max(x1, y1), std::max(x2, y2));
            float sx1 = x1;
            float sy1 = y1;
            float sx2 = x2;
            float sy2 = y2;
            if (max_coord <= 1.5f) {
                sx1 *= width;
                sx2 *= width;
                sy1 *= height;
                sy2 *= height;
            }
            if (score >= 0.2f && sx2 > sx1 && sy2 > sy1) {
                valid++;
            }
        }
        if (valid > best_count) {
            best_count = valid;
            best = p;
        }
    }
    return best;
}

struct FaceBox {
    cv::Rect rect;
    float score = 0.0f;
};

static std::vector<FaceBox> parse_face_boxes(const std::vector<float> &boxes, int width, int height)
{
    std::vector<FaceBox> out;
    if (boxes.size() < 6 || boxes.size() % 6 != 0) {
        return out;
    }
    const int patterns[3][6] = {
        {0, 1, 2, 3, 4, 5},
        {2, 3, 4, 5, 1, 0},
        {1, 2, 3, 4, 0, 5},
    };
    const int pattern = choose_pattern(boxes, width, height);
    const int count = static_cast<int>(boxes.size() / 6);
    for (int i = 0; i < count; ++i) {
        float x1 = boxes[i * 6 + patterns[pattern][0]];
        float y1 = boxes[i * 6 + patterns[pattern][1]];
        float x2 = boxes[i * 6 + patterns[pattern][2]];
        float y2 = boxes[i * 6 + patterns[pattern][3]];
        float score = boxes[i * 6 + patterns[pattern][4]];
        if (score > 1.0f && score <= 100.0f) {
            score /= 100.0f;
        }
        const float max_coord = std::max(std::max(x1, y1), std::max(x2, y2));
        if (max_coord <= 1.5f) {
            x1 *= width;
            x2 *= width;
            y1 *= height;
            y2 *= height;
        }
        if (score < 0.25f || x2 <= x1 || y2 <= y1) {
            continue;
        }
        int ix1 = std::max(0, static_cast<int>(std::round(x1)));
        int iy1 = std::max(0, static_cast<int>(std::round(y1)));
        int ix2 = std::min(width - 1, static_cast<int>(std::round(x2)));
        int iy2 = std::min(height - 1, static_cast<int>(std::round(y2)));
        if (ix2 - ix1 < 10 || iy2 - iy1 < 10) {
            continue;
        }
        FaceBox box;
        box.rect = cv::Rect(ix1, iy1, ix2 - ix1, iy2 - iy1);
        box.score = score;
        out.push_back(box);
    }
    return out;
}

/*
static float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}
*/

static uint8_t quantize_u8(float v, float scale, int32_t zp)
{
    if (scale <= 0.0f) {
        int32_t qi = static_cast<int32_t>(std::round(v));
        qi = std::max(0, std::min(255, qi));
        return static_cast<uint8_t>(qi);
    }
    float q = v / scale + static_cast<float>(zp);
    int32_t qi = static_cast<int32_t>(std::round(q));
    qi = std::max(0, std::min(255, qi));
    return static_cast<uint8_t>(qi);
}

static int8_t quantize_f32(float v, float scale, int32_t zp)
{
    if (scale <= 0.0f) {
        return static_cast<int8_t>(std::round(v));
    }
    float q = v / scale + static_cast<float>(zp);
    int32_t qi = static_cast<int32_t>(std::round(q));
    qi = std::max(-128, std::min(127, qi));
    return static_cast<int8_t>(qi);
}

/*
static float get_tensor_value(const std::vector<float> &data, const rknn_tensor_attr &attr, int c, int y, int x)
{
    if (attr.n_dims == 4) {
        if (attr.fmt == RKNN_TENSOR_NCHW) {
            const int h = attr.dims[2];
            const int w = attr.dims[3];
            return data[(c * h + y) * w + x];
        }
        const int w = attr.dims[2];
        const int ch = attr.dims[3];
        return data[(y * w + x) * ch + c];
    }
    if (attr.n_dims == 3) {
        const int w = attr.dims[2];
        const int idx = y * w + x;
        if (idx >= 0 && idx < static_cast<int>(data.size())) {
            return data[idx];
        }
    }
    return 0.0f;
}
*/

static float calc_iou(const cv::Rect2f &a, const cv::Rect2f &b)
{
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);
    const float w = std::max(0.0f, x2 - x1);
    const float h = std::max(0.0f, y2 - y1);
    const float inter = w * h;
    const float uni = a.width * a.height + b.width * b.height - inter;
    if (uni <= 0.0f) {
        return 0.0f;
    }
    return inter / uni;
}

static float calc_iomin(const cv::Rect2f &a, const cv::Rect2f &b)
{
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);
    const float w = std::max(0.0f, x2 - x1);
    const float h = std::max(0.0f, y2 - y1);
    const float inter = w * h;
    const float area_a = a.width * a.height;
    const float area_b = b.width * b.height;
    const float denom = std::min(area_a, area_b);
    if (denom <= 0.0f) {
        return 0.0f;
    }
    return inter / denom;
}

static std::vector<int> find_score_outputs(const rknn_tensor_attr *attrs, int output_count)
{
    std::vector<int> out;
    out.reserve(output_count);
    for (int i = 0; i < output_count; ++i) {
        const auto &a = attrs[i];
        if (a.n_elems == 0) {
            continue;
        }
        if (a.n_dims == 3) {
            if (a.dims[1] == 1 || a.dims[2] == 1) {
                out.push_back(i);
            }
            continue;
        }
        if (a.n_dims == 4) {
            if (a.dims[1] == 1 || a.dims[3] == 1) {
                out.push_back(i);
            }
            continue;
        }
    }
    return out;
}

static bool score_supports_hw(const rknn_tensor_attr &a, int hw)
{
    if (hw <= 0) {
        return false;
    }
    if (a.n_elems == static_cast<uint32_t>(hw)) {
        return true;
    }
    if (a.n_elems > static_cast<uint32_t>(hw) && (a.n_elems % static_cast<uint32_t>(hw)) == 0) {
        return true;
    }
    if (a.n_dims == 3) {
        if (static_cast<int>(a.dims[1]) == hw || static_cast<int>(a.dims[2]) == hw) {
            return true;
        }
    } else if (a.n_dims == 4) {
        if (static_cast<int>(a.dims[1]) == hw || static_cast<int>(a.dims[2]) == hw || static_cast<int>(a.dims[3]) == hw) {
            return true;
        }
    } else if (a.n_dims == 5) {
        if (static_cast<int>(a.dims[2] * a.dims[3]) == hw || static_cast<int>(a.dims[1] * a.dims[2] * a.dims[3]) == hw) {
            return true;
        }
    }
    return false;
}

static std::vector<int> find_reg_outputs(const rknn_tensor_attr *attrs, int output_count)
{
    std::vector<int> out;
    out.reserve(output_count);
    for (int i = 0; i < output_count; ++i) {
        const auto &a = attrs[i];
        if (a.n_elems == 0) {
            continue;
        }
        if ((a.n_elems % 32) != 0) {
            continue;
        }
        if (a.n_dims >= 4) {
            out.push_back(i);
        }
    }
    return out;
}

static bool try_build_scales(std::vector<int> *score_indices,
                             std::vector<int> *reg_indices,
                             std::vector<int> *strides,
                             const rknn_tensor_attr *attrs,
                             int output_count,
                             int model_w,
                             int model_h)
{
    if (!score_indices || !reg_indices || !strides) {
        return false;
    }
    score_indices->clear();
    reg_indices->clear();
    strides->clear();

    const auto scores = find_score_outputs(attrs, output_count);
    const auto regs = find_reg_outputs(attrs, output_count);
    if (scores.empty() || regs.empty()) {
        return false;
    }

    struct Pair {
        int stride;
        int score_idx;
        int reg_idx;
        int h;
        int w;
    };
    std::vector<Pair> pairs;
    pairs.reserve(regs.size());

    for (int reg_i : regs) {
        const int hw = static_cast<int>(attrs[reg_i].n_elems / 32u);
        if (hw <= 0) {
            continue;
        }
        int s = static_cast<int>(std::round(std::sqrt(static_cast<float>(hw))));
        if (s <= 0 || s * s != hw) {
            continue;
        }
        const int h = s;
        const int w = s;
        if ((model_w % w) != 0 || (model_h % h) != 0) {
            continue;
        }
        const int stride = model_w / w;
        if (!(stride == 8 || stride == 16 || stride == 32 || stride == 64)) {
            continue;
        }
        int matched_score = -1;
        for (int score_i : scores) {
            if (score_supports_hw(attrs[score_i], hw)) {
                matched_score = score_i;
                break;
            }
        }
        if (matched_score < 0) {
            continue;
        }
        pairs.push_back({stride, matched_score, reg_i, h, w});
    }

    if (pairs.size() < 3) {
        return false;
    }
    std::sort(pairs.begin(), pairs.end(), [](const Pair &a, const Pair &b) { return a.stride < b.stride; });
    pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const Pair &a, const Pair &b) { return a.stride == b.stride; }),
                pairs.end());
    if (pairs.size() < 3) {
        return false;
    }

    const size_t take = std::min<size_t>(4, pairs.size());
    for (size_t i = 0; i < take; ++i) {
        score_indices->push_back(pairs[i].score_idx);
        reg_indices->push_back(pairs[i].reg_idx);
        strides->push_back(pairs[i].stride);
    }
    return true;
}

static float score_value_at(const std::vector<float> &data,
                            const rknn_tensor_attr &attr,
                            int pos,
                            int scale_h,
                            int scale_w)
{
    if (pos < 0) {
        return 0.0f;
    }
    const int hw = scale_h * scale_w;
    if (hw <= 0 || pos >= hw) {
        return 0.0f;
    }
    if (attr.n_dims == 3) {
        if (attr.n_elems == static_cast<uint32_t>(hw)) {
            const size_t idx = static_cast<size_t>(pos);
            if (idx < data.size()) {
                return data[idx];
            }
        }
        if (static_cast<int>(attr.dims[1]) == hw) {
            const int cls = static_cast<int>(attr.dims[2]);
            if (cls <= 1) {
                const size_t idx = static_cast<size_t>(pos);
                if (idx < data.size()) {
                    return data[idx];
                }
                return 0.0f;
            }
            const size_t base = static_cast<size_t>(pos) * static_cast<size_t>(cls);
            if (base >= data.size()) {
                return 0.0f;
            }
            float best = data[base];
            const int lim = std::min<int>(cls, static_cast<int>(data.size() - base));
            for (int c = 1; c < lim; ++c) {
                best = std::max(best, data[base + static_cast<size_t>(c)]);
            }
            return best;
        }
        if (static_cast<int>(attr.dims[2]) == hw) {
            const int cls = static_cast<int>(attr.dims[1]);
            if (cls <= 1) {
                const size_t idx = static_cast<size_t>(pos);
                if (idx < data.size()) {
                    return data[idx];
                }
                return 0.0f;
            }
            float best = data[static_cast<size_t>(pos)];
            for (int c = 1; c < cls; ++c) {
                const size_t idx = static_cast<size_t>(c) * static_cast<size_t>(hw) + static_cast<size_t>(pos);
                if (idx >= data.size()) {
                    break;
                }
                best = std::max(best, data[idx]);
            }
            return best;
        }
    } else if (attr.n_dims == 4) {
        if (static_cast<int>(attr.dims[1]) == hw && attr.dims[3] == 1) {
            const int cls = static_cast<int>(attr.dims[2]);
            if (cls <= 1) {
                const size_t idx = static_cast<size_t>(pos);
                if (idx < data.size()) {
                    return data[idx];
                }
                return 0.0f;
            }
            const size_t base = static_cast<size_t>(pos) * static_cast<size_t>(cls);
            if (base >= data.size()) {
                return 0.0f;
            }
            float best = data[base];
            const int lim = std::min<int>(cls, static_cast<int>(data.size() - base));
            for (int c = 1; c < lim; ++c) {
                best = std::max(best, data[base + static_cast<size_t>(c)]);
            }
            return best;
        }
        if (attr.dims[1] == 1 && static_cast<int>(attr.dims[2] * attr.dims[3]) == hw) {
            const size_t idx = static_cast<size_t>(pos);
            if (idx < data.size()) {
                return data[idx];
            }
            return 0.0f;
        }
        if (static_cast<int>(attr.dims[1] * attr.dims[2]) == hw) {
            const int cls = static_cast<int>(attr.dims[3]);
            if (cls <= 1) {
                const size_t idx = static_cast<size_t>(pos);
                if (idx < data.size()) {
                    return data[idx];
                }
                return 0.0f;
            }
            const size_t base = static_cast<size_t>(pos) * static_cast<size_t>(cls);
            if (base >= data.size()) {
                return 0.0f;
            }
            float best = data[base];
            const int lim = std::min<int>(cls, static_cast<int>(data.size() - base));
            for (int c = 1; c < lim; ++c) {
                best = std::max(best, data[base + static_cast<size_t>(c)]);
            }
            return best;
        }
        if (static_cast<int>(attr.dims[2] * attr.dims[3]) == hw) {
            const int cls = static_cast<int>(attr.dims[1]);
            if (cls <= 1) {
                const size_t idx = static_cast<size_t>(pos);
                if (idx < data.size()) {
                    return data[idx];
                }
                return 0.0f;
            }
            float best = data[static_cast<size_t>(pos)];
            for (int c = 1; c < cls; ++c) {
                const size_t idx = static_cast<size_t>(c) * static_cast<size_t>(hw) + static_cast<size_t>(pos);
                if (idx >= data.size()) {
                    break;
                }
                best = std::max(best, data[idx]);
            }
            return best;
        }
    }
    const size_t idx = static_cast<size_t>(pos);
    if (idx >= data.size()) {
        return 0.0f;
    }
    return data[idx];
}

static std::vector<FaceBox> decode_face_outputs(const rknn_tensor_attr *attrs,
                                                const std::vector<rknn_tensor_mem *> &mems,
                                                int output_count,
                                                int model_w,
                                                int model_h,
                                                int frame_w,
                                                int frame_h,
                                                float letterbox_scale,
                                                int letterbox_pad_x,
                                                int letterbox_pad_y,
                                                float *out_max_score = nullptr)
{
    struct ScaleInfo {
        int h;
        int w;
        int stride;
        int score_idx;
        int reg_idx;
    };

    std::vector<int> score_indices;
    std::vector<int> reg_indices;
    std::vector<int> strides;
    const bool built = try_build_scales(&score_indices, &reg_indices, &strides, attrs, output_count, model_w, model_h);

    std::vector<ScaleInfo> scales;
    if (built) {
        scales.reserve(4);
        for (size_t i = 0; i < strides.size() && i < score_indices.size() && i < reg_indices.size(); ++i) {
            const auto &sa = attrs[score_indices[i]];
            int hw = static_cast<int>(sa.n_elems);
            if (sa.n_dims == 3) {
                if (sa.dims[1] == 1) hw = sa.dims[2];
                else if (sa.dims[2] == 1) hw = sa.dims[1];
            } else if (sa.n_dims == 4) {
                if (sa.dims[1] == 1) hw = sa.dims[2] * sa.dims[3];
                else if (sa.dims[3] == 1) hw = sa.dims[1] * sa.dims[2];
            }
            int s = static_cast<int>(std::round(std::sqrt(static_cast<float>(hw))));
            if (s <= 0 || s * s != hw) {
                continue;
            }
            scales.push_back({s, s, strides[i], score_indices[i], reg_indices[i]});
        }
    }
    if (scales.empty()) {
        scales = {
            {40, 40, 8, 0, 1},
            {20, 20, 16, 2, 3},
            {10, 10, 32, 4, 5},
            {5, 5, 64, 6, 7}
        };
    }

    struct FloatBox {
        cv::Rect2f rect;
        float score;
    };

    const float score_thresh = 0.20f;
    const float nms_thresh = 0.30f;
    std::vector<FloatBox> candidates;
    float max_all_score = 0.0f;

    for (const auto &scale : scales) {
        const auto &score_attr = attrs[scale.score_idx];
        const auto &reg_attr = attrs[scale.reg_idx];
        const bool score_mem_ok = (mems[scale.score_idx] && mems[scale.score_idx]->virt_addr);
        const bool reg_mem_ok = (mems[scale.reg_idx] && mems[scale.reg_idx]->virt_addr);
        std::vector<float> score_data = read_tensor(score_attr, mems[scale.score_idx]);
        std::vector<float> reg_data = read_tensor(reg_attr, mems[scale.reg_idx]);
        static int score_dbg_frame = 0;
        if (score_dbg_frame < 20 || score_dbg_frame % 50 == 0) {
            std::cerr << "[FaceTensor] stride=" << scale.stride
                      << " score_idx=" << scale.score_idx
                      << " reg_idx=" << scale.reg_idx
                      << " score_mem=" << (score_mem_ok ? 1 : 0)
                      << " reg_mem=" << (reg_mem_ok ? 1 : 0)
                      << " score_elems=" << score_data.size()
                      << " reg_elems=" << reg_data.size()
                      << " hw=" << (scale.h * scale.w) << std::endl;
        }

        if (score_data.empty() || reg_data.empty()) continue;

        float local_min = std::numeric_limits<float>::infinity();
        float local_max = -std::numeric_limits<float>::infinity();
        const int local_check = std::min<int>(256, static_cast<int>(score_data.size()));
        for (int i = 0; i < local_check; ++i) {
            local_min = std::min(local_min, score_data[i]);
            local_max = std::max(local_max, score_data[i]);
        }
        const bool score_is_prob = (local_min >= -0.05f && local_max <= 1.05f);
        const bool force_sigmoid = !score_is_prob;
        if (score_dbg_frame < 20 || score_dbg_frame % 50 == 0) {
            std::cerr << "[FaceScore] stride=" << scale.stride << " idx=" << scale.score_idx
                      << " min=" << local_min << " max=" << local_max
                      << " is_prob=" << (score_is_prob ? 1 : 0)
                      << " force_sigmoid=" << (force_sigmoid ? 1 : 0)
                      << " elems=" << score_data.size()
                      << " zp=" << score_attr.zp
                      << " scale=" << score_attr.scale
                      << " fmt=" << score_attr.fmt
                      << " type=" << score_attr.type
                      << " dims=";
            for (uint32_t d = 0; d < score_attr.n_dims; ++d) {
                std::cerr << score_attr.dims[d];
                if (d + 1 < score_attr.n_dims) std::cerr << "x";
            }
            std::cerr << " first=";
            const int show = std::min<int>(6, static_cast<int>(score_data.size()));
            for (int i = 0; i < show; ++i) {
                std::cerr << score_data[i];
                if (i + 1 < show) std::cerr << ",";
            }
            if (mems[scale.score_idx] && mems[scale.score_idx]->virt_addr && score_attr.type == RKNN_TENSOR_INT8) {
                const int8_t *raw = static_cast<const int8_t *>(mems[scale.score_idx]->virt_addr);
                std::cerr << " raw=";
                for (int i = 0; i < show; ++i) {
                    std::cerr << static_cast<int>(raw[i]);
                    if (i + 1 < show) std::cerr << ",";
                }
            }
            std::cerr << std::endl;
        }

        for (int y = 0; y < scale.h; ++y) {
            for (int x = 0; x < scale.w; ++x) {
                int score_idx = y * scale.w + x;
                if (score_idx < 0) {
                    continue;
                }
                float score = score_value_at(score_data, score_attr, score_idx, scale.h, scale.w);
                if (force_sigmoid) {
                    score = sigmoid(score);
                }
                if (score > max_all_score) max_all_score = score;

                if (score < score_thresh) continue;

                // x1 = (x + 0.5 * (stride-1)) - l * stride
                // y1 = (y + 0.5 * (stride-1)) - t * stride
                // x2 = (x + 0.5 * (stride-1)) + r * stride
                // y2 = (y + 0.5 * (stride-1)) + b * stride
                float l = decode_dfl(reg_data, reg_attr, y, x, scale.h, scale.w, 0);
                float t = decode_dfl(reg_data, reg_attr, y, x, scale.h, scale.w, 1);
                float r = decode_dfl(reg_data, reg_attr, y, x, scale.h, scale.w, 2);
                float b = decode_dfl(reg_data, reg_attr, y, x, scale.h, scale.w, 3);

                float cx = (static_cast<float>(x) * scale.stride) + 0.5f * (scale.stride - 1);
                float cy = (static_cast<float>(y) * scale.stride) + 0.5f * (scale.stride - 1);

                float x1 = cx - l * scale.stride;
                float y1 = cy - t * scale.stride;
                float x2 = cx + r * scale.stride;
                float y2 = cy + b * scale.stride;

                x1 = std::max(0.0f, std::min(x1, static_cast<float>(model_w)));
                y1 = std::max(0.0f, std::min(y1, static_cast<float>(model_h)));
                x2 = std::max(0.0f, std::min(x2, static_cast<float>(model_w)));
                y2 = std::max(0.0f, std::min(y2, static_cast<float>(model_h)));
                if (x2 <= x1 || y2 <= y1) {
                    continue;
                }
                const float bw = x2 - x1;
                const float bh = y2 - y1;
                if (bw < 20.0f || bh < 20.0f) {
                    continue;
                }
                if (bw > model_w * 0.95f || bh > model_h * 0.95f) {
                    continue;
                }
                const float ratio = bw / bh;
                if (ratio < 0.5f || ratio > 2.0f) {
                    continue;
                }

                candidates.push_back({cv::Rect2f(x1, y1, x2 - x1, y2 - y1), score});
            }
        }
        score_dbg_frame++;
    }

    std::sort(candidates.begin(), candidates.end(), [](const FloatBox &a, const FloatBox &b) {
        return a.score > b.score;
    });
    if (candidates.size() > 1000) {
        candidates.resize(1000);
    }

    static int decode_frame = 0;
    if (decode_frame++ % 50 == 0) {
        std::cerr << std::scientific << std::setprecision(6);
        std::cerr << "[FaceDecode] max_score=" << max_all_score << " candidates=" << candidates.size()
                  << " scales_built=" << (built ? 1 : 0) << std::endl;
        std::cerr << std::defaultfloat;
        if (!built) {
            for (int i = 0; i < output_count; ++i) {
                const auto &a = attrs[i];
                std::cerr << "[FaceDecode] out[" << i << "] fmt=" << a.fmt << " type=" << a.type << " dims=";
                for (uint32_t d = 0; d < a.n_dims; ++d) {
                    std::cerr << a.dims[d];
                    if (d + 1 < a.n_dims) std::cerr << "x";
                }
                std::cerr << " elems=" << a.n_elems << std::endl;
            }
        }
    }

    std::vector<bool> is_suppressed(candidates.size(), false);
    std::vector<FaceBox> out;
    const bool use_letterbox = letterbox_scale > 1e-6f;

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (is_suppressed[i]) continue;
        const auto &best = candidates[i];
        
        FaceBox box;
        if (use_letterbox) {
            const float mx1 = best.rect.x;
            const float my1 = best.rect.y;
            const float mx2 = best.rect.x + best.rect.width;
            const float my2 = best.rect.y + best.rect.height;
            const float fx1 = (mx1 - static_cast<float>(letterbox_pad_x)) / letterbox_scale;
            const float fy1 = (my1 - static_cast<float>(letterbox_pad_y)) / letterbox_scale;
            const float fx2 = (mx2 - static_cast<float>(letterbox_pad_x)) / letterbox_scale;
            const float fy2 = (my2 - static_cast<float>(letterbox_pad_y)) / letterbox_scale;
            const int x1 = std::max(0, std::min(frame_w - 1, static_cast<int>(std::round(fx1))));
            const int y1 = std::max(0, std::min(frame_h - 1, static_cast<int>(std::round(fy1))));
            const int x2 = std::max(0, std::min(frame_w - 1, static_cast<int>(std::round(fx2))));
            const int y2 = std::max(0, std::min(frame_h - 1, static_cast<int>(std::round(fy2))));
            if (x2 <= x1 || y2 <= y1) {
                continue;
            }
            box.rect.x = x1;
            box.rect.y = y1;
            box.rect.width = x2 - x1;
            box.rect.height = y2 - y1;
        } else {
            float sx = (float)frame_w / model_w;
            float sy = (float)frame_h / model_h;
            box.rect.x = std::max(0, (int)(best.rect.x * sx));
            box.rect.y = std::max(0, (int)(best.rect.y * sy));
            box.rect.width = std::min(frame_w - box.rect.x, (int)(best.rect.width * sx));
            box.rect.height = std::min(frame_h - box.rect.y, (int)(best.rect.height * sy));
        }
        box.score = best.score;
        out.push_back(box);

        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (is_suppressed[j]) continue;
            const float iou = calc_iou(best.rect, candidates[j].rect);
            if (iou > nms_thresh) {
                is_suppressed[j] = true;
                continue;
            }
            const float iomin = calc_iomin(best.rect, candidates[j].rect);
            if (iomin > 0.90f) {
                is_suppressed[j] = true;
                continue;
            }
        }
    }

    if (out_max_score) {
        *out_max_score = max_all_score;
    }
    return out;
}
}

struct FatigueDetectorRunner::RknnModel {
    rknn_context ctx = 0;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs = nullptr;
    rknn_tensor_attr *output_attrs = nullptr;
    rknn_tensor_mem *input_mems[1] = {nullptr};
    std::vector<rknn_tensor_mem *> output_mems;
    int width = 0;
    int height = 0;
    int channel = 0;
    bool ready = false;

    bool init(const std::string &model_path)
    {
        if (ready) {
            deinit();
        }
        std::memset(&io_num, 0, sizeof(io_num));
        if (rknn_init(&ctx, const_cast<char *>(model_path.c_str()), 0, 0, nullptr) < 0) {
            return false;
        }
        if (rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num)) != RKNN_SUCC) {
            return false;
        }
        rknn_tensor_attr input_attr[1];
        std::memset(input_attr, 0, sizeof(input_attr));
        input_attr[0].index = 0;
        if (rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &input_attr[0], sizeof(rknn_tensor_attr)) != RKNN_SUCC) {
            return false;
        }
        rknn_tensor_attr output_attr[io_num.n_output];
        std::memset(output_attr, 0, sizeof(output_attr));
        for (uint32_t i = 0; i < io_num.n_output; ++i) {
            output_attr[i].index = i;
            if (rknn_query(ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &output_attr[i], sizeof(rknn_tensor_attr)) != RKNN_SUCC) {
                return false;
            }
        }
        input_mems[0] = rknn_create_mem(ctx, input_attr[0].size_with_stride);
        if (!input_mems[0] || rknn_set_io_mem(ctx, input_mems[0], &input_attr[0]) < 0) {
            return false;
        }
        output_mems.resize(io_num.n_output);
        for (uint32_t i = 0; i < io_num.n_output; ++i) {
            output_mems[i] = rknn_create_mem(ctx, output_attr[i].size_with_stride);
            if (!output_mems[i] || rknn_set_io_mem(ctx, output_mems[i], &output_attr[i]) < 0) {
                return false;
            }
        }
        input_attrs = static_cast<rknn_tensor_attr *>(malloc(sizeof(rknn_tensor_attr)));
        std::memcpy(input_attrs, input_attr, sizeof(rknn_tensor_attr));
        output_attrs = static_cast<rknn_tensor_attr *>(malloc(io_num.n_output * sizeof(rknn_tensor_attr)));
        std::memcpy(output_attrs, output_attr, io_num.n_output * sizeof(rknn_tensor_attr));
        if (input_attr[0].fmt == RKNN_TENSOR_NCHW) {
            channel = input_attr[0].dims[1];
            height = input_attr[0].dims[2];
            width = input_attr[0].dims[3];
        } else {
            height = input_attr[0].dims[1];
            width = input_attr[0].dims[2];
            channel = input_attr[0].dims[3];
        }
        ready = true;
        return true;
    }

    void deinit()
    {
        if (!ready) {
            return;
        }
        if (input_attrs) {
            free(input_attrs);
            input_attrs = nullptr;
        }
        if (output_attrs) {
            free(output_attrs);
            output_attrs = nullptr;
        }
        if (input_mems[0]) {
            rknn_destroy_mem(ctx, input_mems[0]);
            free(input_mems[0]);
            input_mems[0] = nullptr;
        }
        for (auto &mem : output_mems) {
            if (mem) {
                rknn_destroy_mem(ctx, mem);
                free(mem);
                mem = nullptr;
            }
        }
        output_mems.clear();
        if (ctx != 0) {
            rknn_destroy(ctx);
            ctx = 0;
        }
        std::memset(&io_num, 0, sizeof(io_num));
        width = 0;
        height = 0;
        channel = 0;
        ready = false;
    }

    uint8_t *inputBuffer()
    {
        if (!ready || !input_mems[0]) {
            return nullptr;
        }
        return static_cast<uint8_t *>(input_mems[0]->virt_addr);
    }

    bool run()
    {
        if (!ready) {
            return false;
        }
        if (input_mems[0]) {
            rknn_mem_sync(ctx, input_mems[0], RKNN_MEMORY_SYNC_TO_DEVICE);
        }
        if (rknn_run(ctx, nullptr) != RKNN_SUCC) {
            return false;
        }
        for (auto *mem : output_mems) {
            if (mem) {
                rknn_mem_sync(ctx, mem, RKNN_MEMORY_SYNC_FROM_DEVICE);
            }
        }
        return true;
    }
};

FatigueDetectorRunner::FatigueDetectorRunner()
{
    m_mouthPoints.clear();
    m_mouthPoints.insert(m_mouthPoints.end(), kMouthOutline.begin(), kMouthOutline.end());
    m_mouthPoints.insert(m_mouthPoints.end(), kMouthInner.begin(), kMouthInner.end());
}

FatigueDetectorRunner::~FatigueDetectorRunner()
{
    deinit();
}

bool FatigueDetectorRunner::init(const std::string &paddle_model_path, const std::string &pfld_rknn_path, int pfld_size)
{
    if (m_ready) {
        deinit();
    }
    std::memset(&m_retinafaceCtx, 0, sizeof(m_retinafaceCtx));
    if (init_retinaface_model(paddle_model_path.c_str(), &m_retinafaceCtx) != 0) {
        return false;
    }
    m_pfldModel = std::make_unique<RknnModel>();
    if (!m_pfldModel->init(pfld_rknn_path)) {
        release_retinaface_model(&m_retinafaceCtx);
        return false;
    }
    std::cerr << "[FaceModel] input dims=" << 1 << "x" << m_retinafaceCtx.model_height << "x"
              << m_retinafaceCtx.model_width << "x" << m_retinafaceCtx.model_channel
              << " w=" << m_retinafaceCtx.model_width << " h=" << m_retinafaceCtx.model_height
              << " c=" << m_retinafaceCtx.model_channel << std::endl;
    if (m_pfldModel && m_pfldModel->input_attrs) {
        const auto &pa = m_pfldModel->input_attrs[0];
        std::cerr << "[PFLDModel] input dims=";
        for (uint32_t d = 0; d < pa.n_dims; ++d) {
            std::cerr << pa.dims[d];
            if (d + 1 < pa.n_dims) std::cerr << "x";
        }
        std::cerr << " w=" << m_pfldModel->width << " h=" << m_pfldModel->height << " c=" << m_pfldModel->channel
                  << std::endl;
    }
    m_pfldSize = pfld_size;
    m_ready = true;
    return true;
}

void FatigueDetectorRunner::deinit()
{
    m_ready = false;
    m_pfldSize = 0;
    if (m_retinafaceCtx.rknn_ctx != 0) {
        release_retinaface_model(&m_retinafaceCtx);
        std::memset(&m_retinafaceCtx, 0, sizeof(m_retinafaceCtx));
    }
    if (m_pfldModel) {
        m_pfldModel->deinit();
        m_pfldModel.reset();
    }
    m_consecutiveEyeClosed = 0;
    m_consecutiveMouthOpen = 0;
    m_eyeStateBuffer.clear();
    m_perclos = 0.0f;
}

bool FatigueDetectorRunner::isReady() const
{
    return m_ready;
}

void FatigueDetectorRunner::setThresholds(float ear_threshold, float mar_threshold, int eye_closed_frames,
                                          int mouth_open_frames)
{
    m_earThreshold = ear_threshold;
    m_marThreshold = mar_threshold;
    m_eyeClosedFrames = eye_closed_frames;
    m_mouthOpenFrames = mouth_open_frames;
}

void FatigueDetectorRunner::setPerclosWindow(int window)
{
    m_perclosWindow = std::max(1, window);
    m_eyeStateBuffer.clear();
    m_perclos = 0.0f;
}

int FatigueDetectorRunner::run(const cv::Mat &frame, FatigueResult *result, cv::Mat *annotated)
{
    if (!m_ready || frame.empty() || !result || m_retinafaceCtx.rknn_ctx == 0 || !m_pfldModel) {
        return -1;
    }
    result->faces.clear();
    if (annotated) {
        *annotated = frame.clone();
    }
    cv::Mat rgb_frame;
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);

    retinaface_result retina_result;
    std::memset(&retina_result, 0, sizeof(retina_result));

    if (inference_retinaface_model(&m_retinafaceCtx, frame.cols, frame.rows,
                                    rgb_frame.data, rgb_frame.step,
                                    &retina_result) != 0) {
        return -1;
    }

    bool metricsUpdated = false;

    for (int i = 0; i < retina_result.count; ++i) {
        const auto &det = retina_result.object[i];
        int left = det.box.left;
        int top = det.box.top;
        int right = det.box.right;
        int bottom = det.box.bottom;
        left = std::max(0, std::min(frame.cols - 1, left));
        top = std::max(0, std::min(frame.rows - 1, top));
        right = std::max(0, std::min(frame.cols - 1, right));
        bottom = std::max(0, std::min(frame.rows - 1, bottom));
        if (right <= left || bottom <= top) {
            continue;
        }
        cv::Rect face_rect(left, top, right - left, bottom - top);
        face_rect.width = std::min(face_rect.width, frame.cols - face_rect.x);
        face_rect.height = std::min(face_rect.height, frame.rows - face_rect.y);
        if (face_rect.width <= 10 || face_rect.height <= 10) {
            continue;
        }

        cv::Mat face_roi = frame(face_rect);
        cv::Mat pfld_resized;
        cv::resize(face_roi, pfld_resized, cv::Size(m_pfldSize, m_pfldSize));
        cv::cvtColor(pfld_resized, pfld_resized, cv::COLOR_BGR2RGB);
        uint8_t *pfld_input = m_pfldModel->inputBuffer();
        if (!pfld_input) {
            continue;
        }
        const auto &pfld_attr = m_pfldModel->input_attrs[0];
        const int pfld_w = pfld_resized.cols;
        const int pfld_h = pfld_resized.rows;
        if (pfld_attr.fmt == RKNN_TENSOR_NCHW) {
            if (pfld_attr.type == RKNN_TENSOR_INT8) {
                auto *dst = reinterpret_cast<int8_t *>(pfld_input);
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    for (int x = 0; x < pfld_w; ++x) {
                        const uint8_t r = row[x * 3 + 0];
                        const uint8_t g = row[x * 3 + 1];
                        const uint8_t b = row[x * 3 + 2];
                        dst[0 * pfld_w * pfld_h + y * pfld_w + x] =
                            quantize_f32(static_cast<float>(r) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                        dst[1 * pfld_w * pfld_h + y * pfld_w + x] =
                            quantize_f32(static_cast<float>(g) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                        dst[2 * pfld_w * pfld_h + y * pfld_w + x] =
                            quantize_f32(static_cast<float>(b) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                    }
                }
            } else if (pfld_attr.type == RKNN_TENSOR_UINT8) {
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    for (int x = 0; x < pfld_w; ++x) {
                        const uint8_t r = row[x * 3 + 0];
                        const uint8_t g = row[x * 3 + 1];
                        const uint8_t b = row[x * 3 + 2];
                        pfld_input[0 * pfld_w * pfld_h + y * pfld_w + x] = r;
                        pfld_input[1 * pfld_w * pfld_h + y * pfld_w + x] = g;
                        pfld_input[2 * pfld_w * pfld_h + y * pfld_w + x] = b;
                    }
                }
            } else if (pfld_attr.type == RKNN_TENSOR_FLOAT32) {
                auto *dst = reinterpret_cast<float *>(pfld_input);
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    for (int x = 0; x < pfld_w; ++x) {
                        const uint8_t r = row[x * 3 + 0];
                        const uint8_t g = row[x * 3 + 1];
                        const uint8_t b = row[x * 3 + 2];
                        dst[0 * pfld_w * pfld_h + y * pfld_w + x] = static_cast<float>(r) / 255.0f;
                        dst[1 * pfld_w * pfld_h + y * pfld_w + x] = static_cast<float>(g) / 255.0f;
                        dst[2 * pfld_w * pfld_h + y * pfld_w + x] = static_cast<float>(b) / 255.0f;
                    }
                }
            } else {
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    for (int x = 0; x < pfld_w; ++x) {
                        const uint8_t r = row[x * 3 + 0];
                        const uint8_t g = row[x * 3 + 1];
                        const uint8_t b = row[x * 3 + 2];
                        pfld_input[0 * pfld_w * pfld_h + y * pfld_w + x] = r;
                        pfld_input[1 * pfld_w * pfld_h + y * pfld_w + x] = g;
                        pfld_input[2 * pfld_w * pfld_h + y * pfld_w + x] = b;
                    }
                }
            }
        } else {
            if (pfld_attr.type == RKNN_TENSOR_INT8) {
                auto *dst = reinterpret_cast<int8_t *>(pfld_input);
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    int base = y * pfld_w * 3;
                    for (int x = 0; x < pfld_w; ++x) {
                        dst[base + x * 3 + 0] =
                            quantize_f32(static_cast<float>(row[x * 3 + 0]) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                        dst[base + x * 3 + 1] =
                            quantize_f32(static_cast<float>(row[x * 3 + 1]) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                        dst[base + x * 3 + 2] =
                            quantize_f32(static_cast<float>(row[x * 3 + 2]) / 255.0f, pfld_attr.scale, pfld_attr.zp);
                    }
                }
            } else if (pfld_attr.type == RKNN_TENSOR_UINT8) {
                const int pfld_row_bytes = pfld_resized.cols * pfld_resized.channels();
                for (int y = 0; y < pfld_resized.rows; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    std::memcpy(pfld_input + y * pfld_row_bytes, row, pfld_row_bytes);
                }
            } else if (pfld_attr.type == RKNN_TENSOR_FLOAT32) {
                auto *dst = reinterpret_cast<float *>(pfld_input);
                for (int y = 0; y < pfld_h; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    int base = y * pfld_w * 3;
                    for (int x = 0; x < pfld_w; ++x) {
                        dst[base + x * 3 + 0] = static_cast<float>(row[x * 3 + 0]) / 255.0f;
                        dst[base + x * 3 + 1] = static_cast<float>(row[x * 3 + 1]) / 255.0f;
                        dst[base + x * 3 + 2] = static_cast<float>(row[x * 3 + 2]) / 255.0f;
                    }
                }
            } else {
                const int pfld_row_bytes = pfld_resized.cols * pfld_resized.channels();
                for (int y = 0; y < pfld_resized.rows; ++y) {
                    const uint8_t *row = pfld_resized.ptr<uint8_t>(y);
                    std::memcpy(pfld_input + y * pfld_row_bytes, row, pfld_row_bytes);
                }
            }
        }
        if (!m_pfldModel->run()) {
            continue;
        }

        std::vector<float> pfld_out = read_tensor(m_pfldModel->output_attrs[0], m_pfldModel->output_mems[0]);
        if (pfld_out.size() < 212) {
            continue;
        }
        const int total_points = 106;
        std::vector<cv::Point2f> landmarks;
        landmarks.reserve(total_points);
        for (int i = 0; i < total_points; i++) {
            float x = pfld_out[i * 2];
            float y = pfld_out[i * 2 + 1];
            if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f) {
                x = (x + 1.0f) * 0.5f;
                y = (y + 1.0f) * 0.5f;
            }
            x = std::max(0.0f, std::min(1.0f, x));
            y = std::max(0.0f, std::min(1.0f, y));
            x = x * m_pfldSize;
            y = y * m_pfldSize;
            float scale_x = static_cast<float>(face_rect.width) / m_pfldSize;
            float scale_y = static_cast<float>(face_rect.height) / m_pfldSize;
            x = x * scale_x + face_rect.x;
            y = y * scale_y + face_rect.y;
            landmarks.push_back(cv::Point2f(x, y));
        }

        const bool landmark_valid = is_valid_landmark_face(landmarks, face_rect);
        float ear_avg = m_earThreshold + 0.05f;
        float mar = 0.0f;
        bool is_tired = false;

        if (landmark_valid) {
            std::vector<cv::Point2f> left_eye;
            std::vector<cv::Point2f> right_eye;
            for (int idx : kLeftEyePoints) {
                if (idx < static_cast<int>(landmarks.size())) {
                    left_eye.push_back(landmarks[idx]);
                }
            }
            for (int idx : kRightEyePoints) {
                if (idx < static_cast<int>(landmarks.size())) {
                    right_eye.push_back(landmarks[idx]);
                }
            }
            
            std::vector<cv::Point2f> mouth;
            for (int idx : m_mouthPoints) {
                if (idx < static_cast<int>(landmarks.size())) {
                    mouth.push_back(landmarks[idx]);
                }
            }
            
            float ear_left = eye_aspect_ratio(left_eye);
            float ear_right = eye_aspect_ratio(right_eye);
            ear_avg = (ear_left + ear_right) / 2.0f;
            mar = mouth_aspect_ratio(mouth);

            if (!metricsUpdated) {
                if (static_cast<int>(m_eyeStateBuffer.size()) >= m_perclosWindow) {
                    m_eyeStateBuffer.pop_front();
                }
                m_eyeStateBuffer.push_back(ear_avg < m_earThreshold);
                int closed_count = 0;
                for (bool closed : m_eyeStateBuffer) {
                    if (closed) {
                        closed_count++;
                    }
                }
                m_perclos = m_eyeStateBuffer.size() < static_cast<size_t>(m_perclosWindow / 2)
                                ? 0.0f
                                : static_cast<float>(closed_count) / m_eyeStateBuffer.size();

                if (ear_avg < m_earThreshold) {
                    m_consecutiveEyeClosed++;
                } else {
                    m_consecutiveEyeClosed = std::max(0, m_consecutiveEyeClosed - 1);
                }

                if (mar > m_marThreshold) {
                    m_consecutiveMouthOpen++;
                } else {
                    m_consecutiveMouthOpen = std::max(0, m_consecutiveMouthOpen - 1);
                }
                metricsUpdated = true;
            }

            bool eye_fatigue = m_consecutiveEyeClosed >= m_eyeClosedFrames;
            bool mouth_fatigue = m_consecutiveMouthOpen >= m_mouthOpenFrames;
            bool perclos_fatigue = m_perclos > 0.5f;
            is_tired = eye_fatigue || mouth_fatigue || perclos_fatigue;
        }

        FatigueFaceResult face_result;
        face_result.face_rect = face_rect;
        face_result.is_tired = is_tired;
        face_result.metrics.ear = ear_avg;
        face_result.metrics.mar = mar;
        face_result.metrics.perclos = m_perclos;
        face_result.metrics.consecutive_eye_closed = m_consecutiveEyeClosed;
        face_result.metrics.consecutive_mouth_open = m_consecutiveMouthOpen;
        face_result.landmarks = std::move(landmarks);
        result->faces.push_back(std::move(face_result));

        if (annotated) {
            if (landmark_valid) {
                for (const auto &pt : result->faces.back().landmarks) {
                    cv::circle(*annotated, pt, 2, cv::Scalar(0, 0, 255), -1);
                }
            }
        }
    }

    return 0;
}
