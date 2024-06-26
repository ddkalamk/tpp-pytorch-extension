/******************************************************************************
 * Copyright (c) 2022 Intel Corporation - All rights reserved.                *
 *                                                                            *
 * For information on the license, see the LICENSE file.                      *
 * Further information: https://github.com/libxsmm/tpp-pytorch-extension/     *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/
/* Author: Sasikanth Avancha (Intel Corp.)
 ******************************************************************************/

#include <ATen/record_function.h>
#include <torch/extension.h>
#include <cstdlib>

#include <iostream>
#include <mutex>
#include <vector>
#include "ext_tpp.h"
#include "init.h"
#include "timing.h"
#include "xsmm_functors.h"

using namespace tpp;
#include "tensor_helper.h"

static int my_rank = guess_mpi_rank();

REGISTER_SCOPE(go_gemm, "go_gemm");
REGISTER_SCOPE(gdi_gemm, "gdi_gemm");
REGISTER_SCOPE(gdw_gemm, "gdw_gemm");
REGISTER_SCOPE(gdbias, "gdbias");
REGISTER_SCOPE(gdout, "gdout");
REGISTER_SCOPE(go_dropout, "go_dropout");
REGISTER_SCOPE(gdo_dropout, "gdo_dropout");

std::vector<at::Tensor> mlp_fwd(
    long align,
    bool apply_bias,
    float p,
    std::string act,
    bool res,
    bool training,
    std::vector<at::Tensor> inputs) {
  GlobalPass _gp(FWD);
  if (inputs[0].dtype() == at::kFloat) {
    typedef float T;
#include "mlp_flat_fwd.h"
  } else if (inputs[0].dtype() == at::kBFloat16) {
    typedef bfloat16 T;
#include "mlp_flat_fwd.h"
  } else {
    TPP_ASSERT(0, "%s:%d Unsupported type\n", __FILE__, __LINE__);
  }
}

std::vector<at::Tensor> mlp_bwd(
    long align,
    bool apply_bias,
    float p,
    std::string act,
    bool res,
    std::vector<at::Tensor> inputs) {
  GlobalPass _gp(BWD);
  if (inputs[0].dtype() == at::kFloat) {
    typedef float T;
#include "mlp_flat_bwd.h"
  } else if (inputs[0].dtype() == at::kBFloat16) {
    typedef bfloat16 T;
#include "mlp_flat_bwd.h"
  } else {
    TPP_ASSERT(0, "%s:%d Unsupported type\n", __FILE__, __LINE__);
  }
}

std::vector<at::Tensor> dropout_fwd(float p, at::Tensor inp, bool training) {
  GlobalPass _gp(FWD);
  if (inp.dtype() == at::kFloat) {
    typedef float T;
#include "dropout_fwd.h"
  } else if (inp.dtype() == at::kBFloat16) {
    typedef bfloat16 T;
#include "dropout_fwd.h"
  } else {
    TPP_ASSERT(0, "%s:%d Unsupported type\n", __FILE__, __LINE__);
  }
}

at::Tensor dropout_bwd(float p, std::vector<at::Tensor> inputs) {
  GlobalPass _gp(BWD);
  if (inputs[0].dtype() == at::kFloat) {
    typedef float T;
#include "dropout_bwd.h"
  } else if (inputs[0].dtype() == at::kBFloat16) {
    typedef bfloat16 T;
#include "dropout_bwd.h"
  } else {
    TPP_ASSERT(0, "%s:%d Unsupported type\n", __FILE__, __LINE__);
  }
}

REGISTER_SUBMODULE(_fused_gsage, m) {
  m.def("mlp_fwd", &mlp_fwd, "Tpp GraphSAGE MLP forward");
  m.def("mlp_bwd", &mlp_bwd, "Tpp GraphSAGE MLP backward");
  m.def("dropout_fwd", &dropout_fwd, "Tpp Optimized Dropout FWD");
  m.def("dropout_bwd", &dropout_bwd, "Tpp Optimized Dropout BWD");
}
