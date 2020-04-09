#include "rt.h"
#include <sys/mman.h>
#include "android_binder.h"

typedef struct binder_write_read binder_write_read;
typedef struct binder_version binder_version;
typedef struct flat_binder_object flat_binder_object;
typedef struct binder_ptr_cookie binder_ptr_cookie;
typedef struct binder_state binder_state;
typedef struct binder_handle_cookie binder_handle_cookie;

enum { MAX_BIO_SIZE = 1 << 30 };

static void bio_init_from_txn(binder_io* io, binder_transaction_data* txn);

static void hexdump(void* _data, size_t len) {
    char text[128] = {};
    #define text_append(format, ...) snprintf(text + strlen(text), countof(text) - strlen(text), format, __VA_ARGS__)
    unsigned char* data = _data;
    size_t count;
    for (count = 0; count < len; count++) {
        if ((count & 15) == 0)
            text_append("%04zu:", count);
        text_append(" %02x %c", *data,
                (*data < 32) || (*data > 126) ? '.' : *data);
        data++;
        if ((count & 15) == 15) {
            traceln("%s", text); text[0] = 0;
        }
    }
    if ((count & 15) != 15) {
        traceln("%s", text);
    }
}

static void binder_dump_txn(binder_transaction_data* txn) {
    flat_binder_object* obj;
    binder_size_t* offs = (binder_size_t*)(uintptr_t)txn->data.ptr.offsets;
    size_t count = txn->offsets_size / sizeof(binder_size_t);
    traceln("  target %p  cookie %p  code %08x  flags %08x",
            (uint64_t)txn->target.ptr, (uint64_t)txn->cookie, txn->code, txn->flags);
    traceln("  pid %8d  uid %8d  data %p  offs %p",
            txn->sender_pid, txn->sender_euid, (uint64_t)txn->data_size, (uint64_t)txn->offsets_size);
    hexdump((void* )(uintptr_t)txn->data.ptr.buffer, txn->data_size);
    while (count--) {
        obj = (flat_binder_object*)(((char*)(uintptr_t)txn->data.ptr.buffer) + *offs++);
        traceln("  - type %08x  flags %08x  ptr %p  cookie %p",
                obj->type, obj->flags, (uint64_t)obj->binder, (uint64_t)obj->cookie);
    }
}

#define NAME(n) case n: return #n

static const char* cmd_name(uint32_t cmd) {
    switch(cmd) {
        NAME(BR_NOOP);
        NAME(BR_TRANSACTION_COMPLETE);
        NAME(BR_INCREFS);
        NAME(BR_ACQUIRE);
        NAME(BR_RELEASE);
        NAME(BR_DECREFS);
        NAME(BR_TRANSACTION);
        NAME(BR_REPLY);
        NAME(BR_FAILED_REPLY);
        NAME(BR_DEAD_REPLY);
        NAME(BR_DEAD_BINDER);
        default: return "???";
    }
}

enum {
    BIO_F_SHARED   = 0x01,  // needs to be buffer freed
    BIO_F_OVERFLOW = 0x02,  // ran out of space
    BIO_F_IOERROR  = 0x04,
    BIO_F_MALLOCED = 0x08   // needs to be free()'d
};

struct binder_state {
    int fd;
    void* data;
    size_t bytes;
};

binder_state* binder_open(size_t map_size) {
    binder_version vers = {};
    binder_state* bs = allocate(sizeof(*bs));
    int r = 0;
    if (bs == null) {
        r = ENOMEM;
    } else {
        bs->fd = open("/dev/binder", O_RDWR);
        if (bs->fd < 0) {
            r = errno;
            traceln("binder: cannot open device (%s)", strerror(r));
        } else {
            if ((ioctl(bs->fd, BINDER_VERSION, &vers) == -1) ||
                (vers.protocol_version != BINDER_CURRENT_PROTOCOL_VERSION)) {
                traceln("binder: warning driver version %d differs from user space %d", vers.protocol_version, BINDER_CURRENT_PROTOCOL_VERSION);
            } else {
                bs->bytes = map_size;
                bs->data = mmap(null, map_size, PROT_READ, MAP_PRIVATE, bs->fd, 0);
                if (bs->data == MAP_FAILED) {
                    r = errno;
                    traceln("binder: cannot map device (%s)", strerror(r));
                }
            }
        }
    }
    if (r != 0) {
        if (bs != null && bs->fd >= 0) { close(bs->fd); }
        deallocate(bs); bs = null;
    }
    return null;
}

void binder_close(binder_state* bs) {
    munmap(bs->data, bs->bytes);
    close(bs->fd);
    deallocate(bs);
}

int binder_become_context_manager(binder_state* bs) {
    return ioctl(bs->fd, BINDER_SET_CONTEXT_MGR, 0);
}

