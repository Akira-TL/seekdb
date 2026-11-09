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

#define USING_LOG_PREFIX LIB

#include "ob_ai_split_document_util.h"
#include "lib/oblog/ob_log_module.h"
#include "lib/ob_check_macros.h"

namespace oceanbase
{
namespace common
{

// Helper template function to create iterator
template<typename IteratorType>
static int create_iterator_impl(ObIAllocator &allocator, ObDocSplitIterator *&iterator)
{
  int ret = OB_SUCCESS;
  void *buf = allocator.alloc(sizeof(IteratorType));
  if (OB_ISNULL(buf)) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("failed to alloc memory for iterator", K(ret));
  } else {
    iterator = new (buf) IteratorType();
  }
  return ret;
}

int ObAiSplitDocumentUtil::create_doc_split_iterator(const ObString &content,
                                                     const ObAiSplitDocParams &params,
                                                     ObIAllocator &allocator,
                                                     ObDocSplitIterator *&iterator)
{
  int ret = OB_SUCCESS;
  if (params.type_ == ObAiSplitContentType::TEXT) {
    OZ (create_iterator_impl<ObTextSplitIterator>(allocator, iterator));
  } else if (params.type_ == ObAiSplitContentType::MARKDOWN) {
    OZ (create_iterator_impl<ObMarkdownSplitIterator>(allocator, iterator));
  } else {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid content type", K(ret), K(params.type_));
    FORWARD_USER_ERROR(ret, "invalid parameters value for parameters.type, type must be 'text' or 'markdown'");
  }
  return ret;
}

} // namespace common
} // namespace oceanbase
