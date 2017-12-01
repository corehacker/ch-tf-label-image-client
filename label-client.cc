
#include "label-client.h"

using std::to_string;

using indexer::PacketHeader;
using indexer::Packet;
using indexer::Index;
using indexer::IndexEntry;

using ChCppUtils::ThreadJob;
using ChCppUtils::FtsOptions;

LabelClient::LabelClient(Config *config) {
  this->config = config;
  labelImage = NULL;
  fts = NULL;
  fsWatch = NULL;
  mImagePool = NULL;
  mNetworkPool = NULL;
  hl_sock_hdl = NULL;
  puc_dns_name_str = (uint8_t *) "127.0.0.1";
  us_host_port_ho = 8888;
  esPrefix = config->getEsProtocol() + "://" + 
    config->getEsHostname() + ":" + to_string(config->getEsPort()) +
    config->getEsPrefixPath();
  LOG(INFO) << "Elastic search prefix: " << esPrefix;
}

LabelClient::LabelClient(uint8_t *puc_dns_name_str, uint16_t us_host_port_ho) {
  this->puc_dns_name_str = puc_dns_name_str;
  this->us_host_port_ho = us_host_port_ho; 
}

LabelClient::~LabelClient() {
}

void LabelClient::_onLoad(HttpRequestLoadEvent *event, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  return client->onLoad(event);
}

void LabelClient::onLoad(HttpRequestLoadEvent *event) {
  HttpResponse *response = event->getResponse();
  LOG(INFO) << "New Async Request (Complete): " <<
    response->getResponseCode() << " " << response->getResponseText();
}

void * LabelClient::_imageRoutine (void *arg, struct event_base *base) {
  LabelClient *client = (LabelClient *) arg;
  return client->imageRoutine();
}

void * LabelClient::_networkRoutine (void *arg, struct event_base *base) {
  NetworkMessage *message = (NetworkMessage *) arg;
  LabelClient *client = (LabelClient *) message->client();
  return client->networkRoutine(message);
}

void *LabelClient::imageRoutine () {
  fts->walk(LabelClient::_onFile, this);
  return NULL;
}

void *LabelClient::networkRoutine (NetworkMessage *message) {
  HttpRequest *httpRequest = new HttpRequest();
  httpRequest->onLoad(LabelClient::_onLoad).bind(this);

  json body;

  string file = message->image();
  string base64 = base64_encode((unsigned char *) file.data(), file.length());
  body["name"] = file;
  body["base64"] = base64;

  for (int pos = 0; pos < message->labels_size(); ++pos) {
    NetworkMessage_Label label = message->labels(pos);
    LOG(INFO) << message->image() << " (" << label.label() << "): " << label.score();
    body[label.label()] = label.score();
  }

  string b = body.dump(); 
  LOG(INFO) << body.dump(2);

  std::string authorization = "Basic ";
  std::string user = "elastic:changeme";
  authorization += base64_encode((unsigned char *) user.data(), user.length());

  LOG(INFO) << "Authorization: " << authorization;

  string url = esPrefix + "/" + base64;
  httpRequest->open(EVHTTP_REQ_PUT, url)
    .setHeader("Authorization", authorization)
    .setHeader("Content-Type", "application/json; charset=UTF-8")
    .send((void *) b.data(), b.length());

#if 0
  Packet packet;
  PacketHeader *header = packet.mutable_header();
  header->set_payload(0);
  Index *payload = packet.mutable_payload();

  LOG(INFO) << "Payload? " << packet.has_payload();

  IndexEntry *entry = NULL;

  for (int pos = 0; pos < message->labels_size(); ++pos) {
    NetworkMessage_Label label = message->labels(pos);
    LOG(INFO) << message->image() << " (" << label.label() << "): " << label.score();
    entry = payload->add_entry();
    entry->set_path(message->image());
    entry->set_index(pos);
    entry->set_key(label.label());
    entry->set_probability(label.score());
  }
  LOG(INFO) << "Num of entries " << payload->entry_size();

  int size = packet.ByteSize();

  header->set_payload(size);

  LOG(INFO) << "Payload? " << packet.has_payload();
  size = packet.ByteSize();

  uint8_t *buffer = (uint8_t *) malloc(size);
  memset(buffer, 0x00, size);
  packet.SerializeToArray(buffer, size);

  PAL_RET_E e_error = ePAL_RET_FAILURE;
  e_error = pal_sock_send (hl_sock_hdl,
      (uint8_t *) buffer,
      (uint32_t *) &size,
      0);
  if (ePAL_RET_SUCCESS != e_error) {
    LOG(ERROR) << "Error sending to server. " << e_error;
  } else {
    LOG(INFO) << "Success sending to server. " << e_error;
  }
#endif

  return NULL;
}

