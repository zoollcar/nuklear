#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              WINDOW 窗口
 *
 * ===============================================================*/
/* 创建窗口 */
NK_LIB void*
nk_create_window(struct nk_context *ctx)
{
    struct nk_page_element *elem;
    elem = nk_create_page_element(ctx);
    if (!elem) return 0;
    elem->data.win.seq = ctx->seq;
    return &elem->data.win;
}
NK_LIB void
nk_free_window(struct nk_context *ctx, struct nk_window *win)
{
    /* unlink windows from list */
    struct nk_table *it = win->tables;
    if (win->popup.win) {
        nk_free_window(ctx, win->popup.win);
        win->popup.win = 0;
    }
    win->next = 0;
    win->prev = 0;

    while (it) {
        /*free window state tables */
        struct nk_table *n = it->next;
        nk_remove_table(win, it);
        nk_free_table(ctx, it);
        if (it == win->tables)
            win->tables = n;
        it = n;
    }

    /* link windows into freelist */
    {union nk_page_data *pd = NK_CONTAINER_OF(win, union nk_page_data, win);
    struct nk_page_element *pe = NK_CONTAINER_OF(pd, struct nk_page_element, data);
    nk_free_page_element(ctx, pe);}
}
NK_LIB struct nk_window*
nk_find_window(struct nk_context *ctx, nk_hash hash, const char *name)
{
    struct nk_window *iter;
    iter = ctx->begin;
    /* 一个一个的寻找 */
    while (iter) {
        NK_ASSERT(iter != iter->next);
        /* window结构体中的name存储的是哈希值 */
        if (iter->name == hash) {
            /* window中的 name_string 是 name 字符串 */
            int max_len = nk_strlen(iter->name_string);
            /* 去除hash相同而名字不同的情况 */
            if (!nk_stricmpn(iter->name_string, name, max_len))
                return iter;/* 找到了 */
        }
        iter = iter->next;
    }
    return 0;/* 没找到 */
}
NK_LIB void
nk_insert_window(struct nk_context *ctx, struct nk_window *win,
    enum nk_window_insert_location loc)
{
    const struct nk_window *iter;
    NK_ASSERT(ctx);
    NK_ASSERT(win);
    if (!win || !ctx) return;

    /* 从ctx.begin 开头的 链表中找到 win */
    iter = ctx->begin;
    while (iter) {
        NK_ASSERT(iter != iter->next);
        NK_ASSERT(iter != win);
        if (iter == win) return;
        iter = iter->next;
    }
    /* 如果ctx.begin 是空的 */
    if (!ctx->begin) {
        win->next = 0;
        win->prev = 0;
        ctx->begin = win;
        ctx->end = win;
        ctx->count = 1;/* 计数器 */
        return;
    }
    if (loc == NK_INSERT_BACK) {
        /* 如果是插入到最后 */
        struct nk_window *end;
        end = ctx->end;
        end->flags |= NK_WINDOW_ROM;
        end->next = win;
        win->prev = ctx->end;
        win->next = 0;
        ctx->end = win;
        ctx->active = ctx->end;
        ctx->end->flags &= ~(nk_flags)NK_WINDOW_ROM;
    } else {
        /* 插入到开头 */
        /*ctx->end->flags |= NK_WINDOW_ROM;*/
        ctx->begin->prev = win;
        win->next = ctx->begin;
        win->prev = 0;
        ctx->begin = win;
        ctx->begin->flags &= ~(nk_flags)NK_WINDOW_ROM;
    }
    ctx->count++;/* 计数器加一 */
}
NK_LIB void
nk_remove_window(struct nk_context *ctx, struct nk_window *win)
{
    if (win == ctx->begin || win == ctx->end) {
        if (win == ctx->begin) {
            ctx->begin = win->next;
            if (win->next)
                win->next->prev = 0;
        }
        if (win == ctx->end) {
            ctx->end = win->prev;
            if (win->prev)
                win->prev->next = 0;
        }
    } else {
        if (win->next)
            win->next->prev = win->prev;
        if (win->prev)
            win->prev->next = win->next;
    }
    if (win == ctx->active || !ctx->active) {
        ctx->active = ctx->end;
        if (ctx->end)
            ctx->end->flags &= ~(nk_flags)NK_WINDOW_ROM;
    }
    win->next = 0;
    win->prev = 0;
    ctx->count--;
}
/* nk_begin 建立一个新窗口，除非想隐藏它，否则在调用每一帧都要调用   */
/* 其实就是 name 和 title 一样的 nk_begin_titled */
NK_API int
nk_begin(struct nk_context *ctx, const char *title,
    struct nk_rect bounds, nk_flags flags)
{
    return nk_begin_titled(ctx, title, title, bounds, flags);
}
/* 建立一个新窗口，分离的标题和标识符的窗口，允许出现具有相同名称但不同标题的多个窗口  */
/* nk_flags 是标志 追根究底 是一个无符号32位整数 */
NK_API int
nk_begin_titled(struct nk_context *ctx, const char *name, const char *title,
    struct nk_rect bounds, nk_flags flags)
{
    struct nk_window *win;
    struct nk_style *style;
    /* nk_hash 是一个无符号整数 */
    nk_hash title_hash;
    int title_len;
    int ret = 0;
    /* NK_ASSERT 是 NK_ASSERT宏 如果用户没定义 用的是 assert.h 中的定义*/
    /* 作用是 当参数为空时报错，给出错误提示 */
    NK_ASSERT(ctx);
    NK_ASSERT(name);
    NK_ASSERT(title);
    /* 字体检查 */
    NK_ASSERT(ctx->style.font && ctx->style.font->width && "if this triggers you forgot to add a font");
    /* ctx->current 一定要是空的 否则说明上次没调用 nk_end */
    NK_ASSERT(!ctx->current && "if this triggers you missed a `nk_end` call");
    if (!ctx || ctx->current || !title || !name)
        return 0;

