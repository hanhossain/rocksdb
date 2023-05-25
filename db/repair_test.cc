//  Copyright (c) 2016-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "rocksdb/options.h"

#include <algorithm>
#include <string>
#include <vector>

#include "db/db_impl/db_impl.h"
#include "db/db_test_util.h"
#include "file/file_util.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/transaction_log.h"
#include "table/unique_id_impl.h"
#include "util/string_util.h"

namespace rocksdb {

class RepairTest : public DBTestBase {
 public:
  RepairTest() : DBTestBase("repair_test", /*env_do_fsync=*/true) {}

  Status GetFirstSstPath(std::string* first_sst_path) {
    assert(first_sst_path != nullptr);
    first_sst_path->clear();
    uint64_t manifest_size;
    std::vector<std::string> files;
    Status s = db_->GetLiveFiles(files, &manifest_size);
    if (s.ok()) {
      auto sst_iter =
          std::find_if(files.begin(), files.end(), [](const std::string& file) {
            uint64_t number;
            rs::types::FileType type;
            bool ok = ParseFileName(file, &number, &type);
            return ok && type == rs::types::FileType::TableFile;
          });
      *first_sst_path = sst_iter == files.end() ? "" : dbname_ + *sst_iter;
    }
    return s;
  }

  void ReopenWithSstIdVerify() {
    std::atomic_int verify_passed{0};
    SyncPoint::GetInstance()->SetCallBack(
        "BlockBasedTable::Open::PassedVerifyUniqueId", [&](void* arg) {
          // override job status
          auto id = static_cast<UniqueId64x2*>(arg);
          assert(*id != kNullUniqueId64x2);
          verify_passed++;
        });
    SyncPoint::GetInstance()->EnableProcessing();
    auto options = CurrentOptions();
    options.verify_sst_unique_id_in_manifest = true;
    Reopen(options);

    ASSERT_GT(verify_passed, 0);
    SyncPoint::GetInstance()->DisableProcessing();
  }

  std::vector<FileMetaData*> GetLevelFileMetadatas(int level, int cf = 0) {
    VersionSet* const versions = dbfull()->GetVersionSet();
    assert(versions);
    ColumnFamilyData* const cfd =
        versions->GetColumnFamilySet()->GetColumnFamily(cf);
    assert(cfd);
    Version* const current = cfd->current();
    assert(current);
    VersionStorageInfo* const storage_info = current->storage_info();
    assert(storage_info);
    return storage_info->LevelFiles(level);
  }
};

TEST_F(RepairTest, SortRepairedDBL0ByEpochNumber) {
  Options options = CurrentOptions();
  DestroyAndReopen(options);

  ASSERT_OK(Put("k1", "oldest"));
  ASSERT_OK(Put("k1", "older"));
  ASSERT_OK(Flush());
  MoveFilesToLevel(1);

  ASSERT_OK(Put("k1", "old"));
  ASSERT_OK(Flush());

  ASSERT_OK(Put("k1", "new"));

  std::vector<FileMetaData*> level0_files = GetLevelFileMetadatas(0 /* level*/);
  ASSERT_EQ(level0_files.size(), 1);
  ASSERT_EQ(level0_files[0]->epoch_number, 2);
  std::vector<FileMetaData*> level1_files = GetLevelFileMetadatas(1 /* level*/);
  ASSERT_EQ(level1_files.size(), 1);
  ASSERT_EQ(level1_files[0]->epoch_number, 1);

  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());
  Close();
  ASSERT_OK(env_->FileExists(manifest_path));
  ASSERT_OK(env_->DeleteFile(manifest_path));

  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));
  ReopenWithSstIdVerify();

  EXPECT_EQ(Get("k1"), "new");

  level0_files = GetLevelFileMetadatas(0 /* level*/);
  ASSERT_EQ(level0_files.size(), 3);
  EXPECT_EQ(level0_files[0]->epoch_number, 3);
  EXPECT_EQ(level0_files[1]->epoch_number, 2);
  EXPECT_EQ(level0_files[2]->epoch_number, 1);
  level1_files = GetLevelFileMetadatas(1 /* level*/);
  ASSERT_EQ(level1_files.size(), 0);
}

