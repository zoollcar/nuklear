#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          PAGE ELEMENT 页面元素
 *
 * ===============================================================*/
/* 创建一个页面元素 */
NK_LIB struct nk_page_element*
nk_create_page_element(struct nk_context *ctx)
{
    struct nk_page_element *elem;
    /* TODO: 创建一个页面元素的三种情况 */
    if (ctx->freelist) {
        /* unlink page element from free list */
        /* 在当前freelist中建立新页面元素 */
        elem = ctx->freelist;
        ctx->freelist = elem->next;
    } else if (ctx->use_pool) {
        /* allocate page element from memory pool */
        /* 创建一个新的池，存放新的页面元素 */
        elem = nk_pool_alloc(&ctx->pool);
        NK_ASSERT(elem);
        if (!elem) return 0;
    } else {
        /* 从固定大小内存缓冲区的后面分配新的页面元素 allocate new page element from back of fixed size memory buffer */
        NK_STORAGE const nk_size size = sizeof(struct nk_page_element);
        NK_STORAGE const nk_size align = NK_ALIGNOF(struct nk_page_element);
        elem = (struct nk_page_element*)nk_buffer_alloc(&ctx->memory, NK_BUFFER_BACK, size, align);
        NK_ASSERT(elem);
        if (!elem) return 0;
    }
    nk_zero_struct(*elem);
    elem->next = 0;
    elem->prev = 0;
    return elem;
}
NK_LIB void
nk_link_page_element_into_freelist(struct nk_context *ctx,
    struct nk_page_element *elem)
{
    /* link table into freelist */
    if (!ctx->freelist) {
        ctx->freelist = elem;
    } else {
        elem->next = ctx->freelist;
        ctx->freelist = elem;
    }
}
NK_LIB void
nk_free_page_element(struct nk_context *ctx, struct nk_page_element *elem)
{
    /* we have a pool so just add to free list */
    if (ctx->use_pool) {
        nk_link_page_element_into_freelist(ctx, elem);
        return;
    }
    /* if possible remove last element from back of fixed memory buffer */
    {void *elem_end = (void*)(elem + 1);
    void *buffer_end = (nk_byte*)ctx->memory.memory.ptr + ctx->memory.size;
    if (elem_end == buffer_end)
        ctx->memory.size -= sizeof(struct nk_page_element);
    else nk_link_page_element_into_freelist(ctx, elem);}
}

