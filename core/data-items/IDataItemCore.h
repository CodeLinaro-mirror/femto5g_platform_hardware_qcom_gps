/* Copyright (c) 2015, 2017, 2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __IDATAITEMCORE_H__
#define __IDATAITEMCORE_H__

#include <string>
#include <cstring>
#include <inttypes.h>
#include <DataItemId.h>

namespace loc_core {

/**
 * @brief IDataItemCore interface.
 * @details IDataItemCore interface.
 *
 */
class IDataItemCore {
public:
    /**
     * @brief Gets Data item id/blob pointer and blob size.
     */
    inline DataItemId getId() const { return mId; }
    inline std::string getName() const { return mName; }
    inline void* getBlobPtr() const { return mBlob; }
    inline size_t getBlobSize() const { return mLen; }

    /**
     * @brief Stringify.
     * @details Stringify.
     *
     * @param valueStr Reference to string.
     */
    virtual void stringify (std::string & valueStr) = 0;

    /**
     * @brief equal
     * @details equal.
     *
     * @param rls another data item to be compared.
     *
     * @return bool if rls is equal to this.
     */
    inline bool equal(const IDataItemCore* rls) {
        if (mBlob == nullptr || rls->getBlobPtr() == nullptr || mLen != rls->getBlobSize()) {
            return false;
        }
        return (0 == memcmp(mBlob, rls->getBlobPtr(), mLen));
    }

    /**
     * @brief Destructor.
     * @details Destructor.
     */
    virtual ~IDataItemCore () {}
protected:

    //Must be called after all member variables are determined in data item,
    //mBlob and mLen are used for data serialization
    inline void setBlobPtr(void* blob, size_t len) { mBlob = blob; mLen = len; }

    DataItemId mId = INVALID_DATA_ITEM_ID;
    const char* mName = nullptr;
    void * mBlob = nullptr;
    size_t mLen = 0;
};

} // namespace loc_core

#endif // __IDATAITEMCORE_H__