TEST_F(RepairTest, LostManifest) {
  // Add a couple SST files, delete the manifest, and verify RepairDB() saves
  // the day.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  // Need to get path before Close() deletes db_, but delete it after Close() to
  // ensure Close() didn't change the manifest.
  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

  Close();
  ASSERT_OK(env_->FileExists(manifest_path));
  ASSERT_OK(env_->DeleteFile(manifest_path));
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));
  ReopenWithSstIdVerify();

  ASSERT_EQ(Get("key"), "val");
  ASSERT_EQ(Get("key2"), "val2");
}

TEST_F(RepairTest, LostManifestMoreDbFeatures) {
  // Add a couple SST files, delete the manifest, and verify RepairDB() saves
  // the day.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Put("key3", "val3"));
  ASSERT_OK(Put("key4", "val4"));
  ASSERT_OK(Flush());
  // Test an SST file containing only a range tombstone
  ASSERT_OK(db_->DeleteRange(WriteOptions(), db_->DefaultColumnFamily(), "key2",
                             "key3z"));
  ASSERT_OK(Flush());
  // Need to get path before Close() deletes db_, but delete it after Close() to
  // ensure Close() didn't change the manifest.
  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

  Close();
  ASSERT_OK(env_->FileExists(manifest_path));
  ASSERT_OK(env_->DeleteFile(manifest_path));
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));

  // repair from sst should work with unique_id verification
  ReopenWithSstIdVerify();

  ASSERT_EQ(Get("key"), "val");
  ASSERT_EQ(Get("key2"), "NOT_FOUND");
  ASSERT_EQ(Get("key3"), "NOT_FOUND");
  ASSERT_EQ(Get("key4"), "val4");
}

TEST_F(RepairTest, CorruptManifest) {
  // Manifest is in an invalid format. Expect a full recovery.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  // Need to get path before Close() deletes db_, but overwrite it after Close()
  // to ensure Close() didn't change the manifest.
  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

  Close();
  ASSERT_OK(env_->FileExists(manifest_path));

  ASSERT_OK(CreateFile(env_->GetFileSystem(), manifest_path, "blah",
                       false /* use_fsync */));
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));

  ReopenWithSstIdVerify();

  ASSERT_EQ(Get("key"), "val");
  ASSERT_EQ(Get("key2"), "val2");
}

TEST_F(RepairTest, IncompleteManifest) {
  // In this case, the manifest is valid but does not reference all of the SST
  // files. Expect a full recovery.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  std::string orig_manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());
  CopyFile(orig_manifest_path, orig_manifest_path + ".tmp");
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  // Need to get path before Close() deletes db_, but overwrite it after Close()
  // to ensure Close() didn't change the manifest.
  std::string new_manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

  Close();
  ASSERT_OK(env_->FileExists(new_manifest_path));
  // Replace the manifest with one that is only aware of the first SST file.
  CopyFile(orig_manifest_path + ".tmp", new_manifest_path);
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));

  ReopenWithSstIdVerify();

  ASSERT_EQ(Get("key"), "val");
  ASSERT_EQ(Get("key2"), "val2");
}

TEST_F(RepairTest, PostRepairSstFileNumbering) {
  // Verify after a DB is repaired, new files will be assigned higher numbers
  // than old files.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  uint64_t pre_repair_file_num = dbfull()->TEST_Current_Next_FileNo();
  Close();

  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));

  ReopenWithSstIdVerify();

  uint64_t post_repair_file_num = dbfull()->TEST_Current_Next_FileNo();
  ASSERT_GE(post_repair_file_num, pre_repair_file_num);
}

TEST_F(RepairTest, LostSst) {
  // Delete one of the SST files but preserve the manifest that refers to it,
  // then verify the DB is still usable for the intact SST.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  std::string sst_path;
  ASSERT_OK(GetFirstSstPath(&sst_path));
  ASSERT_FALSE(sst_path.empty());
  ASSERT_OK(env_->DeleteFile(sst_path));

  Close();
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));
  ReopenWithSstIdVerify();

  // Exactly one of the key-value pairs should be in the DB now.
  ASSERT_TRUE((Get("key") == "val") != (Get("key2") == "val2"));
}

TEST_F(RepairTest, CorruptSst) {
  // Corrupt one of the SST files but preserve the manifest that refers to it,
  // then verify the DB is still usable for the intact SST.
  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  ASSERT_OK(Put("key2", "val2"));
  ASSERT_OK(Flush());
  std::string sst_path;
  ASSERT_OK(GetFirstSstPath(&sst_path));
  ASSERT_FALSE(sst_path.empty());

  ASSERT_OK(CreateFile(env_->GetFileSystem(), sst_path, "blah",
                       false /* use_fsync */));

  Close();
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));
  ReopenWithSstIdVerify();

  // Exactly one of the key-value pairs should be in the DB now.
  ASSERT_TRUE((Get("key") == "val") != (Get("key2") == "val2"));
}

