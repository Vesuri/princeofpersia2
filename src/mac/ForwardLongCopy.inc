| Ordered longword copy for 68020+ (including unaligned addresses).
| Caller supplies a positive longword count minus one in counter.w and
| source/destination address registers. Overlap preserves individual longword
| load/store ordering; this is neither memcpy nor byte-wise memmove.
| Advances both pointers by 4*(count+1), ends counter.w=-1. MOVE sets NZVC;
| DBF preserves CCR and X is unchanged. No other registers or stack used.
	.macro forward_long_copy source,destination,counter
.Lforward_long_copy_\@:
	move.l (\source)+,(\destination)+
	dbf \counter,.Lforward_long_copy_\@
	.endm
