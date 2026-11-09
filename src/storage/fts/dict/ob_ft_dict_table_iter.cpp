/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "storage/fts/dict/ob_ft_dict_table_iter.h"

#include "common/mysqlclient/ob_mysql_proxy.h"
#include "common/mysqlclient/ob_mysql_result.h"
#include "lib/ob_errno.h"
#include "lib/oblog/ob_log_module.h"
#include "lib/utility/ob_macro_utils.h"
#include "share/ob_server_struct.h"

#define USING_LOG_PREFIX STORAGE_FTS

namespace oceanbase
{
namespace storage
{
static bool is_valid_dict_identifier(const ObString &identifier)
{
  bool valid = !identifier.empty();
  for (int64_t i = 0; valid && i < identifier.length(); ++i) {
    const char c = identifier.ptr()[i];
    valid = (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_' || c == '$';
  }
  return valid;
}

static int append_qualified_dict_table(ObSqlString &sql, const ObString &qualified_table_name)
{
  int ret = OB_SUCCESS;
  int64_t dot_pos = -1;
  for (int64_t i = 0; OB_SUCC(ret) && i < qualified_table_name.length(); ++i) {
    if (qualified_table_name.ptr()[i] == '.') {
      if (dot_pos >= 0) {
        ret = OB_INVALID_ARGUMENT;
      } else {
        dot_pos = i;
      }
    }
  }
  if (OB_FAIL(ret) || dot_pos <= 0 || dot_pos >= qualified_table_name.length() - 1) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("custom dictionary table must be database.table", K(ret), K(qualified_table_name));
  } else {
    const ObString database_name(
        static_cast<int32_t>(dot_pos), qualified_table_name.ptr());
    const ObString table_name(
        static_cast<int32_t>(qualified_table_name.length() - dot_pos - 1),
        qualified_table_name.ptr() + dot_pos + 1);
    if (!is_valid_dict_identifier(database_name) || !is_valid_dict_identifier(table_name)) {
      ret = OB_INVALID_ARGUMENT;
      LOG_WARN("invalid custom dictionary table name", K(ret), K(qualified_table_name));
    } else if (OB_FAIL(sql.append_fmt("`%.*s`.`%.*s`",
                                      database_name.length(),
                                      database_name.ptr(),
                                      table_name.length(),
                                      table_name.ptr()))) {
      LOG_WARN("failed to append custom dictionary table", K(ret), K(qualified_table_name));
    }
  }
  return ret;
}

ObFTDictTableIter::ObFTDictTableIter(ObISQLClient::ReadResult &result)
    : ObIFTDictIterator(), is_inited_(false), res_(result)
{
}

int ObFTDictTableIter::get_key(ObString &str)
{
  int ret = OB_SUCCESS;
  if (!IS_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("Not inited.", K(ret));
  } else if (OB_FAIL(res_.get_result()->get_varchar("word", str))) {
    LOG_WARN("Failed to get varchar", K(ret));
  }
  return ret;
}

int ObFTDictTableIter::get_value()
{
  int ret = OB_SUCCESS;
  return ret;
}

int ObFTDictTableIter::next()
{
  int ret = OB_SUCCESS;
  if (!IS_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("Not inited.", K(ret));
  } else if (OB_FAIL(res_.get_result()->next())) {
    if (OB_ITER_END != ret) {
      LOG_WARN("Failed to get next row", K(ret));
    }
  }
  return ret;
}

int ObFTDictTableIter::init(const ObString &table_name)
{
  int ret = OB_SUCCESS;
  common::ObMySQLProxy *sql_proxy = GCTX.sql_proxy_;

  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("Inited twice.", K(ret));
  } else {
    SMART_VAR(ObSqlString, sql_string)
    {
      if (OB_FAIL(sql_string.append("SELECT word FROM oceanbase."))) {
        LOG_WARN("Failed to append sql", K(ret));
      } else if (OB_FAIL(sql_string.append(table_name))) {
        LOG_WARN("Failed to append sql", K(ret));
      } else if (OB_FAIL(sql_string.append(" ORDER BY word"))) {
        LOG_WARN("Failed to append sql", K(ret));
      } else if (OB_FAIL(sql_proxy->read(res_, sql_string.ptr()))) {
        LOG_WARN("Failed to execute sql", K(ret));
      }
    }

    if (OB_FAIL(ret)) {
      // already logged
    } else if (OB_ISNULL(res_.get_result())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Failed to get result", K(ret));
    } else if (OB_FAIL(res_.get_result()->next())) {
      if (OB_ITER_END != ret) {
        LOG_WARN("Failed to get next row", K(ret));
      } else {
        ret = OB_SUCCESS;
        is_inited_ = true;
      }
    } else {
      is_inited_ = true;
    }
  }

  return ret;
}

int ObFTDictTableIter::init_custom_table(const ObString &qualified_table_name)
{
  int ret = OB_SUCCESS;
  common::ObMySQLProxy *sql_proxy = GCTX.sql_proxy_;

  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("Inited twice.", K(ret));
  } else if (OB_ISNULL(sql_proxy)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("SQL proxy is null", K(ret));
  } else {
    SMART_VAR(ObSqlString, sql_string)
    {
      if (OB_FAIL(sql_string.append("SELECT word FROM "))) {
        LOG_WARN("Failed to append SQL", K(ret));
      } else if (OB_FAIL(append_qualified_dict_table(sql_string, qualified_table_name))) {
        LOG_WARN("Failed to append qualified dictionary table", K(ret), K(qualified_table_name));
      } else if (OB_FAIL(sql_string.append(" ORDER BY word"))) {
        LOG_WARN("Failed to append SQL", K(ret));
      } else if (OB_FAIL(sql_proxy->read(res_, sql_string.ptr()))) {
        LOG_WARN("Failed to execute custom dictionary query", K(ret), K(qualified_table_name));
      }
    }

    if (OB_FAIL(ret)) {
    } else if (OB_ISNULL(res_.get_result())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Failed to get result", K(ret));
    } else if (OB_FAIL(res_.get_result()->next())) {
      if (OB_ITER_END != ret) {
        LOG_WARN("Failed to get next row", K(ret));
      } else {
        ret = OB_SUCCESS;
        is_inited_ = true;
      }
    } else {
      is_inited_ = true;
    }
  }

  return ret;
}

void ObFTDictTableIter::reset()
{
  res_.close();
  is_inited_ = false;
}

} //  namespace storage
} //  namespace oceanbase
