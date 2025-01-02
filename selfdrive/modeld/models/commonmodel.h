#pragma once

#include <cfloat>
#include <cstdlib>
#include <cassert>

#include <memory>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "common/mat.h"
#include "selfdrive/modeld/transforms/loadyuv.h"
#include "selfdrive/modeld/transforms/transform.h"

template <typename T = uint8_t>
class ModelFrame {
public:
  ModelFrame(cl_device_id device_id, cl_context context) {
    q = CL_CHECK_ERR(clCreateCommandQueue(context, device_id, 0, &err));
  }
  virtual ~ModelFrame() {}
  virtual cl_mem* prepare(cl_mem yuv_cl, int frame_width, int frame_height, int frame_stride, int frame_uv_offset, const mat3& projection) { return NULL; }
  T* buffer_from_cl(cl_mem *in_frames, int buffer_size) {
    CL_CHECK(clEnqueueReadBuffer(q, *in_frames, CL_TRUE, 0, buffer_size, input_frames.get(), 0, nullptr, nullptr));
    clFinish(q);
    return &input_frames[0];
  }

  int MODEL_WIDTH;
  int MODEL_HEIGHT;
  int MODEL_FRAME_SIZE;
  int buf_size;

protected:
  cl_mem y_cl, u_cl, v_cl;
  Transform transform;
  cl_command_queue q;
  std::unique_ptr<T[]> input_frames;

  void init_transform(cl_device_id device_id, cl_context context, int model_width, int model_height) {
    y_cl = CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_WRITE, model_width * model_height, NULL, &err));
    u_cl = CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_WRITE, (model_width / 2) * (model_height / 2), NULL, &err));
    v_cl = CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_WRITE, (model_width / 2) * (model_height / 2), NULL, &err));
    transform_init(&transform, context, device_id);
  }

  void deinit_transform() {
    transform_destroy(&transform);
    CL_CHECK(clReleaseMemObject(v_cl));
    CL_CHECK(clReleaseMemObject(u_cl));
    CL_CHECK(clReleaseMemObject(y_cl));
  }

  void run_transform(cl_mem yuv_cl, int model_width, int model_height, int frame_width, int frame_height, int frame_stride, int frame_uv_offset, const mat3& projection) {
    transform_queue(&transform, q,
        yuv_cl, frame_width, frame_height, frame_stride, frame_uv_offset,
        y_cl, u_cl, v_cl, model_width, model_height, projection);
  }
};

template <typename T>
class DrivingModelFrame : public ModelFrame<T> {
  using ModelFrame<T>::q, ModelFrame<T>::y_cl, ModelFrame<T>::u_cl, ModelFrame<T>::v_cl;
  using ModelFrame<T>::init_transform, ModelFrame<T>::deinit_transform, ModelFrame<T>::run_transform;
  using ModelFrame<T>::input_frames;
  
public:
  DrivingModelFrame(cl_device_id device_id, cl_context context);
  ~DrivingModelFrame();
  cl_mem* prepare(cl_mem yuv_cl, int frame_width, int frame_height, int frame_stride, int frame_uv_offset, const mat3& projection);

  const int MODEL_WIDTH = 512;
  const int MODEL_HEIGHT = 256;
  const int MODEL_FRAME_SIZE = MODEL_WIDTH * MODEL_HEIGHT * 3 / 2;
  const int buf_size = MODEL_FRAME_SIZE * 2;
  const size_t frame_size_bytes = MODEL_FRAME_SIZE * sizeof(T);
  const bool is_float = std::is_same<T, float>::value;
  const uint8_t buffer_length = is_float ? 2 : 5;

private:
  LoadYUVState loadyuv;
  cl_mem img_buffer_20hz_cl, last_img_cl, input_frames_cl;
  cl_buffer_region region;
};

class MonitoringModelFrame : public ModelFrame<> {
public:
  MonitoringModelFrame(cl_device_id device_id, cl_context context);
  ~MonitoringModelFrame();
  cl_mem* prepare(cl_mem yuv_cl, int frame_width, int frame_height, int frame_stride, int frame_uv_offset, const mat3& projection);

  const int MODEL_WIDTH = 1440;
  const int MODEL_HEIGHT = 960;
  const int MODEL_FRAME_SIZE = MODEL_WIDTH * MODEL_HEIGHT;
  const int buf_size = MODEL_FRAME_SIZE;

private:
  cl_mem input_frame_cl;
};

class DrivingModelFrameLegacy : public ModelFrame<float> {
  // using ModelFrame<T>::q, ModelFrame<T>::y_cl, ModelFrame<T>::u_cl, ModelFrame<T>::v_cl;
  // using ModelFrame<T>::init_transform, ModelFrame<T>::deinit_transform, ModelFrame<T>::run_transform;
  // using ModelFrame<T>::input_frames;
  
public:
  DrivingModelFrameLegacy(cl_device_id device_id, cl_context context);
  ~DrivingModelFrameLegacy();
  cl_mem* prepare(cl_mem yuv_cl, int frame_width, int frame_height, int frame_stride, int frame_uv_offset, const mat3& projection);

  const int MODEL_WIDTH = 512;
  const int MODEL_HEIGHT = 256;
  const int MODEL_FRAME_SIZE = MODEL_WIDTH * MODEL_HEIGHT * 3 / 2;
  const int buf_size = MODEL_FRAME_SIZE * 2;
  const size_t frame_size_bytes = MODEL_FRAME_SIZE * sizeof(float);

private:
  LoadYUVState loadyuv;
  cl_mem img_buffer_cl, input_frames_cl;
};