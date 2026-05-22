#include "handyolo_runner.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
struct BoxInfo {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int cls;
};

static float dequant(int8_t v, int32_t zp, float scale)
{
    return (static_cast<float>(v) - static_cast<float>(zp)) * scale;
}

static float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

static float unsigmoid(float y)
{
    return -1.0f * std::log((1.0f / y) - 1.0f);
}

static int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    if (dst_val < -128.0f) {
        dst_val = -128.0f;
    } else if (dst_val > 127.0f) {
        dst_val = 127.0f;
    }
    return static_cast<int8_t>(dst_val);
}

static float iou(const BoxInfo &a, const BoxInfo &b)
{
    float xx1 = std::max(a.x1, b.x1);
    float yy1 = std::max(a.y1, b.y1);
    float xx2 = std::min(a.x2, b.x2);
    float yy2 = std::min(a.y2, b.y2);
    float w = std::max(0.0f, xx2 - xx1);
    float h = std::max(0.0f, yy2 - yy1);
    float inter = w * h;
    float area = (a.x2 - a.x1) * (a.y2 - a.y1) + (b.x2 - b.x1) * (b.y2 - b.y1) - inter;
    if (area <= 0.0f) {
        return 0.0f;
    }
    return inter / area;
}

static const int kPropBoxSize = 5 + HANDYOLO_CLASS_NUM;
static const float kNmsThresh = 0.45f;
static const float kBoxThresh = 0.25f;
static const int kAnchor[3][6] = {{10, 13, 16, 30, 33, 23},
                                  {30, 61, 62, 45, 59, 119},
                                  {116, 90, 156, 198, 373, 326}};

static int process_i8_nchw(const int8_t *input, int grid_h, int grid_w, int stride, const int *anchor,
                           int32_t zp, float scale, std::vector<BoxInfo> &boxes,
                           float &max_score, float &max_obj, float &max_cls, int &max_cls_id)
{
    int valid = 0;
    int grid_len = grid_h * grid_w;
    int8_t thres_i8 = qnt_f32_to_affine(unsigmoid(kBoxThresh), zp, scale);
    for (int a = 0; a < 3; ++a) {
        for (int i = 0; i < grid_h; ++i) {
            for (int j = 0; j < grid_w; ++j) {
                int idx_conf = (a * kPropBoxSize + 4) * grid_len + i * grid_w + j;
                if (input[idx_conf] < thres_i8) {
                    continue;
                }
                int base = (a * kPropBoxSize) * grid_len + i * grid_w + j;
                float box_x = sigmoid(dequant(input[base + 0 * grid_len], zp, scale)) * 2.0f - 0.5f;
                float box_y = sigmoid(dequant(input[base + 1 * grid_len], zp, scale)) * 2.0f - 0.5f;
                float box_w = sigmoid(dequant(input[base + 2 * grid_len], zp, scale)) * 2.0f;
                float box_h = sigmoid(dequant(input[base + 3 * grid_len], zp, scale)) * 2.0f;
                box_x = (box_x + j) * static_cast<float>(stride);
                box_y = (box_y + i) * static_cast<float>(stride);
                box_w = box_w * box_w * static_cast<float>(anchor[a * 2]);
                box_h = box_h * box_h * static_cast<float>(anchor[a * 2 + 1]);
                box_x -= box_w * 0.5f;
                box_y -= box_h * 0.5f;

                float box_conf = sigmoid(dequant(input[base + 4 * grid_len], zp, scale));
                float max_cls_local = sigmoid(dequant(input[base + 5 * grid_len], zp, scale));
                int max_id = 0;
                for (int c = 1; c < HANDYOLO_CLASS_NUM; ++c) {
                    float cls = sigmoid(dequant(input[base + (5 + c) * grid_len], zp, scale));
                    if (cls > max_cls_local) {
                        max_cls_local = cls;
                        max_id = c;
                    }
                }
                float score = box_conf * max_cls_local;
                if (score > max_score) {
                    max_score = score;
                    max_obj = box_conf;
                    max_cls = max_cls_local;
                    max_cls_id = max_id;
                }
                if (score < kBoxThresh) {
                    continue;
                }
                BoxInfo box;
                box.x1 = box_x;
                box.y1 = box_y;
                box.x2 = box_x + box_w;
                box.y2 = box_y + box_h;
                box.score = score;
                box.cls = max_id;
                boxes.push_back(box);
                valid++;
            }
        }
    }
    return valid;
}

