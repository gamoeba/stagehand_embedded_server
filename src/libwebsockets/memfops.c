/*
 * libwebsockets - small server side websockets and web server implementation
 *
 * Original code used in this source file:
 *
 * https://github.com/PerBothner/DomTerm.git @912add15f3d0aec
 *
 * ./lws-term/io.c
 * ./lws-term/junzip.c
 *
 * Copyright (C) 2017  Per Bothner <per@bothner.com>
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * ( copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * lws rewrite:
 *
 * Copyright (C) 2017  Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#include "private-libwebsockets.h"

typedef struct {
	struct lws_fop_fd	fop_fd; /* MUST BE FIRST logical fop_fd into
	 	 	 	 	 * file inside zip: fops_zip fops */
	lws_filepos_t		start;

} *lws_fops_mem_t;
extern unsigned char* stagehand_zip; 
extern unsigned int stagehand_zip_len;

struct lws_plat_file_ops fops_mem;
#define fop_fd_to_priv(FD) ((lws_fops_mem_t)(FD))


static lws_fop_fd_t
lws_fops_mem_open(const struct lws_plat_file_ops *fops, const char *vfs_path,
		  const char *vpath, lws_fop_flags_t *flags)
{
	lws_fops_mem_t priv;

	priv = lws_zalloc(sizeof(*priv), "fops_zip priv");
	if (!priv)
		return NULL;

	priv->fop_fd.fops = &fops_mem;
    priv->fop_fd.len = stagehand_zip_len;
    priv->start = (lws_filepos_t)&stagehand_zip;
    priv->fop_fd.pos = 0;

	return &priv->fop_fd;
}

/* ie, we are closing the fop_fd for the file inside the gzip */

static int
lws_fops_mem_close(lws_fop_fd_t *fd)
{
	lws_fops_mem_t priv = fop_fd_to_priv(*fd);
	free(priv);
	*fd = NULL;

	return 0;
}

static lws_fileofs_t
lws_fops_mem_seek_cur(lws_fop_fd_t fd, lws_fileofs_t offset_from_cur_pos)
{
	lws_fops_mem_t priv = fop_fd_to_priv(fd);
	priv->fop_fd.pos += offset_from_cur_pos;
	return priv->fop_fd.pos;
}

static int
lws_fops_mem_read(lws_fop_fd_t fd, lws_filepos_t *amount, uint8_t *buf,
		  lws_filepos_t len)
{
	lws_fops_mem_t priv = fop_fd_to_priv(fd);
    int avail = len;
    if ((priv->fop_fd.pos +len) > priv->fop_fd.len)
        avail = priv->fop_fd.len - priv->fop_fd.pos;
    memcpy(buf, (uint8_t*)priv->start + priv->fop_fd.pos, avail);
    priv->fop_fd.pos += avail;
    *amount = avail;

	return 0;
}

struct lws_plat_file_ops fops_mem = {
	lws_fops_mem_open,
	lws_fops_mem_close,
	lws_fops_mem_seek_cur,
	lws_fops_mem_read,
	NULL,
	{ { ".zip/", 5 }, { ".jar/", 5 }, { ".war/", 5 } },
	NULL,
};
