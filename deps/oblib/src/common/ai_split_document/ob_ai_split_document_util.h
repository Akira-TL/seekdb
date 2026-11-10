/**
 * Copyright (c) 2025 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#pragma once

#include "ob_ai_split_document.h"
#include "lib/alloc/malloc_hook.h"


namespace oceanbase
{
namespace common
{

class ObAiSplitDocumentUtil
{
public:
  static int create_doc_split_iterator(const ObString &content,
                                       const ObAiSplitDocParams &params,
                                       ObIAllocator &allocator,
                                       ObDocSplitIterator *&iterator);
};
} // namespace common
} // namespace oceanbase