TEST_F(RepairTest, UnflushedSst) {
  // This test case invokes repair while some data is unflushed, then verifies
  // that data is in the db.
  ASSERT_OK(Put("key", "val"));
  VectorLogPtr wal_files;
  ASSERT_OK(dbfull()->GetSortedWalFiles(wal_files));
  ASSERT_EQ(wal_files.size(), 1);
  {
    uint64_t total_ssts_size;
    std::unordered_map<std::string, uint64_t> sst_files;
    ASSERT_OK(GetAllDataFiles(rs::types::FileType::TableFile, &sst_files, &total_ssts_size));
    ASSERT_EQ(total_ssts_size, 0);
  }
  // Need to get path before Close() deletes db_, but delete it after Close() to
  // ensure Close() didn't change the manifest.
  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

  Close();
  ASSERT_OK(env_->FileExists(manifest_path));
  ASSERT_OK(env_->DeleteFile(manifest_path));
  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));
  ReopenWithSstIdVerify();

  ASSERT_OK(dbfull()->GetSortedWalFiles(wal_files));
  ASSERT_EQ(wal_files.size(), 0);
  {
    uint64_t total_ssts_size;
    std::unordered_map<std::string, uint64_t> sst_files;
    ASSERT_OK(GetAllDataFiles(rs::types::FileType::TableFile, &sst_files, &total_ssts_size));
    ASSERT_GT(total_ssts_size, 0);
  }
  ASSERT_EQ(Get("key"), "val");
}

TEST_F(RepairTest, SeparateWalDir) {
  do {
    Options options = CurrentOptions();
    DestroyAndReopen(options);
    ASSERT_OK(Put("key", "val"));
    ASSERT_OK(Put("foo", "bar"));
    VectorLogPtr wal_files;
    ASSERT_OK(dbfull()->GetSortedWalFiles(wal_files));
    ASSERT_EQ(wal_files.size(), 1);
    {
      uint64_t total_ssts_size;
      std::unordered_map<std::string, uint64_t> sst_files;
      ASSERT_OK(GetAllDataFiles(rs::types::FileType::TableFile, &sst_files, &total_ssts_size));
      ASSERT_EQ(total_ssts_size, 0);
    }
    std::string manifest_path =
        DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());

    Close();
    ASSERT_OK(env_->FileExists(manifest_path));
    ASSERT_OK(env_->DeleteFile(manifest_path));
    ASSERT_OK(RepairDB(dbname_, options));

    // make sure that all WALs are converted to SSTables.
    options.wal_dir = "";

    ReopenWithSstIdVerify();
    ASSERT_OK(dbfull()->GetSortedWalFiles(wal_files));
    ASSERT_EQ(wal_files.size(), 0);
    {
      uint64_t total_ssts_size;
      std::unordered_map<std::string, uint64_t> sst_files;
      ASSERT_OK(GetAllDataFiles(rs::types::FileType::TableFile, &sst_files, &total_ssts_size));
      ASSERT_GT(total_ssts_size, 0);
    }
    ASSERT_EQ(Get("key"), "val");
    ASSERT_EQ(Get("foo"), "bar");

  } while (ChangeWalOptions());
}

TEST_F(RepairTest, RepairMultipleColumnFamilies) {
  // Verify repair logic associates SST files with their original column
  // families.
  const int kNumCfs = 3;
  const int kEntriesPerCf = 2;
  DestroyAndReopen(CurrentOptions());
  CreateAndReopenWithCF({"pikachu1", "pikachu2"}, CurrentOptions());
  for (int i = 0; i < kNumCfs; ++i) {
    for (int j = 0; j < kEntriesPerCf; ++j) {
      ASSERT_OK(Put(i, "key" + std::to_string(j), "val" + std::to_string(j)));
      if (j == kEntriesPerCf - 1 && i == kNumCfs - 1) {
        // Leave one unflushed so we can verify WAL entries are properly
        // associated with column families.
        continue;
      }
      ASSERT_OK(Flush(i));
    }
  }

  // Need to get path before Close() deletes db_, but delete it after Close() to
  // ensure Close() doesn't re-create the manifest.
  std::string manifest_path =
      DescriptorFileName(dbname_, dbfull()->TEST_Current_Manifest_FileNo());
  Close();
  ASSERT_OK(env_->FileExists(manifest_path));
  ASSERT_OK(env_->DeleteFile(manifest_path));

  ASSERT_OK(RepairDB(dbname_, CurrentOptions()));

  ReopenWithColumnFamilies({"default", "pikachu1", "pikachu2"},
                           CurrentOptions());
  for (int i = 0; i < kNumCfs; ++i) {
    for (int j = 0; j < kEntriesPerCf; ++j) {
      ASSERT_EQ(Get(i, "key" + std::to_string(j)), "val" + std::to_string(j));
    }
  }
}

