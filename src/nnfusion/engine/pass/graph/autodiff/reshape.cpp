//*****************************************************************************
// Copyright 2017-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include "backward_registry.hpp"

REGISTER_BACKWARD_TRANSLATOR(Reshape).translator([](std::shared_ptr<GNode> forward_node,
                                                    const GNodeIndexVector& outputs_grad,
                                                    std::shared_ptr<nnfusion::graph::Graph> graph)
                                                     -> GNodeIndexVector {
    NNFUSION_CHECK(outputs_grad.size() == 1) << "reshape have only 1 output, but "
                                             << outputs_grad.size() << " outputs_grad provided";
    auto reshape_op = std::dynamic_pointer_cast<op::Reshape>(forward_node->get_op_ptr());

    auto x_shape = forward_node->get_input_shape(0);
    auto x_rank = x_shape.size();
    auto input_order = reshape_op->get_input_order();
    Shape permuted_x_shape(x_rank);
    AxisVector x_input_order(x_rank);
    for (size_t i = 0; i < x_rank; i++)
    {
        size_t permuted_i = input_order[i];
        if (!reshape_op->get_is_transpose())
        {
            NNFUSION_CHECK(permuted_i == i);
        }
        permuted_x_shape[i] = x_shape[permuted_i];
        x_input_order[permuted_i] = i;
    }

    auto backprop_op = std::make_shared<op::Reshape>(get_default_order(outputs_grad[0].get_shape()),
                                                     permuted_x_shape);
    auto x_grad = graph->add_node_and_edge(backprop_op, {outputs_grad[0]});
    if (reshape_op->get_is_transpose())
    {
        backprop_op = std::make_shared<op::Reshape>(x_input_order, x_shape);
        x_grad = graph->add_node_and_edge(backprop_op, {x_grad});
    }
    return GNodeIndexVector{GNodeIndex{x_grad, 0}};
});