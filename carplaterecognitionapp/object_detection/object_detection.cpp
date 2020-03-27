/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include "object_detection.h"
#include <unistd.h>
#include <memory>
#include <sstream>
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using ascend::utils::DvppOutput;
using ascend::utils::DvppProcess;
using hiai::ImageData;
using hiai::IMAGEFORMAT;
using namespace std;
using namespace ascend::videoanalysis;

namespace {
const uint32_t kInputPort = 0; // the input port number
const uint32_t kOutputPort = 0; // the output port number

const int kHandleSuccessful = 0; // the process handled successfully

const int kVideoFormatLength = 5; // video format string length

const int kImageDataQueueSize = 20; // the queue default size

const string kVideoTypeH264 = "h264"; // video type h264
const string kVideoTypeH265 = "h265"; // video type h265

const string kImageFormatNv12 = "nv12"; // image format nv12

const string kStrChannelId1 = "channel1"; // channle id 1 string
const string kStrChannelId2 = "channel2"; // channle id 2 string

const int kIntChannelId1 = 1; // channel id 1 integer
const int kIntChannelId2 = 2; // channel id 2 integer

// The width of image in vpc interface need 128-byte alignment
const int kVpcWidthAlign = 128;

// The height of image in vpc interface need 16-byte alignment
const int kVpcHeightAlign = 16;

// standard: 4096 * 4096 * 4 = 67108864 (64M)
const int kAllowedMaxImageMemory = 67108864;

// standard: 1024 * 1024 * 128 = 134217728 (128M)
const int kMaxNewMemory = 134217728;

const int kKeyFrameInterval = 5; // key fram interval

const int kWaitTimeShort = 20000; // the short wait time: 20ms
const int kWaitTimeLong = 100000; // the short wait time: 100ms
const int kRetryTimeShort = 5; // the short retry time
const int kRetryTimeLong = 100; // the long retry time

const int kCompareEqual = 0; // string compare equal

// ssd input image width and height.
const uint32_t kInputWidth = 512;
const uint32_t kInputHeight = 512;

const int kDvppProcSuccess = 0; // call dvpp success return

const int kVdecSingleton = 0; // dvpp vdec singleton parameter

const string kModelPath = "model_path"; // model path item

uint32_t frame_id_1 = 0; // frame id used for channle1

uint32_t frame_id_2 = 0; // frame id used for channle2
}

HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageParaT", VideoImageParaT);
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("DetectionEngineTransT", DetectionEngineTransT);

ObjectDetectionInferenceEngine::ObjectDetectionInferenceEngine() {
  dvpp_api_channel1_ = nullptr;
  dvpp_api_channel2_ = nullptr;
}

ObjectDetectionInferenceEngine::~ObjectDetectionInferenceEngine() {
  if (dvpp_api_channel1_ != nullptr) { // check create dvpp api result
    DestroyVdecApi(dvpp_api_channel1_, 0);
  }

  if (dvpp_api_channel2_ != nullptr) { // check create dvpp api result
    DestroyVdecApi(dvpp_api_channel2_, 0);
  }
}