    /* 寻找或是创建一个窗口 find or create window */
    style = &ctx->style;
    title_len = (int)nk_strlen(name);
    /* 将窗口名字计算为一个哈希值 */
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    /* 寻找窗口，返回值为0表示没找到，返回值为一个nk_window指针说明找到了 */
    win = nk_find_window(ctx, title_hash, name);
    /* 创建或更新窗口 */
    if (!win) {
        /* 创建一个新窗口 create new window */
        nk_size name_length = (nk_size)nk_strlen(name);
        win = (struct nk_window*)nk_create_window(ctx);
        NK_ASSERT(win);
        if (!win) return 0;
        /* 将窗口添加到 ctx */
        if (flags & NK_WINDOW_BACKGROUND)
            nk_insert_window(ctx, win, NK_INSERT_FRONT);/* 顶部添加 */
        else nk_insert_window(ctx, win, NK_INSERT_BACK);/* 尾部添加 */
        /* 初始化命令缓冲区 */
        nk_command_buffer_init(&win->buffer, &ctx->memory, NK_CLIPPING_ON);
        win->flags = flags;
        win->bounds = bounds;
        win->name = title_hash;
        /* 如果窗口名字比最大长度长，会被截断 */
        name_length = NK_MIN(name_length, NK_WINDOW_MAX_NAME-1);
        NK_MEMCPY(win->name_string, name, name_length);
        win->name_string[name_length] = 0;
        /* 不是弹出式窗口 */
        win->popup.win = 0;
        if (!ctx->active)
            ctx->active = win;
    } else {
        /* 更新窗口 update window */
        win->flags &= ~(nk_flags)(NK_WINDOW_PRIVATE-1);
        win->flags |= flags;
        /* 如果窗口可移动 就移动 */
        if (!(win->flags & (NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)))
            win->bounds = bounds;
        /* 如果这个断言被触发了，您可能：
         *
         * I.) 有一个以上的具有相同 name 的窗口
         * II.) 忘了绘制窗口
         *      具体来说 您没有调用 `nk_clear` 
         *      (如果您使用的是 demo 后端，nk_clear 会自动被调用). 
         * */
        NK_ASSERT(win->seq != ctx->seq);
        win->seq = ctx->seq;
        /* 调整活动的窗口 */
        if (!ctx->active && !(win->flags & NK_WINDOW_HIDDEN)) {
            ctx->active = win;
            ctx->end = win;
        }
    }
    if (win->flags & NK_WINDOW_HIDDEN) {
        /* 如果窗口不显示 */
        ctx->current = win;
        win->layout = 0;
        return 0;
    } else nk_start(ctx, win);/* TODO:显示窗口， 将绘制命令缓冲区加入？？ */