static int process_i8_rv1106_nhwc(const int8_t *input, int grid_h, int grid_w, int stride, const int *anchor,
                                  int32_t zp, float scale, std::vector<BoxInfo> &boxes,
                                  float &max_score, float &max_obj, float &max_cls, int &max_cls_id)
{
    int valid = 0;
    int anchor_per_branch = 3;
    int align_c = kPropBoxSize * anchor_per_branch;
    int8_t thres_i8 = qnt_f32_to_affine(unsigmoid(kBoxThresh), zp, scale);
    for (int h = 0; h < grid_h; ++h) {
        for (int w = 0; w < grid_w; ++w) {
            for (int a = 0; a < anchor_per_branch; ++a) {
                int hw_offset = h * grid_w * align_c + w * align_c + a * kPropBoxSize;
                const int8_t *hw_ptr = input + hw_offset;
                if (hw_ptr[4] < thres_i8) {
                    continue;
                }
                float box_conf = sigmoid(dequant(hw_ptr[4], zp, scale));
                float max_cls_local = sigmoid(dequant(hw_ptr[5], zp, scale));
                int max_id = 0;
                for (int c = 1; c < HANDYOLO_CLASS_NUM; ++c) {
                    float cls = sigmoid(dequant(hw_ptr[5 + c], zp, scale));
                    if (cls > max_cls_local) {
                        max_cls_local = cls;
                    }
                }
                if (box_conf < kBoxThresh) {
                    continue;
                }
                float score = box_conf * max_cls_local;
                if (score > max_score) {
                    max_score = score;
                    max_obj = box_conf;
                    max_cls = max_cls_local;
                    max_cls_id = max_id;
                }
                if (score < kBoxThresh) {
                    continue;
                }
                float box_x = sigmoid(dequant(hw_ptr[0], zp, scale)) * 2.0f - 0.5f;
                float box_y = sigmoid(dequant(hw_ptr[1], zp, scale)) * 2.0f - 0.5f;
                float box_w = sigmoid(dequant(hw_ptr[2], zp, scale)) * 2.0f;
                float box_h = sigmoid(dequant(hw_ptr[3], zp, scale)) * 2.0f;
                box_w = box_w * box_w;
                box_h = box_h * box_h;
                box_x = (box_x + w) * static_cast<float>(stride);
                box_y = (box_y + h) * static_cast<float>(stride);
                box_w *= static_cast<float>(anchor[a * 2]);
                box_h *= static_cast<float>(anchor[a * 2 + 1]);
                box_x -= box_w * 0.5f;
                box_y -= box_h * 0.5f;
                BoxInfo box;
                box.x1 = box_x;
                box.y1 = box_y;
                box.x2 = box_x + box_w;
                box.y2 = box_y + box_h;
                box.score = score;
                box.cls = max_id;
                boxes.push_back(box);
                valid++;
            }
        }
    }
    return valid;
}

static std::vector<BoxInfo> nms(const std::vector<BoxInfo> &boxes, float iou_thresh)
{
    std::vector<BoxInfo> sorted = boxes;
    std::sort(sorted.begin(), sorted.end(),
              [](const BoxInfo &a, const BoxInfo &b) { return a.score > b.score; });
    std::vector<BoxInfo> keep;
    std::vector<bool> removed(sorted.size(), false);
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (removed[i]) {
            continue;
        }
        keep.push_back(sorted[i]);
        for (size_t j = i + 1; j < sorted.size(); ++j) {
            if (removed[j]) {
                continue;
            }
            if (sorted[i].cls == sorted[j].cls && iou(sorted[i], sorted[j]) > iou_thresh) {
                removed[j] = true;
            }
        }
    }
    return keep;
}
} 

HandYoloRunner::HandYoloRunner()
{
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
}

HandYoloRunner::~HandYoloRunner()
{
    deinit();
}

