# Correctness-only observer. Never combine these stops with CPU timings.
set $boundsCount=0
break PrinceScreen::lists if g_integrationStage==3
commands
 silent
 set $boundsCount=$boundsCount+1
 set $nd='NativeDisplay::(anonymous namespace)::dirtyCount'
 set $dr=&'NativeDisplay::(anonymous namespace)::dirty'[0]
 printf "BOUNDS frame=%lu crop=%d,%d dirty=%u converted=%u\n",$boundsCount,display.viewport_.top,display.viewport_.left,$nd,display.syncCount_
 set $i=0
 while $i<$nd
  set $r=$dr[$i]
  printf "DIRTY %d %d %d %d\n",$r.top,$r.left,$r.bottom,$r.right
  set $i=$i+1
 end
 set $i=0
 while $i<display.syncCount_
  set $r=display.sync_[$i]
  printf "CONVERTED %d %d %d %d\n",$r.top,$r.left,$r.bottom,$r.right
  set $i=$i+1
 end
 eval "dump binary memory @RUN@/bounds-%d-chunky.bin g_integrationScreen g_integrationScreen+307200",$boundsCount
 set $bpixels=display.pixels_[display.front_^1]
 eval "dump binary memory @RUN@/bounds-%d-planes.bin $bpixels $bpixels+196608",$boundsCount
 continue
end
