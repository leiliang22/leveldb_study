#ifndef STORAGE_LEVELDB_INCLUDE_EXPORT_H_
#define STORAGE_LEVELDB_INCLUDE_EXPORT_H_

#define LEVELDB_EXPORT
#if 0
#if !defined(LEVELDB_EXPORT)

#if defined(LEVELDB_SHARED_LIBRARY)

#if defined(LEVELDB_COMPILE_LIBRARY)
#define LEVELDB_EXPORT __attribute__((visibility("default")))
#else
#define LEVELDB_EXPORT
#endif

#else  // defined(LEVELDB_SHARED_LIBRARY)
#define LEVELDB_EXPORT
#endif

#endif  // !defined(LEVELDB_EXPORT)

#endif

#endif  // STORAGE_LEVELDB_INCLUDE_EXPORT_H_
