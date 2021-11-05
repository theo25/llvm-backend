target datalayout = "@BACKEND_TARGET_DATALAYOUT@"
target triple = "@BACKEND_TARGET_TRIPLE@"

%blockheader = type { i64 } 
%block = type { %blockheader, [0 x i64 *] } ; 16-bit layout, 8-bit length, 32-bit tag, children

declare tailcc %block* @step(%block*)
declare tailcc %block** @stepAll(%block*, i64*)

@depth = thread_local global i64 zeroinitializer
@steps = thread_local global i64 zeroinitializer
@current_interval = thread_local global i64 0

define i1 @finished_rewriting() {
entry:
  %depth = load i64, i64* @depth
  %hasDepth = icmp sge i64 %depth, 0
  %steps = load i64, i64* @steps
  %stepsPlusOne = add i64 %steps, 1
  store i64 %stepsPlusOne, i64* @steps
  br i1 %hasDepth, label %if, label %else
if:
  %depthMinusOne = sub i64 %depth, 1
  store i64 %depthMinusOne, i64* @depth
  %finished = icmp eq i64 %depth, 0
  ret i1 %finished
else:
  ret i1 false
}

define %block* @take_steps(i64 %depth, %block* %subject) {
  store i64 %depth, i64* @depth
  %result = call tailcc %block* @step(%block* %subject)
  ret %block* %result
}

define %block** @take_search_step(%block* %subject, i64* %count) {
  store i64 -1, i64* @depth
  %result = call tailcc %block** @stepAll(%block* %subject, i64* %count)
  ret %block** %result
}

define i64 @get_steps() {
entry:
  %steps = load i64, i64* @steps
  ret i64 %steps
}