int binder_write(binder_state* bs, void* data, size_t len) {
    binder_write_read bwr = {};
    bwr.write_size = len;
    bwr.write_consumed = 0;
    bwr.write_buffer = (uintptr_t) data;
    bwr.read_size = 0;
    bwr.read_consumed = 0;
    bwr.read_buffer = 0;
    int res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);
    if (res < 0) {
        traceln("binder_write: ioctl failed (%s)",
                strerror(errno));
    }
    return res;
}

void binder_send_reply(binder_state* bs, binder_io* reply, binder_uintptr_t buffer_to_free, int status) {
    struct {
        uint32_t cmd_free;
        binder_uintptr_t buffer;
        uint32_t cmd_reply;
        binder_transaction_data txn;
    } __attribute__((packed)) data = {};
    data.cmd_free = BC_FREE_BUFFER;
    data.buffer = buffer_to_free;
    data.cmd_reply = BC_REPLY;
    data.txn.target.ptr = 0;
    data.txn.cookie = 0;
    data.txn.code = 0;
    data.txn.sender_pid  = getpid();
    data.txn.sender_euid = getuid();
    if (status) {
        data.txn.flags = TF_STATUS_CODE;
        data.txn.data_size = sizeof(int);
        data.txn.offsets_size = 0;
        data.txn.data.ptr.buffer = (uintptr_t)&status;
        data.txn.data.ptr.offsets = 0;
    } else {
        data.txn.flags = 0;
        data.txn.data_size = reply->data - reply->data0;
        data.txn.offsets_size = ((char*)reply->offs) - ((char*)reply->offs0);
        data.txn.data.ptr.buffer = (uintptr_t)reply->data0;
        data.txn.data.ptr.offsets = (uintptr_t)reply->offs0;
    }
    binder_write(bs, &data, sizeof(data));
}

int binder_parse(binder_state* bs, binder_io* bio, uintptr_t p, size_t size, binder_handler func) {
    int r = 1;
    uintptr_t e = p + (uintptr_t)size;
    while (p < e) {
        uint32_t cmd = *(uint32_t*)p;
        p += sizeof(uint32_t);
        traceln("%s:", cmd_name(cmd));
        switch (cmd) {
            case BR_NOOP:
                break;
            case BR_TRANSACTION_COMPLETE:
                break;
            case BR_INCREFS:
            case BR_ACQUIRE:
            case BR_RELEASE:
            case BR_DECREFS:
                traceln("  %p, %p", (void*)p, (void*)(p + sizeof(void* )));
                p += sizeof(binder_ptr_cookie);
                break;
            case BR_TRANSACTION: {
                binder_transaction_data* txn = (binder_transaction_data*)p;
                if (e - p < sizeof(*txn)) {
                    traceln("parse: txn too small!");
                    return -1;
                }
                binder_dump_txn(txn);
                if (func != null) {
                    unsigned rdata[256/4];
                    binder_io msg = {};
                    binder_io reply = {};
                    bio_init(&reply, rdata, sizeof(rdata), 4);
                    bio_init_from_txn(&msg, txn);
                    int res = func(bs, txn, &msg, &reply);
                    binder_send_reply(bs, &reply, txn->data.ptr.buffer, res);
                }
                p += sizeof(*txn);
                break;
            }
            case BR_REPLY: {
                binder_transaction_data* txn = (binder_transaction_data*)p;
                if (e - p < sizeof(*txn)) {
                    traceln("parse: reply too small!");
                    return -1;
                }
                binder_dump_txn(txn);
                if (bio) {
                    bio_init_from_txn(bio, txn);
                    bio = 0;
                } else {
                    /* todo FREE BUFFER */
                }
                p += sizeof(*txn);
                r = 0;
                break;
            }
            case BR_DEAD_BINDER: {
                binder_death* death = (binder_death*)(uintptr_t)*(binder_uintptr_t*)p;
                p += sizeof(binder_uintptr_t);
                death->func(bs, death->ptr);
                break;
            }
            case BR_FAILED_REPLY:
                r = -1;
                break;
            case BR_DEAD_REPLY:
                r = -1;
                break;
            default:
                traceln("parse: OOPS %d", cmd);
                r = -1;
                break;
        }
    }
    return r;
}

void binder_acquire(binder_state* bs, uint32_t target) {
    uint32_t cmd[2];
    cmd[0] = BC_ACQUIRE;
    cmd[1] = target;
    binder_write(bs, cmd, sizeof(cmd));
}

void binder_release(binder_state* bs, uint32_t target) {
    uint32_t cmd[2];
    cmd[0] = BC_RELEASE;
    cmd[1] = target;
    binder_write(bs, cmd, sizeof(cmd));
}

