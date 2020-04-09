#pragma once // Copyright 2008 The Android Open Source Project
#include <sys/ioctl.h>
#include <linux/binder.h>

typedef struct binder_state binder_state;
typedef struct binder_io binder_io;
typedef struct binder_death binder_death;
typedef struct binder_transaction_data binder_transaction_data;

struct binder_io {
    char* data;            // pointer to read/write from
    binder_size_t* offs;   // array of offsets
    size_t data_avail;     // bytes available in data buffer
    size_t offs_avail;     // entries available in offsets array
    char* data0;           // start of data buffer
    binder_size_t *offs0;  // start of offsets buffer
    uint32_t flags;
    uint32_t unused;
};

struct binder_death {
    void (*func)(struct binder_state* bs, void* ptr);
    void* ptr;
};

// the `magic' handle:
#define BINDER_SERVICE_MANAGER  0
#define SERVICE_MANAGER_INTERFACE "android.os.IServiceManager"

enum {
    GET_SERVICE_TRANSACTION = 1, // aka IBinder::FIRST_CALL_TRANSACTION,
    CHECK_SERVICE_TRANSACTION,
    ADD_SERVICE_TRANSACTION,
    LIST_SERVICES_TRANSACTION
};

typedef int (*binder_handler)(binder_state* bs, binder_transaction_data* txn,
                              binder_io* msg, binder_io* reply);

binder_state* binder_open(size_t mapsize);
void binder_close(binder_state* bs);

// initiate a blocking binder call - returns zero on success
int binder_call(binder_state* bs, binder_io* msg, binder_io* reply, uint32_t target, uint32_t code);

// release any state associate with the binder_io
// - call once any necessary data has been extracted from the
//   binder_io after binder_call() returns
// - can safely be called even if binder_call() fails
void binder_done(binder_state* bs, binder_io* msg, binder_io* reply);

// manipulate strong references
void binder_acquire(binder_state* bs, uint32_t target);
void binder_release(binder_state* bs, uint32_t target);

void binder_link_to_death(binder_state* bs, uint32_t target, binder_death* death);

void binder_loop(binder_state* bs, binder_handler func);

int binder_become_context_manager(binder_state* bs);

// allocate a binder_io, providing a stack-allocated working
// buffer, size of the working buffer, and how many object
// offset entries to reserve from the buffer
void bio_init(binder_io* bio, void* data, size_t maxdata, size_t maxobjects);

void bio_put_obj(binder_io* bio, void* p);
void bio_put_ref(binder_io* bio, uint32_t handle);
void bio_put_uint32(binder_io* bio, uint32_t v);
void bio_put_uint64(binder_io* bio, uint64_t v);
void bio_put_string16(binder_io* bio, const uint16_t* s);
void bio_put_string16_x(binder_io* bio, const char* s);

uint32_t  bio_get_uint32(binder_io* bio);
uint16_t* bio_get_string16(binder_io* bio, size_t* size);
uint32_t  bio_get_ref(binder_io* bio);
