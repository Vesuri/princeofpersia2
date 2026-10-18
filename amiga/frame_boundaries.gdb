# Stop only after the completed game frame's NativeDisplay::publish returns.
# Read pending-or-front output, so this also checks skipped publications.
define install-c2p-bounds
printf "INTEGRATION DISPLAY planes=%08lx copper=%08lx parity=%u\n",g_displayPlanes,g_displayCopper,g_displayParity
dump binary memory @RUN@/first-room-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/first-room-copper.bin g_displayCopper g_displayCopper+2236
set $boundaryCount=0
break CoreRuntime.cpp:@BOUNDARY_LINE@ if g_integrationStage==3
commands
 silent
 if g_timingPhase!=0x1002 || g_openingCount!=$boundaryCount
  echo FRAME BOUNDARY / ORDER OR PHASE MISMATCH\n
  quit 1
 end
 set $boundaryCount=$boundaryCount+1
 set $buffer=display.front_
 if display.pending_
  set $buffer=$buffer^1
 end
 set $pixels=display.pixels_[$buffer]
 printf "FRAME_BOUNDARY index=%lu game=%u crop=%d,%d pending=%u pixels=%08lx\n",$boundaryCount,*(unsigned short*)(g_coreA5-0x349a),display.viewport_.top,display.viewport_.left,display.pending_,$pixels
 eval "dump binary memory @RUN@/frame-%d-chunky.bin g_integrationScreen g_integrationScreen+307200",$boundaryCount
 eval "dump binary memory @RUN@/frame-%d-planes.bin $pixels $pixels+196608",$boundaryCount
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/frame-%d-colors.bin $colors $colors+2056",$boundaryCount
 set $copper=display.copper_+$buffer*2*559
 eval "dump binary memory @RUN@/frame-%d-copper.bin $copper $copper+559",$boundaryCount
 eval "dump binary memory @RUN@/frame-%d-kid.bin g_coreA5-0x50d2 g_coreA5-0x5092",$boundaryCount
 continue
end
end
