#include <cstdio>

#include "BufferTensor.hpp"
#include "RomTensor.hpp"
#include "arenaAllocator.hpp"
#include "context.hpp"
#include "operatorBase.hpp"

using namespace std;
using namespace uTensor;

// MyOperator is an operator which takes 2 input tensors and produce 1 output
// tensor. Normally, for reusability, we will implement a kernel function and
// invoke the function in the compute method
void mykernel(const Tensor& tensor1, const Tensor& tensor2, Tensor& output) {
  printf("mykernel invoked!\n");
  TensorShape shape1 = tensor1->get_shape();
  TensorShape shape2 = tensor2->get_shape();
  // input shapes should be the same
  if (shape1 != shape2) {
    // you can throw error as following:
    Context::get_default_context()->throwError(new InvalidReshapeError);
    // Please refer to error_handling tutorial to see how to set a error handler
    // to handle the error in uTensor. Without an handler, all errors are
    // ignored by default.
    return;
  }
  if (tensor1->get_type() != flt || tensor2->get_type() != flt ||
      output->get_type() != flt) {
    // we only support float tensors for MyOperator
    return;
  }
  for (uint32_t i = 0; i < tensor1->num_elems(); ++i) {
    // When reading from tensors, you must cast it to proper data types.
    // You can access tensor element by flatten index:
    float a = static_cast<float>(tensor1(i));
    // or by indices of axis:
    float b = static_cast<float>(tensor2(0, i));
    // write result to output tensor
    output(i) = a + b;
  }
}
class MyOperator : public OperatorInterface<2, 1> {
 public:
  // identifiers for setting up the input tensors
  enum names_in { a, b };
  // identifiers for setting up the output tensors
  enum names_out { out };

 protected:
  void compute() {
    // you can retrieve input/output tensors by its identifier
    mykernel(inputs[a].tensor(), inputs[b].tensor(), outputs[out].tensor());
  }
};

static const float data_a[6] = {1, 2, 3, 4, 5, 6};
static const float data_b[6] = {1, 1, 1, 1, 1, 1};
static localCircularArenaAllocator<1024> meta_allocator;

int main(int argc, const char** argv) {
  // we only use RomTensor and BufferTensor, which use no ram, in this tutorial,
  // so we only need to setup metadata allocator, which is responsible for
  // allocating spaces storing meta data of tensors
  Context::get_default_context()->set_metadata_allocator(&meta_allocator);
  Tensor tensor_a = new RomTensor({1, 6}, data_a);
  Tensor tensor_b = new RomTensor({1, 6}, data_b);
  float* data_out = new float[3];
  Tensor tensor_out = new BufferTensor({6, 1}, flt, data_out);

  MyOperator op;
  op.set_inputs({{MyOperator::a, tensor_a}, {MyOperator::b, tensor_b}})
      .set_outputs({{MyOperator::out, tensor_out}})
      .eval();
  // after eval(), you can read the output with () operator
  for (uint32_t i = 0; i < tensor_out->num_elems(); ++i) {
    float elem = static_cast<float>(tensor_out(i));
    printf("%ith element of output tensor: %0.1f\n", i, elem);
  }
  return 0;
}