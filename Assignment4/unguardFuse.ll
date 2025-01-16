; ModuleID = 'TEST/unguardFuse.bc'
source_filename = "TEST/LoopFuse.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @foo(i32 noundef %0, ptr noundef %1, ptr noundef %2, ptr noundef %3, ptr noundef %4, i32 noundef %5) #0 {
  br label %7

7:                                                ; preds = %20, %6
  %.0 = phi i32 [ 0, %6 ], [ %21, %20 ]
  %8 = icmp slt i32 %.0, %0
  br i1 %8, label %9, label %22

9:                                                ; preds = %7
  %10 = sext i32 %.0 to i64
  %11 = getelementptr inbounds i32, ptr %2, i64 %10
  %12 = load i32, ptr %11, align 4
  %13 = sdiv i32 1, %12
  %14 = sext i32 %.0 to i64
  %15 = getelementptr inbounds i32, ptr %3, i64 %14
  %16 = load i32, ptr %15, align 4
  %17 = mul nsw i32 %13, %16
  %18 = sext i32 %.0 to i64
  %19 = getelementptr inbounds i32, ptr %1, i64 %18
  store i32 %17, ptr %19, align 4
  br label %20

20:                                               ; preds = %9
  %21 = add nsw i32 %.0, 1
  br label %7, !llvm.loop !6

22:                                               ; preds = %7
  br label %23

23:                                               ; preds = %35, %22
  %.1 = phi i32 [ 0, %22 ], [ %36, %35 ]
  %24 = icmp slt i32 %.1, %0
  br i1 %24, label %25, label %37

25:                                               ; preds = %23
  %26 = sext i32 %.1 to i64
  %27 = getelementptr inbounds i32, ptr %1, i64 %26
  %28 = load i32, ptr %27, align 4
  %29 = sext i32 %.1 to i64
  %30 = getelementptr inbounds i32, ptr %3, i64 %29
  %31 = load i32, ptr %30, align 4
  %32 = add nsw i32 %28, %31
  %33 = sext i32 %.1 to i64
  %34 = getelementptr inbounds i32, ptr %4, i64 %33
  store i32 %32, ptr %34, align 4
  br label %35

35:                                               ; preds = %25
  %36 = add nsw i32 %.1, 1
  br label %23, !llvm.loop !8

37:                                               ; preds = %23
  ret i32 0
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.6"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
