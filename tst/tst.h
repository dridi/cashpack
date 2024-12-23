/*-
 * License: BSD-2-Clause
 * (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#define WRT(buf, len) fwrite(buf, len, 1, stdout)

#define OUT(...) fprintf(stdout, __VA_ARGS__)

#define ERR(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, __VA_ARGS__)

struct dec_ctx;
typedef int tst_decode_f(struct dec_ctx *, const void *, size_t, unsigned);

struct dec_ctx {
	void		*priv;
	tst_decode_f	*dec;
	tst_decode_f	*skp;
	tst_decode_f	*rsz;
	const char	*spec;
	const void	*blk;
	size_t		blk_len;
	size_t		acc_len;
};

struct hpack;
extern struct hpack *hp;

void TST_print_table(void);
int  TST_decode(struct dec_ctx *);

enum hpack_result_e TST_translate_error(const char *);

void TST_signal(void);
