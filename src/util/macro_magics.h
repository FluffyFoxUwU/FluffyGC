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
_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
_61,_62,_63,N,...) N
#define MACRO_RSEQ_N() \
63,62,61,60, \
59,58,57,56,55,54,53,52,51,50, \
49,48,47,46,45,44,43,42,41,40, \
39,38,37,36,35,34,33,32,31,30, \
29,28,27,26,25,24,23,22,21,20, \
19,18,17,16,15,14,13,12,11,10, \
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
#define ___MACRO_FOR_EACH16(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH15(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH17(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH16(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH18(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH17(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH19(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH18(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH20(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH19(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH21(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH20(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH22(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH21(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH23(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH22(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH24(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH23(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH25(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH24(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH26(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH25(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH27(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH26(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH28(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH27(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH29(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH28(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH30(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH29(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH31(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH30(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH32(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH31(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH33(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH32(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH34(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH33(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH35(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH34(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH36(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH35(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH37(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH36(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH38(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH37(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH39(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH38(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH40(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH39(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH41(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH40(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH42(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH41(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH43(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH42(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH44(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH43(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH45(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH44(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH46(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH45(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH47(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH46(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH48(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH47(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH49(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH48(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH50(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH49(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH51(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH50(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH52(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH51(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH53(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH52(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH54(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH53(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH55(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH54(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH56(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH55(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH57(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH56(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH58(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH57(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH59(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH58(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH60(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH59(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH61(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH60(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH62(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH61(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH63(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH62(func, __VA_ARGS__)
#define ___MACRO_FOR_EACH64(func, a, ...) func(a, __VA_ARGS__) ___MACRO_FOR_EACH63(func, __VA_ARGS__)

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
#define ___MACRO_FOR_EACH_STRIDE2_16(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_15(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_17(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_16(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_18(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_17(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_19(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_18(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_20(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_19(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_21(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_20(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_22(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_21(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_23(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_22(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_24(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_23(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_25(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_24(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_26(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_25(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_27(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_26(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_28(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_27(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_29(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_28(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_30(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_29(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_31(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_30(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_32(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_31(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_33(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_32(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_34(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_33(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_35(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_34(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_36(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_35(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_37(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_36(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_38(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_37(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_39(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_38(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_40(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_39(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_41(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_40(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_42(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_41(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_43(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_42(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_44(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_43(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_45(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_44(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_46(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_45(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_47(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_46(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_48(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_47(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_49(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_48(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_50(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_49(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_51(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_50(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_52(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_51(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_53(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_52(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_54(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_53(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_55(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_54(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_56(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_55(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_57(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_56(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_58(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_57(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_59(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_58(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_60(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_59(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_61(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_60(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_62(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_61(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_63(funcA, funcB, a, ...) funcB(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_62(funcA, funcB, __VA_ARGS__)
#define ___MACRO_FOR_EACH_STRIDE2_64(funcA, funcB, a, ...) funcA(a, __VA_ARGS__) ___MACRO_FOR_EACH_STRIDE2_63(funcA, funcB, __VA_ARGS__)

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
#define ___MACRO_EXPAND_IF_NOT1_16(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_17(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_18(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_19(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_20(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_21(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_22(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_23(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_24(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_25(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_26(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_27(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_28(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_29(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_30(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_31(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_32(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_33(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_34(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_35(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_36(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_37(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_38(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_39(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_40(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_41(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_42(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_43(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_44(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_45(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_46(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_47(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_48(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_49(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_50(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_51(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_52(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_53(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_54(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_55(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_56(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_57(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_58(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_59(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_60(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_61(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_62(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_63(...) __VA_ARGS__
#define ___MACRO_EXPAND_IF_NOT1_64(...) __VA_ARGS__

#endif

