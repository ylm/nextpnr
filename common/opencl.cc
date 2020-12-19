/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2020  David Shah <dave@ds0.me>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef USE_OPENCL

#include "opencl.h"
#include "log.h"
#include "nextpnr.h"
#include "util.h"

NEXTPNR_NAMESPACE_BEGIN

std::unique_ptr<cl::Context> get_opencl_ctx(Context *ctx)
{
    cl_int err;
    auto dev_type = CL_DEVICE_TYPE_DEFAULT;
    if (bool_or_default(ctx->settings, ctx->id("opencl/useCPU")))
        dev_type = CL_DEVICE_TYPE_CPU;
    std::unique_ptr<cl::Context> clctx(new cl::Context(dev_type, nullptr, nullptr, nullptr, &err));
    if (err != CL_SUCCESS)
        log_error("Failed to create OpenCL context: %d\n", err);
    std::vector<cl::Device> devices;
    clctx->getInfo(CL_CONTEXT_DEVICES, &devices);
    log_info("Using OpenCL devices: \n");
    for (auto &dev : devices) {
        std::string dev_name = dev.getInfo<CL_DEVICE_NAME>();
        log("        %s\n", dev_name.c_str());
    }
    return clctx;
}
std::unique_ptr<cl::Program> get_opencl_program(cl::Context &clctx, const std::string &name)
{
    auto src_code = get_opencl_source(name);
    std::unique_ptr<cl::Program> prog(new cl::Program(clctx, src_code));
    try {
        prog->build();
    } catch (cl::Error &e) {
        if (e.err() == CL_BUILD_PROGRAM_FAILURE) {
            std::vector<cl::Device> devices;
            clctx.getInfo(CL_CONTEXT_DEVICES, &devices);
            for (auto &dev : devices) {
                std::string dev_name = dev.getInfo<CL_DEVICE_NAME>();
                std::string log = prog->getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
                log_nonfatal_error("Log for device %s:\n%s\n", dev_name.c_str(), log.c_str());
            }
            log_error("OpenCL build of '%s' failed.\n", name.c_str());
        } else {
            throw e;
        }
    }
    return prog;
}

NEXTPNR_NAMESPACE_END

#endif