void binder_link_to_death(binder_state* bs, uint32_t target, binder_death* death) {
    struct {
        uint32_t cmd;
        binder_handle_cookie payload;
    } __attribute__((packed)) data;
    data.cmd = BC_REQUEST_DEATH_NOTIFICATION;
    data.payload.handle = target;
    data.payload.cookie = (uintptr_t) death;
    binder_write(bs, &data, sizeof(data));
}

int binder_call(binder_state* bs, binder_io* msg, binder_io* reply, uint32_t target, uint32_t code) {
    binder_write_read bwr = {};
    struct {
        uint32_t cmd;
        binder_transaction_data txn;
    } __attribute__((packed)) writebuf = {};
    unsigned readbuf[32] = {};
    if (msg->flags & BIO_F_OVERFLOW) {
        traceln("binder: txn buffer overflow");
    } else {
        writebuf.cmd = BC_TRANSACTION;
        writebuf.txn.target.handle = target;
        writebuf.txn.code = code;
        writebuf.txn.flags = 0;
        writebuf.txn.sender_pid  = getpid();
        writebuf.txn.sender_euid = getuid();
        writebuf.txn.data_size = msg->data - msg->data0;
        writebuf.txn.offsets_size = ((char*)msg->offs) - ((char*)msg->offs0);
        writebuf.txn.data.ptr.buffer = (uintptr_t)msg->data0;
        writebuf.txn.data.ptr.offsets = (uintptr_t)msg->offs0;
        bwr.write_size = sizeof(writebuf);
        bwr.write_consumed = 0;
        bwr.write_buffer = (uintptr_t)&writebuf;
        hexdump(msg->data0, msg->data - msg->data0);
        for (;;) {
            bwr.read_size = sizeof(readbuf);
            bwr.read_consumed = 0;
            bwr.read_buffer = (uintptr_t) readbuf;
            int res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);
            if (res < 0) {
                traceln("binder: ioctl failed (%s)", strerror(errno));
                break;
            }
            res = binder_parse(bs, reply, (uintptr_t) readbuf, bwr.read_consumed, 0);
            if (res == 0) { return 0; }
            if (res < 0) { break; }
        }
    }
    memset(reply, 0, sizeof(*reply));
    reply->flags |= BIO_F_IOERROR;
    return -1;
}

void binder_loop(binder_state* bs, binder_handler func) {
    binder_write_read bwr = {};
    uint32_t readbuf[32] = {};
    bwr.write_size = 0;
    bwr.write_consumed = 0;
    bwr.write_buffer = 0;
    readbuf[0] = BC_ENTER_LOOPER;
    binder_write(bs, readbuf, sizeof(uint32_t));
    for (;;) {
        bwr.read_size = sizeof(readbuf);
        bwr.read_consumed = 0;
        bwr.read_buffer = (uintptr_t) readbuf;
        int res = ioctl(bs->fd, BINDER_WRITE_READ, &bwr);
        if (res < 0) {
            traceln("binder_loop: ioctl failed (%s)", strerror(errno));
            break;
        }
        res = binder_parse(bs, 0, (uintptr_t) readbuf, bwr.read_consumed, func);
        if (res == 0) {
            traceln("binder_loop: unexpected reply?!");
            break;
        }
        if (res < 0) {
            traceln("binder_loop: io error %d %s", res, strerror(errno));
            break;
        }
    }
}

void bio_init_from_txn(binder_io* bio, binder_transaction_data* txn) {
    bio->data = bio->data0 = (char* )(intptr_t)txn->data.ptr.buffer;
    bio->offs = bio->offs0 = (binder_size_t*)(intptr_t)txn->data.ptr.offsets;
    bio->data_avail = txn->data_size;
    bio->offs_avail = txn->offsets_size / sizeof(size_t);
    bio->flags = BIO_F_SHARED;
}

void bio_init(binder_io* bio, void* data, size_t maxdata, size_t maxoffs) {
    size_t n = maxoffs * sizeof(size_t);
    if (n > maxdata) {
        bio->flags = BIO_F_OVERFLOW;
        bio->data_avail = 0;
        bio->offs_avail = 0;
    } else {
        bio->data = bio->data0 = (char*)data + n;
        bio->offs = bio->offs0 = data;
        bio->data_avail = maxdata - n;
        bio->offs_avail = maxoffs;
        bio->flags = 0;
    }
}

static void* bio_alloc(binder_io* bio, size_t size) {
    size = (size + 3) & (~3);
    void* p = null;
    if (size > bio->data_avail) {
        bio->flags |= BIO_F_OVERFLOW;
    } else {
        p = bio->data;
        bio->data += size;
        bio->data_avail -= size;
    }
    return p;
}

