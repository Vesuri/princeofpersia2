break *pop2InputProbeComplete
break *pop2CoreStopped
continue
if $pc != (unsigned long)&pop2InputProbeComplete
 echo INPUT WRONG STOP\n
 quit 1
end
printf "INPUT PROBE stage=%lu fields=%lu\n",g_inputProbeStage,g_scaffoldFields
if g_inputProbeStage != 9
 quit 1
end
set $i=0
while $i<9
 printf "INPUT EVENT message=%08lx modifiers=%04lx when=%lu keys=",g_inputProbeEvents[$i][0],g_inputProbeEvents[$i][1],g_inputProbeEvents[$i][2]
 set $k=0
 while $k<16
  printf "%02x",g_inputProbeKeys[$i][$k]
  set $k=$k+1
 end
 printf "\n"
 set $i=$i+1
end
delete breakpoints
source @RUN@/core.gdb
