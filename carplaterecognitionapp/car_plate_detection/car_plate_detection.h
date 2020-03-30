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

#ifndef CAR_PLATE_DETECTION_INFERENCE_H_
#define CAR_PLATE_DETECTION_INFERENCE_H_

#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"
#include "video_analysis_params.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1

class CarPlateDetection : public hiai::Engine {
 public:
  /**
   * @brief Engine initialize method
   * @return HIAI_StatusT
   */
  HIAI_StatusT Init(const hiai::AIConfig &config,
                    const std::vector<hiai::AIModelDescription> &model_desc);

  /**
   * @ingroup hiaiengine
   * @brief HIAI_DEFINE_PROCESS : override Engine Process logic
   * @[in]: define a input port, a output port
   * @return HIAI_StatusT
   */
HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)

 private:
  /**
   * @brief call ez_dvpp interface for resizing image
   * @param [in] batch_image_input:  batch image from previous engine
   * @param [out] batch_image_output: batch image for processing
   */
  void BatchImageResize(
      std::shared_ptr<VideoDetectionImageParaT> &image_input,
      std::shared_ptr<VideoDetectionImageParaT> &image_output);

  /**
   * @brief : object detection inference
   * @param [in] resized_image: input resized image data
   * @param [out] car_plate_image: detection result, car plate image
   * @return HIAI_StatusT
   */
  HIAI_StatusT PerformInference(
      std::shared_ptr<VideoDetectionImageParaT> &resized_image,
      std::shared_ptr<VideoDetectionImageParaT> &car_plate_image);

  /**
   * @brief : correct the coordinate value between 0.0f and 1.0f
   * @param [in] input: coordinate value
   * @return value bewteen 0.0 and 1.0
   */
  float CorrectCoordinate(float value);

  /**
   * @brief : crop object image from input image
   * @param [in] src_img: input image
   * @param [out] target_img: output object image
   * @param [in] bbox: bounding box coordinate
   * @return HIAI_StatusT
   */
  HIAI_StatusT CropObjectFromImage(const hiai::ImageData<u_int8_t> &src_img,
                                   hiai::ImageData<u_int8_t> &target_img,
                                   const BoundingBox &bbox);

  /**
   * @brief : send inference results to next engine
   * @param [in] detection_trans: inference results tensor
   * @return HIAI_StatusT
   */
  HIAI_StatusT SendDetectionResult(
      std::shared_ptr<VideoDetectionImageParaT> &detection_trans);

  /**
   * @brief : crop object image from input image
   * @param [in] out_bbox: the bbox out data
   * @param [out] out_num: the bbox out number
   * @return true:BBox data valid, false:BBox data invalid
   */
  bool CheckBBoxData(const OutputT &out_bbox, const OutputT &out_num);

  // shared ptr to load ai model
  std::shared_ptr<hiai::AIModelManager> ai_model_manager_;
};

#endif /* CAR_PLATE_DETECTION_INFERENCE_H_ */
