#ifndef _headers_1689761736_FluffyGC_black_magic_uwu
#define _headers_1689761736_FluffyGC_black_magic_uwu

/*

Generator

for i=0,64 do
  if i % 2 == 0 then
    print(("#define ___MACRO_FOR_EACH_STRIDE2_%d(funcA, funcB, a, ...) funcA(a) ___MACRO_FOR_EACH_STRIDE2_%d(funcA, funcB, __VA_ARGS__)"):format(i, i - 1))
  else
    print(("#define ___MACRO_FOR_EACH_STRIDE2_%d(funcA, funcB, a, ...) funcB(a) ___MACRO_FOR_EACH_STRIDE2_%d(funcA, funcB, __VA_ARGS__)"):format(i, i - 1))
  end
end

*/

// All preprocessor macro magics UwU
// (actually not magic but i love
// to name it that way because its magical
// in some way for me)

// From https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s?pli=1
#define MACRO_NARG(...) \
MACRO_NARG_(__VA_ARGS__,MACRO_RSEQ_N())
#define MACRO_NARG_(...) \
MACRO_ARG_N(__VA_ARGS__)
#define MACRO_ARG_N( \
_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
_11,_12,_13,_14,_15,N,...) N
#define MACRO_RSEQ_N() \
15,14,13,12,11,10, \
9,8,7,6,5,4,3,2,1,0

#define ___MACRO_PASTE(a, b) a ## b
#define MACRO_PASTE(a, b) ___MACRO_PASTE(a, b)

#define MACRO_FOR_EACH(func, ...) MACRO_PASTE(___MACRO_FOR_EACH, MACRO_NARG(__VA_ARGS__))(func, __VA_ARGS__)
#define MACRO_FOR_EACH_STRIDE2(funcA, funcB, ...) MACRO_PASTE(___MACRO_FOR_EACH_STRIDE2_, MACRO_NARG(__VA_ARGS__))(funcA, funcB, __VA_ARGS__)

#define MACRO_EXPAND_IF_NOT1(val, ...) MACRO_PASTE(___MACRO_EXPAND_IF_NOT1_, val)(__VA_ARGS__)

#define ___MACRO_FOR_EACH0(func, a, ...)
#define ___MACRO_FOR_EACH1(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH0(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH2(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH1(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH3(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH2(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH4(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH3(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH5(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH4(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH6(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH5(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH7(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH6(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH8(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH7(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH9(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH8(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH10(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH9(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH11(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH10(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH12(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH11(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH13(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH12(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH14(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH13(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH15(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH14(func, __VA_ARGS__)

#define ___MACRO_FOR_EACH_STRIDE2_0(funcA, funcB, a, ...) 
#define ___MACRO_FOR_EACH_STRIDE2_1(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) 
#define ___MACRO_FOR_EACH_STRIDE2_2(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_1(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_3(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_2(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_4(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_3(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_5(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_4(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_6(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_5(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_7(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_6(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_8(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_7(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_9(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_8(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_10(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_9(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_11(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_10(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_12(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_11(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_13(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_12(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_14(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_13(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_15(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_14(funcA, funcB, __VA_ARGS__)

#define ___MACRO_EXPAND_IF_NOT1_0(...)__VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_1(...) 
#define ___MACRO_EXPAND_IF_NOT1_2(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_3(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_4(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_5(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_6(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_7(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_8(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_9(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_10(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_11(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_12(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_13(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_14(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_15(...) __VA_ARGS__

#endif

