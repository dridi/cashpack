/*-
 * License: BSD-2-Clause
 * (c) 2016-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

/**********************************************************************
 * Utility Macros
 */

#define HPACK_CTX	struct hpack_ctx *ctx
#define HPACK_FLD	const struct hpack_field *fld

#define HPACK_LIMIT(hp) \
	(((hp)->sz.lim >= 0 ? (size_t)(hp)->sz.lim : (hp)->sz.max))

#define CALL(func, ...)					\
	do {						\
		if ((func)(__VA_ARGS__) != 0)		\
			return (-1);			\
	} while (0)

#define EXPECT(ctx, err, cond)				\
	do {						\
		if (!(cond)) {				\
			(ctx)->res = HPACK_RES_##err;	\
			return (HPACK_RES_##err);	\
		}					\
	} while (0)

#if defined(__has_attribute) && __has_attribute(__fallthrough__)
#  define fallthrough __attribute__((__fallthrough__))
#else
#  define fallthrough do { } while (0) /* fall through */
#endif

#define HPD_FLG_MON	0x01

#define HPT_FLG_STATIC	0x01
#define HPT_FLG_DYNAMIC	0x02

/**********************************************************************
 * Data Structures
 */

enum hpi_prefix_e {
#define HPP(nam, pfx, pat) HPACK_PFX_##nam = pfx,
#include "tbl/hpack_tbl.h"
#undef HPP
};

enum hpi_pattern_e {
#define HPP(nam, pfx, pat) HPACK_PAT_##nam = pat,
#include "tbl/hpack_tbl.h"
#undef HPP
};

enum hpack_step_e {
#define HSTP(nam) HPACK_STP_##nam,
#include "tbl/hpack_tbl.h"
#undef HSTP
};

struct hpt_field {
	const char	*nam;
	const char	*val;
	uint16_t	nam_sz;
	uint16_t	val_sz;
	uint16_t	idx;
};

struct hpt_entry {
	uint32_t	magic;
#define HPT_ENTRY_MAGIC	0xe4582b39
	uint32_t	align; /* fill a hole on 64-bit systems */
	uint64_t	pre_sz;
	uint16_t	nam_sz;
	uint16_t	val_sz;
	uint16_t	pad[5];
	/* NB: The last two bytes are never written nor read. They are here
	 * only to guarantee that this struct size is exactly 32 bytes, the
	 * per-entry overhead defined in RFC 7541 section 4.1.
	 */
	uint16_t	unused;
};

#define HPACK_CTX_CAN_UPD (unsigned)1
#define HPACK_CTX_TOO_BIG (unsigned)2

struct hpack_ctx {
	struct hpack				*hp;
	union {
		const struct hpack_decoding	*dec;
		const struct hpack_encoding	*enc;
	} arg;
	union {
		const uint8_t			*blk;
		uint8_t				*cur;
	} ptr;
	size_t					ptr_len;
	char					*buf;
	size_t					buf_len;
	struct {
		const char			*nam;
		const char			*val;
		size_t				nam_sz;
		size_t				val_sz;
	} fld;
	hpack_event_f				*cb;
	void					*priv;
	enum hpack_result_e			res;
	unsigned				flg;
};

struct hpack_size {
	/* NB: mem is the table size currently allocated. It may get out of
	 * sync with the maximum size in some cases. Like when the realloc
	 * function is omitted.
	 */
	size_t			mem;
	/* NB: max represents the maximum table size defined by the decoder,
	 * conveyed out of band with for example HTTP/2 settings. The lim
	 * field represents the soft limit chosen by the encoder and it must
	 * not exceed the maximum.
	 *
	 * See RFC 7541 section 4.2. for the details.
	 */
	size_t			max;
	ssize_t			lim;
	/* NB: when the table limit is capped to a new value, it is stored in
	 * the cap field to be applied when the next header list is encoded.
	 */
	ssize_t			cap;
	/* NB: len is the current length of the dynamic table. */
	size_t			len;
	/* NB: When the size is updated out of band by the decoder, it must be
	 * signalled by the encoder in an HPACK block. However, this change is
	 * deferred until the encoder acknowledges the change happening out of
	 * band. The decoder may also resize the table more than once in which
	 * case we keep track of the last (nxt) change and the smallest (min)
	 * one.
	 *
	 * A negative value is used when no update-after-resize is expected.
	 *
	 * See RFC 7541 section 4.2. for the details.
	 */
	ssize_t			nxt;
	ssize_t			min;
};

struct hpack_int_state {
	uint16_t	v;
	uint8_t		m;
	uint8_t		l;
};

struct hpack_str_state {
	const struct hph_dec	*dec;
	const struct hph_oct	*oct;
	uint16_t		len;
	uint16_t		bits;
	uint8_t			blen;
};

struct hpack_state {
	uint32_t				magic;
#define INT_STATE_MAGIC				0x494E5453
#define STR_STATE_MAGIC				0x53545253
#define HUF_STATE_MAGIC				0x48554653
	enum hpack_step_e			stp;
	int					bsy;
	uint16_t				idx;
	uint8_t					typ;
	union {
		struct hpack_int_state		hpi;
		struct hpack_str_state		str;
	} stt;
};

struct hpack {
	uint32_t		magic;
#define ENCODER_MAGIC		0x8ab1fb4c
#define DECODER_MAGIC		0xab0e3218
#define DEFUNCT_MAGIC		0xdffadae9
	uint32_t		flg;
	struct hpack_alloc	alloc;
	struct hpack_size	sz;
	struct hpack_state	state;
	size_t			cnt; /* number of entries in the table */
	struct hpack_ctx	ctx;
	struct hpt_entry	tbl[];
};

typedef int hpack_validate_f(HPACK_CTX, const char *, size_t);

/**********************************************************************
 * Function Signatures
 */

void HPC_notify(HPACK_CTX, enum hpack_event_e, const void *, size_t);

int  HPD_putc(HPACK_CTX, char);
int  HPD_puts(HPACK_CTX, const char *, size_t);
int  HPD_cat(HPACK_CTX, const char *, size_t);
void HPD_notify(HPACK_CTX);

void HPE_putb(HPACK_CTX, uint8_t);
void HPE_bcat(HPACK_CTX, const void *, size_t);
void HPE_send(HPACK_CTX);

int  HPI_decode(HPACK_CTX, enum hpi_prefix_e, uint16_t *);
void HPI_encode(HPACK_CTX, enum hpi_prefix_e, enum hpi_pattern_e, uint16_t);

int    HPH_decode(HPACK_CTX, size_t);
void   HPH_encode(HPACK_CTX, const char *);
size_t HPH_size(const char *);

hpack_validate_f HPV_token;
hpack_validate_f HPV_value;

void HPT_adjust(HPACK_CTX, size_t);
int  HPT_field(HPACK_CTX, size_t, struct hpt_field *);
void HPT_foreach(HPACK_CTX, int);
int  HPT_search(HPACK_CTX, struct hpt_field *);
int  HPT_decode(HPACK_CTX, size_t);
int  HPT_decode_name(HPACK_CTX);
void HPT_index(HPACK_CTX);
