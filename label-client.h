

#include <event2/event.h>
#include <event2/http.h>
#include <ch-pal/exp_pal.h>
#include <ch-utils/exp_sock_utils.h>
#include <ch-cpp-utils/fts.hpp>
#include <ch-cpp-utils/base64.h>
#include <ch-cpp-utils/fs-watch.hpp>
#include <ch-cpp-utils/thread-pool.hpp>
#include <ch-cpp-utils/http-client.hpp>
#include <ch-cpp-utils/http-request.hpp>
#include <ch-cpp-utils/third-party/json/json.hpp>
#include <ch-protos/packet.pb.h>
#include <ch-protos/communication.pb.h>
#include <ch-protos/label-client-internal.pb.h>

#include "config.h"
#include "label-image.h"


using label_client_internal::NetworkMessage;
using label_client_internal::NetworkMessage_Label;

using ChCppUtils::base64_encode;
using ChCppUtils::FsWatch;
using ChCppUtils::ThreadPool;
using ChCppUtils::Fts;
using ChCppUtils::OnFileData;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpResponse;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

using json = nlohmann::json;

class LabelClient;

class LabelClient {
private:
    LabelImage *labelImage;
    Fts *fts;
    FsWatch *fsWatch;
    PAL_SOCK_HDL hl_sock_hdl;
    uint8_t *puc_dns_name_str;
    uint16_t us_host_port_ho;
    ThreadPool *mImagePool;
    ThreadPool *mNetworkPool;
    Config *config;
    string esPrefix;

    static void _onFile (OnFileData &data, void *this_);
    void onFile (OnFileData &data);

    static void _onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores, void *this_);
    void onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores);

    static void *_imageRoutine (void *arg, struct event_base *base);
    static void *_networkRoutine (void *arg, struct event_base *base);

    void *imageRoutine ();
    void *networkRoutine (NetworkMessage *message);

    static void _onLoad(HttpRequestLoadEvent *event, void *this_);
    void onLoad(HttpRequestLoadEvent *event);

    static void _onNewFile (OnFileData &data, void *this_);
    void onNewFile (OnFileData &data);
public:
    LabelClient(Config *config);
    LabelClient(uint8_t *puc_dns_name_str, uint16_t us_host_port_ho);
    ~LabelClient();
    void init();
    void process();
};
