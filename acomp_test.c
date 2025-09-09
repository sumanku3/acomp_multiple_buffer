// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/module.h>
#include <crypto/acompress.h>
#include <linux/scatterlist.h>

#define SRC_BUFFER_SIZE (4096 * 4)
#define DST_BUFFER_SIZE (4096 * 4)
#define NUM_BUFFERS 2

static int mem_cmp_and_dump(u8 **pExpected, u8 **pActual, u32 len, u32 loop)
{
    int i = 0;

    for (i = 0; i < loop; i++)
    {
	    if (memcmp(pExpected[i], pActual[i], len/loop) != 0)
	    {
		    return 1;
	    }
    }
    return 0;
}

static int test_init(void)
{
    struct scatterlist src[NUM_BUFFERS], dst[NUM_BUFFERS], dec[NUM_BUFFERS];
    struct crypto_acomp *tfm;
    struct crypto_wait wait;
    struct acomp_req *req;
    u8 *input[NUM_BUFFERS];
    u8 *output[NUM_BUFFERS];
    u8 *decomp[NUM_BUFFERS];
    int i;
    int ret;

    tfm = crypto_alloc_acomp("qat_deflate", 0, 0);
    if (IS_ERR(tfm))
    {
        pr_err("Error allocating zstd tfm\n");
        return PTR_ERR(tfm);
    }

    req = acomp_request_alloc(tfm);
    if (!req)
    {
        pr_err("Error allocating acomp request\n");
        return -ENOMEM;
    }

    for (i = 0; i < NUM_BUFFERS; i++) {
	    input[i] = kmalloc(SRC_BUFFER_SIZE, GFP_KERNEL);
	    if (!input[i])
	    {
		    pr_err("Failed to allocate input buffer\n");
		    ret = -ENOMEM;
		    goto err;
	    }
    }

    for (i = 0; i < SRC_BUFFER_SIZE; i++)
        input[0][i] = i;
    for (i = 0; i < SRC_BUFFER_SIZE; i++)
        input[1][i] = i;

    for (i = 0; i < NUM_BUFFERS; i++) {
	    output[i] = kmalloc(DST_BUFFER_SIZE, GFP_KERNEL);
	    decomp[i] = kmalloc(SRC_BUFFER_SIZE, GFP_KERNEL);
    }
    if (!output[0] || !decomp[0] || !output[1] || !decomp[1])
    {
        pr_err("Failed to allocate output buffer\n");
        ret = -ENOMEM;
        goto err;
    }

    sg_init_table(src, NUM_BUFFERS);
    sg_init_table(dst, NUM_BUFFERS);
    sg_init_table(dec, NUM_BUFFERS);
    for (i = 0; i < NUM_BUFFERS; i++) {
        sg_set_buf(&src[i], input[i], SRC_BUFFER_SIZE);
    }
    for (i = 0; i < NUM_BUFFERS; i++) {
        sg_set_buf(&dst[i], output[i], DST_BUFFER_SIZE);
    }
    for (i = 0; i < NUM_BUFFERS; i++) {
        sg_set_buf(&dec[i], decomp[i], SRC_BUFFER_SIZE);
    }

    acomp_request_set_params(req, src, dst, NUM_BUFFERS * SRC_BUFFER_SIZE, NUM_BUFFERS * DST_BUFFER_SIZE);

    acomp_request_set_callback(
        req, CRYPTO_TFM_REQ_MAY_BACKLOG, crypto_req_done, &wait);

    crypto_init_wait(&wait);
    ret = crypto_acomp_compress(req);
    ret = crypto_wait_req(ret, &wait);
    if (ret)
    {
        pr_err("Failed to compress buffer\n");
        goto err;
    }
    else
    {
        pr_info("Compression Successful\n");
    }

    printk("Compression: req->slen = %d\n", req->slen);
    printk("Compression: req->dlen = %d\n", req->dlen);

    acomp_request_set_params(req, dst, dec, req->dlen, req->slen);

    ret = crypto_acomp_decompress(req);
    ret = crypto_wait_req(ret, &wait);
    if (ret)
    {
        pr_err("Failed to decompress buffer\n");
        goto err;
    }
    else
    {
        pr_info("Decompression Successful\n");
    }

    printk("Decompression: req->slen = %d\n", req->slen);
    printk("Decompression: req->dlen = %d\n", req->dlen);

    if (req->dlen != NUM_BUFFERS * SRC_BUFFER_SIZE)
    {
        pr_err("produced from decomp does not match: %d != %d\n",
               req->dlen,
               SRC_BUFFER_SIZE);
        goto err;
    }

    ret = mem_cmp_and_dump(input, decomp, req->dlen, NUM_BUFFERS);
    if (ret)
    {
        pr_err("Mismatch in input and decompressed data!\n");
    }

err:
    for (i = 0; i < NUM_BUFFERS; i++) {
	    kfree(output[i]);
	    kfree(input[i]);
	    kfree(decomp[i]);
    }
    acomp_request_free(req);
    crypto_free_acomp(tfm);

    return ret;
}

static void test_exit(void)
{
    printk("Sample compression-decompression test completed\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel");
MODULE_DESCRIPTION("Acomp Compression-decompression test module");
