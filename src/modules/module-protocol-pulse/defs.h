/* PipeWire
 *
 * Copyright © 2020 Wim Taymans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#define FLAG_SHMDATA			0x80000000LU
#define FLAG_SHMDATA_MEMFD_BLOCK	0x20000000LU
#define FLAG_SHMRELEASE			0x40000000LU
#define FLAG_SHMREVOKE			0xC0000000LU
#define FLAG_SHMMASK			0xFF000000LU
#define FLAG_SEEKMASK			0x000000FFLU
#define FLAG_SHMWRITABLE		0x00800000LU

#define FRAME_SIZE_MAX_ALLOW (1024*1024*16)

#define PROTOCOL_FLAG_MASK	0xffff0000u
#define PROTOCOL_VERSION_MASK	0x0000ffffu
#define PROTOCOL_VERSION	34

#define NATIVE_COOKIE_LENGTH 256
#define MAX_TAG_SIZE (64*1024)

#define MIN_BUFFERS     8u
#define MAX_BUFFERS     64u

#define MIN_SAMPLES	16u
#define MIN_USEC	(MIN_SAMPLES * SPA_USEC_PER_SEC / 48000u)

#define MAXLENGTH		(4*1024*1024) /* 4MB */
#define DEFAULT_TLENGTH_MSEC	2000 /* 2s */
#define DEFAULT_PROCESS_MSEC	20   /* 20ms */
#define DEFAULT_FRAGSIZE_MSEC	DEFAULT_TLENGTH_MSEC

#define SCACHE_ENTRY_SIZE_MAX	(1024*1024*16)

#define INDEX_MASK		0xffffu
#define MONITOR_FLAG		(1u << 16)
#define EXTENSION_FLAG		(1u << 17)

#define DEFAULT_SINK		"@DEFAULT_SINK@"
#define DEFAULT_SOURCE		"@DEFAULT_SOURCE@"
#define DEFAULT_MONITOR		"@DEFAULT_MONITOR@"

enum error_code {
	ERR_OK = 0,			/**< No error */
	ERR_ACCESS,			/**< Access failure */
	ERR_COMMAND,			/**< Unknown command */
	ERR_INVALID,			/**< Invalid argument */
	ERR_EXIST,			/**< Entity exists */
	ERR_NOENTITY,			/**< No such entity */
	ERR_CONNECTIONREFUSED,		/**< Connection refused */
	ERR_PROTOCOL,			/**< Protocol error */
	ERR_TIMEOUT,			/**< Timeout */
	ERR_AUTHKEY,			/**< No authentication key */
	ERR_INTERNAL,			/**< Internal error */
	ERR_CONNECTIONTERMINATED,	/**< Connection terminated */
	ERR_KILLED,			/**< Entity killed */
	ERR_INVALIDSERVER,		/**< Invalid server */
	ERR_MODINITFAILED,		/**< Module initialization failed */
	ERR_BADSTATE,			/**< Bad state */
	ERR_NODATA,			/**< No data */
	ERR_VERSION,			/**< Incompatible protocol version */
	ERR_TOOLARGE,			/**< Data too large */
	ERR_NOTSUPPORTED,		/**< Operation not supported \since 0.9.5 */
	ERR_UNKNOWN,			/**< The error code was unknown to the client */
	ERR_NOEXTENSION,		/**< Extension does not exist. \since 0.9.12 */
	ERR_OBSOLETE,			/**< Obsolete functionality. \since 0.9.15 */
	ERR_NOTIMPLEMENTED,		/**< Missing implementation. \since 0.9.15 */
	ERR_FORKED,			/**< The caller forked without calling execve() and tried to reuse the context. \since 0.9.15 */
	ERR_IO,				/**< An IO error happened. \since 0.9.16 */
	ERR_BUSY,			/**< Device or resource busy. \since 0.9.17 */
	ERR_MAX				/**< Not really an error but the first invalid error code */
};

