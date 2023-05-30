//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).


#include "rocksdb/utilities/debug.h"

#include "db/db_impl/db_impl.h"
#include "rocksdb/utilities/options_type.h"

namespace ROCKSDB_NAMESPACE {

static std::unordered_map<std::string, rs::db::dbformat::ValueType> value_type_string_map = {
    {"TypeDeletion", rs::db::dbformat::ValueType::kTypeDeletion},
    {"TypeValue", rs::db::dbformat::ValueType::kTypeValue},
    {"TypeMerge", rs::db::dbformat::ValueType::kTypeMerge},
    {"TypeLogData", rs::db::dbformat::ValueType::kTypeLogData},
    {"TypeColumnFamilyDeletion", rs::db::dbformat::ValueType::kTypeColumnFamilyDeletion},
    {"TypeColumnFamilyValue", rs::db::dbformat::ValueType::kTypeColumnFamilyValue},
    {"TypeColumnFamilyMerge", rs::db::dbformat::ValueType::kTypeColumnFamilyMerge},
    {"TypeSingleDeletion", rs::db::dbformat::ValueType::kTypeSingleDeletion},
    {"TypeColumnFamilySingleDeletion",
     rs::db::dbformat::ValueType::kTypeColumnFamilySingleDeletion},
    {"TypeBeginPrepareXID", rs::db::dbformat::ValueType::kTypeBeginPrepareXID},
    {"TypeEndPrepareXID", rs::db::dbformat::ValueType::kTypeEndPrepareXID},
    {"TypeCommitXID", rs::db::dbformat::ValueType::kTypeCommitXID},
    {"TypeRollbackXID", rs::db::dbformat::ValueType::kTypeRollbackXID},
    {"TypeNoop", rs::db::dbformat::ValueType::kTypeNoop},
    {"TypeColumnFamilyRangeDeletion",
     rs::db::dbformat::ValueType::kTypeColumnFamilyRangeDeletion},
    {"TypeRangeDeletion", rs::db::dbformat::ValueType::kTypeRangeDeletion},
    {"TypeColumnFamilyBlobIndex", rs::db::dbformat::ValueType::kTypeColumnFamilyBlobIndex},
    {"TypeBlobIndex", rs::db::dbformat::ValueType::kTypeBlobIndex},
    {"TypeBeginPersistedPrepareXID", rs::db::dbformat::ValueType::kTypeBeginPersistedPrepareXID},
    {"TypeBeginUnprepareXID", rs::db::dbformat::ValueType::kTypeBeginUnprepareXID},
    {"TypeDeletionWithTimestamp", rs::db::dbformat::ValueType::kTypeDeletionWithTimestamp},
    {"TypeCommitXIDAndTimestamp", rs::db::dbformat::ValueType::kTypeCommitXIDAndTimestamp},
    {"TypeWideColumnEntity", rs::db::dbformat::ValueType::kTypeWideColumnEntity},
    {"TypeColumnFamilyWideColumnEntity",
     rs::db::dbformat::ValueType::kTypeColumnFamilyWideColumnEntity}};

std::string KeyVersion::GetTypeName() const {
  std::string type_name;
  if (SerializeEnum<rs::db::dbformat::ValueType>(value_type_string_map,
                               static_cast<rs::db::dbformat::ValueType>(type), &type_name)) {
    return type_name;
  } else {
    return "Invalid";
  }
}

Status GetAllKeyVersions(DB* db, Slice begin_key, Slice end_key,
                         size_t max_num_ikeys,
                         std::vector<KeyVersion>* key_versions) {
  if (nullptr == db) {
    return Status::InvalidArgument("db cannot be null.");
  }
  return GetAllKeyVersions(db, db->DefaultColumnFamily(), begin_key, end_key,
                           max_num_ikeys, key_versions);
}

Status GetAllKeyVersions(DB* db, ColumnFamilyHandle* cfh, Slice begin_key,
                         Slice end_key, size_t max_num_ikeys,
                         std::vector<KeyVersion>* key_versions) {
  if (nullptr == db) {
    return Status::InvalidArgument("db cannot be null.");
  }
  if (nullptr == cfh) {
    return Status::InvalidArgument("Column family handle cannot be null.");
  }
  if (nullptr == key_versions) {
    return Status::InvalidArgument("key_versions cannot be null.");
  }
  key_versions->clear();

  DBImpl* idb = static_cast<DBImpl*>(db->GetRootDB());
  auto icmp = InternalKeyComparator(idb->GetOptions(cfh).comparator);
  ReadOptions read_options;
  Arena arena;
  ScopedArenaIterator iter(
      idb->NewInternalIterator(read_options, &arena, kMaxSequenceNumber, cfh));

  if (!begin_key.empty()) {
    InternalKey ikey;
    ikey.SetMinPossibleForUserKey(begin_key);
    iter->Seek(ikey.Encode());
  } else {
    iter->SeekToFirst();
  }

  size_t num_keys = 0;
  for (; iter->Valid(); iter->Next()) {
    ParsedInternalKey ikey;
    Status pik_status =
        ParseInternalKey(iter->key(), &ikey, true /* log_err_key */);  // TODO
    if (!pik_status.ok()) {
      return pik_status;
    }

    if (!end_key.empty() &&
        icmp.user_comparator()->Compare(ikey.user_key, end_key) > 0) {
      break;
    }

    key_versions->emplace_back(ikey.user_key.ToString() /* _user_key */,
                               iter->value().ToString() /* _value */,
                               ikey.sequence /* _sequence */,
                               ikey.type /* _type */);
    if (++num_keys >= max_num_ikeys) {
      break;
    }
  }
  return Status::OK();
}

}  // namespace ROCKSDB_NAMESPACE

