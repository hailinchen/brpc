// Copyright (c) 2014 Baidu, Inc.
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

// Authors: Ge,Jun (gejun@baidu.com)

#include "butil/time.h"
#include "butil/logging.h"
#include "brpc/controller.h"           // Controller
#include "brpc/closure_guard.h"        // ClosureGuard
#include "brpc/builtin/threads_service.h"
#include "brpc/builtin/common.h"
#include "butil/string_printf.h"

namespace brpc {

void ThreadsService::default_method(::google::protobuf::RpcController* cntl_base,
                                    const ::brpc::ThreadsRequest*,
                                    ::brpc::ThreadsResponse*,
                                    ::google::protobuf::Closure* done) {
    ClosureGuard done_guard(done);
    Controller *cntl = static_cast<Controller*>(cntl_base);
    cntl->http_response().set_content_type("text/plain");
    butil::IOBuf& resp = cntl->response_attachment();

    butil::IOPortal read_portal;
    std::string cmd = butil::string_printf("pstack %lld", (long long)getpid());
    butil::Timer tm;
    tm.start();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe == NULL) {
        LOG(FATAL) << "Fail to popen `" << cmd << "'";
        return;
    }
    read_portal.append_from_file_descriptor(fileno(pipe), MAX_READ);
    resp.swap(read_portal);
    
    // Call fread, otherwise following error will be reported:
    //   sed: couldn't flush stdout: Broken pipe
    // and pclose will fail:
    //   CHECK failed: 0 == pclose(pipe): Resource temporarily unavailable
    size_t fake_buf;
    butil::ignore_result(fread(&fake_buf, sizeof(fake_buf), 1, pipe));
    CHECK_EQ(0, pclose(pipe)) << berror();
    tm.stop();
    resp.append(butil::string_printf("\n\ntime=%lums", tm.m_elapsed()));
}

} // namespace brpc