static inline int res_to_err(int res)
{
	switch (res) {
	case 0: return ERR_OK;
	case -EACCES: case -EPERM: return ERR_ACCESS;
	case -ENOTTY: return ERR_COMMAND;
	case -EINVAL: return ERR_INVALID;
	case -EEXIST: return ERR_EXIST;
	case -ENOENT: case -ESRCH: case -ENXIO: case -ENODEV: return ERR_NOENTITY;
	case -ECONNREFUSED: case -ENONET: case -EHOSTDOWN: case -ENETDOWN: return ERR_CONNECTIONREFUSED;
	case -EPROTO: case -EBADMSG: return ERR_PROTOCOL;
	case -ETIMEDOUT: case -ETIME: return ERR_TIMEOUT;
#ifdef ENOKEY
	case -ENOKEY: return ERR_AUTHKEY;
#endif
	case -ECONNRESET: case -EPIPE: return ERR_CONNECTIONTERMINATED;
#ifdef EBADFD
	case -EBADFD: return ERR_BADSTATE;
#endif
#ifdef ENODATA
	case -ENODATA: return ERR_NODATA;
#endif
	case -EOVERFLOW: case -E2BIG: case -EFBIG:
	case -ERANGE: case -ENAMETOOLONG: return ERR_TOOLARGE;
	case -ENOTSUP: case -EPROTONOSUPPORT: case -ESOCKTNOSUPPORT: return ERR_NOTSUPPORTED;
	case -ENOSYS: return ERR_NOTIMPLEMENTED;
	case -EIO: return ERR_IO;
	case -EBUSY: return ERR_BUSY;
	}
	return ERR_UNKNOWN;
}

enum {
	/* Generic commands */
	COMMAND_ERROR,
	COMMAND_TIMEOUT, /* pseudo command */
	COMMAND_REPLY,

	/* CLIENT->SERVER */
	COMMAND_CREATE_PLAYBACK_STREAM,        /* Payload changed in v9, v12 (0.9.0, 0.9.8) */
	COMMAND_DELETE_PLAYBACK_STREAM,
	COMMAND_CREATE_RECORD_STREAM,          /* Payload changed in v9, v12 (0.9.0, 0.9.8) */
	COMMAND_DELETE_RECORD_STREAM,
	COMMAND_EXIT,
	COMMAND_AUTH,
	COMMAND_SET_CLIENT_NAME,
	COMMAND_LOOKUP_SINK,
	COMMAND_LOOKUP_SOURCE,
	COMMAND_DRAIN_PLAYBACK_STREAM,
	COMMAND_STAT,
	COMMAND_GET_PLAYBACK_LATENCY,
	COMMAND_CREATE_UPLOAD_STREAM,
	COMMAND_DELETE_UPLOAD_STREAM,
	COMMAND_FINISH_UPLOAD_STREAM,
	COMMAND_PLAY_SAMPLE,
	COMMAND_REMOVE_SAMPLE,

	COMMAND_GET_SERVER_INFO,
	COMMAND_GET_SINK_INFO,
	COMMAND_GET_SINK_INFO_LIST,
	COMMAND_GET_SOURCE_INFO,
	COMMAND_GET_SOURCE_INFO_LIST,
	COMMAND_GET_MODULE_INFO,
	COMMAND_GET_MODULE_INFO_LIST,
	COMMAND_GET_CLIENT_INFO,
	COMMAND_GET_CLIENT_INFO_LIST,
	COMMAND_GET_SINK_INPUT_INFO,          /* Payload changed in v11 (0.9.7) */
	COMMAND_GET_SINK_INPUT_INFO_LIST,     /* Payload changed in v11 (0.9.7) */
	COMMAND_GET_SOURCE_OUTPUT_INFO,
	COMMAND_GET_SOURCE_OUTPUT_INFO_LIST,
	COMMAND_GET_SAMPLE_INFO,
	COMMAND_GET_SAMPLE_INFO_LIST,
	COMMAND_SUBSCRIBE,

	COMMAND_SET_SINK_VOLUME,
	COMMAND_SET_SINK_INPUT_VOLUME,
	COMMAND_SET_SOURCE_VOLUME,

	COMMAND_SET_SINK_MUTE,
	COMMAND_SET_SOURCE_MUTE,

	COMMAND_CORK_PLAYBACK_STREAM,
	COMMAND_FLUSH_PLAYBACK_STREAM,
	COMMAND_TRIGGER_PLAYBACK_STREAM,

	COMMAND_SET_DEFAULT_SINK,
	COMMAND_SET_DEFAULT_SOURCE,

	COMMAND_SET_PLAYBACK_STREAM_NAME,
	COMMAND_SET_RECORD_STREAM_NAME,

	COMMAND_KILL_CLIENT,
	COMMAND_KILL_SINK_INPUT,
	COMMAND_KILL_SOURCE_OUTPUT,

	COMMAND_LOAD_MODULE,
	COMMAND_UNLOAD_MODULE,

	/* Obsolete */
	COMMAND_ADD_AUTOLOAD___OBSOLETE,
	COMMAND_REMOVE_AUTOLOAD___OBSOLETE,
	COMMAND_GET_AUTOLOAD_INFO___OBSOLETE,
	COMMAND_GET_AUTOLOAD_INFO_LIST___OBSOLETE,