    /* 窗口重叠 window overlapping */
    /* 处理所有没有隐藏或是禁止输入的窗口 处理拖动和点击事件  */
    if (!(win->flags & NK_WINDOW_HIDDEN) && !(win->flags & NK_WINDOW_NO_INPUT))
    {
        int inpanel, ishovered;
        struct nk_window *iter = win;
        /* 这里的h 是除窗口标题栏之外的高 */
        float h = ctx->style.font->height + 2.0f * style->window.header.padding.y +
            (2.0f * style->window.header.label_padding.y);
        struct nk_rect win_bounds = (!(win->flags & NK_WINDOW_MINIMIZED))?
            win->bounds: nk_rect(win->bounds.x, win->bounds.y, win->bounds.w, h);

        /* 如果窗口 hovered 并且没有其他窗口和它重叠则 激活窗口 activate window if hovered and no other window is overlapping this window */
        inpanel = nk_input_has_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_LEFT, win_bounds, nk_true);
        inpanel = inpanel && ctx->input.mouse.buttons[NK_BUTTON_LEFT].clicked;

        /* 下面处理长时间点下窗口 */
        ishovered = nk_input_is_mouse_hovering_rect(&ctx->input, win_bounds);
        if ((win != ctx->active) && ishovered && !ctx->input.mouse.buttons[NK_BUTTON_LEFT].down) {
            /* TODO:为什么改变下一个的坐标？而不是当前这个 */
            iter = win->next;
            while (iter) {
                struct nk_rect iter_bounds = (!(iter->flags & NK_WINDOW_MINIMIZED))?
                    iter->bounds: nk_rect(iter->bounds.x, iter->bounds.y, iter->bounds.w, h);
                if (NK_INTERSECT(win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                    iter_bounds.x, iter_bounds.y, iter_bounds.w, iter_bounds.h) &&
                    (!(iter->flags & NK_WINDOW_HIDDEN)))
                    break;

                if (iter->popup.win && iter->popup.active && !(iter->flags & NK_WINDOW_HIDDEN) &&
                    NK_INTERSECT(win->bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                    iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                    iter->popup.win->bounds.w, iter->popup.win->bounds.h))
                    break;
                iter = iter->next;
            }
        }