void LabelClient::init() {
    PAL_RET_E e_error = ePAL_RET_FAILURE;
    SOCK_UTIL_HOST_INFO_X x_host_info = {0};
    x_host_info.ui_bitmask |= eSOCK_UTIL_HOST_INFO_DNS_NAME_BM;
    x_host_info.ui_bitmask |= eSOCK_UTIL_HOST_INFO_HOST_PORT_BM;
    x_host_info.puc_dns_name_str = puc_dns_name_str;
    x_host_info.us_host_port_ho = us_host_port_ho;
       
    e_error = tcp_connect_sock_create (&hl_sock_hdl, &x_host_info, 1000);
    if (ePAL_RET_SUCCESS != e_error) {
      LOG(ERROR) << "Error connecting to server. " << e_error;
    } else {
      LOG(INFO) << "Success connecting to server. " << e_error;
    }

    labelImage = new LabelImage();
    labelImage->init(LabelClient::_onLabel, this);

    vector<string> filters;
    filters.emplace_back("jpg");
    filters.emplace_back("png");
    fsWatch = new FsWatch("/tensorflow/tensorflow/examples/ch-tf-label-image-client");
    fsWatch->init();
    fsWatch->OnNewFileCbk(LabelClient::_onNewFile, this);
    fsWatch->start(filters);

    FtsOptions options;
    memset(&options, 0x00, sizeof(FtsOptions));
    options.bIgnoreRegularFiles = false;
    options.bIgnoreHiddenFiles = true;
    options.bIgnoreHiddenDirs = true;
    options.bIgnoreRegularDirs = true;
    options.filters.emplace_back<string>("jpg");
    fts = new Fts ("./tensorflow/examples/ch-tf-label-image-client", &options);

    mImagePool = new ThreadPool (1, false);
    mNetworkPool = new ThreadPool (1, false);
}

void LabelClient::process() {
  ThreadJob *job = new ThreadJob (LabelClient::_imageRoutine, this);
  mImagePool->addJob(job);
  std::chrono::milliseconds ms(1000);
  while (true) {
     std::this_thread::sleep_for(ms);
  }
}

void LabelClient::_onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  client->onLabel(image, labels, scores);
}

void LabelClient::onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores) {
  NetworkMessage *message = new NetworkMessage();
  message->set_client((::google::protobuf::uint64) this);
  message->set_image(image);

  for (unsigned int pos = 0; pos < labels.size(); ++pos) {
    NetworkMessage_Label *label = message->add_labels();
    label->set_label(labels[pos]);
    label->set_score(scores[pos]);
  }
  ThreadJob *job = new ThreadJob (LabelClient::_networkRoutine, message);
  mNetworkPool->addJob(job);
}

void LabelClient::_onFile (OnFileData &data, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  client->onFile(data);
}

void LabelClient::onFile (OnFileData &data) {
  LOG(INFO) << "File: " << data.path.data();
  labelImage->process(data.path);
}

void LabelClient::_onNewFile (OnFileData &data, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  client->onNewFile(data);
}

void LabelClient::onNewFile (OnFileData &data) {
  LOG(INFO) << "New File: " << data.path.data();
  labelImage->process(data.path);
}