	COMMAND_GET_RECORD_LATENCY,
	COMMAND_CORK_RECORD_STREAM,
	COMMAND_FLUSH_RECORD_STREAM,
	COMMAND_PREBUF_PLAYBACK_STREAM,

	/* SERVER->CLIENT */
	COMMAND_REQUEST,
	COMMAND_OVERFLOW,
	COMMAND_UNDERFLOW,
	COMMAND_PLAYBACK_STREAM_KILLED,
	COMMAND_RECORD_STREAM_KILLED,
	COMMAND_SUBSCRIBE_EVENT,

	/* A few more client->server commands */

	/* Supported since protocol v10 (0.9.5) */
	COMMAND_MOVE_SINK_INPUT,
	COMMAND_MOVE_SOURCE_OUTPUT,

	/* Supported since protocol v11 (0.9.7) */
	COMMAND_SET_SINK_INPUT_MUTE,

	COMMAND_SUSPEND_SINK,
	COMMAND_SUSPEND_SOURCE,

	/* Supported since protocol v12 (0.9.8) */
	COMMAND_SET_PLAYBACK_STREAM_BUFFER_ATTR,
	COMMAND_SET_RECORD_STREAM_BUFFER_ATTR,

	COMMAND_UPDATE_PLAYBACK_STREAM_SAMPLE_RATE,
	COMMAND_UPDATE_RECORD_STREAM_SAMPLE_RATE,

	/* SERVER->CLIENT */
	COMMAND_PLAYBACK_STREAM_SUSPENDED,
	COMMAND_RECORD_STREAM_SUSPENDED,
	COMMAND_PLAYBACK_STREAM_MOVED,
	COMMAND_RECORD_STREAM_MOVED,

	/* Supported since protocol v13 (0.9.11) */
	COMMAND_UPDATE_RECORD_STREAM_PROPLIST,
	COMMAND_UPDATE_PLAYBACK_STREAM_PROPLIST,
	COMMAND_UPDATE_CLIENT_PROPLIST,
	COMMAND_REMOVE_RECORD_STREAM_PROPLIST,
	COMMAND_REMOVE_PLAYBACK_STREAM_PROPLIST,
	COMMAND_REMOVE_CLIENT_PROPLIST,

	/* SERVER->CLIENT */
	COMMAND_STARTED,

	/* Supported since protocol v14 (0.9.12) */
	COMMAND_EXTENSION,
	/* Supported since protocol v15 (0.9.15) */
	COMMAND_GET_CARD_INFO,
	COMMAND_GET_CARD_INFO_LIST,
	COMMAND_SET_CARD_PROFILE,

	COMMAND_CLIENT_EVENT,
	COMMAND_PLAYBACK_STREAM_EVENT,
	COMMAND_RECORD_STREAM_EVENT,

	/* SERVER->CLIENT */
	COMMAND_PLAYBACK_BUFFER_ATTR_CHANGED,
	COMMAND_RECORD_BUFFER_ATTR_CHANGED,

	/* Supported since protocol v16 (0.9.16) */
	COMMAND_SET_SINK_PORT,
	COMMAND_SET_SOURCE_PORT,

	/* Supported since protocol v22 (1.0) */
	COMMAND_SET_SOURCE_OUTPUT_VOLUME,
	COMMAND_SET_SOURCE_OUTPUT_MUTE,

	/* Supported since protocol v27 (3.0) */
	COMMAND_SET_PORT_LATENCY_OFFSET,

	/* Supported since protocol v30 (6.0) */
	/* BOTH DIRECTIONS */
	COMMAND_ENABLE_SRBCHANNEL,
	COMMAND_DISABLE_SRBCHANNEL,

	/* Supported since protocol v31 (9.0)
	 * BOTH DIRECTIONS */
	COMMAND_REGISTER_MEMFD_SHMID,

	COMMAND_MAX
};

enum {
	SUBSCRIPTION_MASK_NULL = 0x0000U,
	SUBSCRIPTION_MASK_SINK = 0x0001U,
	SUBSCRIPTION_MASK_SOURCE = 0x0002U,
	SUBSCRIPTION_MASK_SINK_INPUT = 0x0004U,
	SUBSCRIPTION_MASK_SOURCE_OUTPUT = 0x0008U,
	SUBSCRIPTION_MASK_MODULE = 0x0010U,
	SUBSCRIPTION_MASK_CLIENT = 0x0020U,
	SUBSCRIPTION_MASK_SAMPLE_CACHE = 0x0040U,
	SUBSCRIPTION_MASK_SERVER = 0x0080U,
	SUBSCRIPTION_MASK_AUTOLOAD = 0x0100U,
	SUBSCRIPTION_MASK_CARD = 0x0200U,
	SUBSCRIPTION_MASK_ALL = 0x02ffU
};