        /* 如果窗口被点击则激活窗口 activate window if clicked */
        /* 先找到正确的 iter */
        if (iter && inpanel && (win != ctx->end)) {
            iter = win->next;
            while (iter) {
                /* 尝试在同一位置寻找具有更高优先级的窗口 try to find a panel with higher priority in the same position */
                struct nk_rect iter_bounds = (!(iter->flags & NK_WINDOW_MINIMIZED))?
                iter->bounds: nk_rect(iter->bounds.x, iter->bounds.y, iter->bounds.w, h);
                if (NK_INBOX(ctx->input.mouse.pos.x, ctx->input.mouse.pos.y,
                    iter_bounds.x, iter_bounds.y, iter_bounds.w, iter_bounds.h) &&
                    !(iter->flags & NK_WINDOW_HIDDEN))
                    break;
                if (iter->popup.win && iter->popup.active && !(iter->flags & NK_WINDOW_HIDDEN) &&
                    NK_INTERSECT(win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
                    iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                    iter->popup.win->bounds.w, iter->popup.win->bounds.h))
                    break;
                iter = iter->next;
            }
        }
        /* 然后激活窗口 */
        if (iter && !(win->flags & NK_WINDOW_ROM) && (win->flags & NK_WINDOW_BACKGROUND)) {
            win->flags |= (nk_flags)NK_WINDOW_ROM;
            iter->flags &= ~(nk_flags)NK_WINDOW_ROM;
            /* 设置激活窗口 */
            ctx->active = iter;
            if (!(iter->flags & NK_WINDOW_BACKGROUND)) {
                /* 当前位置的窗口是激活的，所以将它移动到堆栈顶端 current window is active in that position so transfer to top
                 * at the highest priority in stack */
                nk_remove_window(ctx, iter);
                nk_insert_window(ctx, iter, NK_INSERT_BACK);
            }
        } else {
            if (!iter && ctx->end != win) {
                if (!(win->flags & NK_WINDOW_BACKGROUND)) {
                    /* 当前位置的窗口是激活的，所以将它移动到堆栈顶端 current window is active in that position so transfer to top
                     * at the highest priority in stack */
                    nk_remove_window(ctx, win);
                    nk_insert_window(ctx, win, NK_INSERT_BACK);
                }
                win->flags &= ~(nk_flags)NK_WINDOW_ROM;
                ctx->active = win;
            }
            if (ctx->end != win && !(win->flags & NK_WINDOW_BACKGROUND))
                win->flags |= NK_WINDOW_ROM;
        }
    }
    win->layout = (struct nk_panel*)nk_create_panel(ctx);
    ctx->current = win;
    ret = nk_panel_begin(ctx, title, NK_PANEL_WINDOW);
    win->layout->offset_x = &win->scrollbar.x;
    win->layout->offset_y = &win->scrollbar.y;
    return ret;
}
NK_API void
nk_end(struct nk_context *ctx)
{
    struct nk_panel *layout;
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current && "if this triggers you forgot to call `nk_begin`");
    if (!ctx || !ctx->current)
        return;

    layout = ctx->current->layout;
    if (!layout || (layout->type == NK_PANEL_WINDOW && (ctx->current->flags & NK_WINDOW_HIDDEN))) {
        ctx->current = 0;
        return;
    }
    nk_panel_end(ctx);
    nk_free_panel(ctx, ctx->current->layout);
    ctx->current = 0;
}
NK_API struct nk_rect
nk_window_get_bounds(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_rect(0,0,0,0);
    return ctx->current->bounds;
}
NK_API struct nk_vec2
nk_window_get_position(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_vec2(0,0);
    return nk_vec2(ctx->current->bounds.x, ctx->current->bounds.y);
}
NK_API struct nk_vec2
nk_window_get_size(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_vec2(0,0);
    return nk_vec2(ctx->current->bounds.w, ctx->current->bounds.h);
}
NK_API float
nk_window_get_width(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->bounds.w;
}
NK_API float
nk_window_get_height(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->bounds.h;
}
NK_API struct nk_rect
nk_window_get_content_region(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_rect(0,0,0,0);
    return ctx->current->layout->clip;
}
NK_API struct nk_vec2
nk_window_get_content_region_min(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return nk_vec2(0,0);
    return nk_vec2(ctx->current->layout->clip.x, ctx->current->layout->clip.y);
}
NK_API struct nk_vec2
nk_window_get_content_region_max(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return nk_vec2(0,0);
    return nk_vec2(ctx->current->layout->clip.x + ctx->current->layout->clip.w,
        ctx->current->layout->clip.y + ctx->current->layout->clip.h);
}
NK_API struct nk_vec2
nk_window_get_content_region_size(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return nk_vec2(0,0);
    return nk_vec2(ctx->current->layout->clip.w, ctx->current->layout->clip.h);
}
NK_API struct nk_command_buffer*
nk_window_get_canvas(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return 0;
    return &ctx->current->buffer;
}
NK_API struct nk_panel*
nk_window_get_panel(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->layout;
}
NK_API int
nk_window_has_focus(const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return 0;
    return ctx->current == ctx->active;
}
NK_API int
nk_window_is_hovered(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    if(ctx->current->flags & NK_WINDOW_HIDDEN)
        return 0;
    return nk_input_is_mouse_hovering_rect(&ctx->input, ctx->current->bounds);
}
NK_API int
nk_window_is_any_hovered(struct nk_context *ctx)
{
    struct nk_window *iter;
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    iter = ctx->begin;
    while (iter) {
        /* check if window is being hovered */
        if(!(iter->flags & NK_WINDOW_HIDDEN)) {
            /* check if window popup is being hovered */
            if (iter->popup.active && iter->popup.win && nk_input_is_mouse_hovering_rect(&ctx->input, iter->popup.win->bounds))
                return 1;

            if (iter->flags & NK_WINDOW_MINIMIZED) {
                struct nk_rect header = iter->bounds;
                header.h = ctx->style.font->height + 2 * ctx->style.window.header.padding.y;
                if (nk_input_is_mouse_hovering_rect(&ctx->input, header))
                    return 1;
            } else if (nk_input_is_mouse_hovering_rect(&ctx->input, iter->bounds)) {
                return 1;
            }
        }
        iter = iter->next;
    }
    return 0;
}
NK_API int
nk_item_is_any_active(struct nk_context *ctx)
{
    int any_hovered = nk_window_is_any_hovered(ctx);
    int any_active = (ctx->last_widget_state & NK_WIDGET_STATE_MODIFIED);
    return any_hovered || any_active;
}
NK_API int
nk_window_is_collapsed(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return 0;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return 0;
    return win->flags & NK_WINDOW_MINIMIZED;
}
NK_API int
nk_window_is_closed(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return 1;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return 1;
    return (win->flags & NK_WINDOW_CLOSED);
}
NK_API int
nk_window_is_hidden(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return 1;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return 1;
    return (win->flags & NK_WINDOW_HIDDEN);
}
NK_API int
nk_window_is_active(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return 0;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return 0;
    return win == ctx->active;
}
NK_API struct nk_window*
nk_window_find(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    return nk_find_window(ctx, title_hash, name);
}
NK_API void
nk_window_close(struct nk_context *ctx, const char *name)
{
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return;
    win = nk_window_find(ctx, name);
    if (!win) return;
    NK_ASSERT(ctx->current != win && "You cannot close a currently active window");
    if (ctx->current == win) return;
    win->flags |= NK_WINDOW_HIDDEN;
    win->flags |= NK_WINDOW_CLOSED;
}
NK_API void
nk_window_set_bounds(struct nk_context *ctx,
    const char *name, struct nk_rect bounds)
{
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return;
    win = nk_window_find(ctx, name);
    if (!win) return;
    NK_ASSERT(ctx->current != win && "You cannot update a currently in procecss window");
    win->bounds = bounds;
}
NK_API void
nk_window_set_position(struct nk_context *ctx,
    const char *name, struct nk_vec2 pos)
{
    struct nk_window *win = nk_window_find(ctx, name);
    if (!win) return;
    win->bounds.x = pos.x;
    win->bounds.y = pos.y;
}
NK_API void
nk_window_set_size(struct nk_context *ctx,
    const char *name, struct nk_vec2 size)
{
    struct nk_window *win = nk_window_find(ctx, name);
    if (!win) return;
    win->bounds.w = size.x;
    win->bounds.h = size.y;
}
NK_API void
nk_window_collapse(struct nk_context *ctx, const char *name,
                    enum nk_collapse_states c)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return;
    if (c == NK_MINIMIZED)
        win->flags |= NK_WINDOW_MINIMIZED;
    else win->flags &= ~(nk_flags)NK_WINDOW_MINIMIZED;
}
NK_API void
nk_window_collapse_if(struct nk_context *ctx, const char *name,
    enum nk_collapse_states c, int cond)
{
    NK_ASSERT(ctx);
    if (!ctx || !cond) return;
    nk_window_collapse(ctx, name, c);
}
NK_API void
nk_window_show(struct nk_context *ctx, const char *name, enum nk_show_states s)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (!win) return;
    if (s == NK_HIDDEN) {
        win->flags |= NK_WINDOW_HIDDEN;
    } else win->flags &= ~(nk_flags)NK_WINDOW_HIDDEN;
}
NK_API void
nk_window_show_if(struct nk_context *ctx, const char *name,
    enum nk_show_states s, int cond)
{
    NK_ASSERT(ctx);
    if (!ctx || !cond) return;
    nk_window_show(ctx, name, s);
}

NK_API void
nk_window_set_focus(struct nk_context *ctx, const char *name)
{
    int title_len;
    nk_hash title_hash;
    struct nk_window *win;
    NK_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)nk_strlen(name);
    title_hash = nk_murmur_hash(name, (int)title_len, NK_WINDOW_TITLE);
    win = nk_find_window(ctx, title_hash, name);
    if (win && ctx->end != win) {
        nk_remove_window(ctx, win);
        nk_insert_window(ctx, win, NK_INSERT_BACK);
    }
    ctx->active = win;
}

