/*
All modification made by Intel Corporation: © 2016 Intel Corporation

All contributions by the University of California:
Copyright (c) 2014, 2015, The Regents of the University of California (Regents)
All rights reserved.

All other contributions:
Copyright (c) 2014, 2015, the respective contributors
All rights reserved.
For the list of contributors go to https://github.com/BVLC/caffe/blob/master/CONTRIBUTORS.md


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>

#include "caffe/layers/log_layer.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template <typename Dtype>
void LogLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  NeuronLayer<Dtype>::LayerSetUp(bottom, top);
  const Dtype base = this->layer_param_.log_param().base();
  if (base != Dtype(-1)) {
    CHECK_GT(base, 0) << "base must be strictly positive.";
  }
  // If base == -1, interpret the base as e and set log_base = 1 exactly.
  // Otherwise, calculate its log explicitly.
  const Dtype log_base = (base == Dtype(-1)) ? Dtype(1) : log(base);
  CHECK(!isnan(log_base))
      << "NaN result: log(base) = log(" << base << ") = " << log_base;
  CHECK(!isinf(log_base))
      << "Inf result: log(base) = log(" << base << ") = " << log_base;
  base_scale_ = Dtype(1) / log_base;
  CHECK(!isnan(base_scale_))
      << "NaN result: 1/log(base) = 1/log(" << base << ") = " << base_scale_;
  CHECK(!isinf(base_scale_))
      << "Inf result: 1/log(base) = 1/log(" << base << ") = " << base_scale_;
  input_scale_ = this->layer_param_.log_param().scale();
  input_shift_ = this->layer_param_.log_param().shift();
  backward_num_scale_ = input_scale_ / log_base;
}

template <typename Dtype>
void LogLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  const int count = bottom[0]->count();
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = top[0]->mutable_cpu_data();
  if (input_scale_ == Dtype(1) && input_shift_ == Dtype(0)) {
    caffe_log(count, bottom_data, top_data);
  } else {
    caffe_copy(count, bottom_data, top_data);
    if (input_scale_ != Dtype(1)) {
      caffe_scal(count, input_scale_, top_data);
    }
    if (input_shift_ != Dtype(0)) {
      caffe_add_scalar(count, input_shift_, top_data);
    }
    caffe_log(count, top_data, top_data);
  }
  if (base_scale_ != Dtype(1)) {
    caffe_scal(count, base_scale_, top_data);
  }
}

template <typename Dtype>
void LogLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {
  if (!propagate_down[0]) { return; }
  const int count = bottom[0]->count();
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* top_diff = top[0]->cpu_diff();
  Dtype* bottom_diff = bottom[0]->mutable_cpu_diff();
  caffe_copy(count, bottom_data, bottom_diff);
  if (input_scale_ != Dtype(1)) {
    caffe_scal(count, input_scale_, bottom_diff);
  }
  if (input_shift_ != Dtype(0)) {
    caffe_add_scalar(count, input_shift_, bottom_diff);
  }
  caffe_powx(count, bottom_diff, Dtype(-1), bottom_diff);
  if (backward_num_scale_ != Dtype(1)) {
    caffe_scal(count, backward_num_scale_, bottom_diff);
  }
  caffe_mul(count, top_diff, bottom_diff, bottom_diff);
}

#ifdef CPU_ONLY
STUB_GPU(LogLayer);
#endif

INSTANTIATE_CLASS(LogLayer);
REGISTER_LAYER_CLASS(Log);

}  // namespace caffe
