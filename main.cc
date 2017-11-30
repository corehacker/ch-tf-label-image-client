
#include "label-client.h"

#include "config.h"

static Config *config = nullptr;

int main(int argc, char* argv[]) {

  google::InstallFailureSignalHandler();

  config = new Config();
  config->init();

  PAL_LOGGER_INIT_PARAMS_X x_init_params = {false};
  pal_env_init ();                                                                 

  
  x_init_params.e_level = eLOG_LEVEL_HIGH;                                         
  x_init_params.b_enable_console_logging = true;                                   
  pal_logger_env_init(&x_init_params);

  // These are the command-line flags the program can understand.
  // They define where the graph and input data is located, and what kind of
  // input the model expects. If you train your own model, or use something
  // other than inception_v3, then you'll need to update these.
  string image = "tensorflow/examples/ch-tf-label-image-client/data/grace_hopper.jpg";
  string graph =
      "tensorflow/examples/ch-tf-label-image-client/data/inception_v3_2016_08_28_frozen.pb";
  string labels =
      "tensorflow/examples/ch-tf-label-image-client/data/imagenet_slim_labels.txt";
  int32 input_width = 299;
  int32 input_height = 299;
  float input_mean = 0;
  float input_std = 255;
  string input_layer = "input";
  string output_layer = "InceptionV3/Predictions/Reshape_1";
  bool self_test = false;
  string root_dir = "";
  bool daemon = false;
  std::vector<Flag> flag_list = {
      Flag("daemon", &daemon, "Daemonize the process"),
      Flag("image", &image, "image to be processed"),
      Flag("graph", &graph, "graph to be executed"),
      Flag("labels", &labels, "name of file containing labels"),
      Flag("input_width", &input_width, "resize image to this width in pixels"),
      Flag("input_height", &input_height,
           "resize image to this height in pixels"),
      Flag("input_mean", &input_mean, "scale pixel values to this mean"),
      Flag("input_std", &input_std, "scale pixel values to this std deviation"),
      Flag("input_layer", &input_layer, "name of input layer"),
      Flag("output_layer", &output_layer, "name of output layer"),
      Flag("self_test", &self_test, "run a self test"),
      Flag("root_dir", &root_dir,
           "interpret image and graph file names relative to this directory"),
  };
  string usage = tensorflow::Flags::Usage(argv[0], flag_list);
  const bool parse_result = tensorflow::Flags::Parse(&argc, argv, flag_list);
  if (!parse_result) {
    LOG(ERROR) << usage;
    return -1;
  }

  if (daemon) {
    PAL_DAMONIZE_PROCESS_PARAMS_X x_daemon = {0};
    pal_daemonize_process (&x_daemon);
  }

  // We need to call this to set up global state for TensorFlow.
  tensorflow::port::InitMain(argv[0], &argc, &argv);
  if (argc > 1) {
    LOG(ERROR) << "Unknown argument " << argv[1] << "\n" << usage;
    return -1;
  }

  LabelClient *client = new LabelClient(config);
  client->init();
  client->process();

  return 0;
}
