/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2017, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2017, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   config.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <string>
#include <glog/logging.h>

#include "config.h"

using std::ifstream;

Config::Config() :
				ChCppUtils::Config("/etc/ch-tf-label-image-client/ch-tf-label-image-client.json",
						"./ch-tf-label-image-client.json") {
        esProtocol = "http";
        esHostname = "127.0.0.1";
        esPort = 9200;
}

Config::~Config() {
	LOG(INFO) << "*****************~Config";
}

bool Config::populateConfigValues() {
	LOG(INFO) << "<-----------------------Config";

        esProtocol = mJson["elastic-search"]["protocol"];
        LOG(INFO) << "elastic-search.protocol : " << esProtocol;

        esHostname = mJson["elastic-search"]["hostname"];
        LOG(INFO) << "elastic-search.hostname : " << esHostname;

        esPort = mJson["elastic-search"]["port"];
        LOG(INFO) << "elastic-search.port : " << esPort;


	LOG(INFO) << "----------------------->Config";
	return true;
}

void Config::init() {
	ChCppUtils::Config::init();

	populateConfigValues();
}

string &Config::getEsProtocol() {
        return esProtocol;
}

string &Config::getEsHostname() {
        return esHostname;
}

uint16_t Config::getEsPort() {
        return esPort;
}

