#pragma once

/* Disable dynamic allocation (malloc/free) */
#define MPACK_MALLOC 0
#define MPACK_FREE 0

/* Disable node/tree tracking to save RAM */
#define MPACK_READER_TRACKING 0
#define MPACK_WRITER_TRACKING 0
#define MPACK_NODE_TRACKING 0

/* Optional: turn off error strings to shrink flash */
#define MPACK_ERROR_STRINGS 0