HIAI_StatusT ObjectDetectionInferenceEngine::Init(
    const hiai::AIConfig &config,
    const vector<hiai::AIModelDescription> &model_desc) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] start to initialize!");

  if (ai_model_manager_ == nullptr) { // check ai model manager is nullptr
    ai_model_manager_ = make_shared<hiai::AIModelManager>();
  }

  vector<hiai::AIModelDescription> od_model_descs;
  hiai::AIModelDescription model_description;

  // load model path.
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem &item = config.items(index);
    if (item.name() == kModelPath) { // current item is model path
      const char* model_path = item.value().data();
      model_description.set_path(model_path);
    }
  }
  od_model_descs.push_back(model_description);

  // init ssd model
  HIAI_StatusT ret = ai_model_manager_->Init(config, od_model_descs);
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                    "[ODInferenceEngine] failed to initialize AI model!");
    return HIAI_ERROR;
  }

  // create vdec api for channel1, and check the result
  if (CreateVdecApi(dvpp_api_channel1_, kVdecSingleton) != kHandleSuccessful) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[ODInferenceEngine] fail to create dvpp vdec api for channel1!");
  }
  // create vdec api for channel2, and check the result
  if (CreateVdecApi(dvpp_api_channel2_, kVdecSingleton) != kHandleSuccessful) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[ODInferenceEngine] fail to create dvpp vdec api for channel2!");
  }

  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] engine initialized!");
  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionInferenceEngine::ImagePreProcess(
    const ImageData<u_int8_t> &src_img, ImageData<u_int8_t> &resized_img) {
  if (src_img.format != IMAGEFORMAT::YUV420SP) {
    // input image must be yuv420sp nv12.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] input image type does not match");
    return HIAI_ERROR;
  }

  ascend::utils::DvppBasicVpcPara dvpp_basic_vpc_para;

  /**
   * when use dvpp_process only for resize function:
   *
   * 1.DVPP limits crop_right and crop_down should be Odd number,
   * if it is even number, subtract 1, otherwise Equal to origin width
   * or height.
   *
   * 2.crop_left and crop_up should be set to zero.
   */
  dvpp_basic_vpc_para.input_image_type = INPUT_YUV420_SEMI_PLANNER_UV; // nv12
  dvpp_basic_vpc_para.output_image_type = OUTPUT_YUV420SP_UV; // nv12
  dvpp_basic_vpc_para.src_resolution.width = (int) src_img.width;
  dvpp_basic_vpc_para.src_resolution.height = (int) src_img.height;
  dvpp_basic_vpc_para.dest_resolution.width = kInputWidth;
  dvpp_basic_vpc_para.dest_resolution.height = kInputHeight;
  // DVPP limits crop_left should be even number, 0 means without crop
  dvpp_basic_vpc_para.crop_left = 0;
  // DVPP limits crop_right should be Odd number
  dvpp_basic_vpc_para.crop_right =
      src_img.width % 2 == 0 ? src_img.width - 1 : src_img.width;
  // DVPP limits crop_up should be even number, 0 means without crop
  dvpp_basic_vpc_para.crop_up = 0;
  // DVPP limits crop_down should be Odd number
  dvpp_basic_vpc_para.crop_down =
      src_img.height % 2 == 0 ? src_img.height - 1 : src_img.height;
  dvpp_basic_vpc_para.is_input_align = true;

  ascend::utils::DvppProcess dvpp_process(dvpp_basic_vpc_para);

  ascend::utils::DvppVpcOutput dvpp_out;
  int ret = dvpp_process.DvppBasicVpcProc(src_img.data.get(),
                                          (int32_t) src_img.size, &dvpp_out);
  if (ret != kDvppProcSuccess) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] call dvpp resize failed with code %d!",
                    ret);
    return HIAI_ERROR;
  }

  // dvpp_out->pbuf
  resized_img.data.reset(dvpp_out.buffer, default_delete<uint8_t[]>());
  resized_img.size = dvpp_out.size;

  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionInferenceEngine::PerformInference(
    shared_ptr<DetectionEngineTransT> &detection_trans,
    ImageData<u_int8_t> &input_img) {
  // init neural buffer.
  shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer = shared_ptr<
      hiai::AINeuralNetworkBuffer>(
      new (nothrow) hiai::AINeuralNetworkBuffer(),
      default_delete<hiai::AINeuralNetworkBuffer>());
  if (neural_buffer.get() == nullptr) { // check new memory result
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Fail to new memory when initialize neural buffer!");
    return HIAI_ERROR;
  }

  neural_buffer->SetBuffer((void*) input_img.data.get(), input_img.size);

  shared_ptr<hiai::IAITensor> input_tensor =
      static_pointer_cast<hiai::IAITensor>(neural_buffer);

  vector<shared_ptr<hiai::IAITensor>> input_tensors;
  vector<shared_ptr<hiai::IAITensor>> output_tensors;
  input_tensors.push_back(input_tensor);

  HIAI_StatusT ret = hiai::SUCCESS;
  ret = ai_model_manager_->CreateOutputTensor(input_tensors, output_tensors);
  if (ret != hiai::SUCCESS) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] output tensor created failed!");
    return HIAI_ERROR;
  }

  hiai::AIContext ai_context;
  // neural network inference.
  ret = ai_model_manager_->Process(ai_context, input_tensors, output_tensors,
                                   0);
  if (ret != hiai::SUCCESS) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] image inference failed!");
    return HIAI_ERROR;
  }

  // set trans_data
  detection_trans->status = true;
  for (uint32_t index = 0; index < output_tensors.size(); ++index) {
    shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor = static_pointer_cast<
        hiai::AINeuralNetworkBuffer>(output_tensors[index]);

    OutputT out;
    out.size = result_tensor->GetSize();
    if (out.size <= 0 || out.size > kMaxNewMemory) { // check data size is valid
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "the inference output size:%d is invalid! value range: 1~134217728",
          out.size);
      return HIAI_ERROR;
    }

    out.data =  std::shared_ptr<uint8_t>(new (nothrow)uint8_t[out.size],
                                        std::default_delete<uint8_t[]>());
    if (out.data.get() == nullptr) { // check new memory result
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "Fail to new memory when handle ai model inference output!");
      return HIAI_ERROR;
    }

    errno_t ret = memcpy_s(out.data.get(), out.size, result_tensor->GetBuffer(),
                           out.size);
    if (ret != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[ODInferenceEngine] output tensor copy failure!");
      continue;
    }
    detection_trans->output_datas.push_back(out);
  }
  if (detection_trans->output_datas.empty()) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] result tensor is empty!");
    return HIAI_ERROR;
  }

  // sendData
  return SendDetectionResult(detection_trans);
}