enum {
	SUBSCRIPTION_EVENT_SINK = 0x0000U,
	SUBSCRIPTION_EVENT_SOURCE = 0x0001U,
	SUBSCRIPTION_EVENT_SINK_INPUT = 0x0002U,
	SUBSCRIPTION_EVENT_SOURCE_OUTPUT = 0x0003U,
	SUBSCRIPTION_EVENT_MODULE = 0x0004U,
	SUBSCRIPTION_EVENT_CLIENT = 0x0005U,
	SUBSCRIPTION_EVENT_SAMPLE_CACHE = 0x0006U,
	SUBSCRIPTION_EVENT_SERVER = 0x0007U,
	SUBSCRIPTION_EVENT_AUTOLOAD = 0x0008U,
	SUBSCRIPTION_EVENT_CARD = 0x0009U,
	SUBSCRIPTION_EVENT_FACILITY_MASK = 0x000FU,

	SUBSCRIPTION_EVENT_NEW = 0x0000U,
	SUBSCRIPTION_EVENT_CHANGE = 0x0010U,
	SUBSCRIPTION_EVENT_REMOVE = 0x0020U,
	SUBSCRIPTION_EVENT_TYPE_MASK = 0x0030U
};

enum {
	STATE_INVALID = -1,
	STATE_RUNNING = 0,
	STATE_IDLE = 1,
	STATE_SUSPENDED = 2,
	STATE_INIT = -2,
	STATE_UNLINKED = -3
};

static inline int node_state(enum pw_node_state state)
{
	switch (state) {
	case PW_NODE_STATE_ERROR:
		return STATE_UNLINKED;
	case PW_NODE_STATE_CREATING:
		return STATE_INIT;
	case PW_NODE_STATE_SUSPENDED:
		return STATE_SUSPENDED;
	case PW_NODE_STATE_IDLE:
		return STATE_IDLE;
	case PW_NODE_STATE_RUNNING:
		return STATE_RUNNING;
	}
	return STATE_INVALID;
}

static inline bool pw_endswith(const char *s, const char *sfx)
{
        size_t l1, l2;
        l1 = strlen(s);
        l2 = strlen(sfx);
        return l1 >= l2 && strcmp(s + l1 - l2, sfx) == 0;
}

enum {
	SINK_HW_VOLUME_CTRL = 0x0001U,
	SINK_LATENCY = 0x0002U,
	SINK_HARDWARE = 0x0004U,
	SINK_NETWORK = 0x0008U,
	SINK_HW_MUTE_CTRL = 0x0010U,
	SINK_DECIBEL_VOLUME = 0x0020U,
	SINK_FLAT_VOLUME = 0x0040U,
	SINK_DYNAMIC_LATENCY = 0x0080U,
	SINK_SET_FORMATS = 0x0100U,
};

enum {
	SOURCE_HW_VOLUME_CTRL = 0x0001U,
	SOURCE_LATENCY = 0x0002U,
	SOURCE_HARDWARE = 0x0004U,
	SOURCE_NETWORK = 0x0008U,
	SOURCE_HW_MUTE_CTRL = 0x0010U,
	SOURCE_DECIBEL_VOLUME = 0x0020U,
	SOURCE_DYNAMIC_LATENCY = 0x0040U,
	SOURCE_FLAT_VOLUME = 0x0080U,
};

static const char *port_types[] = {
	"unknown",
	"aux",
	"speaker",
	"headphones",
	"line",
	"mic",
	"headset",
	"handset",
	"earpiece",
	"spdif",
	"hdmi",
	"tv",
	"radio",
	"video",
	"usb",
	"bluetooth",
	"portable",
	"handsfree",
	"car",
	"hifi",
	"phone",
	"network",
	"analog",
};

static uint32_t port_type_value(const char *port_type)
{
	uint32_t i;
	for (i = 0; i < SPA_N_ELEMENTS(port_types); i++) {
		if (strcmp(port_types[i], port_type) == 0)
			return i;
	}
	return 0;
}

#define METADATA_DEFAULT_SINK           "default.audio.sink"
#define METADATA_DEFAULT_SOURCE         "default.audio.source"
#define METADATA_TARGET_NODE            "target.node"
