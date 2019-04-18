# 不再维护说明
本汉化项目不再维护，望意愿接管者，请联系我

# Nuklear 中文汉化计划

这是 [vurtun/nuklear](https://github.com/vurtun/nuklear) 的中文版本

包括

* [ ] [中文文档（未完成）](https://github.com/zoollcar/nuklear/blob/master/doc/nuklear.html)
* [ ] 源代码注释翻译（未完成）
* [ ] 中文示例（未完成）

## 关于 Nuklear

这是一个小型的开箱即用的图形用户界面工具包
使用 ANSI C 编写。
这是一个小型的嵌入式用户界面，可以配置为没有任何依赖
a default renderbackend or OS window and input handling but instead provides a very modular
library approach by using simple input state for input and draw
commands describing primitive shapes as output. So instead of providing 
a layered library that tries to abstract over a number of platform and
render backends it only focuses on the actual UI.

## 特色

- Immediate mode graphical user interface toolkit
- 单一头文件
- 使用 C89 (ANSI C) 编写
- 小文件 (~18kLOC)
- 专注于可移植性，效率和简洁
- 无依赖 (通过设置甚至可以不依赖标准库)
- 完全可定制皮肤
- Low memory footprint with total memory control if needed or wanted
- 支持 UTF-8
- No global or hidden state
- 可定制的库模块（您可以只编译和使用您所需要的）
- Optional font baker and vertex buffer output
- [中文文档](https://github.com/zoollcar/nuklear/blob/master/doc/nuklear.html)

## 用法
这个库自包含在一个单独的头文件中，
分为 仅头文件模式 和 定义模式. 默认使用的是 仅头文件模式
如果要在包含头文件的同时包含定义 需要在 #include 前定义一个宏 NK_IMPLEMENTATION
e.g.:
使用它需要在程序开头定义一个宏和引入一个头文件

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~C
    #define NK_IMPLEMENTATION
    #include "nuklear.h"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
注意: 每次包含nuklear时都应定义相同的编译器标志。这非常重要，否则可能会导致编译错误，甚至堆栈损坏。

**注意：中文版编译有问题？是中文注释的问题，将 nuklear.h 文件编码转换为 GB 2312 即可**

## 画廊

![screenshot](https://cloud.githubusercontent.com/assets/8057201/11761525/ae06f0ca-a0c6-11e5-819d-5610b25f6ef4.gif)
![screen](https://cloud.githubusercontent.com/assets/8057201/13538240/acd96876-e249-11e5-9547-5ac0b19667a0.png)
![screen2](https://cloud.githubusercontent.com/assets/8057201/13538243/b04acd4c-e249-11e5-8fd2-ad7744a5b446.png)
![node](https://cloud.githubusercontent.com/assets/8057201/9976995/e81ac04a-5ef7-11e5-872b-acd54fbeee03.gif)
![skinning](https://cloud.githubusercontent.com/assets/8057201/15991632/76494854-30b8-11e6-9555-a69840d0d50b.png)
![gamepad](https://cloud.githubusercontent.com/assets/8057201/14902576/339926a8-0d9c-11e6-9fee-a8b73af04473.png)

## 示例

```c
// 初始化 gui 状态
enum {EASY, HARD};
static int op = EASY;
static float value = 0.6f;
static int i =  20;
struct nk_context ctx;

nk_init_fixed(&ctx, calloc(1, MAX_MEMORY), MAX_MEMORY, &font);
if (nk_begin(&ctx, "Show", nk_rect(50, 50, 220, 220),
    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
    // 固定小部件像素宽度
    nk_layout_row_static(&ctx, 30, 80, 1);
    if (nk_button_label(&ctx, "button")) {
        // event handling
    }

    // 固定小部件宽高比
    nk_layout_row_dynamic(&ctx, 30, 2);
    if (nk_option_label(&ctx, "easy", op == EASY)) op = EASY;
    if (nk_option_label(&ctx, "hard", op == HARD)) op = HARD;

    // 自定义小部件像素宽度
    nk_layout_row_begin(&ctx, NK_STATIC, 30, 2);
    {
        nk_layout_row_push(&ctx, 50);
        nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
        nk_layout_row_push(&ctx, 110);
        nk_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
    }
    nk_layout_row_end(&ctx);
}
nk_end(&ctx);
```
![example](https://cloud.githubusercontent.com/assets/8057201/10187981/584ecd68-675c-11e5-897c-822ef534a876.png)

## Bindings
There are a number of nuklear bindings for different languges created by other authors.
I cannot atest for their quality since I am not necessarily proficient in either of these
languages. Furthermore there are no guarantee that all bindings will always be kept up to date:

- [Java](https://github.com/glegris/nuklear4j) by Guillaume Legris
- [Golang](https://github.com/golang-ui/nuklear) by golang-ui@github.com
- [Rust](https://github.com/snuk182/nuklear-rust) by snuk182@github.com
- [Chicken](https://github.com/wasamasa/nuklear) by wasamasa@github.com
- [Nim](https://github.com/zacharycarter/nuklear-nim) by zacharycarter@github.com
- Lua
  - [LÖVE-Nuklear](https://github.com/keharriso/love-nuklear) by Kevin Harrison
  - [MoonNuklear](https://github.com/stetre/moonnuklear) by Stefano Trettel
- Python
  - [pyNuklear](https://github.com/billsix/pyNuklear) by William Emerison Six (ctypes-based wrapper)
  - [pynk](https://github.com/nathanrw/nuklear-cffi) by nathanrw@github.com (cffi binding)
- [CSharp/.NET](https://github.com/cartman300/NuklearDotNet) by cartman300@github.com

## Credits
Developed by Micha Mettke and every direct or indirect contributor to the GitHub.


Embeds `stb_texedit`, `stb_truetype` and `stb_rectpack` by Sean Barrett (public domain)
Embeds `ProggyClean.ttf` font by Tristan Grimmer (MIT license).


Big thank you to Omar Cornut (ocornut@github) for his [imgui](https://github.com/ocornut/imgui) library and
giving me the inspiration for this library, Casey Muratori for handmade hero
and his original immediate mode graphical user interface idea and Sean
Barrett for his amazing single header [libraries](https://github.com/nothings/stb) which restored my faith
in libraries and brought me to create some of my own. Finally Apoorva Joshi for his singe-header [file packer](http://apoorvaj.io/single-header-packer.html).

## License
```
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Micha Mettke
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------
```