bool HandYoloRunner::init(const char *model_path)
{
    if (m_ready) {
        deinit();
    }
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
    if (rknn_init(&m_rknn, const_cast<char *>(model_path), 0, 0, nullptr) < 0) {
        return false;
    }
    if (rknn_query(m_rknn, RKNN_QUERY_IN_OUT_NUM, &m_ioNum, sizeof(m_ioNum)) != RKNN_SUCC) {
        return false;
    }

    rknn_tensor_attr input_attrs[m_ioNum.n_input];
    std::memset(input_attrs, 0, sizeof(input_attrs));
    for (uint32_t i = 0; i < m_ioNum.n_input; i++) {
        input_attrs[i].index = i;
        if (rknn_query(m_rknn, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr)) != RKNN_SUCC) {
            return false;
        }
    }

    rknn_tensor_attr output_attrs[m_ioNum.n_output];
    std::memset(output_attrs, 0, sizeof(output_attrs));
    for (uint32_t i = 0; i < m_ioNum.n_output; i++) {
        output_attrs[i].index = i;
        if (rknn_query(m_rknn, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr)) !=
            RKNN_SUCC) {
            return false;
        }
    }

    input_attrs[0].type = RKNN_TENSOR_INT8;
    input_attrs[0].fmt = RKNN_TENSOR_NCHW;
    m_inputMems[0] = rknn_create_mem(m_rknn, input_attrs[0].size_with_stride);
    if (rknn_set_io_mem(m_rknn, m_inputMems[0], &input_attrs[0]) < 0) {
        return false;
    }

    for (uint32_t i = 0; i < m_ioNum.n_output && i < 3; ++i) {
        m_outputMems[i] = rknn_create_mem(m_rknn, output_attrs[i].size_with_stride);
        if (rknn_set_io_mem(m_rknn, m_outputMems[i], &output_attrs[i]) < 0) {
            return false;
        }
    }

    m_isQuant = (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC);
    m_inputAttrs = static_cast<rknn_tensor_attr *>(malloc(m_ioNum.n_input * sizeof(rknn_tensor_attr)));
    std::memcpy(m_inputAttrs, input_attrs, m_ioNum.n_input * sizeof(rknn_tensor_attr));
    m_outputAttrs = static_cast<rknn_tensor_attr *>(malloc(m_ioNum.n_output * sizeof(rknn_tensor_attr)));
    std::memcpy(m_outputAttrs, output_attrs, m_ioNum.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        m_modelChannel = input_attrs[0].dims[1];
        m_modelHeight = input_attrs[0].dims[2];
        m_modelWidth = input_attrs[0].dims[3];
    } else {
        m_modelHeight = input_attrs[0].dims[1];
        m_modelWidth = input_attrs[0].dims[2];
        m_modelChannel = input_attrs[0].dims[3];
    }

    m_ready = true;
    std::printf("[HandYolo] input dims=%d,%d,%d,%d fmt=%d type=%d scale=%g zp=%d\n",
                input_attrs[0].dims[0], input_attrs[0].dims[1], input_attrs[0].dims[2], input_attrs[0].dims[3],
                input_attrs[0].fmt, input_attrs[0].type, input_attrs[0].scale, input_attrs[0].zp);
    for (uint32_t i = 0; i < m_ioNum.n_output; ++i) {
        std::printf("[HandYolo] output[%u] dims=%d,%d,%d,%d fmt=%d type=%d scale=%g zp=%d elems=%d\n",
                    i, output_attrs[i].dims[0], output_attrs[i].dims[1], output_attrs[i].dims[2], output_attrs[i].dims[3],
                    output_attrs[i].fmt, output_attrs[i].type, output_attrs[i].scale, output_attrs[i].zp,
                    output_attrs[i].n_elems);
    }
    return true;
}

void HandYoloRunner::deinit()
{
    if (!m_ready) {
        return;
    }
    if (m_inputAttrs) {
        free(m_inputAttrs);
        m_inputAttrs = nullptr;
    }
    if (m_outputAttrs) {
        free(m_outputAttrs);
        m_outputAttrs = nullptr;
    }
    if (m_inputMems[0]) {
        rknn_destroy_mem(m_rknn, m_inputMems[0]);
        free(m_inputMems[0]);
        m_inputMems[0] = nullptr;
    }
    for (uint32_t i = 0; i < m_ioNum.n_output && i < 3; ++i) {
        if (m_outputMems[i]) {
            rknn_destroy_mem(m_rknn, m_outputMems[i]);
            free(m_outputMems[i]);
            m_outputMems[i] = nullptr;
        }
    }
    if (m_rknn != 0) {
        rknn_destroy(m_rknn);
        m_rknn = 0;
    }
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
    m_modelWidth = 0;
    m_modelHeight = 0;
    m_modelChannel = 0;
    m_isQuant = false;
    m_ready = false;
}

bool HandYoloRunner::isReady() const
{
    return m_ready;
}

