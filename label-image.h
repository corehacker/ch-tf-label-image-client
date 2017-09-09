

#include <fstream>
#include <utility>
#include <vector>

#include "tensorflow/cc/ops/const_op.h"
#include "tensorflow/cc/ops/image_ops.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

// These are all common classes it's handy to reference with no namespace.
using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::int32;

typedef void (*OnLabel) (std::string image, std::vector<std::string> labels, std::vector<float> scores, void *this_);

class LabelImage {
private:
  std::unique_ptr<tensorflow::Session> session;
  string root;
  string graph;
  int32 input_width;
  int32 input_height;
  float input_mean;
  float input_std;
  string input_layer;
  string output_layer;
  bool self_test;
  string labels;
  OnLabel onLabel;
  void *onLabelThis;

  Status LoadGraph(const string& graph_file_name,
        std::unique_ptr<tensorflow::Session>* session);
  Status ReadEntireFile(tensorflow::Env* env,
        const string& filename,
        Tensor* output);
  Status ReadTensorFromImageFile(const string& file_name,
        const int input_height,
        const int input_width,
        const float input_mean,
        const float input_std,
        std::vector<Tensor>* out_tensors);
  Status ReadLabelsFile(const string& file_name,
        std::vector<string>* result,
        size_t* found_label_count);
  Status GetTopLabels(const std::vector<Tensor>& outputs,
        int how_many_labels,
        Tensor* indices, Tensor* scores);
  Status PrintTopLabels(
        const std::string image,
        const std::vector<Tensor>& outputs,
        const string& labels_file_name);
  Status CheckTopLabel(const std::vector<Tensor>& outputs,
        int expected,
        bool* is_expected);
public:
  LabelImage();
  LabelImage(string root, string graph);
  ~LabelImage();
  int init(OnLabel onLabel, void *this_);
  int process(string image);
};