void binder_done(binder_state* bs, binder_io* msg, binder_io* reply) {
    struct {
        uint32_t cmd;
        uintptr_t buffer;
    } __attribute__((packed)) data;
    if (reply->flags & BIO_F_SHARED) {
        data.cmd = BC_FREE_BUFFER;
        data.buffer = (uintptr_t) reply->data0;
        binder_write(bs, &data, sizeof(data));
        reply->flags = 0;
    }
}

static flat_binder_object* bio_alloc_obj(binder_io* bio) {
    flat_binder_object* obj = bio_alloc(bio, sizeof(*obj));
    if (obj != null && bio->offs_avail > 0) {
        bio->offs_avail--;
        *bio->offs++ = ((char*)obj) - ((char*)bio->data0);
    } else {
        bio->flags |= BIO_F_OVERFLOW;
        obj = null;
    }
    return obj;
}

void bio_put_uint32(binder_io* bio, uint32_t v) {
    uint32_t* p = bio_alloc(bio, sizeof(v));
    if (p != null) { *p = v; }
}

void bio_put_uint64(binder_io* bio, uint64_t v) {
    uint64_t* p = bio_alloc(bio, sizeof(v));
    if (p != null) { *p = v; }
}

void bio_put_obj(binder_io* bio, void* p) {
    flat_binder_object* obj = bio_alloc_obj(bio);
    if (obj != null) {
        obj->flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
        obj->type = BINDER_TYPE_BINDER;
        obj->binder = (uintptr_t)p;
        obj->cookie = 0;
    }
}

void bio_put_ref(binder_io* bio, uint32_t handle) {
    flat_binder_object* obj;
    if (handle)
        obj = bio_alloc_obj(bio);
    else
        obj = bio_alloc(bio, sizeof(*obj));
    if (obj == null) {
        return;
    }
    obj->flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
    obj->type = BINDER_TYPE_HANDLE;
    obj->handle = handle;
    obj->cookie = 0;
}

void bio_put_string16(binder_io* bio, const uint16_t* str) {
    if (str == null) {
        bio_put_uint32(bio, 0xffffffff);
    } else {
        size_t len = 0;
        while (str[len]) len++;
        if (len >= (MAX_BIO_SIZE / sizeof(uint16_t))) {
            bio_put_uint32(bio, 0xffffffff);
            return;
        }
        /* Note: The payload will carry 32bit size instead of size_t */
        bio_put_uint32(bio, (uint32_t) len);
        len = (len + 1) * sizeof(uint16_t);
        uint16_t* ptr = bio_alloc(bio, len);
        if (ptr != null) {
            memcpy(ptr, str, len);
        }
    }
}

void bio_put_string16_x(binder_io* bio, const char* s) {
    if (s == null) {
        bio_put_uint32(bio, 0xffffffff);
    } else {
        size_t len = strlen(s);
        if (len >= (MAX_BIO_SIZE / sizeof(uint16_t))) {
            bio_put_uint32(bio, 0xffffffff);
        } else {
            /* Note: The payload will carry 32bit size instead of size_t */
            bio_put_uint32(bio, len);
            uint16_t* p = bio_alloc(bio, (len + 1) * sizeof(uint16_t));
            if (p != null) {
                while (*s) { *p++ = (byte)*s++; }
                *p++ = 0;
            }
        }
    }
}

static void* bio_get(binder_io* bio, size_t size) {
    size = (size + 3) & (~3);
    if (bio->data_avail < size){
        bio->data_avail = 0;
        bio->flags |= BIO_F_OVERFLOW;
        return null;
    }  else {
        void* p = bio->data;
        bio->data += size;
        bio->data_avail -= size;
        return p;
    }
}

uint32_t bio_get_uint32(binder_io* bio) {
    uint32_t* p = bio_get(bio, sizeof(*p));
    return p ? *p : 0;
}

uint16_t* bio_get_string16(binder_io* bio, size_t* sz) {
    /* Note: The payload will carry 32bit size instead of size_t */
    size_t len = (size_t)bio_get_uint32(bio);
    if (sz != null) {
        *sz = len;
    }
    return bio_get(bio, (len + 1) * sizeof(uint16_t));
}

static flat_binder_object* _bio_get_obj(binder_io* bio) {
    size_t n;
    size_t off = bio->data - bio->data0;
    /* TODO: be smarter about this? */
    for (n = 0; n < bio->offs_avail; n++) {
        if (bio->offs[n] == off) {
            return bio_get(bio, sizeof(flat_binder_object));
        }
    }
    bio->data_avail = 0;
    bio->flags |= BIO_F_OVERFLOW;
    return null;
}

uint32_t bio_get_ref(binder_io* bio) {
    flat_binder_object* obj = _bio_get_obj(bio);
    if (obj == null) {
        return 0;
    }
    if (obj->type == BINDER_TYPE_HANDLE) { return obj->handle; }
    return 0;
}
