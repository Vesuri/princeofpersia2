break *pop2DisplayProbeComplete
continue
if $pc != (unsigned long)&pop2DisplayProbeComplete || g_displayProbe != 1
 echo DISPLAY / WRONG CHECKPOINT\n
 quit 1
end
printf "DISPLAY_FRAME id=1 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/display-1-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/display-1-copper.bin g_displayCopper g_displayCopper+2236
continue
if $pc != (unsigned long)&pop2DisplayProbeComplete || g_displayProbe != 2
 echo DISPLAY / WRONG CHECKPOINT\n
 quit 1
end
printf "DISPLAY_FRAME id=2 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/display-2-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/display-2-copper.bin g_displayCopper g_displayCopper+2236
continue
if $pc != (unsigned long)&pop2DisplayProbeComplete || g_displayProbe != 3
 echo DISPLAY / WRONG CHECKPOINT\n
 quit 1
end
printf "DISPLAY_FRAME id=3 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/display-3-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/display-3-copper.bin g_displayCopper g_displayCopper+2236
continue
if $pc != (unsigned long)&pop2DisplayProbeComplete || g_displayProbe != 4
 echo DISPLAY / WRONG CHECKPOINT\n
 quit 1
end
printf "DISPLAY_FRAME id=4 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/display-4-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/display-4-copper.bin g_displayCopper g_displayCopper+2236
delete breakpoints
source scaffold.gdb
