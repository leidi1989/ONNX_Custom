/*
 * @Description: 
 * @Version: 
 * @Author: Leidi
 * @Date: 2021-04-14 17:44:58
 * @LastEditors: Leidi
 * @LastEditTime: 2021-04-14 17:44:59
 */
#include <torch/script.h>
#include "Eigen/Dense"

template <typename T>
using ConstEigenVectorArrayMap = Eigen::Map<const Eigen::Array<T, Eigen::Dynamic, 1>>;
template <typename T>
using EigenVectorArrayMap = Eigen::Map<Eigen::Array<T, Eigen::Dynamic, 1>>;

torch::Tensor custom_group_norm(torch::Tensor X, torch::Tensor num_groups, torch::Tensor scale, torch::Tensor bias, torch::Tensor eps) {

  float* X_data = X.data<float>();
  float* scale_data = scale.data<float>();
  float* bias_data = bias.data<float>();
  int num_groups_i = int(num_groups.data<float>()[0]);
  float epsilon_ = eps.data<float>()[0];
  torch::Tensor output = torch::zeros(X.sizes());
  float* out = output.data<float>();
  const int64_t N = X.size(0);
  const int64_t C = X.size(1) / num_groups_i;  // assume [N C*num_groups H W]  per the spec

  int64_t sample_size = 1;
  for (size_t i = 2; i < X.dim(); ++i) {
    sample_size *= X.size(i);
  }
  sample_size *= C;

  std::vector<float> Xi;
  for (auto i = 0; i < N * num_groups_i; ++i) {
    ConstEigenVectorArrayMap<float> Xi(X_data + sample_size * i, sample_size);
    const float Xi_mean = Xi.mean();
    const float squared_norm = (Xi - Xi_mean).matrix().squaredNorm();
    const float inv_stdev = 1.0f / std::sqrt(squared_norm / sample_size + epsilon_);
    EigenVectorArrayMap<float> Yi(out + sample_size * i, sample_size);
    const float channel_scale = inv_stdev * scale_data[i % (C * num_groups_i)];
    const float channel_shift = bias_data[i % (C * num_groups_i)] - Xi_mean * channel_scale;
    Yi = Xi * channel_scale + channel_shift;
   }

  return output;
}