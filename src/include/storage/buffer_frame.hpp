//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/buffer_frame.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "storage/buffer_manager.hpp"

namespace duckdb {

class BufferFrame {
	friend class BufferManager;

public:
	BufferFrame();
	~BufferFrame() {
	}

	// A method giving access to the buffered page
	void *GetData();
	void lockFrame(bool write);
	bool tryLockFrame(bool write);
	void unlockFrame();
	bool isClient();
	// The id of the page held in the frame.
	size_t pageId;
	// Whether the page is dirty (true) or not (false)
	bool isDirty;
	// Whether the page is fixed in the frame and thus cannot be replaced.
	bool pageFixed;

private:
	// Pointer to the page, which is of known size.
	void *data;
};
} // namespace duckdb