uint8_t *HandYoloRunner::inputBuffer()
{
    if (!m_ready || !m_inputMems[0]) {
        return nullptr;
    }
    return static_cast<uint8_t *>(m_inputMems[0]->virt_addr);
}

int HandYoloRunner::inputWidth() const
{
    return m_modelWidth;
}

int HandYoloRunner::inputHeight() const
{
    return m_modelHeight;
}

int HandYoloRunner::inputChannels() const
{
    return m_modelChannel;
}

float HandYoloRunner::inputScale() const
{
    if (!m_inputAttrs) {
        return 1.0f;
    }
    return m_inputAttrs[0].scale;
}

int HandYoloRunner::inputZp() const
{
    if (!m_inputAttrs) {
        return 0;
    }
    return m_inputAttrs[0].zp;
}

int HandYoloRunner::run(HandYoloDetections *results)
{
    if (!m_ready || !results) {
        return -1;
    }
    int ret = rknn_run(m_rknn, nullptr);
    if (ret < 0) {
        return -1;
    }

    std::memset(results, 0, sizeof(HandYoloDetections));
    if (!m_outputAttrs) {
        return 0;
    }
    std::vector<BoxInfo> boxes;
    boxes.reserve(1024);
    static int call_count = 0;
    float max_score = 0.0f;
    float max_obj = 0.0f;
    float max_cls = 0.0f;
    int max_cls_id = -1;
    int raw_min = 127;
    int raw_max = -128;
    float last_scale = 1.0f;
    int last_zp = 0;
    for (uint32_t i = 0; i < m_ioNum.n_output && i < 3; ++i) {
        if (!m_outputMems[i] || !m_outputMems[i]->virt_addr) {
            continue;
        }
        const int8_t *data = static_cast<const int8_t *>(m_outputMems[i]->virt_addr);
        float scale = m_outputAttrs[i].scale;
        const int32_t zp = m_outputAttrs[i].zp;
        if (scale == 0.0f) {
            scale = 1.0f;
        }
        last_scale = scale;
        last_zp = static_cast<int>(zp);
        int grid_h = 0;
        int grid_w = 0;
        int grid_len = m_outputAttrs[i].n_elems / (kPropBoxSize * 3);
        if (m_outputAttrs[i].n_dims >= 4) {
            int d1 = m_outputAttrs[i].dims[1];
            int d2 = m_outputAttrs[i].dims[2];
            if (d1 > 0 && d2 > 0 && d1 * d2 == grid_len) {
                grid_h = d1;
                grid_w = d2;
            }
        }
        if (grid_h <= 0 || grid_w <= 0) {
            int g = static_cast<int>(std::sqrt(static_cast<float>(grid_len)));
            grid_h = g;
            grid_w = g;
        }
        int stride = m_modelHeight / grid_h;
        process_i8_rv1106_nhwc(data, grid_h, grid_w, stride, kAnchor[i], zp, scale, boxes,
                               max_score, max_obj, max_cls, max_cls_id);
        int elems = m_outputAttrs[i].n_elems;
        for (int k = 0; k < elems; ++k) {
            int v = data[k];
            raw_min = std::min(raw_min, v);
            raw_max = std::max(raw_max, v);
        }
    }

    if (call_count % 30 == 0) {
        std::printf("[HandYolo] max_score=%.4f obj=%.4f cls=%.4f id=%d raw=[%d,%d] scale=%g zp=%d\n",
                    max_score, max_obj, max_cls, max_cls_id, raw_min, raw_max, last_scale, last_zp);
    }
    call_count++;
    if (boxes.empty()) {
        return 0;
    }

    std::vector<BoxInfo> keep = nms(boxes, kNmsThresh);
    int out_count = std::min(static_cast<int>(keep.size()), HANDYOLO_OBJ_MAX);
    results->count = out_count;
    for (int i = 0; i < out_count; ++i) {
        const auto &b = keep[i];
        results->results[i].box.left = std::max(0, std::min(static_cast<int>(b.x1), m_modelWidth - 1));
        results->results[i].box.top = std::max(0, std::min(static_cast<int>(b.y1), m_modelHeight - 1));
        results->results[i].box.right = std::max(0, std::min(static_cast<int>(b.x2), m_modelWidth - 1));
        results->results[i].box.bottom = std::max(0, std::min(static_cast<int>(b.y2), m_modelHeight - 1));
        results->results[i].prop = b.score;
        results->results[i].cls_id = b.cls;
    }
    return 0;
}