TEST_F(RepairTest, RepairColumnFamilyOptions) {
  // Verify repair logic uses correct ColumnFamilyOptions when repairing a
  // database with different options for column families.
  const int kNumCfs = 2;
  const int kEntriesPerCf = 2;

  Options opts(CurrentOptions()), rev_opts(CurrentOptions());
  opts.comparator = BytewiseComparator();
  rev_opts.comparator = ReverseBytewiseComparator();

  DestroyAndReopen(opts);
  CreateColumnFamilies({"reverse"}, rev_opts);
  ReopenWithColumnFamilies({"default", "reverse"},
                           std::vector<Options>{opts, rev_opts});
  for (int i = 0; i < kNumCfs; ++i) {
    for (int j = 0; j < kEntriesPerCf; ++j) {
      ASSERT_OK(Put(i, "key" + std::to_string(j), "val" + std::to_string(j)));
      if (i == kNumCfs - 1 && j == kEntriesPerCf - 1) {
        // Leave one unflushed so we can verify RepairDB's flush logic
        continue;
      }
      ASSERT_OK(Flush(i));
    }
  }
  Close();

  // RepairDB() records the comparator in the manifest, and DB::Open would fail
  // if a different comparator were used.
  ASSERT_OK(RepairDB(dbname_, opts, {{"default", opts}, {"reverse", rev_opts}},
                     opts /* unknown_cf_opts */));
  ASSERT_OK(TryReopenWithColumnFamilies({"default", "reverse"},
                                        std::vector<Options>{opts, rev_opts}));
  for (int i = 0; i < kNumCfs; ++i) {
    for (int j = 0; j < kEntriesPerCf; ++j) {
      ASSERT_EQ(Get(i, "key" + std::to_string(j)), "val" + std::to_string(j));
    }
  }

  // Examine table properties to verify RepairDB() used the right options when
  // converting WAL->SST
  TablePropertiesCollection fname_to_props;
  ASSERT_OK(db_->GetPropertiesOfAllTables(handles_[1], &fname_to_props));
  ASSERT_EQ(fname_to_props.size(), 2U);
  for (const auto& fname_and_props : fname_to_props) {
    std::string comparator_name(rev_opts.comparator->Name());
    ASSERT_EQ(comparator_name, fname_and_props.second->comparator_name);
  }
  Close();

  // Also check comparator when it's provided via "unknown" CF options
  ASSERT_OK(RepairDB(dbname_, opts, {{"default", opts}},
                     rev_opts /* unknown_cf_opts */));
  ASSERT_OK(TryReopenWithColumnFamilies({"default", "reverse"},
                                        std::vector<Options>{opts, rev_opts}));
  for (int i = 0; i < kNumCfs; ++i) {
    for (int j = 0; j < kEntriesPerCf; ++j) {
      ASSERT_EQ(Get(i, "key" + std::to_string(j)), "val" + std::to_string(j));
    }
  }
}

TEST_F(RepairTest, DbNameContainsTrailingSlash) {
  {
    bool tmp;
    if (env_->AreFilesSame("", "", &tmp).IsNotSupported()) {
      fprintf(stderr,
              "skipping RepairTest.DbNameContainsTrailingSlash due to "
              "unsupported Env::AreFilesSame\n");
      return;
    }
  }

  ASSERT_OK(Put("key", "val"));
  ASSERT_OK(Flush());
  Close();

  ASSERT_OK(RepairDB(dbname_ + "/", CurrentOptions()));
  ReopenWithSstIdVerify();
  ASSERT_EQ(Get("key"), "val");
}
}  // namespace rocksdb

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