HIAI_StatusT ObjectDetectionInferenceEngine::SendDetectionResult(
    shared_ptr<DetectionEngineTransT> &detection_trans, bool inference_success,
    string err_msg) {
  if (!inference_success) {
    // inference error.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, err_msg.c_str());
    detection_trans->status = false;
    detection_trans->msg = err_msg;
  }
  HIAI_StatusT ret;
  do {
  // send data to next engine.
  ret = SendData(kOutputPort, "DetectionEngineTransT",
                 static_pointer_cast<void>(detection_trans));
  if (ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] output queue full");
      usleep(kWaitTimeShort);
    }
  } while (ret == HIAI_QUEUE_FULL);
  if (ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] send inference data failed!");
    return HIAI_ERROR;
  }
  return HIAI_OK;
}

void ObjectDetectionInferenceEngine::ObjectDetectInference(
 shared_ptr<VideoImageParaT> &video_image) {
  HIAI_ENGINE_LOG(
      "[ODInferenceEngine] start object detection inference");
  
    // init inference results tensor shared_ptr.
  shared_ptr<DetectionEngineTransT> detection_trans = make_shared<
      DetectionEngineTransT>();

  //shared_ptr<VideoImageParaT> video_image = yuv_image_queue.Pop(); video_imageֱ�ӻ�ȡ��OK
  if (video_image == nullptr) {
    HIAI_ENGINE_LOG(
        "[ODInferenceEngine] image data is null!");
    return;
  }

  detection_trans->video_image = *video_image;

  // resize input image.
  ImageData<u_int8_t> resized_img;
  HIAI_StatusT dvpp_ret = ImagePreProcess(detection_trans->video_image.img,
                                            resized_img);
  if (dvpp_ret != HIAI_OK) {
    // if preprocess error,send input image to the next engine.
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] image preprocessed failure!");
  return;
  }
  
    // inference object detection
  if (PerformInference(detection_trans, resized_img) != HIAI_OK) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[ODInferenceEngine] fail to inference object detection!");
  }
}

HIAI_IMPL_ENGINE_PROCESS("object_detection", ObjectDetectionInferenceEngine,
    INPUT_SIZE) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] start process!");

  if (arg0 == nullptr) {
    // inputer data is nullptr.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] input data is null!");
    return HIAI_ERROR;
  }

  shared_ptr<VideoImageParaT> video_image =
      static_pointer_cast<VideoImageParaT>(arg0);

  // convert hfbc data to yuv image from queue, and detect object
  ObjectDetectInference(video_image);

  return HIAI_OK;
}
